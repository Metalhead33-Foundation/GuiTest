#include "GameSystem.hpp"
#include <Euphemy/Localization/EuphLocalization.hpp>
#include <Euphemy/Media/Image/EuphPNG.hpp>
#include <Haruka/Core/HrkSoundPlayer.hpp>
#include <Lotte/Text/LteMml.hpp>
#include <Lotte/Gui/LteThemeDefinition.hpp>
#include <Lotte/Procedural/LteGuiAtlas.hpp>
#include <Lotte/Text/LteRichText.hpp>
#include <Lotte/Text/LteUtf.hpp>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <optional>
#include <string_view>
#include <utility>
#include "GuiScene.hpp"
#include "LocalizationYaml.hpp"
#include "ScriptHost.hpp"
#include "ThemeDemo.hpp"
#include "ThemeJson.hpp"
#include "JS/JsCore.hpp"
#include "JS/Wrappers/Audio/JsAudioPlayer.hpp"
#include "JS/Wrappers/Audio/JsSound.hpp"
#include "JS/Wrappers/GUI/JsGuiLabel.hpp"
#include "JS/Wrappers/GUI/JsGuiLayout.hpp"
#include "JS/Wrappers/GUI/JsGuiSlider.hpp"
#include "JS/Wrappers/GUI/JsGuiWrappers.hpp"
#include "JS/Wrappers/Io/JsIO.hpp"
#include "JS/Wrappers/Localization/JsLocalization.hpp"
#include "JS/Wrappers/System/JsSystem.hpp"

namespace {

/** @brief Expands a packed `0xAARRGGBB` colour into four linear RGBA components. */
void expandColor(std::uint32_t packed, float (&out)[4])
{
	out[0] = float((packed >> 16) & 0xFFu) / 255.0f;
	out[1] = float((packed >> 8) & 0xFFu) / 255.0f;
	out[2] = float(packed & 0xFFu) / 255.0f;
	out[3] = float((packed >> 24) & 0xFFu) / 255.0f;
}

/** @brief The nine anchors the number keys 1-9 select, in declaration order. */
constexpr Lotte::Gui::CursorAnchor numberKeyAnchors[] = {
	Lotte::Gui::CursorAnchor::TopLeft,
	Lotte::Gui::CursorAnchor::Top,
	Lotte::Gui::CursorAnchor::TopRight,
	Lotte::Gui::CursorAnchor::Left,
	Lotte::Gui::CursorAnchor::Center,
	Lotte::Gui::CursorAnchor::Right,
	Lotte::Gui::CursorAnchor::BottomLeft,
	Lotte::Gui::CursorAnchor::Bottom,
	Lotte::Gui::CursorAnchor::BottomRight
};

/** @brief Maps SDLK_1..SDLK_9 to an anchor, or @c std::nullopt for any other key. */
std::optional<Lotte::Gui::CursorAnchor> anchorForNumberKey(SDL_Keycode key)
{
	if (key < SDLK_1 || key > SDLK_9) {
		return std::nullopt;
	}
	return numberKeyAnchors[key - SDLK_1];
}

/**
 * @brief Asks the main loop to stop without tearing SDL down first.
 *
 * `SDL_Quit()` destroys the event subsystem while `AppSystem::run()` is still polling
 * it: the loop would never see a quit event and would keep spinning on a dead SDL (and
 * the configuration would never be written). Posting `SDL_QUIT` is what the window
 * manager's close button does, and the loop already treats it as "stop".
 */
void postQuitEvent()
{
	SDL_Event quitEvent {};
	quitEvent.type = SDL_QUIT;
	SDL_PushEvent(&quitEvent);
}

/** @brief Short name of a clip-space y axis, for the startup log. */
const char* clipSpaceAxisName(Kld::ClipSpaceYAxis axis) noexcept
{
	return axis == Kld::ClipSpaceYAxis::Down ? "down (Vulkan-style)" : "up (GL/D3D-style)";
}

} // namespace


