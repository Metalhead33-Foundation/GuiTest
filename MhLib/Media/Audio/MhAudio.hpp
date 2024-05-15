#ifndef MHAUDIO_HPP
#define MHAUDIO_HPP
#include <MhLib/Util/MhGlobals.hpp>
#if defined(MH_AUDIO_EXPORT)
#define MH_AUDIO_API MH33_API_EXPORT
#else
#define MH_AUDIO_API MH33_API_IMPORT
#endif
#endif // MHAUDIO_HPP
