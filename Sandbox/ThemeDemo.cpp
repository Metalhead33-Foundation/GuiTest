#include "ThemeDemo.hpp"
#include <Lotte/Gui/LteThemeCss.hpp>
#include "ThemeJson.hpp"
#include <Lotte/Procedural/LteGuiAtlas.hpp>
#include <Lotte/Procedural/LteSurfaceCanvas.hpp>
#include <Euphemy/Io/EuphFile.hpp>
#include <Euphemy/Media/Image/EuphPNG.hpp>
#include <cstdio>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace Sandbox {

/// @brief Reads a whole virtual file, or returns an empty string.
std::string readVirtualFile(Elv::Io::System& io, const std::string& path)
{
	try {
		Elv::Io::uDevice device = io.openUnique(path.c_str(), Elv::Io::Mode::READ);
		if (!device || !device->isValid()) return {};
		return device->readAllAsString<char>();
	} catch (const std::exception& error) {
		std::printf("could not read '%s': %s\n", path.c_str(), error.what());
		return {};
	}
}

/// @brief Turns a definition's procedural entry into builder settings.
Lotte::Procedural::GuiAtlasSettings atlasSettingsFor(const Lotte::Gui::ThemeAtlasDefinition& atlas)
{
	Lotte::Procedural::GuiAtlasSettings settings;
	settings.seed = atlas.proceduralSeed;
	const std::uint32_t scale = atlas.proceduralScalePercent == 0 ? 100u : atlas.proceduralScalePercent;
	settings.atlasWidth = 1024u * scale / 100u;
	settings.panelWidth = 256u * scale / 100u;
	settings.panelHeight = 128u * scale / 100u;
	settings.buttonWidth = 128u * scale / 100u;
	settings.buttonHeight = 48u * scale / 100u;
	settings.slotSize = 64u * scale / 100u;
	settings.titleWidth = 256u * scale / 100u;
	settings.titleHeight = 48u * scale / 100u;
	if (!atlas.css.empty()) {
		// The same CSS vocabulary the text uses decides the generator's bevel and face
		// tint: libcss computes `border-width` and `background-color`, the builder obeys.
		const Lotte::Gui::ThemeCssStyle css = Lotte::Gui::resolveThemeCss(atlas.css);
		if (css.valid) {
			if (css.atlas.hasBorderWidth) settings.bevelWidth = css.atlas.borderWidth;
			if (css.atlas.hasBackground) settings.faceTint = css.atlas.background;
		}
	}
	return settings;
}

