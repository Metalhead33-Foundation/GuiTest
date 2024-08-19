#include "JsAudioPlayer.hpp"
#include <MhLib/Io/MhIoDevice.hpp>
#include <MhLib/Media/AdvancedAudio/MhAudioBuffer.hpp>
#include <MhLib/Media/AdvancedAudio/MhSoundSource.hpp>
#include <MhLib/Media/AdvancedAudio/MhAudioStreamer.hpp>
#include <MhLib/Media/AdvancedAudio/MhAudioMixer.hpp>
#include <jsapi.h>
#include <js/Object.h>

namespace JS {

ClassCreator MhAudioStreamerClass;
ClassCreator MhAudioBufferClass;
ClassCreator MhSoundSourceClass;
ClassCreator MhAudioMixerClass;
static bool hasIoBeenInitialized = false;

/*template <> struct ReturnValueWrapper<MH33::Audio::PlayStatus> {
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, MH33::Audio::PlayStatus value) {
		// Default case for non-specialized types
		rval.setInt32(static_cast<int32_t>(value));
	}
	static MH33::Audio::PlayStatus fromValue(JSContext* cx, const HandleValue& arg) {
		return static_cast<MH33::Audio::PlayStatus>(arg.toInt32());
	}
};
template <> struct ReturnValueWrapper<MH33::Audio::ChannelCount> {
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, MH33::Audio::ChannelCount value) {
		// Default case for non-specialized types
		rval.setInt32(static_cast<int32_t>(value.var));
	}
	static MH33::Audio::ChannelCount fromValue(JSContext* cx, const HandleValue& arg) {
		return MH33::Audio::ChannelCount(arg.toInt32());
	}
};
template <> struct ReturnValueWrapper<MH33::Audio::FrameRate> {
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, MH33::Audio::FrameRate value) {
		// Default case for non-specialized types
		rval.setInt32(static_cast<int32_t>(value.var));
	}
	static MH33::Audio::FrameRate fromValue(JSContext* cx, const HandleValue& arg) {
		return MH33::Audio::FrameRate(arg.toInt32());
	}
};*/

// Playable
/*
class Playable {
public:
	virtual ~Playable() = default;
	virtual FrameCount outputTo(const Output& dst) = 0;
	virtual FrameRate getFrameRate() const = 0;
	virtual ChannelCount getChannelCount() const = 0;
};
*/
static bool js_audio_playable_getFrameRate(JSContext* cx, unsigned argc, JS::Value* vp) {
	return jsMemberFunctionWrapper(cx,argc,vp,&MH33::Audio::Playable::getFrameRate);
}
static bool js_audio_playable_getChannelCount(JSContext* cx, unsigned argc, JS::Value* vp) {
	return jsMemberFunctionWrapper(cx,argc,vp,&MH33::Audio::Playable::getChannelCount);
}

// Constructors
static bool js_audio_buffer_constructor(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto firstArg = args.get(0);
		if(!firstArg.isObject()) throw std::runtime_error("Excepted object as first argument!");
		auto iohandle = getSmartPointerFromObj<MH33::Io::Device>(firstArg.toObjectOrNull(),0);
		JS::RootedObject obj(cx, JS_NewObjectWithGivenProto(cx,&MhAudioBufferClass.protoClass,*MhAudioBufferClass.prototype));
		storeSmartPointerToObj( obj, 0, std::make_shared<MH33::Audio::Buffer>(std::move(iohandle)));
		args.rval().setObject(*obj);
		return true;
	}, cx, argc, vp);
}
static bool js_sound_source_constructor(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		JS::RootedObject obj(cx, JS_NewObjectWithGivenProto(cx,&MhSoundSourceClass.protoClass,*MhSoundSourceClass.prototype));
		auto firstArg = args.get(0);
		if(firstArg.isObject()) {
			auto bufferHandle = getSmartPointerFromObj<MH33::Audio::Buffer>(firstArg.toObjectOrNull(),0);
			storeSmartPointerToObj( obj, 0, std::make_shared<MH33::Audio::SoundSource>(std::move(bufferHandle)));
		}
		else storeSmartPointerToObj( obj, 0, std::make_shared<MH33::Audio::SoundSource>());
		args.rval().setObject(*obj);
		return true;
	}, cx, argc, vp);
}
static bool js_audio_streamer_streamer_constructor(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto firstArg = args.get(0);
		if(!firstArg.isObject()) throw std::runtime_error("Excepted object as first argument!");
		auto iohandle = getSmartPointerFromObj<MH33::Io::Device>(firstArg.toObjectOrNull(),0);
		JS::RootedObject obj(cx, JS_NewObjectWithGivenProto(cx,&MhAudioStreamerClass.protoClass,*MhAudioStreamerClass.prototype));
		storeSmartPointerToObj( obj, 0, std::make_shared<MH33::Audio::Streamer>(std::move(iohandle)));
		args.rval().setObject(*obj);
		return true;
	}, cx, argc, vp);
}
static bool js_audio_mixer_constructor(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		// Args
		auto arg0 = args.get(0);
		if(!arg0.isInt32()) throw std::runtime_error("Excepted integer as first argument!");
		auto arg1 = args.get(1);
		if(!arg1.isInt32()) throw std::runtime_error("Excepted integer as second argument!");
		auto arg2 = args.get(2);
		if(!arg2.isInt32()) throw std::runtime_error("Excepted integer as third argument!");
		// FrameCount frameCount, FrameRate frameRate, ChannelCount channelCount
		auto handle = std::make_shared<MH33::Audio::Mixer>(
					MH33::Audio::FrameCount(arg0.toInt32()),
					MH33::Audio::FrameRate(arg1.toInt32()),
					MH33::Audio::ChannelCount(arg2.toInt32())
					);
		JS::RootedObject obj(cx, JS_NewObjectWithGivenProto(cx,&MhAudioMixerClass.protoClass,*MhAudioMixerClass.prototype));
		storeSmartPointerToObj( obj, 0, std::move(handle));
		args.rval().setObject(*obj);
		return true;
	}, cx, argc, vp);
}

