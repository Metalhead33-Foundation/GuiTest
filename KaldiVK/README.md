# KaldiVK

Vulkan backend for the Kaldi graphics HAL (see [Kaldi](../Kaldi/README.md)).

**Status: stub.** The library builds and exports the backend C ABI, and `Vk::Device` now
implements `capabilities()` by probing the selected physical device, but
`processCommandQueue()` is still missing, so the class is abstract and nothing can be
submitted to it yet. What *does* work is the instance/device and capability half:
`Vk::Context` creates the instance, the window surface, picks a physical device with
graphics+present queues and creates the logical device, and `Vk::probeCapabilities()`
translates `VkPhysicalDeviceProperties::limits` plus the queue-family and BC-format
queries into the portable snapshot. The remaining work is command translation.

Vulkan consumes SPIR-V directly, so this backend needs no shader cross-compiler: it takes
the words straight out of `Kld::ShaderBinaryDescriptor`.

## Build

| Item | Value |
|---|---|
| qmake target | `KaldiVK` (`TEMPLATE = lib`, shared, C++20, no Qt) |
| Export macro | `MH_KALDI_IMPL_EXPORT` while building, `MH_KALDI_IMPL_API` for consumers |
| Files | `KaldiVulkanDevice.hpp` / `KaldiVulkanDevice.cpp` / `VkPlatform.hpp` / `VkContext.hpp` / `VkCapabilities.hpp` / `VkUtil.hpp` (+ `.cpp`) / `KldImplLib.h` |
| Depends on | Kaldi headers via `include(../Kaldi.pri)`; the glad Vulkan 1.0 loader from `../glad/src/vulkan.c` |

## What exists today

| File | State |
|---|---|
| `KldImplLib.h` | Declares the backend C ABI: `kld_create_rendering_device`, `kld_destroy_rendering_device`, `kld_get_renderer_name`, `kld_get_version`, `kld_get_last_error_message` |
| `KaldiVulkanDevice.hpp` / `.cpp` | `Vk::Device : Kld::Device`; owns the `Vk::Context` (instance + surface + device + queues), exposes `context()` and `apiName()`, and implements `capabilities()` by probing the selected physical device; still abstract because the command-queue half is unimplemented |
| `VkCapabilities.hpp` / `.cpp` | `Vk::probeCapabilities(const Context&)`: maps `VkPhysicalDeviceLimits`, queue-family compute support and BC1/BC3 format support onto `Kld::HalCapabilities`; `supportsDebugLabels` stays false until the debug-marker extensions are enabled |
| `VkPlatform.hpp` / `.cpp` | glad/Vulkan bootstrap and `VkSurfaceKHR` creation from `Kld::NativeWindowHandles` (`VK_KHR_xlib_surface`, `VK_KHR_wayland_surface`, `VK_KHR_win32_surface`, `VK_KHR_android_surface`), plus `resultName()` |
| `VkContext.hpp` / `.cpp` | Instance creation (with optional `VK_LAYER_KHRONOS_validation`), physical-device scoring, graphics/present queue selection, logical device and queue retrieval |
| `KaldiVulkanDevice.cpp` | Stub exports: create returns `nullptr`, destroy is a no-op, renderer name is `"Vulkan Renderer"`, version is `"0.0.1"`, last error is `nullptr` |
| `VkUtil.hpp` / `VkUtil.cpp` | The Vulkan counterpart of `KaldiGLCommon/GlUtil`: Euphemy `Format` and `Kld::PrimitiveType` -> `VkFormat`, vertex binding/attribute descriptions, image/view types, sampler state, image usage, image layouts and access/stage masks |

`VkUtil` is a pure translation layer - no `VkDevice`, no allocation, no command buffer - so
it is unit-testable and reusable by any tooling that needs the same mappings.

## TODO

- [x] Implement `Device::capabilities()` and report `KaldiHalVersion1_0` with limits probed from `VkPhysicalDevice` (the C ABI still waits for `processCommandQueue()`).
- [ ] Implement `Device::processCommandQueue(std::span<const GfxOp>)` for the Kaldi opcode set.
- [ ] Create instance/device/queues and a swapchain from the native surface (`VK_KHR_surface` + platform surface extension).
- [ ] Implement descriptor sets, pipeline layouts and render passes from the Kaldi binding-layout and render-pass descriptors.
- [ ] Implement buffer/image uploads through a staging ring; `VkUtil` already supplies the formats and usages.
- [ ] Implement the backend-owned `DefaultFramebuffer`, explicit swapchains, acquire/present and `SwapchainLost` handling.
- [ ] Implement fences, barriers and deferred resource retirement.
- [ ] Return real status/error strings instead of `nullptr`.
- [ ] Add a smoke test that creates a device, uploads a texture and presents a cleared frame.
