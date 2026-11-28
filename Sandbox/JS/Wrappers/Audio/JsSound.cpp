#include "JsSound.hpp"
#include <ScriptHost.hpp>
#include <HarukaContext.hpp>
#include <Euphemy/Media/Audio/EuphSoundClip.hpp>
#include <Haruka/Core/HrkSoundPlayer.hpp>
#include <Haruka/IO/HrkSoundBuffer.hpp>
#include <SDL2/SDL_log.h>
#include <js/Object.h>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <utility>

namespace JS {

namespace {

/// @brief The host the registered functions talk to; see `JsSystem.cpp`.
Sandbox::ScriptHost* gHost = nullptr;

/// @brief The `Audio.Sound` wrapper class.
ClassCreator SoundClass;

/// @brief Native half of one sound: its prepared buffer and the gain it plays at.
struct SoundHandle {
	/** @brief Mixer-ready buffer; invalid when the file was missing or unreadable. */
	Hrk::SoundBufferHandle buffer;
	/** @brief Linear gain applied to each voice this sound starts. */
	float volume = 1.0f;
	/** @brief Length of the source clip in seconds, for a script that wants to pace it. */
	double seconds = 0.0;
};

/// @brief Reads a numeric property, falling back when it is missing or not a number.
double numberProperty(JSContext* cx, JS::HandleObject object, const char* name, double fallback)
{
	JS::RootedValue value(cx);
	if (!JS_GetProperty(cx, object, name, &value) || !value.isNumber()) {
		return fallback;
	}
	return value.toNumber();
}

/// @brief The sound behind `this`, never null.
SoundHandle* soundFromThis(JS::CallArgs& args)
{
	SoundHandle* handle = getNativePointer<SoundHandle>(args.thisv().toObjectOrNull());
	if (handle == nullptr) {
		throw std::runtime_error("Audio.Sound: the sound is not valid");
	}
	return handle;
}

/// @brief Decodes a file and conforms it to the mixer's format.
Hrk::SoundBufferHandle prepareSound(Sandbox::ScriptHost& host, const std::string& path,
									double& seconds)
{
	Euph::Media::Audio::SoundClip clip;
	{
		Elv::Io::uDevice device;
		try {
			device = host.io.openUnique(path.c_str(), Elv::Io::Mode::READ);
		} catch (const std::exception& error) {
			SDL_Log("Sound '%s' unavailable: %s", path.c_str(), error.what());
			return Hrk::SoundBufferHandle();
		}
		if (!device || !device->isValid()) {
			// A missing file is a normal state, not a failure: the script asked for a
			// click that this installation simply does not have.
			SDL_Log("Sound '%s' not found; it stays silent", path.c_str());
			return Hrk::SoundBufferHandle();
		}
		clip = Euph::Media::Audio::SoundClip::load(std::move(device));
	}
	seconds = clip.duration();
	// `Hrk::SoundSource::outputTo()` throws on a channel or rate mismatch and it runs
	// inside the audio callback, so the conversion has to happen here, once.
	const Hrk::ChannelCount channels = host.mixer ? host.mixer->getChannelCount()
												  : Hrk::ChannelCount(2);
	const Hrk::SampleRate rate = host.mixer ? host.mixer->getSampleRate() : Hrk::SampleRate(48000);
	if (clip.conformTo(channels, rate)) {
		SDL_Log("Sound '%s': conformed to %u channel(s) at %u Hz", path.c_str(),
				unsigned(channels.var), unsigned(rate.var));
	}
	return Hrk::makeSoundBuffer(clip.data(), clip.frameCount(), clip.channels(),
								clip.sampleRate());
}

/// @brief `new Audio.Sound(path, options)`.
bool js_audio_sound_constructor(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		Sandbox::ScriptHost* host = gHost;
		if (host == nullptr) {
			throw std::runtime_error("Audio.Sound: the audio system is not available");
		}
		if (argc < 1) {
			throw std::runtime_error("Audio.Sound: expected a file path");
		}
		const std::string path = JsToNativeString(cx, args.get(0).toString());
		auto handle = std::make_unique<SoundHandle>();
		if (argc >= 2 && args.get(1).isObject()) {
			JS::RootedObject options(cx, &args.get(1).toObject());
			handle->volume = static_cast<float>(numberProperty(cx, options, "volume", 1.0));
		}
		try {
			handle->buffer = prepareSound(*host, path, handle->seconds);
		} catch (const std::exception& error) {
			SDL_Log("Sound '%s' could not be decoded: %s", path.c_str(), error.what());
			handle->buffer = Hrk::SoundBufferHandle();
		}
		JS::RootedObject object(cx,
			JS_NewObjectWithGivenProto(cx, &SoundClass.protoClass, *SoundClass.prototype));
		setNativePointer(object, handle.release());
		args.rval().setObject(*object);
		return true;
	}, cx, argc, vp);
}

