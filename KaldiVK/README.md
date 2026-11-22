# KaldiVK

Vulkan backend for the Kaldi graphics HAL (see [Kaldi](../Kaldi/README.md)).

**Status: partial executor (bring-up).** The library builds and exports the backend C
ABI, `Vk::Device` owns the context and a `CommandExecutor`, and
`processCommandQueue()` records into one command buffer and submits synchronously.
Buffers (host-visible, persistent mappings), the default swapchain lifecycle
(`BeginFrame`/`EndFrame`/`Present`), render-state tracking, clears, render passes and
barriers are implemented. Textures, images, samplers, framebuffers, pipelines,
descriptor sets, draws, compute, queries and copies are still **pending**: each of
those handlers answers `HalStatusCode::UnsupportedBaseline` with its opcode name
(see `VkExecutorUnsupported.cpp`). The device/buffer/frame path has been validated on
NVIDIA and lavapipe with a live smoke test. `Vk::Context` still creates the instance,
surface, physical/logical device and queues; `Vk::probeCapabilities()` fills the
portable snapshot.

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
| `KaldiVulkanDevice.hpp` / `.cpp` | `Vk::Device : Kld::Device`; owns the `Vk::Context` and a `CommandExecutor`, implements `capabilities()` and `processCommandQueue()`, exposes `context()` and `executor()`; the C ABI creates/destroys a real device and keeps a thread-local error message |
| `VkCommandExecutor.hpp` / `.cpp` | The submission core: command pool/buffer, synchronous submit-and-wait, deferred destruction, handle registries, state tracking, barriers, fences and the swapchain operations |
| `VkExecutorFrames.cpp` | Default swapchain creation, render pass/framebuffers, acquire/present, render-pass scoping and clears |
| `VkExecutorResources.cpp` | Host-visible buffer creation/update/map, image storage/view creation, layout transitions |
| `VkExecutorUnsupported.cpp` | Named `UnsupportedBaseline` handlers for the not-yet-ported texture/image/sampler/framebuffer/pipeline/descriptor/draw/copy/query commands, with the porting notes in the file header |
| `VkCapabilities.hpp` / `.cpp` | `Vk::probeCapabilities(const Context&)`: maps `VkPhysicalDeviceLimits`, queue-family compute support and BC1/BC3 format support onto `Kld::HalCapabilities`; `supportsDebugLabels` stays false until the debug-marker extensions are enabled |
| `VkPlatform.hpp` / `.cpp` | glad/Vulkan bootstrap and `VkSurfaceKHR` creation from `Kld::NativeWindowHandles` (`VK_KHR_xlib_surface`, `VK_KHR_wayland_surface`, `VK_KHR_win32_surface`, `VK_KHR_android_surface`), plus `resultName()` |
| `VkContext.hpp` / `.cpp` | Instance creation (with optional `VK_LAYER_KHRONOS_validation`), physical-device scoring, graphics/present queue selection, logical device and queue retrieval |
| `KaldiVulkanDevice.cpp` | Stub exports: create returns `nullptr`, destroy is a no-op, renderer name is `"Vulkan Renderer"`, version is `"0.0.1"`, last error is `nullptr` |
| `VkUtil.hpp` / `VkUtil.cpp` | The Vulkan counterpart of `KaldiGLCommon/GlUtil`: Euphemy `Format` and `Kld::PrimitiveType` -> `VkFormat`, vertex binding/attribute descriptions, image/view types, sampler state, image usage, image layouts and access/stage masks |

`VkUtil` is a pure translation layer - no `VkDevice`, no allocation, no command buffer - so
it is unit-testable and reusable by any tooling that needs the same mappings.

## TODO

- [x] Implement `Device::capabilities()` and report `KaldiHalVersion1_0` with limits probed from `VkPhysicalDevice`.
- [x] Add the command executor core: one command buffer, synchronous submit-and-wait, deferred destruction, host-visible buffers, the default swapchain lifecycle, render state, clears, render passes and barriers.
- [x] Validate the device/buffer/frame path on NVIDIA and lavapipe.
- [ ] Port the resource handlers: 2D textures and images (create/update/readback, mip blits), samplers and user framebuffers (render passes per framebuffer).
- [ ] Port the pipeline half: descriptor set layouts from `Kld::PipelineBindingLayout`, graphics pipeline variants (topology/blend/depth/stencil/cull), compute pipelines, bindings and every draw/indirect form.
- [ ] Port transfers and queries: buffer/image copies, readback through staging, occlusion/timestamp queries, and indirect dispatch.
- [ ] Implement `Device::processCommandQueue(std::span<const GfxOp>)` for the Kaldi opcode set. (The entry point exists; the handlers above are pending.)
- [x] Create instance/device/queues and a swapchain from the native surface (`VK_KHR_surface` + platform surface extension).
- [ ] Implement descriptor sets, pipeline layouts and render passes from the Kaldi binding-layout and render-pass descriptors.
- [ ] Implement buffer/image uploads through a staging ring; `VkUtil` already supplies the formats and usages.
- [ ] Implement the backend-owned `DefaultFramebuffer`, explicit swapchains, acquire/present and `SwapchainLost` handling. (The default path works; explicit swapchains are `UnsupportedBaseline`.)
- [ ] Implement frames in flight and replace the synchronous submit-and-wait with fences and a deferred retirement queue.
- [ ] Return real status/error strings instead of `nullptr`. (The device and executor report messages.)
