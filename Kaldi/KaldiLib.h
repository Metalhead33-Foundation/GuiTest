#ifndef KALDILIB_H
#define KALDILIB_H
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
typedef struct MhRenderingDeviceCreationInfo_t {
	int width;
	int height;
} MhRenderingDeviceCreationInfo;

// --- Function Pointer Typedefs for Dynamic Loading ---

// Canonically exported as "kld_create_rendering_device"
typedef MhRenderingDeviceHandle (*MhCreateRenderingDevice)(const SDL_SysWMinfo* window_info, const MhRenderingDeviceCreationInfo& creationInfo);
// Canonically exported as "kld_destroy_rendering_device"
typedef void                   (*MhDestroyRenderingDevice)(MhRenderingDeviceHandle device);
// Canonically exported as "kld_get_renderer_name"
typedef const char* (*MhGetRendererName)(void);
// Canonically exported as "kld_get_version"
typedef const char* (*MhGetVersion)(void);
// Canonically exported as "kld_get_last_error_message"
typedef const char* (*MhGetLastError)(void);

#ifdef __cplusplus
}
#endif
#endif // KALDILIB_H
