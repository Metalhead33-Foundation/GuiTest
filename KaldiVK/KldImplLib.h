#ifndef KLDIMPLLIB_H
#define KLDIMPLLIB_H
#include <Elvavena/Util/ElvUtilGlobals.hpp>
#include <Kaldi/KaldiLib.h>
#ifdef __cplusplus
extern "C" {
#endif

#if defined(MH_KALDI_IMPL_EXPORT)
#define MH_KALDI_IMPL_API MH33_API_EXPORT
#else
#define MH_KALDI_IMPL_API MH33_API_IMPORT
#endif

MH_KALDI_IMPL_API MhRenderingDeviceHandle kld_create_rendering_device(const SDL_SysWMinfo* window_info, const MhRenderingDeviceCreationInfo& creationInfo);
MH_KALDI_IMPL_API void                   kld_destroy_rendering_device(MhRenderingDeviceHandle device);
MH_KALDI_IMPL_API const char* kld_get_renderer_name(void);
MH_KALDI_IMPL_API const char* kld_get_version(void);
MH_KALDI_IMPL_API const char* kld_get_last_error_message(void);

#ifdef __cplusplus
}
#endif
#endif // KLDIMPLLIB_H