/// @brief Finalizer of `Audio.Sound`: releases the buffer.
void js_audio_sound_finalizer(JS::GCContext* gcx, JSObject* obj)
{
	finalizeNative<SoundHandle>(gcx, obj);
}

} // namespace

namespace {

/// @brief `sound.play()`: starts one voice.
bool js_sound_play(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		SoundHandle* handle = soundFromThis(args);
		bool started = false;
		if (handle->buffer.valid() && gHost != nullptr && gHost->soundPlayer != nullptr) {
			// Mutating the mixer's child map races the audio callback without the lock.
			AudioLock lock(gHost->audio);
			started = gHost->soundPlayer->play(handle->buffer, handle->volume);
		}
		setReturnValue(cx, args.rval(), started);
		return true;
	}, cx, argc, vp);
}

/// @brief `sound.valid`.
bool js_sound_get_valid(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), soundFromThis(args)->buffer.valid());
		return true;
	}, cx, argc, vp);
}

/// @brief `sound.volume`.
bool js_sound_get_volume(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), soundFromThis(args)->volume);
		return true;
	}, cx, argc, vp);
}

/// @brief `sound.volume = value`.
bool js_sound_set_volume(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext*, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		soundFromThis(args)->volume = static_cast<float>(args.get(0).toNumber());
		args.rval().set(args.get(0));
		return true;
	}, cx, argc, vp);
}

/// @brief `sound.duration`.
bool js_sound_get_duration(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), soundFromThis(args)->seconds);
		return true;
	}, cx, argc, vp);
}

} // namespace

void RegisterAudioSoundClass(JSContext* cx, const JS::HandleObject& global, Sandbox::ScriptHost& host)
{
	gHost = &host;

	std::memset(&SoundClass.protoClass, 0, sizeof(JSClass));
	std::memset(&SoundClass.ops, 0, sizeof(JSClassOps));
	SoundClass.name = "Sound";
	SoundClass.nargs = 1;
	SoundClass.constructor = js_audio_sound_constructor;
	SoundClass.ops.construct = js_audio_sound_constructor;
	SoundClass.ops.finalize = js_audio_sound_finalizer;
	SoundClass.protoClass.cOps = &SoundClass.ops;
	SoundClass.protoClass.name = "Sound";
	// Foreground finalization: the buffer's slab is process-allocated and a live voice
	// may still be reading it.
	SoundClass.protoClass.flags = JSCLASS_HAS_RESERVED_SLOTS(1);
	SoundClass.prototype = std::make_unique<JS::RootedObject>(cx,
		JS_NewObject(cx, &SoundClass.protoClass));
	SoundClass.ps.push_back(JS_PSG("valid", js_sound_get_valid, JSPROP_ENUMERATE));
	SoundClass.ps.push_back(JS_PSGS("volume", js_sound_get_volume, js_sound_set_volume,
		JSPROP_ENUMERATE));
	SoundClass.ps.push_back(JS_PSG("duration", js_sound_get_duration, JSPROP_ENUMERATE));
	SoundClass.ps.push_back(JS_PS_END);
	SoundClass.static_ps.push_back(JS_PS_END);
	SoundClass.fs.push_back(JS_FN("play", js_sound_play, 0, 0));
	SoundClass.fs.push_back(JS_FS_END);
	SoundClass.static_fs.push_back(JS_FS_END);
	SoundClass.registerToJS(cx, global, *SoundClass.prototype);
}

} // namespace JS
