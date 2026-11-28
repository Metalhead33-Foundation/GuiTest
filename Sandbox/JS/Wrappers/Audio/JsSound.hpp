#ifndef JSSOUND_HPP
#define JSSOUND_HPP
/**
 * @file JsSound.hpp
 * @brief Declares `Audio.Sound`: a short clip the script plays on demand.
 *
 * The wrapper owns the *prepared* audio, not the decoding strategy: Euphemy decodes
 * the file and conforms it to the mixer's channel count and rate once, Haruka wraps
 * the result in a sound buffer, and `Hrk::SoundPlayer` starts a voice per `play()`.
 * A missing file is not an error - the object reports `valid === false` and `play()`
 * does nothing, which is what "play a click if one exists" needs.
 */
#include <JS/JsUtil.hpp>

namespace Sandbox {
class ScriptHost;
}

namespace JS {

/**
 * @brief Adds the `Sound` class to an existing `Audio` module object.
 * @param cx Context that owns every object created here.
 * @param global Module object to define the class on.
 * @param host Engine services the class talks to; must outlive the context.
 */
void RegisterAudioSoundClass(JSContext* cx, const JS::HandleObject& global, Sandbox::ScriptHost& host);

}

#endif // JSSOUND_HPP
