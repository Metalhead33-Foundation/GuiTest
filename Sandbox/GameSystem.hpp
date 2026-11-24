#ifndef GAMESYSTEM_HPP
#define GAMESYSTEM_HPP
#include "AppSystem.hpp"
#include <Elvavena/Io/ElvIoSystem.hpp>
#include <Euphemy/Config/EuphConfiguration.hpp>
#include "HarukaContext.hpp"
#include <Haruka/Core/HrkMixer.hpp>
#include <Haruka/IO/HrkModuleStreamer.hpp>
#include <Lotte/GFX/LteKaldiContext.hpp>
#include <Lotte/GFX/LteWriteableTexture2D.hpp>
#include <Lotte/Gui/LteCursor.hpp>
#include <Lotte/Gui/LteKaldiGuiRenderer.hpp>
#include <Lotte/Text/LteFontRepository.hpp>
#include <Lotte/Text/LteTextLayout.hpp>
#include <Lotte/Text/LteTextRenderer.hpp>
#include <memory>
#include <string>
#include <vector>

class GameSystem : public AppSystem
{
private:
	/**
	 * @brief One loadable cursor: its file, its GPU texture and its origin logic.
	 *
	 * The texture is declared before the cursor so the cursor (which only stores a
	 * raw handle) is destroyed first and never outlives the handle it names.
	 */
	struct CursorEntry {
		std::string path;
		std::unique_ptr<Lotte::Gfx::WriteableTexture2D> texture;
		Lotte::Gui::Cursor cursor;
	};

	Elv::Io::System& iosys;
	Euph::Conf::Configuration& config;
	Hrk::sMixer mixer;
	Hrk::sModuleStreamer modplayer;
	std::unique_ptr<Haruka_SDL_Driver> sdlDriver;
	/**
	 * @brief The Lotte rendering context, or null when the backend could not start.
	 *
	 * Declared before every object that owns a Kaldi proxy: the renderer's buffers,
	 * pipelines, sampler and the cursor textures record their destroy commands into
	 * this context's dispatcher, so they must be released before it.
	 */
	std::unique_ptr<Lotte::Gfx::KaldiContext> kaldi;
	/** @brief Records GUI geometry into the context's dispatcher. */
	std::unique_ptr<Lotte::Gui::KaldiGuiRenderer> guiRenderer;
	/** @brief Owns the loaded faces and their lazy glyph atlases. */
	std::unique_ptr<Lotte::Text::FontRepository> fontRepository;
	/** @brief Records SDF text into the context's dispatcher. */
	std::unique_ptr<Lotte::Text::TextRenderer> textRenderer;
	/** @brief Layout of the sandbox's sample text. */
	Lotte::Text::TextLayout sampleLayout;
	/** @brief True once the sample font has been loaded and laid out. */
	bool fontReady = false;
	/** @brief True once the first text frame has been rendered, for the one-shot log. */
	bool textRendered = false;
	/** @brief The cursors loaded from `cursors/`; Tab cycles, 1-9 set the anchor. */
	std::vector<CursorEntry> cursors;
	/** @brief Index into @ref cursors of the cursor being drawn. */
	std::size_t activeCursor = 0;
	/** @brief Last known mouse position in window pixels. */
	glm::fvec2 cursorPosition = glm::fvec2(0.0f);
	/** @brief Linear RGBA the window is cleared to every frame (`[Graphics]`). */
	float clearColor[4] = { 0.05f, 0.06f, 0.09f, 1.0f };
	/** @brief True once a frame has failed, so the log is not spammed per frame. */
	bool frameFailureLogged = false;

	/** @brief Loads the backend, the GUI renderer and the cursor textures. */
	void startGraphics();
	/** @brief Loads the sample `.wodf` and lays out the demo text. */
	void loadFont();
	/** @brief Decodes the cursor PNGs and builds their @ref Lotte::Gui::Cursor objects. */
	void loadCursors();
	/** @brief Replaces the active cursor's anchor and re-resolves its origin. */
	void setActiveCursorAnchor(Lotte::Gui::CursorAnchor anchor);
public:
	GameSystem(Euph::Conf::Configuration& config, Elv::Io::System& iosys);
	/** @brief Shows the system pointer again before the window goes away. */
	~GameSystem() override;

	// AppSystem interface
protected:
	void initialize() override;
	void updateLogic() override;
	void render() override;
	void handleAudioDeviceEvent(const SDL_AudioDeviceEvent& event) override;
	void handleControllerAxisEvent(const SDL_ControllerAxisEvent& event) override;
	void handleControllerButtonEvent(const SDL_ControllerButtonEvent& event) override;
	void handleControllerDeviceEvent(const SDL_ControllerDeviceEvent& event) override;
	void handleDollarGestureEvent(const SDL_DollarGestureEvent& event) override;
	void handleDropEvent(const SDL_DropEvent& event) override;
	void handleTouchFingerEvent(const SDL_TouchFingerEvent& event) override;
	void handleKeyboardEvent(const SDL_KeyboardEvent& event) override;
	void handleJoyAxisEvent(const SDL_JoyAxisEvent& event) override;
	void handleJoyBallEvent(const SDL_JoyBallEvent& event) override;
	void handleJoyHatEvent(const SDL_JoyHatEvent& event) override;
	void handleJoyButtonEvent(const SDL_JoyButtonEvent& event) override;
	void handleJoyDeviceEvent(const SDL_JoyDeviceEvent& event) override;
	void handleMouseMotionEvent(const SDL_MouseMotionEvent& event) override;
	void handleMouseButtonEvent(const SDL_MouseButtonEvent& event) override;
	void handleMouseWheelEvent(const SDL_MouseWheelEvent& event) override;
	void handleMultiGestureEvent(const SDL_MultiGestureEvent& event) override;
	void handleQuitEvent(const SDL_QuitEvent& event) override;
	void handleTextEditingEvent(const SDL_TextEditingEvent& event) override;
	void handleTextInputEvent(const SDL_TextInputEvent& event) override;
	void handleUserEvent(const SDL_UserEvent& event) override;
	void handleWindowEvent(const SDL_WindowEvent& event) override;
};

#endif // GAMESYSTEM_HPP
