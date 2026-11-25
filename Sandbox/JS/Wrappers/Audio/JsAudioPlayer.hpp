#ifndef JSAUDIOPLAYER_HPP
#define JSAUDIOPLAYER_HPP
/**
 * @file JsAudioPlayer.hpp
 * @brief Declares the `Audio` module: module-file playback the script controls.
 *
 * The legacy wrapper exposed a whole graph (`Buffer`, `SoundSource`, `Streamer`,
 * `Mixer`). None of those types survive in Haruka: a module file is one
 * `Hrk::ModuleStreamer` that the sandbox's mixer already owns a slot in. So the
 * module exposes the one object a sandbox script actually needs - a playing
 * module with a transport - and the mixer stays engine-owned.
 */
#include <JS/JsUtil.hpp>

namespace Sandbox {
class ScriptHost;
}

namespace JS {

/**
 * @brief Registers the `Audio` module on a module object.
 * @param cx Context that owns every object created here.
 * @param global Module object to define the classes on.
 * @param host Engine services the module talks to; must outlive the context.
 */
void RegisterAudioPlayerClasses(JSContext* cx, const JS::HandleObject& global,
								Sandbox::ScriptHost& host);

}

#endif // JSAUDIOPLAYER_HPP