GameSystem::GameSystem(Euph::Conf::Configuration& config, Elv::Io::System& iosys)
	: AppSystem(std::string(config.getString("Game", "sTitle", "GameDemo")),
				0,
				0,
				config.getUInteger("Video","uWidth", 640),
				config.getUInteger("Video","uHeight", 480),
				config.getBool("Video","bFullscreen", false) ? SDL_WINDOW_FULLSCREEN : 0), iosys(iosys), config(config)
{
	auto samplerate = config.getUInteger("Audio","uSamplerate",48000);
	auto channels = config.getUInteger("Audio","uChannels",2);
	auto bufferSize = config.getUInteger("Audio","uBufferSize",4096);
	sdlDriver = std::make_unique<Haruka_SDL_Driver>(
		samplerate,
		channels,
		bufferSize
		);
	mixer = Elv::Util::pmr_make_shared<Hrk::Mixer>(std::pmr::get_default_resource(),
		Hrk::ChannelCount(channels), Hrk::InterleavingType::INTERLEAVED, Hrk::SampleRate(samplerate), Hrk::FrameCount(bufferSize)
		);
	soundPlayer = Elv::Util::pmr_make_shared<Hrk::SoundPlayer>(std::pmr::get_default_resource(),
		*mixer);
	sdlDriver->setPlayable(mixer);
	startGraphics();
}

void GameSystem::installLabelLayout()
{
	if (!guiScene) {
		return;
	}
	guiScene->setLabelLayoutService([this](const Sandbox::ScriptLabel& label) {
		Lotte::Text::TextLayout empty;
		if (!fontRepository) {
			return empty;
		}
		Lotte::Text::TextRunList runs;
		Lotte::Text::TextRun run;
		run.text = Lotte::Text::decodeUtf8(label.text());
		run.attributes = label.attributes();
		runs.push_back(std::move(run));
		Lotte::Text::TextLayoutOptions options;
		// The service lays the text out around the origin; the scene centres the result
		// inside the label's box, so nothing here needs to know the box.
		//
		// Two scales meet here and both are wanted: `scale()` is the script's own
		// multiplier, `pixelScale()` is the viewport's (real pixels per canvas unit). A
		// 34 canvas-unit caption on a 0.8x viewport lays out at 27.2 px, and the SDF
		// atlas rasterizes that size rather than the canvas size, so the text stays sharp
		// instead of being a scaled bitmap.
		options.scale = label.scale() * label.pixelScale();
		return Lotte::Text::layoutRuns(runs,
			[this](const std::string& family, Lotte::Text::FontWeight weight) {
				return fontRepository->faceFor(family, weight);
			},
			options);
	});
}

void GameSystem::loadLocalization()
{
	localization.clear();
	const std::string language { config.getString("Game", "sLanguage", "english") };
	const std::string directory = "/localization/" + language;
	std::size_t files = 0;
	// PhysFS enumeration gives the full virtual path, which is what the device opens;
	// the files are independent, so they simply merge into one table.
	iosys.enumerate(directory.c_str(),
		[this, &files](Elv::Io::System*, const char* fullpath, const char* /*name*/) {
			Elv::Io::uDevice device;
			try {
				device = iosys.openUnique(fullpath, Elv::Io::Mode::READ);
			} catch (const std::exception& error) {
				SDL_Log("Localization '%s' unreadable: %s", fullpath, error.what());
				return;
			}
			if (!device || !device->isValid()) {
				return;
			}
			const Sandbox::LocalizationParseResult parsed =
				Sandbox::parseLocalizationYaml(device->readAllAsString<char>());
			for (const std::string& problem : parsed.problems) {
				SDL_Log("Localization '%s': %s", fullpath, problem.c_str());
			}
			if (parsed.ok) {
				localization.merge(parsed.table);
				++files;
			}
		});
	SDL_Log("Localization '%s': %zu key(s) from %zu file(s)", language.c_str(),
			localization.size(), files);
}

