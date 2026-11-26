#include "GameSystem.hpp"
#include <Euphemy/Media/Image/EuphPNG.hpp>
#include <Lotte/Text/LteMml.hpp>
#include <Lotte/Text/LteRichText.hpp>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <optional>
#include <string_view>
#include <utility>
#include "GuiScene.hpp"
#include "ScriptHost.hpp"
#include "JS/JsCore.hpp"
#include "JS/Wrappers/Audio/JsAudioPlayer.hpp"
#include "JS/Wrappers/GUI/JsGuiWrappers.hpp"
#include "JS/Wrappers/Io/JsIO.hpp"
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
				0), iosys(iosys), config(config)
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
	sdlDriver->setPlayable(mixer);
	startGraphics();
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
	// Order matters. A widget's click callback owns a JavaScript root, so the
	// widgets have to die while the context is still usable; the context has to
	// die before the renderer and the resource factory its textures point into;
	// and the host has to outlive the context, because module finalizers reach the
	// mixer and the audio device through it.
	guiScene.reset();
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
	startScript();
}

void GameSystem::startScript()
{
	scriptHost = std::make_unique<Sandbox::ScriptHost>(iosys, mixer);
	scriptHost->audio = sdlDriver.get();
	scriptHost->gui = guiScene.get();
	scriptHost->viewport = glm::uvec2(std::uint32_t(width), std::uint32_t(height));
	scriptHost->setRichText = [this](const Sandbox::RichTextRequest& request) {
		setRichText(request);
	};

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
	});
	jsCore->insertModule("GUI", [this](JSContext& ctx, JS::RootedObject& obj) {
		JS::RegisterGuiClasses(&ctx, obj, *scriptHost);
	});
	jsCore->insertModule("IO", [this](JSContext& ctx, JS::RootedObject& obj) {
		JS::CreateIOClasses(&ctx, obj, *scriptHost);
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
			if (textRenderer->prepare(sampleLayout)) {
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
	if (textRenderer && fontReady) {
		try {
			textRenderer->setViewportSize(std::uint16_t(width), std::uint16_t(height));
			textRenderer->beginFrame();
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
	if (event.keysym.sym == SDLK_ESCAPE) SDL_Quit();
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
	SDL_Quit();
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