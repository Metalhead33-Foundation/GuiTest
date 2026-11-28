#include "JsAudioPlayer.hpp"
#include <ScriptHost.hpp>
#include <HarukaContext.hpp>
#include <Haruka/IO/HrkModuleStreamer.hpp>
#include <js/Object.h>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace JS {

namespace {

/// @brief The host the registered functions talk to; see `JsSystem.cpp`.
Sandbox::ScriptHost* gHost = nullptr;

/// @brief The `Audio.Module` wrapper class.
ClassCreator ModuleClass;

/**
 * @brief Native half of one `Audio.Module`: a streamer plus its mixer gain.
 *
 * The gain is cached because `Hrk::Mixer` stores it in its child map; keeping a
 * copy lets the `volume` getter answer without a map lookup and lets the setter
 * write back the thing the mixer actually reads.
 */
struct ModuleHandle {
	/** @brief The playing module. */
	Hrk::sModuleStreamer streamer;
	/** @brief Linear gain the mixer multiplies the streamer by. */
	float volume = 0.0f;
};

/**
 * @brief Serializes mixer and transport mutation with the SDL audio callback.
 *
 * `Hrk::Mixer` and `Hrk::PlaybackState` are plain containers read by the audio
 * thread. `SDL_LockAudioDevice` guarantees the callback is not running, so a
 * script that inserts a module or toggles playback cannot race the decoder.
 * The class itself lives in `HarukaContext.hpp`, so the one-shot player and this
 * module share one definition.
 */

/// @brief Reads a numeric property, falling back when it is missing or not a number.
double numberProperty(JSContext* cx, JS::HandleObject object, const char* name, double fallback)
{
	JS::RootedValue value(cx);
	if (!JS_GetProperty(cx, object, name, &value) || !value.isNumber()) {
		return fallback;
	}
	return value.toNumber();
}

/// @brief Reads a boolean property, falling back when it is missing.
bool boolProperty(JSContext* cx, JS::HandleObject object, const char* name, bool fallback)
{
	JS::RootedValue value(cx);
	if (!JS_GetProperty(cx, object, name, &value) || value.isUndefined()) {
		return fallback;
	}
	return value.toBoolean();
}

/// @brief The module behind `this`, never null.
ModuleHandle* moduleFromThis(JS::CallArgs& args)
{
	ModuleHandle* handle = getNativePointer<ModuleHandle>(args.thisv().toObjectOrNull());
	if (handle == nullptr || !handle->streamer) {
		throw std::runtime_error("Audio.Module: the module is not valid");
	}
	return handle;
}

} // namespace

namespace {

/// @brief The driver pointer to lock, or null when there is no audio device.
Haruka_SDL_Driver* audioDriver()
{
	return gHost != nullptr ? gHost->audio : nullptr;
}

/// @brief `new Audio.Module(path, options)`.
bool js_audio_module_constructor(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		Sandbox::ScriptHost* host = gHost;
		if (host == nullptr || !host->mixer) {
			throw std::runtime_error("Audio.Module: the mixer is not available");
		}
		if (argc < 1) {
			throw std::runtime_error("Audio.Module: expected a module path");
		}
		const std::string path = JsToNativeString(cx, args.get(0).toString());
		float volume = 0.5f;
		bool repeating = false;
		bool playing = true;
		if (argc >= 2 && args.get(1).isObject()) {
			JS::RootedObject options(cx, &args.get(1).toObject());
			volume = static_cast<float>(numberProperty(cx, options, "volume", volume));
			repeating = boolProperty(cx, options, "repeating", repeating);
			playing = boolProperty(cx, options, "playing", playing);
		}
		Elv::Io::uDevice file = host->io.openUnique(path.c_str(), Elv::Io::Mode::READ);
		if (!file || !file->isValid()) {
			throw std::runtime_error("Audio.Module: could not open '" + path + "'");
		}
		auto streamer = Elv::Util::pmr_make_shared<Hrk::ModuleStreamer>(
			std::pmr::get_default_resource(), std::move(file));
		if (!streamer->isValid()) {
			throw std::runtime_error("Audio.Module: could not parse '" + path + "'");
		}
		{
			AudioLock lock(host->audio);
			streamer->getState().setCursor(Hrk::FrameIndex(0));
			streamer->getState().setRepeat(repeating);
			streamer->getState().setPlayStatus(playing ? Hrk::PlayStatus::PLAYING
													   : Hrk::PlayStatus::PAUSED);
			host->mixer->getChildren().emplace(streamer.get(), volume);
		}
		JS::RootedObject object(cx,
			JS_NewObjectWithGivenProto(cx, &ModuleClass.protoClass, *ModuleClass.prototype));
		setNativePointer(object, new ModuleHandle{ std::move(streamer), volume });
		args.rval().setObject(*object);
		return true;
	}, cx, argc, vp);
}