int runThemeDemo(Elv::Io::System& io, const std::string& themePath,
const std::string& outputDirectory)
{
	std::error_code directoryError;
	std::filesystem::create_directories(outputDirectory, directoryError);
	if (directoryError) {
		std::printf("theme demo: could not create '%s': %s\n", outputDirectory.c_str(),
					directoryError.message().c_str());
		return 1;
	}
	const std::string text = readVirtualFile(io, themePath);
	if (text.empty()) {
		std::printf("theme demo: '%s' is empty or could not be read\n", themePath.c_str());
		return 1;
	}
	const ThemeParseResult parsed = parseThemeJson(text);
	std::size_t elementCount = 0;
	for (const auto& element : parsed.definition.elements) {
		if (element.has_value()) ++elementCount;
	}
	std::printf("theme '%s': %zu atlas(es), %zu element(s)\n", parsed.definition.name.c_str(),
	parsed.definition.atlases.size(), elementCount);
	for (const std::string& problem : parsed.problems) {
		std::printf("  problem: %s\n", problem.c_str());
	}
	if (!parsed.ok) return 1;

	// Generate every procedural atlas and remember what was produced, so the resolver
	// below can answer alias queries with a size even without a GPU.
	std::map<std::string, std::unique_ptr<Lotte::Procedural::GuiAtlas>> generated;
	for (const Lotte::Gui::ThemeAtlasDefinition& atlas : parsed.definition.atlases) {
		if (atlas.source != Lotte::Gui::ThemeAtlasSource::Procedural) {
			std::printf("atlas '%s': file source '%s' (not uploaded by this demo)\n",
			atlas.alias.c_str(), atlas.path.c_str());
			continue;
		}
		auto built = std::make_unique<Lotte::Procedural::GuiAtlas>(
		Lotte::Procedural::buildGuiAtlas(atlasSettingsFor(atlas)));
		if (!built->image.isValid()) {
			std::printf("atlas '%s': the generator produced no image\n", atlas.alias.c_str());
			continue;
		}
		const std::string outputPath = outputDirectory + "/" + atlas.alias + ".png";
		Euph::Io::File file(outputPath.c_str(), Elv::Io::Mode::WRITE);
		if (file.isValid()) {
			Euph::Media::Image::PNG::encode(
			file, Lotte::Procedural::toDecodeTarget(built->image), 6.0f);
			std::printf("atlas '%s': %ux%u, %zu part(s) -> %s\n", atlas.alias.c_str(),
			unsigned(built->image.width), unsigned(built->image.height),
			built->parts.size(), outputPath.c_str());
		} else {
			std::printf("atlas '%s': could not open '%s' for writing\n", atlas.alias.c_str(),
			outputPath.c_str());
		}
		for (const Lotte::Procedural::GuiAtlasRect& rect : built->parts) {
			std::printf("  %-16s %4u %4u %4u %4u\n",
			Lotte::Procedural::guiAtlasPartName(rect.part), rect.x, rect.y,
			rect.width, rect.height);
		}
		generated.emplace(atlas.alias, std::move(built));
	}

	// Hand the theme a resolver over the generated atlases. The texture handle is a
	// stand-in (1): this demo has no Kaldi device, it only proves the data path.
	Lotte::Gui::DefinitionTheme theme(
	parsed.definition, [&generated](std::string_view alias) -> std::optional<Lotte::Gui::ThemeAtlasBinding> {
		const auto found = generated.find(std::string(alias));
		if (found == generated.end()) return std::nullopt;
		Lotte::Gui::ThemeAtlasBinding binding;
		binding.binding.texture = Kld::HandleId(1);
		binding.pixelSize = glm::fvec2(static_cast<float>(found->second->image.width),
									   static_cast<float>(found->second->image.height));
		// The generated manifest answers "where did this part land?", which is what a
		// part-based skin in the theme document needs.
		const Lotte::Procedural::GuiAtlas* manifest = found->second.get();
		binding.partRect = [manifest](Lotte::Procedural::GuiAtlasPart part)
			-> std::optional<Lotte::Gui::ThemeRect> {
			const Lotte::Procedural::GuiAtlasRect* rect = manifest->find(part);
			if (rect == nullptr) return std::nullopt;
			Lotte::Gui::ThemeRect result;
			result.x = static_cast<std::int32_t>(rect->x);
			result.y = static_cast<std::int32_t>(rect->y);
			result.width = rect->width;
			result.height = rect->height;
			return result;
		};
		return binding;
	});

	// Prove the definition answers the interface questions the renderer will ask.
	const Lotte::Gui::WidgetState states[2] = { 0, 0x7u };
	std::size_t resolved = 0;
	for (std::size_t index = 0; index < Lotte::Gui::ThemeElementCount; ++index) {
		const auto element = static_cast<Lotte::Gui::ThemeElement>(index);
		for (const Lotte::Gui::WidgetState state : states) {
			if (theme.skin(element, state).has_value()) ++resolved;
		}
	}
	std::printf("DefinitionTheme '%s': %zu skin query(s) resolved\n",
	std::string(theme.name()).c_str(), resolved);
	// Text styles are the other half of the contract, and the ones the document states in
	// CSS are resolved through libcss at construction: print what the cascade produced, so
	// a broken block shows up here rather than on screen.
	const auto channel = [](float value) {
		return unsigned(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f));
	};
	const Lotte::Gui::ThemeTextRole roles[] = { Lotte::Gui::ThemeTextRole::Heading,
											   Lotte::Gui::ThemeTextRole::Label };
	for (const Lotte::Gui::ThemeTextRole role : roles) {
		const Lotte::Text::TextAttributes style = theme.textStyle(role, 0u);
		std::printf("  text role %-10s: %5.1f px  weight %4u  #%02x%02x%02x%s%s\n",
					Lotte::Gui::themeTextRoleName(role), double(style.size),
					unsigned(style.effectiveWeight()), channel(style.colour.r),
					channel(style.colour.g), channel(style.colour.b),
					style.italic ? " italic" : "", style.underline ? " underlined" : "");
	}

	return 0;
}

} // namespace Sandbox
