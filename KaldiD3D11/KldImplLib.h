#ifndef KLDIMPLLIB_H
#define KLDIMPLLIB_H
/**
 * @file KldImplLib.h
 * @brief Declares the KldImplLib API in the KaldiD3D11 module.
 *
 * This header is part of the public declaration surface for KaldiD3D11.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Elvavena/Util/ElvUtilGlobals.hpp>
#include <Kaldi/KaldiLib.h>
#ifdef __cplusplus
extern "C" {
#endif

#if defined(MH_KALDI_IMPL_EXPORT)
#define MH_KALDI_IMPL_API MH33_API_EXPORT
#else
/** @brief Documents the MH_KALDI_IMPL_API constant or macro. */
#define MH_KALDI_IMPL_API MH33_API_IMPORT
#endif

/** @brief Creates a backend rendering device for the supplied SDL window metadata. */
MH_KALDI_IMPL_API MhRenderingDeviceHandle kld_create_rendering_device(const SDL_SysWMinfo* window_info, const MhRenderingDeviceCreationInfo& creationInfo);
/** @brief Destroys a rendering device created by @ref kld_create_rendering_device. */
MH_KALDI_IMPL_API void                   kld_destroy_rendering_device(MhRenderingDeviceHandle device);
/** @brief Returns the human-readable renderer/backend name. */
MH_KALDI_IMPL_API const char* kld_get_renderer_name(void);
/** @brief Returns the backend implementation version string. */
MH_KALDI_IMPL_API const char* kld_get_version(void);
/** @brief Returns the last backend error message, or an implementation-defined empty value. */
MH_KALDI_IMPL_API const char* kld_get_last_error_message(void);

#ifdef __cplusplus
}
#endif
#endif // KLDIMPLLIB_H