/// @brief Finalizer of `Audio.Module`: unlinks the streamer from the mixer.
void js_audio_module_finalizer(JS::GCContext* gcx, JSObject* obj)
{
	(void)gcx;
	ModuleHandle* handle = getNativePointer<ModuleHandle>(obj);
	if (handle == nullptr) return;
	// Erasing the child is mandatory: the mixer stores a raw `Playable*`, so a
	// destroyed streamer that is still registered would be pulled by the audio
	// callback after it is gone.
	Sandbox::ScriptHost* host = gHost;
	if (host != nullptr && host->mixer && handle->streamer) {
		AudioLock lock(host->audio);
		host->mixer->getChildren().erase(handle->streamer.get());
	}
	delete handle;
}

/// @brief `module.playing`.
bool js_module_get_playing(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		ModuleHandle* handle = moduleFromThis(args);
		AudioLock lock(audioDriver());
		setReturnValue(cx, args.rval(),
			handle->streamer->getState().getPlayStatus() == Hrk::PlayStatus::PLAYING);
		return true;
	}, cx, argc, vp);
}

/// @brief `module.playing = value`: toggles between playing and paused.
bool js_module_set_playing(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext*, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		ModuleHandle* handle = moduleFromThis(args);
		AudioLock lock(audioDriver());
		handle->streamer->getState().setPlayStatus(
			args.get(0).toBoolean() ? Hrk::PlayStatus::PLAYING : Hrk::PlayStatus::PAUSED);
		args.rval().set(args.get(0));
		return true;
	}, cx, argc, vp);
}

/// @brief `module.paused`.
bool js_module_get_paused(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		ModuleHandle* handle = moduleFromThis(args);
		AudioLock lock(audioDriver());
		setReturnValue(cx, args.rval(),
			handle->streamer->getState().getPlayStatus() == Hrk::PlayStatus::PAUSED);
		return true;
	}, cx, argc, vp);
}

/// @brief `module.paused = value`.
bool js_module_set_paused(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext*, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		ModuleHandle* handle = moduleFromThis(args);
		AudioLock lock(audioDriver());
		handle->streamer->getState().setPlayStatus(
			args.get(0).toBoolean() ? Hrk::PlayStatus::PAUSED : Hrk::PlayStatus::PLAYING);
		args.rval().set(args.get(0));
		return true;
	}, cx, argc, vp);
}

} // namespace


namespace {

/// @brief `module.status`: the raw `Hrk::PlayStatus` value (0 stopped, 1 playing, 2 paused).
bool js_module_get_status(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		ModuleHandle* handle = moduleFromThis(args);
		AudioLock lock(audioDriver());
		setReturnValue(cx, args.rval(),
			static_cast<std::int32_t>(handle->streamer->getState().getPlayStatus()));
		return true;
	}, cx, argc, vp);
}

/// @brief `module.volume`.
bool js_module_get_volume(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		ModuleHandle* handle = moduleFromThis(args);
		setReturnValue(cx, args.rval(), handle->volume);
		return true;
	}, cx, argc, vp);
}

/// @brief `module.volume = value`: writes the gain the mixer reads.
bool js_module_set_volume(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext*, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		ModuleHandle* handle = moduleFromThis(args);
		const float volume = static_cast<float>(args.get(0).toNumber());
		Sandbox::ScriptHost* host = gHost;
		AudioLock lock(audioDriver());
		handle->volume = volume;
		if (host != nullptr && host->mixer) {
			auto& children = host->mixer->getChildren();
			const auto child = children.find(handle->streamer.get());
			if (child != children.end()) child->second = volume;
		}
		args.rval().set(args.get(0));
		return true;
	}, cx, argc, vp);
}

/// @brief `module.repeating`.
bool js_module_get_repeating(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		ModuleHandle* handle = moduleFromThis(args);
		AudioLock lock(audioDriver());
		setReturnValue(cx, args.rval(), handle->streamer->getState().getRepeat());
		return true;
	}, cx, argc, vp);
}

