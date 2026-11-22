#ifndef KALDIVKPLATFORM_HPP
#define KALDIVKPLATFORM_HPP
/**
 * @file VkPlatform.hpp
 * @brief glad/Vulkan bootstrap and platform surface creation.
 *
 * Vulkan itself is platform-neutral, but *presenting to a window* is not: the
 * instance has to enable `VK_KHR_surface` plus one platform surface extension,
 * and creating the surface needs the very handles `Kld::NativeWindowHandles`
 * already carries for the GL backends.
 *
 * @section kaldivk_platform_macros Platform macros
 *
 * glad only declares `vkCreateXlibSurfaceKHR`, `vkCreateWin32SurfaceKHR`, ... when
 * the matching `VK_USE_PLATFORM_*` macro is defined *before* `<glad/vulkan.h>` is
 * included - and glad's header also pulls in the platform headers (`X11/Xlib.h`,
 * `wayland-client.h`, `windows.h`) for those. This file is therefore the only
 * place in KaldiVK that includes glad's Vulkan header; everything else includes
 * `VkPlatform.hpp` and inherits the same configuration.
 *
 * Supported today:
 *
 * | Host | Macro enabled | Extension |
 * |---|---|---|
 * | Win32 | `VK_USE_PLATFORM_WIN32_KHR` | `VK_KHR_win32_surface` |
 * | Android | `VK_USE_PLATFORM_ANDROID_KHR` | `VK_KHR_android_surface` |
 * | Linux/BSD | `VK_USE_PLATFORM_XLIB_KHR` | `VK_KHR_xlib_surface` |
 * | Linux/BSD | `VK_USE_PLATFORM_WAYLAND_KHR` | `VK_KHR_wayland_surface` |
 *
 * Define `KALDIVK_NO_XLIB_SURFACE` / `KALDIVK_NO_WAYLAND_SURFACE` to drop one of
 * the Linux paths when its development headers are unavailable.
 */
#if defined(_WIN32)
#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR 1
#endif
#elif defined(__ANDROID__)
#ifndef VK_USE_PLATFORM_ANDROID_KHR
#define VK_USE_PLATFORM_ANDROID_KHR 1
#endif
#else
#if !defined(VK_USE_PLATFORM_XLIB_KHR) && !defined(KALDIVK_NO_XLIB_SURFACE)
#define VK_USE_PLATFORM_XLIB_KHR 1
#endif
#if !defined(VK_USE_PLATFORM_WAYLAND_KHR) && !defined(KALDIVK_NO_WAYLAND_SURFACE)
#define VK_USE_PLATFORM_WAYLAND_KHR 1
#endif
#endif

#include <Kaldi/KldNativeWindow.hpp>
#include <glad/vulkan.h>
#include <vector>

/**
 * @section kaldivk_debug_utils Debug-utils declarations
 *
 * glad's generated header covers Vulkan 1.0 core only, so the debug-label entry
 * points and their label structure are declared here. They are used only when
 * `VK_EXT_debug_utils` is enabled on the instance (see `Vk::Context`), which is
 * also what makes `HalCapabilities::supportsDebugLabels` true.
 */
#ifndef VK_EXT_debug_utils
#define VK_EXT_DEBUG_UTILS_EXTENSION_NAME "VK_EXT_debug_utils"
#define VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT (VkStructureType)1000128002
typedef struct VkDebugUtilsLabelEXT {
	VkStructureType sType;
	const void* pNext;
	const char* pLabelName;
	float color[4];
} VkDebugUtilsLabelEXT;
typedef void(VKAPI_PTR* PFN_vkCmdBeginDebugUtilsLabelEXT)(VkCommandBuffer commandBuffer,
							  const VkDebugUtilsLabelEXT* pLabelInfo);
typedef void(VKAPI_PTR* PFN_vkCmdEndDebugUtilsLabelEXT)(VkCommandBuffer commandBuffer);
typedef void(VKAPI_PTR* PFN_vkCmdInsertDebugUtilsLabelEXT)(VkCommandBuffer commandBuffer,
							   const VkDebugUtilsLabelEXT* pLabelInfo);
#endif

namespace Vk {

/** @brief Symbolic name of a `VkResult` ("VK_SUCCESS", "VK_ERROR_..."). */
const char* resultName(VkResult result) noexcept;

/** @brief Surface extension name for @p system, or @c nullptr when unsupported. */
const char* surfaceExtensionName(Kld::WindowSystem system) noexcept;

/** @brief True when this build can create a surface for @p system. */
bool supportsSurface(Kld::WindowSystem system) noexcept;

/**
 * @brief Instance extensions required to present to @p system.
 *
 * Always starts with `VK_KHR_surface`; adds the platform extension when the
 * window system is known.
 */
std::vector<const char*> requiredInstanceExtensions(Kld::WindowSystem system);

/**
 * @brief Creates a `VkSurfaceKHR` for @p handles.
 * @throws std::invalid_argument When @p handles has no usable drawable or the
 *         window system is not supported by this build.
 * @throws std::runtime_error When the driver rejects the surface; the message
 *         carries `resultName()`.
 */
VkSurfaceKHR createSurface(VkInstance instance, const Kld::NativeWindowHandles& handles);

}

#endif // KALDIVKPLATFORM_HPP
