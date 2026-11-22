#include "GameSystem.hpp"
#include <Euphemy/Io/EuphFile.hpp>
#include <Euphemy/Media/Image/EuphPNG.hpp>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <optional>
#include <string_view>
#include <utility>

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


GameSystem::GameSystem(Euph::Conf::Configuration& config)
	: AppSystem(std::string(config.getString("Game", "sTitle", "GameDemo")),
				0,
				0,
				config.getUInteger("Video","uWidth", 640),
				config.getUInteger("Video","uHeight", 480),
				0), config(config)
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
		cursors.clear();
		guiRenderer.reset();
		kaldi.reset();
		SDL_Log("Graphics disabled: %s", error.what());
	}
}

GameSystem::~GameSystem()
{
	SDL_ShowCursor(SDL_ENABLE);
}

void GameSystem::loadCursors()
{
	const std::string primary { config.getString("Graphics", "sCursor", "cursors/wodmouse.png") };
	const std::vector<std::string> paths = { primary, "cursors/hourglass2.png" };
	for (const std::string& path : paths) {
		Euph::Io::uFile file = Elv::Util::pmr_make_unique<Euph::Io::File>(
			std::pmr::get_default_resource(), path.c_str(), Elv::Io::Mode::READ);
		if (!file->isValid()) {
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
	std::string modpath = std::string(config.getString("Game", "sSongPath", "/home/legacy/zene/GameMusic/Unreal Tournament/mod/Magnus.it"));
	Euph::Io::uFile file = Elv::Util::pmr_make_unique<Euph::Io::File>(std::pmr::get_default_resource(), modpath.c_str(), Elv::Io::Mode::READ);
	if(file->isValid()) {
		modplayer = Elv::Util::pmr_make_shared<Hrk::ModuleStreamer>(std::pmr::get_default_resource(),
																	std::move(file));
		mixer->getChildren().emplace(modplayer.get(), config.getFloat("Game","fModVolume",0.10F));
		modplayer->getState().setCursor(Hrk::FrameIndex(0));
		modplayer->getState().setPlayStatus(Hrk::PlayStatus::PLAYING);
	} else {
		SDL_Log("Could not open module file: %s", modpath.c_str());
	}
	// SDL audio devices start paused: without this the callback never runs,
	// so nothing ever reaches ModuleStreamer::decodeFrames().
	sdlDriver->pause(false);
}

void GameSystem::updateLogic()
{
}

void GameSystem::render()
{
	if (!kaldi || !guiRenderer) {
		return;
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
}

void GameSystem::handleMouseButtonEvent(const SDL_MouseButtonEvent& event)
{
	cursorPosition = glm::fvec2(float(event.x), float(event.y));
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