void GameSystem::startGraphics()
{
	Lotte::Gfx::KaldiContextSettings settings;
	settings.backendLibrary =
		std::string(config.getString("Graphics", "sBackendLibrary", "libKaldiVK.so"));
	// The configuration parses numbers in base 10, so the packed colour travels as a
	// string: "0xAARRGGBB" or "AARRGGBB", whichever reads better in the ini file.
	const std::string clearColorText { config.getString("Graphics", "sClearColor", "0xFF102030") };
	const std::uint32_t packedColor =
		static_cast<std::uint32_t>(std::strtoul(clearColorText.c_str(), nullptr, 16));
	expandColor(packedColor, clearColor);
	for (int index = 0; index < 4; ++index) {
		settings.clearColor[index] = clearColor[index];
	}
	try {
		kaldi = std::make_unique<Lotte::Gfx::KaldiContext>(windowInfo,
				std::uint16_t(width), std::uint16_t(height), settings);
		SDL_Log("Kaldi HAL %s via %s (%s), clip-space y %s, clearing to 0x%08X every frame",
			kaldi->halVersionName().c_str(), kaldi->rendererName().data(),
			kaldi->backendVersion().data(),
			clipSpaceAxisName(kaldi->resources().conventions().yAxis()),
			static_cast<unsigned>(packedColor));
		guiRenderer = std::make_unique<Lotte::Gui::KaldiGuiRenderer>(kaldi->resources(),
			Lotte::Gui::KaldiGuiRendererSettings{ std::uint16_t(width),
							      std::uint16_t(height) });
		guiScene = std::make_unique<Sandbox::GuiScene>(kaldi->resources());
		// Until a script asks for a designed canvas (`new GUI.Layout`), the canvas is the
		// window itself: one canvas unit is one pixel, which is what a layout written in
		// window pixels expects. The scene owns the mapping from here on, so a script can
		// replace it without the application caring.
		{
			Lotte::Gui::ViewportSettings viewportSettings;
			viewportSettings.canvas = glm::fvec2(float(width), float(height));
			guiScene->setViewport(std::make_shared<Lotte::Gui::GuiViewport>(
				viewportSettings, glm::fvec2(float(width), float(height))));
		}
		loadFont();
		loadCursors();
		// Pipeline creation, texture creation and the initial texture uploads are
		// all recorded; hand them to the backend before the first frame.
		const Kld::HalStatus status = kaldi->submit();
		if (!status.ok()) {
			SDL_Log("Graphics upload failed (%d): %s", static_cast<int>(status.code),
				status.message != nullptr ? status.message : "(no message)");
		}
		// The engine draws its own cursor now, so the window system's pointer must
		// not overlap it. It comes back when the window loses focus (see
		// handleWindowEvent) and when the system shuts down.
		SDL_ShowCursor(SDL_DISABLE);
	} catch (const std::exception& error) {
		// Rendering is optional here: without a usable GPU the audio half still runs
		// and `render()` becomes a no-op, which keeps the playground usable.
		textRenderer.reset();
		fontRepository.reset();
		fontReady = false;
		guiScene.reset();
		cursors.clear();
		guiRenderer.reset();
		kaldi.reset();
		SDL_Log("Graphics disabled: %s", error.what());
	}
}

GameSystem::~GameSystem()
{
	SDL_ShowCursor(SDL_ENABLE);
	stopScript();
}

void GameSystem::stopScript()
{
	// Order matters. A widget's click callback owns a JavaScript root, so the widgets
	// have to die while the context is still usable; the widgets also borrow the theme,
	// so it is released only after them; the context has to die before the renderer and
	// the resource factory its textures point into; and the host has to outlive the
	// context, because module finalizers reach the mixer and the audio device through it.
	// The one-shot voices go first of all: they are registered in the mixer, which the
	// audio callback reads independently of the script.
	if (soundPlayer) {
		AudioLock lock(sdlDriver.get());
		soundPlayer->stopAll();
	}
	guiScene.reset();
	theme.reset();
	jsCore.reset();
	scriptHost.reset();
}

void GameSystem::loadFont()
{
	try {
		const std::string path {
			config.getString("Graphics", "sFont", "vinque.wodf")
		};
		Lotte::Text::FontAtlasSettings atlasSettings;
		atlasSettings.pageWidth = 1024;
		atlasSettings.pageHeight = 1024;
		atlasSettings.gutter = 1;

		fontRepository = std::make_unique<Lotte::Text::FontRepository>(kaldi->resources(),
																	  atlasSettings);
		const auto face = fontRepository->addFaceFromFile("Vinque", iosys, path);
		fontRepository->setDefaultFaceId("Vinque");

		Lotte::Text::TextRendererSettings rendererSettings;
		rendererSettings.viewportWidth = std::uint16_t(width);
		rendererSettings.viewportHeight = std::uint16_t(height);
		textRenderer = std::make_unique<Lotte::Text::TextRenderer>(kaldi->resources(),
																   *fontRepository,
																   rendererSettings);
		fontReady = true;
		SDL_Log("Font '%s': %zu stored glyphs, %u px tiles", face->familyName.c_str(),
				face->glyphs.size(), face->storedBitmapSize());
	} catch (const std::exception& error) {
		textRenderer.reset();
		fontRepository.reset();
		fontReady = false;
		SDL_Log("Text disabled: %s", error.what());
	}
}

