#ifndef SANDBOX_SCRIPTHOST_HPP
#define SANDBOX_SCRIPTHOST_HPP
/**
 * @file ScriptHost.hpp
 * @brief The engine services a sandbox script is allowed to touch.
 *
 * The JavaScript wrappers in `Sandbox/JS/Wrappers/` are deliberately kept away
 * from `GameSystem`: they receive this narrow facade instead. That is what makes
 * the scripting layer replaceable - swapping SpiderMonkey for another engine only
 * touches the wrappers, and what a script can do is answered by this struct
 * rather than by whichever members `GameSystem` happens to have.
 *
 * Every field is non-owning. The host is alive exactly as long as the script
 * context is, which is what lets a finalizer stop playback or release a widget
 * without guessing whether the engine half still exists.
 */
#include "GuiScene.hpp"
#include <Euphemy/Config/EuphConfiguration.hpp>
#include <Euphemy/Localization/EuphLocalization.hpp>
#include <Haruka/Core/HrkMixer.hpp>
#include <Haruka/Core/HrkSoundPlayer.hpp>
#include <Elvavena/Io/ElvIoSystem.hpp>
#include <Lotte/Gui/LteTheme.hpp>
#include <glm/glm.hpp>
#include <functional>
#include <string>
#include <utility>

class Haruka_SDL_Driver;

namespace Sandbox {

/** @brief A rich-text block handed to the host by `System.setRichText`. */
struct RichTextRequest {
	/** @brief MML source, the same dialect `GameSystem::loadFont()` used to hard-code. */
	std::string mml;
	/** @brief Layout origin in window pixels; `y` is the first baseline. */
	glm::fvec2 origin = glm::fvec2(0.0f);
	/** @brief Extra gap between lines, in pixels. */
	float spacing = 0.0f;
	/** @brief Extra gap after every space, in pixels. */
	float wordSpacing = 0.0f;
	/** @brief Global scale applied on top of every run's size. */
	float scale = 1.0f;
};

/**
 * @brief Engine services exposed to the script modules.
 *
 * A field is null when the corresponding half of the sandbox is unavailable (no
 * GPU, no audio device); the wrappers turn that into a JavaScript error instead
 * of dereferencing it.
 */
struct ScriptHost {
	/** @brief Filesystem the script reads assets from. */
	Elv::Io::System& io;
	/** @brief Mixer the `Audio` module inserts playables into. */
	Hrk::sMixer mixer;
	/** @brief SDL audio device; its lock serializes mixer mutations with the callback. */
	Haruka_SDL_Driver* audio = nullptr;
	/** @brief The widget scene, or null when the renderer did not start. */
	GuiScene* gui = nullptr;
	/**
	 * @brief The theme widgets are drawn through, or null when none was loaded.
	 *
	 * Owned by the application; the scene and the widgets only borrow it, and it has to
	 * outlive every widget that draws through it (see `GameSystem::stopScript`).
	 */
	const Lotte::Gui::Theme* theme = nullptr;
	/** @brief Window size in pixels, so a script can lay out relative to it. */
	glm::uvec2 viewport = glm::uvec2(0);
	/** @brief Replaces the rendered rich text; see @ref RichTextRequest. */
	std::function<void(const RichTextRequest&)> setRichText;

	/**
	 * @brief The localization table the script reads its captions from, or null.
	 *
	 * Null means "no strings were loaded"; the `Localization` module is then empty and
	 * a script that indexes it gets `undefined`, which is exactly what a fallback chain
	 * in JavaScript wants.
	 */
	const Euph::Loc::Localization* localization = nullptr;
	/**
	 * @brief The one-shot player the `Audio.Sound` objects feed, or null.
	 *
	 * Null when the mixer could not be created (no audio device); the wrapper then
	 * reports every sound as invalid instead of throwing from `play()`.
	 */
	Hrk::SoundPlayer* soundPlayer = nullptr;
	/**
	 * @brief The configuration a script may write settings into, or null.
	 *
	 * Writing rather than replacing is what makes the settings survive a restart: the
	 * application serialises this object back to `config.ini` when it exits.
	 */
	Euph::Conf::Configuration* config = nullptr;
	/**
	 * @brief Asks the application to leave the main loop.
	 *
	 * Set by the application, because only it knows what "quit" means for its event
	 * loop; an empty function turns `System.quit()` into a no-op.
	 */
	std::function<void()> requestQuit;

	/**
	 * @brief Creates the facade.
	 * @param io Filesystem the script reads assets from.
	 * @param mixer Mixer the audio wrappers feed.
	 */
	ScriptHost(Elv::Io::System& io, Hrk::sMixer mixer) : io(io), mixer(std::move(mixer)) {}

	ScriptHost(const ScriptHost&) = delete;
	ScriptHost& operator=(const ScriptHost&) = delete;
};

} // namespace Sandbox

#endif // SANDBOX_SCRIPTHOST_HPP