/// @brief `module.repeating = value`.
bool js_module_set_repeating(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext*, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		ModuleHandle* handle = moduleFromThis(args);
		AudioLock lock(audioDriver());
		handle->streamer->getState().setRepeat(args.get(0).toBoolean());
		args.rval().set(args.get(0));
		return true;
	}, cx, argc, vp);
}

/// @brief `module.cursor`: the playback position in frames.
bool js_module_get_cursor(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		ModuleHandle* handle = moduleFromThis(args);
		AudioLock lock(audioDriver());
		setReturnValue(cx, args.rval(), handle->streamer->getState().getCursor().var);
		return true;
	}, cx, argc, vp);
}

/// @brief `module.cursor = value`: seeks the decoder on the next pull.
bool js_module_set_cursor(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext*, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		ModuleHandle* handle = moduleFromThis(args);
		const std::int64_t cursor = static_cast<std::int64_t>(args.get(0).toNumber());
		AudioLock lock(audioDriver());
		handle->streamer->getState().setCursor(Hrk::FrameIndex(cursor));
		args.rval().set(args.get(0));
		return true;
	}, cx, argc, vp);
}

/// @brief `module.valid`.
bool js_module_get_valid(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), moduleFromThis(args)->streamer->isValid());
		return true;
	}, cx, argc, vp);
}

/// @brief `module.stop()`: rewinds and stops.
bool js_module_stop(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext*, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		ModuleHandle* handle = moduleFromThis(args);
		AudioLock lock(audioDriver());
		// `setPlayStatus(STOPPED)` also resets the cursor to zero.
		handle->streamer->getState().setPlayStatus(Hrk::PlayStatus::STOPPED);
		args.rval().setUndefined();
		return true;
	}, cx, argc, vp);
}

} // namespace



void RegisterAudioPlayerClasses(JSContext* cx, const JS::HandleObject& global,
								Sandbox::ScriptHost& host)
{
	gHost = &host;

	std::memset(&ModuleClass.protoClass, 0, sizeof(JSClass));
	std::memset(&ModuleClass.ops, 0, sizeof(JSClassOps));
	ModuleClass.name = "Module";
	ModuleClass.nargs = 2;
	ModuleClass.constructor = js_audio_module_constructor;
	ModuleClass.ops.construct = js_audio_module_constructor;
	ModuleClass.ops.finalize = js_audio_module_finalizer;
	ModuleClass.protoClass.cOps = &ModuleClass.ops;
	ModuleClass.protoClass.name = "Module";
	// Foreground finalization: the finalizer removes the streamer from the mixer
	// and closes a PhysFS device, both of which must happen on this thread.
	ModuleClass.protoClass.flags = JSCLASS_HAS_RESERVED_SLOTS(1);
	ModuleClass.prototype = std::make_unique<JS::RootedObject>(cx,
		JS_NewObject(cx, &ModuleClass.protoClass));
	ModuleClass.ps.push_back(JS_PSGS("playing", js_module_get_playing, js_module_set_playing,
		JSPROP_ENUMERATE));
	ModuleClass.ps.push_back(JS_PSGS("paused", js_module_get_paused, js_module_set_paused,
		JSPROP_ENUMERATE));
	ModuleClass.ps.push_back(JS_PSG("status", js_module_get_status, JSPROP_ENUMERATE));
	ModuleClass.ps.push_back(JS_PSGS("volume", js_module_get_volume, js_module_set_volume,
		JSPROP_ENUMERATE));
	ModuleClass.ps.push_back(JS_PSGS("repeating", js_module_get_repeating,
		js_module_set_repeating, JSPROP_ENUMERATE));
	ModuleClass.ps.push_back(JS_PSGS("cursor", js_module_get_cursor, js_module_set_cursor,
		JSPROP_ENUMERATE));
	ModuleClass.ps.push_back(JS_PSG("valid", js_module_get_valid, JSPROP_ENUMERATE));
	ModuleClass.ps.push_back(JS_PS_END);
	ModuleClass.static_ps.push_back(JS_PS_END);
	ModuleClass.fs.push_back(JS_FN("stop", js_module_stop, 0, 0));
	ModuleClass.fs.push_back(JS_FS_END);
	ModuleClass.static_fs.push_back(JS_FS_END);
	ModuleClass.registerToJS(cx, global, *ModuleClass.prototype);
}

} // namespace JS
