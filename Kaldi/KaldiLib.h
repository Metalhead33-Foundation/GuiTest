#ifndef KALDILIB_H
#define KALDILIB_H
/**
 * @file KaldiLib.h
 * @brief Declares the KaldiLib API in the Kaldi module.
 *
 * This header is part of the public declaration surface for Kaldi.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <stddef.h>
#include <SDL2/SDL_syswm.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Error codes for API operations
 */
typedef enum {
	MH_SUCCESS = 0,
	MH_ERROR_GENERIC = -1,
	MH_ERROR_INVALID_WINDOW = -2,
	MH_ERROR_OUT_OF_MEMORY = -3
} MhResult;

/**
 * Opaque handle to the Rendering Device.
 * This provides type safety while hiding C++ implementation details.
 */
typedef struct MhRenderingDevice_t* MhRenderingDeviceHandle;
/** @brief Window-size metadata used when creating a rendering device. */
typedef struct MhRenderingDeviceCreationInfo_t {
	/** @brief Initial rendering surface width in pixels. */
	int width;
	/** @brief Initial rendering surface height in pixels. */
	int height;
} MhRenderingDeviceCreationInfo;

// --- Function Pointer Typedefs for Dynamic Loading ---

/** @brief Function pointer for the `kld_create_rendering_device` backend export. */
typedef MhRenderingDeviceHandle (*MhCreateRenderingDevice)(const SDL_SysWMinfo* window_info, const MhRenderingDeviceCreationInfo& creationInfo);
/** @brief Function pointer for the `kld_destroy_rendering_device` backend export. */
typedef void                   (*MhDestroyRenderingDevice)(MhRenderingDeviceHandle device);
/** @brief Function pointer for the `kld_get_renderer_name` backend export. */
typedef const char* (*MhGetRendererName)(void);
/** @brief Function pointer for the `kld_get_version` backend export. */
typedef const char* (*MhGetVersion)(void);
/** @brief Function pointer for the `kld_get_last_error_message` backend export. */
typedef const char* (*MhGetLastError)(void);

#ifdef __cplusplus
}
#endif
#endif // KALDILIB_H