// Buffer
static bool js_audio_buffer_getFrameCount(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto buff = JS::getSmartPointerFromObj<MH33::Audio::Buffer>(args.thisv().toObjectOrNull(), 0);
		args.rval().setInt32(buff->getFrameCount().var);
		return true;
	}, cx, argc, vp);
}
static bool js_audio_buffer_getFrameRate(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto buff = JS::getSmartPointerFromObj<MH33::Audio::Buffer>(args.thisv().toObjectOrNull(), 0);
		args.rval().setInt32(buff->getFrameRate().var);
		return true;
	}, cx, argc, vp);
	//return wrapMemberFunction<MH33::Audio::Buffer, int, void, &MH33::Audio::Buffer::getFrameRate>(cx, argc, vp);
}
static bool js_audio_buffer_getChannelCount(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto buff = JS::getSmartPointerFromObj<MH33::Audio::Buffer>(args.thisv().toObjectOrNull(), 0);
		args.rval().setInt32(buff->getChannelCount().var);
		return true;
	}, cx, argc, vp);
}
static bool js_audio_buffer_resample(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto buff = JS::getSmartPointerFromObj<MH33::Audio::Buffer>(args.thisv().toObjectOrNull(), 0);
		auto arg0 = args.get(1).toInt32();
		buff->resample(MH33::Audio::FrameRate(arg0));
		args.rval().setNull();
		return true;
	}, cx, argc, vp);
}