void GameSystem::setRichText(const Sandbox::RichTextRequest& request)
{
	if (!textRenderer || !fontRepository) {
		throw std::runtime_error("setRichText: no font is loaded");
	}
	Lotte::Text::RichTextProcessor processor;
	Lotte::Text::MmlParser parser(&processor);
	parser.parse(request.mml);
	processor.flush();
	if (processor.getRuns().empty()) {
		// An empty request clears the text: dropping the old layout is the only
		// honest answer, because its face pointers may already be stale.
		sampleLayout.clear();
		textRendered = false;
		return;
	}
	Lotte::Text::TextLayoutOptions options;
	options.origin = request.origin;
	options.spacing = request.spacing;
	options.wordSpacing = request.wordSpacing;
	options.scale = request.scale;
	sampleLayout = Lotte::Text::layoutRuns(
		processor.getRuns(),
		[this](const std::string& family, Lotte::Text::FontWeight weight) {
			return fontRepository->faceFor(family, weight);
		},
		options);
	// The next frame renders the new layout, so the one-shot log may report again.
	textRendered = false;
	SDL_Log("Text: %zu glyphs from %zu runs", sampleLayout.glyphs.size(),
			processor.getRuns().size());
}

void GameSystem::loadCursors()
{
	const std::string primary { config.getString("Graphics", "sCursor", "/gfx/wodmouse.png") };
	const std::vector<std::string> paths = { primary, "/gfx/hourglass2.png" };
	for (const std::string& path : paths) {
		Elv::Io::uDevice file;
		try {
			file = iosys.openUnique(path.c_str(), Elv::Io::Mode::READ);
		} catch (const std::exception& error) {
			SDL_Log("Could not open cursor image '%s': %s", path.c_str(), error.what());
			continue;
		}
		if (!file || !file->isValid()) {
			SDL_Log("Could not open cursor image: %s", path.c_str());
			continue;
		}
		try {
			Euph::Media::Image::DecodeTarget decoded;
			Euph::Media::Image::PNG::decode(*file, decoded);
			const Lotte::Gui::AlphaCoverage coverage =
				Lotte::Gui::AlphaCoverage::fromDecodeTarget(decoded);
			CursorEntry entry;
			entry.path = path;
			entry.texture = std::make_unique<Lotte::Gfx::WriteableTexture2D>(
				Lotte::Gfx::WriteableTexture2D::fromDecodeTarget(
					kaldi->resources(), decoded));
			Lotte::Gui::CursorMetadata metadata;
			metadata.alphaCrawl = true;
			// The hourglass is authored around its center, the arrow around its tip.
			metadata.anchor = path.find("hourglass") != std::string::npos
				? Lotte::Gui::CursorAnchor::Center
				: Lotte::Gui::CursorAnchor::TopLeft;
			Lotte::Gui::CursorImageVisual visual;
			visual.texture = entry.texture->handle();
			visual.pixelSize = glm::uvec2(entry.texture->width(),
						      entry.texture->height());
			entry.cursor = Lotte::Gui::Cursor(visual, &coverage, metadata);
			const glm::fvec2 hotspot = entry.cursor.hotspotPixels();
			SDL_Log("Cursor '%s' %ux%u, anchor %s, hotspot (%.1f, %.1f)", path.c_str(),
				unsigned(entry.texture->width()), unsigned(entry.texture->height()),
				Lotte::Gui::cursorAnchorName(metadata.anchor), hotspot.x, hotspot.y);
			cursors.push_back(std::move(entry));
		} catch (const std::exception& error) {
			SDL_Log("Could not load cursor '%s': %s", path.c_str(), error.what());
		}
	}
	activeCursor = 0;
	cursorPosition = glm::fvec2(float(width) * 0.5f, float(height) * 0.5f);
}

