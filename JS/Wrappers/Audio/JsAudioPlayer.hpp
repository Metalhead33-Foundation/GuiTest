#ifndef JSAUDIOPLAYER_HPP
#define JSAUDIOPLAYER_HPP
#include <JS/JsUtil.hpp>
namespace JS {

extern ClassCreator MhAudioStreamerClass;
extern ClassCreator MhAudioBufferClass;
extern ClassCreator MhSoundSourceClass;
extern ClassCreator MhAudioMixerClass;
void RegisterAudioPlayerClasses(JSContext* cx, const JS::HandleObject &global);

}

#endif // JSAUDIOPLAYER_HPP