static bool js_soundsurce_getBuffer(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto zobj = JS::getSmartPointerFromObj<MH33::Audio::SoundSource>(args.thisv().toObjectOrNull(), 0);
		JS::RootedObject rootedObj(cx, JS_NewObjectWithGivenProto(cx, &MhAudioBufferClass.protoClass, *MhAudioBufferClass.prototype ) );
		auto buffer = zobj->getBuffer();
		storeSmartPointerToObj(rootedObj, 0, std::move(buffer));
		args.rval().setObjectOrNull(rootedObj);
		return true;
	}, cx, argc, vp);
}
static bool js_soundsurce_setBuffer(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto zobj = JS::getSmartPointerFromObj<MH33::Audio::SoundSource>(args.thisv().toObjectOrNull(), 0);
		auto firstArg = args.get(0);
		if(!firstArg.isObject()) throw std::runtime_error("Excepted object as first argument!");
		auto returnObj = JS::RootedObject(cx, firstArg.toObjectOrNull());
		auto toSet = JS::getSmartPointerFromObj<MH33::Audio::sBuffer>(returnObj,0);
		args.rval().setObjectOrNull(returnObj);
		return true;
	}, cx, argc, vp);
}
static bool js_soundsurce_getCursor(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto zobj = JS::getSmartPointerFromObj<MH33::Audio::SoundSource>(args.thisv().toObjectOrNull(), 0);
		args.rval().setInt32(zobj->getCursor().var);
		return true;
	}, cx, argc, vp);
}
static bool js_soundsurce_setCursor(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto zobj = JS::getSmartPointerFromObj<MH33::Audio::SoundSource>(args.thisv().toObjectOrNull(), 0);
		auto firstArg = args.get(0);
		if(!firstArg.isInt32()) throw std::runtime_error("Excepted integer as first argument!");
		zobj->setCursor(MH33::Audio::FrameIndex(firstArg.toInt32()));
		args.rval().set(firstArg);
		return true;
	}, cx, argc, vp);
}
static bool js_soundsurce_getState(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto zobj = JS::getSmartPointerFromObj<MH33::Audio::SoundSource>(args.thisv().toObjectOrNull(), 0);
		args.rval().setInt32(static_cast<int32_t>(zobj->getState()));
		return true;
	}, cx, argc, vp);
}
static bool js_soundsurce_setState(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto zobj = JS::getSmartPointerFromObj<MH33::Audio::SoundSource>(args.thisv().toObjectOrNull(), 0);
		auto firstArg = args.get(0);
		if(!firstArg.isInt32()) throw std::runtime_error("Excepted integer as first argument!");
		switch (firstArg.toInt32()) {
			case static_cast<int32_t>(MH33::Audio::PlayStatus::STOPPED): zobj->setState(MH33::Audio::PlayStatus::STOPPED); break;
			case static_cast<int32_t>(MH33::Audio::PlayStatus::PLAYING): zobj->setState(MH33::Audio::PlayStatus::PLAYING); break;
			case static_cast<int32_t>(MH33::Audio::PlayStatus::PAUSED): zobj->setState(MH33::Audio::PlayStatus::PAUSED); break;
			default: throw std::runtime_error("Invalid audio playing status!");
		}
		args.rval().set(firstArg);
		return true;
	}, cx, argc, vp);
}
static bool js_soundsurce_getRepeating(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto zobj = JS::getSmartPointerFromObj<MH33::Audio::SoundSource>(args.thisv().toObjectOrNull(), 0);
		args.rval().setBoolean(static_cast<int32_t>(zobj->getRepeating()));
		return true;
	}, cx, argc, vp);
}
static bool js_soundsurce_setRepeating(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto zobj = JS::getSmartPointerFromObj<MH33::Audio::SoundSource>(args.thisv().toObjectOrNull(), 0);
		auto firstArg = args.get(0);
		if(!firstArg.isBoolean()) throw std::runtime_error("Excepted boolean as first argument!");
		zobj->setRepeating(firstArg.toBoolean());
		args.rval().set(firstArg);
		return true;
	}, cx, argc, vp);
}