void GameSystem::setActiveCursorAnchor(Lotte::Gui::CursorAnchor anchor)
{
	if (cursors.empty()) {
		return;
	}
	CursorEntry& entry = cursors[activeCursor];
	Lotte::Gui::CursorMetadata metadata = entry.cursor.metadata();
	metadata.anchor = anchor;
	entry.cursor.setMetadata(metadata);
	const glm::fvec2 hotspot = entry.cursor.hotspotPixels();
	SDL_Log("Cursor '%s': anchor %s, hotspot (%.1f, %.1f)", entry.path.c_str(),
		Lotte::Gui::cursorAnchorName(anchor), hotspot.x, hotspot.y);
}

void GameSystem::initialize()
{
	// SDL audio devices start paused: without this the callback never runs, so a
	// module the script inserts into the mixer is never pulled.
	sdlDriver->pause(false);
	// The captions come first: the script reads them the moment it runs, and they need
	// neither a GPU nor a font.
	loadLocalization();
	// The theme comes next: the script's `GUI.Button`s are dressed through it, and the
	// scene has to know the atlases before the first widget asks for a skin.
	startTheme();
	startScript();
}

void GameSystem::startTheme()
{
	if (!guiScene) {
		SDL_Log("Theme skipped: graphics is disabled");
		return;
	}
	const std::string text = Sandbox::readVirtualFile(iosys, "gfx/theme_000.json");
	if (text.empty()) {
		SDL_Log("Theme skipped: gfx/theme_000.json is not readable");
		return;
	}
	const Sandbox::ThemeParseResult parsed = Sandbox::parseThemeJson(text);
	for (const std::string& problem : parsed.problems) {
		SDL_Log("Theme: %s", problem.c_str());
	}
	if (!parsed.ok) {
		return;
	}
	std::size_t uploaded = 0;
	for (const Lotte::Gui::ThemeAtlasDefinition& atlas : parsed.definition.atlases) {
		if (atlas.source != Lotte::Gui::ThemeAtlasSource::Procedural) {
			// File atlases need an upload path of their own; the demo document only uses
			// the generated one, so this is a log line rather than a failure.
			SDL_Log("Theme atlas '%s': file source '%s' is not uploaded yet",
					atlas.alias.c_str(), atlas.path.c_str());
			continue;
		}
		const Lotte::Procedural::GuiAtlas built =
			Lotte::Procedural::buildGuiAtlas(Sandbox::atlasSettingsFor(atlas));
		if (!built.image.isValid()) {
			SDL_Log("Theme atlas '%s': the generator produced no image", atlas.alias.c_str());
			continue;
		}
		if (guiScene->addAtlas(atlas.alias, built.image, &built)) ++uploaded;
	}
	// The scene owns the atlases and answers the alias/part questions; the theme is a
	// thin layer over the parsed document that resolves those answers into skins.
	theme = std::make_unique<Lotte::Gui::DefinitionTheme>(
		parsed.definition, [this](std::string_view alias) { return guiScene->resolveAtlas(alias); });
	guiScene->setTheme(theme.get());
	SDL_Log("Theme '%s': %zu atlas(es) uploaded", std::string(theme->name()).c_str(), uploaded);
}

