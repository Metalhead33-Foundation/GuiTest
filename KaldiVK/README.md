# KaldiVK

Vulkan backend for the Kaldi graphics HAL (see [Kaldi](../Kaldi/README.md)).

**Status: stub.** The library builds and exports the backend C ABI, but no rendering is
implemented. `Vk::Device` declares no overrides for `Kld::Device`'s pure virtual
functions, so it is abstract and nothing can be submitted to it yet.

Vulkan consumes SPIR-V directly, so this backend needs no shader cross-compiler: it takes
the words straight out of `Kld::ShaderBinaryDescriptor`.

## Build

| Item | Value |
|---|---|
| qmake target | `KaldiVK` (`TEMPLATE = lib`, shared, C++20, no Qt) |
| Export macro | `MH_KALDI_IMPL_EXPORT` while building, `MH_KALDI_IMPL_API` for consumers |
| Files | `KaldiVulkanDevice.hpp` / `KaldiVulkanDevice.cpp` / `KldImplLib.h` / `VkUtil.hpp` / `VkUtil.cpp` |
| Depends on | Kaldi headers via `include(../Kaldi.pri)`; the glad Vulkan 1.0 loader from `../glad/src/vulkan.c` |

## What exists today

| File | State |
|---|---|
| `KldImplLib.h` | Declares the backend C ABI: `kld_create_rendering_device`, `kld_destroy_rendering_device`, `kld_get_renderer_name`, `kld_get_version`, `kld_get_last_error_message` |
| `KaldiVulkanDevice.hpp` | `Vk::Device : Kld::Device` placeholder class (constructor only) |
| `KaldiVulkanDevice.cpp` | Stub exports: create returns `nullptr`, destroy is a no-op, renderer name is `"Vulkan Renderer"`, version is `"0.0.1"`, last error is `nullptr` |
| `VkUtil.hpp` / `VkUtil.cpp` | The Vulkan counterpart of `KaldiGLCommon/GlUtil`: Euphemy `Format` and `Kld::PrimitiveType` -> `VkFormat`, vertex binding/attribute descriptions, image/view types, sampler state, image usage, image layouts and access/stage masks |

`VkUtil` is a pure translation layer - no `VkDevice`, no allocation, no command buffer - so
it is unit-testable and reusable by any tooling that needs the same mappings.

## TODO

- [ ] Implement `Device::capabilities()` and report `KaldiHalVersion1_0` with limits probed from `VkPhysicalDevice`.
- [ ] Implement `Device::processCommandQueue(std::span<const GfxOp>)` for the Kaldi opcode set.
- [ ] Create instance/device/queues and a swapchain from the native surface (`VK_KHR_surface` + platform surface extension).
- [ ] Implement descriptor sets, pipeline layouts and render passes from the Kaldi binding-layout and render-pass descriptors.
- [ ] Implement buffer/image uploads through a staging ring; `VkUtil` already supplies the formats and usages.
- [ ] Implement the backend-owned `DefaultFramebuffer`, explicit swapchains, acquire/present and `SwapchainLost` handling.
- [ ] Implement fences, barriers and deferred resource retirement.
- [ ] Return real status/error strings instead of `nullptr`.
- [ ] Add a smoke test that creates a device, uploads a texture and presents a cleared frame.
