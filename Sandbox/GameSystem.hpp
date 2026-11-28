#ifndef GAMESYSTEM_HPP
#define GAMESYSTEM_HPP
#include "AppSystem.hpp"
#include <Elvavena/Io/ElvIoSystem.hpp>
#include <Euphemy/Config/EuphConfiguration.hpp>
#include <Euphemy/Localization/EuphLocalization.hpp>
#include "HarukaContext.hpp"
#include <Haruka/Core/HrkMixer.hpp>
#include <Haruka/Core/HrkSoundPlayer.hpp>
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

namespace JS {
class Core;
}

namespace Sandbox {
class GuiScene;
struct RichTextRequest;
struct ScriptHost;
}

namespace Lotte {
namespace Gui {
class DefinitionTheme;
}
}

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
	/**
	 * @brief Plays the short sounds a script starts.
	 *
	 * Declared right after the mixer because it unregisters its voices from it when it
	 * is destroyed: the mixer's child map stores raw `Playable*`, so a voice must never
	 * outlive the mixer that pulls it.
	 */
	Hrk::sSoundPlayer soundPlayer;
	/**
	 * @brief The loaded captions, in the language the configuration asked for.
	 *
	 * Filled by @ref loadLocalization() and read by the `Localization` module and by
	 * `GUI.Label`; empty when no localization file was readable, which turns every
	 * caption into its own key rather than into an empty string.
	 */
	Euph::Loc::Localization localization;
	std::unique_ptr<Haruka_SDL_Driver> sdlDriver;
	/**
	 * @brief Engine services the script modules talk to.
	 *
	 * Declared before the context and the scene: module finalizers run while the
	 * context dies and still reach the mixer and the audio device through it.
	 */
	std::unique_ptr<Sandbox::ScriptHost> scriptHost;
	/**
	 * @brief The script context.
	 *
	 * The script is executed on this thread, not a worker one: a click handler
	 * runs from the SDL event loop and has to observe and mutate the same state
	 * the frame does. A blocking script pauses the frame, which is an accepted
	 * property of the playground (see `README.md`).
	 */
	std::unique_ptr<JS::Core> jsCore;

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
	/**
	 * @brief The theme the script's widgets are drawn through, or null.
	 *
	 * Declared before the scene so it is destroyed *after* the widgets: a widget only
	 * borrows the theme (it queries it every frame), so the theme must outlive them.
	 */
	std::unique_ptr<Lotte::Gui::DefinitionTheme> theme;
	/**
	 * @brief Widgets and textures the script created, or null without graphics.
	 *
	 * Destroyed before the script context: a widget holds a JavaScript root through
	 * its click callback.
	 */
	std::unique_ptr<Sandbox::GuiScene> guiScene;
	/** @brief Owns the loaded faces and their lazy glyph atlases. */
	std::unique_ptr<Lotte::Text::FontRepository> fontRepository;
	/** @brief Records SDF text into the context's dispatcher. */
	std::unique_ptr<Lotte::Text::TextRenderer> textRenderer;
	/** @brief Layout of the text the script last requested; empty until it does. */
	Lotte::Text::TextLayout sampleLayout;
	/** @brief True once a font and the text renderer are available. */
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
	/** @brief Loads the `.wodf` face and creates the text renderer; lays out no text. */
	void loadFont();
	/** @brief Decodes the cursor PNGs and builds their @ref Lotte::Gui::Cursor objects. */
	void loadCursors();
	/** @brief Replaces the active cursor's anchor and re-resolves its origin. */
	void setActiveCursorAnchor(Lotte::Gui::CursorAnchor anchor);
	/** @brief Creates the script host and context, and runs `main.js`. */
	void startScript();
	/** @brief Loads the theme document, uploads its atlases and installs the theme. */
	void startTheme();
	/** @brief Reads the configured language's localization files into @ref localization. */
	void loadLocalization();
	/** @brief Installs the label layout producer that uses the loaded fonts. */
	void installLabelLayout();
	/** @brief Releases the widgets, then the context, then the host. */
	void stopScript();
public:
	GameSystem(Euph::Conf::Configuration& config, Elv::Io::System& iosys);
	/** @brief Releases the script before the window and the audio device go away. */
	~GameSystem() override;

	/**
	 * @brief Lays out and replaces the rendered rich text.
	 * @param request MML source and layout options a script asked for.
	 * @throws std::runtime_error When no font or text renderer is available.
	 */
	void setRichText(const Sandbox::RichTextRequest& request);

	/** @brief The `AppSystem` callbacks this system fills in. */
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