void GameSystem::startScript()
{
	scriptHost = std::make_unique<Sandbox::ScriptHost>(iosys, mixer);
	scriptHost->audio = sdlDriver.get();
	scriptHost->gui = guiScene.get();
	scriptHost->theme = theme.get();
	scriptHost->viewport = glm::uvec2(std::uint32_t(width), std::uint32_t(height));
	scriptHost->localization = &localization;
	scriptHost->soundPlayer = soundPlayer.get();
	scriptHost->config = &config;
	scriptHost->requestQuit = [] { postQuitEvent(); };
	scriptHost->setRichText = [this](const Sandbox::RichTextRequest& request) {
		setRichText(request);
	};
	installLabelLayout();

	jsCore = std::make_unique<JS::Core>(iosys);
	jsCore->insertModule("CONFIG", [this](JSContext& ctx, JS::RootedObject& obj) {
	for( auto it = std::begin(config) ; it != std::end(config) ; ++it ) {
		JS::RootedObject section(&ctx, JS_NewPlainObject(&ctx));
		for( auto zt = std::begin(it->second) ; zt != std::end(it->second) ; ++zt ) {
			switch (zt->second.type) {
				case Euph::Conf::ConfigurationElementType::CONF_BOOL: {
					JS::RootedValue rootedBool(&ctx, zt->second.storage.b ? JS::TrueValue() : JS::FalseValue() );
					JS_DefineProperty(&ctx, section, zt->first.c_str(), rootedBool, JSPROP_READONLY | JSPROP_ENUMERATE);
					break; }
				case Euph::Conf::ConfigurationElementType::CONF_FLOAT: {
					JS::RootedValue rootedFloat(&ctx, JS_NumberValue(zt->second.storage.f) );
					JS_DefineProperty(&ctx, section, zt->first.c_str(), rootedFloat, JSPROP_READONLY | JSPROP_ENUMERATE );
					break;  }
				case Euph::Conf::ConfigurationElementType::CONF_INT: {
					JS::RootedValue rootedInt(&ctx, JS_NumberValue(zt->second.storage.i) );
					JS_DefineProperty(&ctx, section, zt->first.c_str(), rootedInt, JSPROP_READONLY | JSPROP_ENUMERATE );
					break; }
				case Euph::Conf::ConfigurationElementType::CONF_UINT: {
					JS::RootedValue rootedUint(&ctx, JS_NumberValue(zt->second.storage.u) );
					JS_DefineProperty(&ctx, section, zt->first.c_str(), rootedUint, JSPROP_READONLY  | JSPROP_ENUMERATE);
					break; }
				case Euph::Conf::ConfigurationElementType::CONF_STRING: {
					auto strview = zt->second.storage.s.asStringView();
					JS::UTF8Chars uChars( strview.data(), strview.size() );
					JS::RootedString rootedString(&ctx, JS_NewStringCopyUTF8N(&ctx, uChars) );
					JS_DefineProperty(&ctx, section, zt->first.c_str(), rootedString, JSPROP_READONLY | JSPROP_ENUMERATE );
					break; }
				default: break;
			}
		}
		JS_DefineProperty(&ctx,obj, it->first.c_str(), section, JSPROP_READONLY | JSPROP_ENUMERATE);
	}
	});
	jsCore->insertModule("System", [this](JSContext& ctx, JS::RootedObject& obj) {
		JS::RegisterSystemClasses(&ctx, obj, *scriptHost);
	});
	jsCore->insertModule("Audio", [this](JSContext& ctx, JS::RootedObject& obj) {
		JS::RegisterAudioPlayerClasses(&ctx, obj, *scriptHost);
		JS::RegisterAudioSoundClass(&ctx, obj, *scriptHost);
	});
	jsCore->insertModule("GUI", [this](JSContext& ctx, JS::RootedObject& obj) {
		JS::RegisterGuiClasses(&ctx, obj, *scriptHost);
		JS::RegisterGuiSliderClass(&ctx, obj, *scriptHost);
		JS::RegisterGuiLabelClass(&ctx, obj, *scriptHost);
		JS::RegisterGuiLayoutClass(&ctx, obj, *scriptHost);
	});
	jsCore->insertModule("IO", [this](JSContext& ctx, JS::RootedObject& obj) {
		JS::CreateIOClasses(&ctx, obj, *scriptHost);
	});
	jsCore->insertModule("Localization", [this](JSContext& ctx, JS::RootedObject& obj) {
		JS::RegisterLocalizationModule(&ctx, obj, *scriptHost);
	});

	try {
		// `run()` executes `main.js` once. The script declares the scene and
		// returns; its callbacks stay rooted in the context and are invoked from the
		// event handlers, so nothing has to poll JavaScript per frame.
		jsCore->run();
	} catch (const std::exception& error) {
		SDL_Log("Script failed: %s", error.what());
	}

	if (kaldi) {
		// Textures the script created recorded their create/upload commands; hand
		// them to the backend before the first frame opens a render pass.
		const Kld::HalStatus status = kaldi->submit();
		if (!status.ok()) {
			SDL_Log("Script upload failed (%d): %s", static_cast<int>(status.code),
				status.message != nullptr ? status.message : "(no message)");
		}
	}
	SDL_Log("Script ready: %zu button(s)", guiScene ? guiScene->buttonCount() : std::size_t(0));
}