// Streamer
static bool js_audiostreamer_getCursor(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto zobj = JS::getSmartPointerFromObj<MH33::Audio::Streamer>(args.thisv().toObjectOrNull(), 0);
		args.rval().setInt32(zobj->getSoundfile().seekCur(MH33::Audio::FrameCount(0)).var);
		return true;
	}, cx, argc, vp);
}
static bool js_audiostreamer_setCursor(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto zobj = JS::getSmartPointerFromObj<MH33::Audio::Streamer>(args.thisv().toObjectOrNull(), 0);
		auto firstArg = args.get(0);
		if(!firstArg.isInt32()) throw std::runtime_error("Excepted integer as first argument!");
		args.rval().setInt32(zobj->getSoundfile().seekSet(MH33::Audio::FrameCount(firstArg.toInt32() )).var);
		return true;
	}, cx, argc, vp);
}
static bool js_audiostreamer_getState(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto zobj = JS::getSmartPointerFromObj<MH33::Audio::Streamer>(args.thisv().toObjectOrNull(), 0);
		args.rval().setInt32(static_cast<int32_t>(zobj->getState()));
		return true;
	}, cx, argc, vp);
}
static bool js_audiostreamer_setState(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto zobj = JS::getSmartPointerFromObj<MH33::Audio::Streamer>(args.thisv().toObjectOrNull(), 0);
		auto firstArg = args.get(0);
		if(!firstArg.isInt32()) throw std::runtime_error("Excepted integer as first argument!");
		switch (firstArg.toInt32()) {
			case static_cast<int32_t>(MH33::Audio::PlayStatus::STOPPED): zobj->setState(MH33::Audio::PlayStatus::STOPPED); break;
			case static_cast<int32_t>(MH33::Audio::PlayStatus::PLAYING): zobj->setState(MH33::Audio::PlayStatus::PLAYING); break;
			case static_cast<int32_t>(MH33::Audio::PlayStatus::PAUSED): zobj->setState(MH33::Audio::PlayStatus::PAUSED); break;
			default: throw std::runtime_error("Invalid audio playing status!");
		}
		args.rval().set(firstArg);
		return true;
	}, cx, argc, vp);
}
static bool js_audiostreamer_getRepeating(JSContext* cx, unsigned argc, JS::Value* vp) {
	return jsMemberFunctionWrapper(cx,argc,vp,&MH33::Audio::Streamer::getRepeating);
}
static bool js_audiostreamer_setRepeating(JSContext* cx, unsigned argc, JS::Value* vp) {
	return jsMemberFunctionWrapper(cx,argc,vp,&MH33::Audio::Streamer::setRepeating);
}

static bool js_mixer_InserPlayable(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto zobj = JS::getSmartPointerFromObj<MH33::Audio::Mixer>(args.thisv().toObjectOrNull(), 0);
		auto firstArg = args.get(0);
		if(!firstArg.isObject()) throw std::runtime_error("Excepted object as first argument!");
		auto returnObj = JS::RootedObject(cx, firstArg.toObjectOrNull());
		auto toSet = JS::getSmartPointerFromObj<MH33::Audio::Playable>(returnObj,0);
		auto secondArg = args.get(1);
		if(!secondArg.isNumber()) throw std::runtime_error("Excepted number as first argument!");
		float volume = secondArg.toNumber();
		zobj->set(std::move(toSet),volume);
		args.rval().set(args.thisv());
		return true;
	}, cx, argc, vp);
}

