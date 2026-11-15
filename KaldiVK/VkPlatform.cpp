/**
 * @file VkPlatform.cpp
 * @brief Implements VkPlatform.hpp.
 */
#include "VkPlatform.hpp"
#include <stdexcept>
#include <string>

namespace Vk {

const char* resultName(VkResult result) noexcept
{
	switch (result) {
		case VK_SUCCESS: return "VK_SUCCESS";
		case VK_NOT_READY: return "VK_NOT_READY";
		case VK_TIMEOUT: return "VK_TIMEOUT";
		case VK_EVENT_SET: return "VK_EVENT_SET";
		case VK_EVENT_RESET: return "VK_EVENT_RESET";
		case VK_INCOMPLETE: return "VK_INCOMPLETE";
		case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
		case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
		default: return "VK_<unknown>";
	}
}

const char* surfaceExtensionName(Kld::WindowSystem system) noexcept
{
	switch (system) {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
		case Kld::WindowSystem::Windows: return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
#endif
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
		case Kld::WindowSystem::Android: return VK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
#endif
#if defined(VK_USE_PLATFORM_XLIB_KHR)
		case Kld::WindowSystem::X11: return VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
#endif
#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
		case Kld::WindowSystem::Wayland: return VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
#endif
		default: return nullptr;
	}
}

bool supportsSurface(Kld::WindowSystem system) noexcept
{
	return surfaceExtensionName(system) != nullptr;
}

std::vector<const char*> requiredInstanceExtensions(Kld::WindowSystem system)
{
	std::vector<const char*> extensions{ VK_KHR_SURFACE_EXTENSION_NAME };
	if (const char* platform = surfaceExtensionName(system); platform != nullptr) {
		extensions.push_back(platform);
	}
	return extensions;
}

VkSurfaceKHR createSurface(VkInstance instance, const Kld::NativeWindowHandles& handles)
{
	if (!handles.usable()) {
		throw std::invalid_argument("VkPlatform: the window has no usable native drawable");
	}

	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkResult result = VK_ERROR_EXTENSION_NOT_PRESENT;

	switch (handles.system) {
#if defined(VK_USE_PLATFORM_XLIB_KHR)
		case Kld::WindowSystem::X11: {
			VkXlibSurfaceCreateInfoKHR info{};
			info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
			info.dpy = static_cast<Display*>(handles.display);
			info.window = static_cast<::Window>(reinterpret_cast<uintptr_t>(handles.window));
			result = vkCreateXlibSurfaceKHR(instance, &info, nullptr, &surface);
			break;
		}
#endif
#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
		case Kld::WindowSystem::Wayland: {
			VkWaylandSurfaceCreateInfoKHR info{};
			info.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
			info.display = static_cast<wl_display*>(handles.display);
			info.surface = static_cast<wl_surface*>(handles.window);
			result = vkCreateWaylandSurfaceKHR(instance, &info, nullptr, &surface);
			break;
		}
#endif
#if defined(VK_USE_PLATFORM_WIN32_KHR)
		case Kld::WindowSystem::Windows: {
			VkWin32SurfaceCreateInfoKHR info{};
			info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			info.hinstance = static_cast<HINSTANCE>(handles.display);
			info.hwnd = static_cast<HWND>(handles.window);
			result = vkCreateWin32SurfaceKHR(instance, &info, nullptr, &surface);
			break;
		}
#endif
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
		case Kld::WindowSystem::Android: {
			VkAndroidSurfaceCreateInfoKHR info{};
			info.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
			info.window = static_cast<ANativeWindow*>(handles.window);
			result = vkCreateAndroidSurfaceKHR(instance, &info, nullptr, &surface);
			break;
		}
#endif
		default:
			throw std::invalid_argument(std::string("VkPlatform: no Vulkan surface support for window system ") +
										std::string(Kld::windowSystemName(handles.system)));
	}

	if (result != VK_SUCCESS) {
		throw std::runtime_error(std::string("VkPlatform: surface creation failed (") + resultName(result) + ")");
	}
	return surface;
}

}