void GameSystem::updateLogic()
{
	// A one-shot voice stops itself when its buffer runs out; somebody has to take it
	// out of the mixer, and that somebody is the frame thread.
	if (soundPlayer) {
		AudioLock lock(sdlDriver.get());
		soundPlayer->update();
	}
}

void GameSystem::render()
{
	if (!kaldi || !guiRenderer) {
		return;
	}
	// New glyphs create/upload atlas pages. On a backend with render passes that
	// has to happen before beginFrame(), so prepare() and submit() are separate.
	if (textRenderer && fontReady) {
		try {
			bool recorded = textRenderer->prepare(sampleLayout);
			if (guiScene) {
				// Labels are prepared with the sample layout's pass: their glyphs have to
				// exist and be uploaded before the frame opens.
				recorded = guiScene->prepareText(*textRenderer) || recorded;
			}
			if (recorded) {
				const Kld::HalStatus upload = kaldi->submit();
				if (!upload.ok()) {
					if (!frameFailureLogged) {
						frameFailureLogged = true;
						SDL_Log("Text upload failed (%d): %s", static_cast<int>(upload.code),
								upload.message != nullptr ? upload.message : "(no message)");
					}
					textRenderer.reset();
					fontRepository.reset();
					fontReady = false;
					sampleLayout.clear();
				}
			}
		} catch (const std::exception& error) {
			SDL_Log("Text disabled: %s", error.what());
			textRenderer.reset();
			fontRepository.reset();
			fontReady = false;
			sampleLayout.clear();
		}
	}
	const Kld::HalStatus beginStatus = kaldi->beginFrame();
	if (!beginStatus.ok()) {
		if (!frameFailureLogged) {
			frameFailureLogged = true;
			SDL_Log("Kaldi beginFrame failed (%d): %s", static_cast<int>(beginStatus.code),
				beginStatus.message != nullptr ? beginStatus.message : "(no message)");
		}
		return;
	}
	guiRenderer->beginFrame();
	// The widgets first, then the text on top of them, then the cursor: the same
	// layering the hard-coded demo used, now driven by what the script created.
	if (guiScene) {
		guiScene->render(*guiRenderer);
	}
	// The GUI renderer batches geometry and emits it in `flush()`. The text pass records
	// into the *same* command buffer and binds its own pipeline, so the widget batch has
	// to be emitted here: without this flush every widget would be drawn after - and
	// therefore on top of - the text, which hides exactly the captions that sit on a
	// button. (`GuiRenderer::flush()` is documented to be callable at any time.)
	guiRenderer->flush();
	if (textRenderer && fontReady) {
		try {
			textRenderer->setViewportSize(std::uint16_t(width), std::uint16_t(height));
			textRenderer->beginFrame();
			// The scene's labels are ordinary text blocks from the text pipeline's point
			// of view: they were laid out and prepared before the frame opened, so this
			// pass only queues them and one flush covers all of them.
			if (guiScene) {
				guiScene->renderText(*textRenderer);
			}
			const std::size_t glyphs = textRenderer->render(sampleLayout);
			const std::size_t runs = textRenderer->queuedRunCount();
			textRenderer->flush();
			if (!textRendered && glyphs > 0) {
				textRendered = true;
				Lotte::Text::FontAtlas* atlas =
					fontRepository->atlasFor(fontRepository->defaultFaceId());
				SDL_Log("Text: %zu glyphs, %zu atlas glyphs in %zu pages, %zu draw runs",
						glyphs, atlas != nullptr ? atlas->glyphCount() : 0,
						atlas != nullptr ? atlas->pageCount() : 0, runs);
			}
		} catch (const std::exception& error) {
			SDL_Log("Text disabled: %s", error.what());
			textRenderer.reset();
			fontRepository.reset();
			fontReady = false;
			sampleLayout.clear();
		}
	}
	if (!cursors.empty()) {
		cursors[activeCursor].cursor.render(*guiRenderer, cursorPosition);
	}
	guiRenderer->flush();
	const Kld::HalStatus status = kaldi->endFrame();
	if (!status.ok() && !frameFailureLogged) {
		// `status.message` is backend storage: log it now instead of keeping it.
		frameFailureLogged = true;
		SDL_Log("Kaldi frame failed (%d): %s", static_cast<int>(status.code),
			status.message != nullptr ? status.message : "(no message)");
	}
}