void RegisterAudioPlayerClasses(JSContext* cx, const HandleObject& global)
{
	if(hasIoBeenInitialized) return;

	// Let's start with the audio buffer

	// AudioBuffer
	std::memset(&MhAudioBufferClass.protoClass, 0, sizeof(JSClass));
	std::memset(&MhAudioBufferClass.ops, 0, sizeof(JSClassOps));
	MhAudioBufferClass.constructor = js_audio_buffer_constructor;
	MhAudioBufferClass.name = "Buffer";
	MhAudioBufferClass.nargs = 1;
	MhAudioBufferClass.ops.finalize = js_finalizer_shared;
	MhAudioBufferClass.ops.construct = js_audio_buffer_constructor;
	MhAudioBufferClass.protoClass.cOps = &MhAudioBufferClass.ops;
	MhAudioBufferClass.protoClass.name = "Buffer";
	MhAudioBufferClass.protoClass.flags = JSCLASS_HAS_RESERVED_SLOTS(1) | JSCLASS_BACKGROUND_FINALIZE;
	MhAudioBufferClass.prototype = std::make_unique<JS::RootedObject>(cx, JS_NewObject(cx,&MhAudioBufferClass.protoClass));
	MhAudioBufferClass.ps.push_back(JS_PSG("frameCount", js_audio_buffer_getFrameCount, JSPROP_ENUMERATE ) );
	MhAudioBufferClass.ps.push_back(JS_PSG("frameRate", js_audio_buffer_getFrameRate, JSPROP_ENUMERATE ) );
	MhAudioBufferClass.ps.push_back(JS_PSG("channelCount", js_audio_buffer_getChannelCount, JSPROP_ENUMERATE ) );
	MhAudioBufferClass.ps.push_back(JS_PS_END);
	MhAudioBufferClass.static_ps.push_back(JS_PS_END);
	MhAudioBufferClass.fs.push_back(JS_FN("resample",js_audio_buffer_resample,0,0));
	MhAudioBufferClass.fs.push_back(JS_FS_END);
	MhAudioBufferClass.static_fs.push_back(JS_FS_END);
	{
		auto toReturn = MhAudioBufferClass.registerToJS(cx, global, *MhAudioBufferClass.prototype);
		auto reg = JS::RootedValue(cx,JS::ObjectOrNullValue(toReturn) );
		auto rootedReg = JS::RootedObject(cx,toReturn);
		JS_SetProperty(cx, global, MhAudioBufferClass.name.c_str(), reg );
	}
	// AudioSource
	std::memset(&MhSoundSourceClass.protoClass, 0, sizeof(JSClass));
	std::memset(&MhSoundSourceClass.ops, 0, sizeof(JSClassOps));
	MhSoundSourceClass.constructor = js_sound_source_constructor;
	MhSoundSourceClass.name = "SoundSource";
	MhSoundSourceClass.nargs = 1;
	MhSoundSourceClass.ops.finalize = js_finalizer_shared;
	MhSoundSourceClass.ops.construct = js_sound_source_constructor;
	MhSoundSourceClass.protoClass.cOps = &MhSoundSourceClass.ops;
	MhSoundSourceClass.protoClass.name = "SoundSource";
	MhSoundSourceClass.protoClass.flags = JSCLASS_HAS_RESERVED_SLOTS(1) | JSCLASS_BACKGROUND_FINALIZE;
	MhSoundSourceClass.prototype = std::make_unique<JS::RootedObject>(cx, JS_NewObject(cx,&MhAudioBufferClass.protoClass));
	MhSoundSourceClass.ps.push_back(JS_PSG("channelCount", js_audio_playable_getChannelCount, JSPROP_ENUMERATE ) );
	MhSoundSourceClass.ps.push_back(JS_PSG("frameRate", js_audio_playable_getFrameRate, JSPROP_ENUMERATE ) );
	MhSoundSourceClass.ps.push_back(JS_PSGS("buffer", js_soundsurce_getBuffer, js_soundsurce_setBuffer, JSPROP_ENUMERATE ) );
	MhSoundSourceClass.ps.push_back(JS_PSGS("cursor", js_soundsurce_getCursor, js_soundsurce_setCursor, JSPROP_ENUMERATE ) );
	MhSoundSourceClass.ps.push_back(JS_PSGS("state", js_soundsurce_getState, js_soundsurce_setState, JSPROP_ENUMERATE ) );
	MhSoundSourceClass.ps.push_back(JS_PSGS("repeating", js_soundsurce_getRepeating, js_soundsurce_setRepeating, JSPROP_ENUMERATE ) );
	MhSoundSourceClass.ps.push_back(JS_PS_END);
	MhSoundSourceClass.static_ps.push_back(JS_PS_END);
	MhSoundSourceClass.fs.push_back(JS_FS_END);
	MhSoundSourceClass.static_fs.push_back(JS_FS_END);
	{
		auto toReturn = MhSoundSourceClass.registerToJS(cx, global, *MhSoundSourceClass.prototype);
		auto reg = JS::RootedValue(cx,JS::ObjectOrNullValue(toReturn) );
		auto rootedReg = JS::RootedObject(cx,toReturn);
		JS_SetProperty(cx, global, MhSoundSourceClass.name.c_str(), reg );
	}
	// AudioStreamer
	std::memset(&MhAudioStreamerClass.protoClass, 0, sizeof(JSClass));
	std::memset(&MhAudioStreamerClass.ops, 0, sizeof(JSClassOps));
	MhAudioStreamerClass.constructor = js_audio_streamer_streamer_constructor;
	MhAudioStreamerClass.name = "Streamer";
	MhAudioStreamerClass.nargs = 1;
	MhAudioStreamerClass.ops.finalize = js_finalizer_shared;
	MhAudioStreamerClass.ops.construct = js_audio_streamer_streamer_constructor;
	MhAudioStreamerClass.protoClass.cOps = &MhAudioStreamerClass.ops;
	MhAudioStreamerClass.protoClass.name = "Streamer";
	MhAudioStreamerClass.protoClass.flags = JSCLASS_HAS_RESERVED_SLOTS(1) | JSCLASS_BACKGROUND_FINALIZE;
	MhAudioStreamerClass.prototype = std::make_unique<JS::RootedObject>(cx, JS_NewObject(cx,&MhAudioBufferClass.protoClass));
	MhAudioStreamerClass.ps.push_back(JS_PSG("channelCount", js_audio_playable_getChannelCount, JSPROP_ENUMERATE ) );
	MhAudioStreamerClass.ps.push_back(JS_PSG("frameRate", js_audio_playable_getFrameRate, JSPROP_ENUMERATE ) );
	MhAudioStreamerClass.ps.push_back(JS_PSGS("cursor", js_audiostreamer_getCursor, js_audiostreamer_setCursor, JSPROP_ENUMERATE ) );
	MhAudioStreamerClass.ps.push_back(JS_PSGS("state", js_audiostreamer_getState, js_audiostreamer_setState, JSPROP_ENUMERATE ) );
	MhAudioStreamerClass.ps.push_back(JS_PSGS("repeating", js_audiostreamer_getRepeating, js_audiostreamer_setRepeating, JSPROP_ENUMERATE ) );
	MhAudioStreamerClass.ps.push_back(JS_PS_END);
	MhAudioStreamerClass.static_ps.push_back(JS_PS_END);
	MhAudioStreamerClass.fs.push_back(JS_FS_END);
	MhAudioStreamerClass.static_fs.push_back(JS_FS_END);
	{
		auto toReturn = MhAudioStreamerClass.registerToJS(cx, global, *MhAudioStreamerClass.prototype);
		auto reg = JS::RootedValue(cx,JS::ObjectOrNullValue(toReturn) );
		auto rootedReg = JS::RootedObject(cx,toReturn);
		JS_SetProperty(cx, global, MhAudioStreamerClass.name.c_str(), reg );
	}

	// AudioMixer
	std::memset(&MhAudioMixerClass.protoClass, 0, sizeof(JSClass));
	std::memset(&MhAudioMixerClass.ops, 0, sizeof(JSClassOps));
	MhAudioMixerClass.constructor = js_audio_mixer_constructor;
	MhAudioMixerClass.name = "Mixer";
	MhAudioMixerClass.nargs = 1;
	MhAudioMixerClass.ops.finalize = js_finalizer_shared;
	MhAudioMixerClass.ops.construct = js_audio_mixer_constructor;
	MhAudioMixerClass.protoClass.cOps = &MhAudioStreamerClass.ops;
	MhAudioMixerClass.protoClass.name = "Mixer";
	MhAudioMixerClass.protoClass.flags = JSCLASS_HAS_RESERVED_SLOTS(1) | JSCLASS_BACKGROUND_FINALIZE;
	MhAudioMixerClass.prototype = std::make_unique<JS::RootedObject>(cx, JS_NewObject(cx,&MhAudioBufferClass.protoClass));
	MhAudioMixerClass.ps.push_back(JS_PSG("channelCount", js_audio_playable_getChannelCount, JSPROP_ENUMERATE ) );
	MhAudioMixerClass.ps.push_back(JS_PSG("frameRate", js_audio_playable_getFrameRate, JSPROP_ENUMERATE ) );
	MhAudioMixerClass.ps.push_back(JS_PS_END);
	MhAudioMixerClass.static_ps.push_back(JS_PS_END);
	MhAudioMixerClass.fs.push_back(JS_FN("insert",js_mixer_InserPlayable,2,0));
	MhAudioMixerClass.fs.push_back(JS_FS_END);
	MhAudioMixerClass.static_fs.push_back(JS_FS_END);
	{
		auto toReturn = MhAudioMixerClass.registerToJS(cx, global, *MhAudioMixerClass.prototype);
		auto reg = JS::RootedValue(cx,JS::ObjectOrNullValue(toReturn) );
		auto rootedReg = JS::RootedObject(cx,toReturn);
		JS_SetProperty(cx, global, MhAudioMixerClass.name.c_str(), reg );
	}
	hasIoBeenInitialized = true;
}

}