void GameSystem::handleAudioDeviceEvent(const SDL_AudioDeviceEvent& event)
{
}

void GameSystem::handleControllerAxisEvent(const SDL_ControllerAxisEvent& event)
{
}

void GameSystem::handleControllerButtonEvent(const SDL_ControllerButtonEvent& event)
{
}

void GameSystem::handleControllerDeviceEvent(const SDL_ControllerDeviceEvent& event)
{
}

void GameSystem::handleDollarGestureEvent(const SDL_DollarGestureEvent& event)
{
}

void GameSystem::handleDropEvent(const SDL_DropEvent& event)
{
}

void GameSystem::handleTouchFingerEvent(const SDL_TouchFingerEvent& event)
{
}

void GameSystem::handleKeyboardEvent(const SDL_KeyboardEvent& event)
{
	// Escape is the "abort" of a playground session; it goes through the same path as
	// the Exit menu entry so the configuration still gets written.
	if (event.keysym.sym == SDLK_ESCAPE) postQuitEvent();
	if (event.type != SDL_KEYDOWN || event.repeat != 0) return;
	if (event.keysym.sym == SDLK_TAB && !cursors.empty()) {
		activeCursor = (activeCursor + 1) % cursors.size();
		SDL_Log("Cursor switched to '%s'", cursors[activeCursor].path.c_str());
		return;
	}
	if (const std::optional<Lotte::Gui::CursorAnchor> anchor =
			anchorForNumberKey(event.keysym.sym)) {
		setActiveCursorAnchor(*anchor);
	}
}

void GameSystem::handleJoyAxisEvent(const SDL_JoyAxisEvent& event)
{
}

void GameSystem::handleJoyBallEvent(const SDL_JoyBallEvent& event)
{
}

void GameSystem::handleJoyHatEvent(const SDL_JoyHatEvent& event)
{
}

void GameSystem::handleJoyButtonEvent(const SDL_JoyButtonEvent& event)
{
}

void GameSystem::handleJoyDeviceEvent(const SDL_JoyDeviceEvent& event)
{
}

void GameSystem::handleMouseMotionEvent(const SDL_MouseMotionEvent& event)
{
	cursorPosition = glm::fvec2(float(event.x), float(event.y));
	if (guiScene) {
		guiScene->setMousePosition(cursorPosition);
	}
}

void GameSystem::handleMouseButtonEvent(const SDL_MouseButtonEvent& event)
{
	cursorPosition = glm::fvec2(float(event.x), float(event.y));
	// Only the primary button activates a widget; the scene stays backend-neutral
	// and receives the raw index for the widget that wants to know.
	if (!guiScene || event.button != SDL_BUTTON_LEFT) {
		return;
	}
	const bool down = event.type == SDL_MOUSEBUTTONDOWN;
	guiScene->setMouseButton(cursorPosition, event.button, down, event.clicks);
}

void GameSystem::handleMouseWheelEvent(const SDL_MouseWheelEvent& event)
{
}

void GameSystem::handleMultiGestureEvent(const SDL_MultiGestureEvent& event)
{
}

void GameSystem::handleQuitEvent(const SDL_QuitEvent& event)
{
	// The window was closed (or `System.quit()` posted a quit event): leave the loop
	// without tearing SDL down, which is `AppSystem::run()`'s job once it returns.
	(void)event;
}

void GameSystem::handleTextEditingEvent(const SDL_TextEditingEvent& event)
{
}

void GameSystem::handleTextInputEvent(const SDL_TextInputEvent& event)
{
}

void GameSystem::handleUserEvent(const SDL_UserEvent& event)
{
}

void GameSystem::handleWindowEvent(const SDL_WindowEvent& event)
{
	// The engine cursor only replaces the system pointer while the window is
	// focused; losing focus must give the pointer back so the desktop stays usable.
	switch (event.event) {
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			if (kaldi && guiRenderer) {
				SDL_ShowCursor(SDL_DISABLE);
			}
			break;
		case SDL_WINDOWEVENT_FOCUS_LOST:
			SDL_ShowCursor(SDL_ENABLE);
			break;
		default:
			break;
	}
}