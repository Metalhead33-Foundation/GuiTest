# KaldiVK

Vulkan backend for the Kaldi graphics HAL (see [Kaldi](../Kaldi/README.md)).

**Status: complete executor.** The library builds and exports the backend C ABI,
`Vk::Device` owns the context and a `CommandExecutor`, and
`processCommandQueue()` records into one command buffer and submits
synchronously. Every opcode in `Kld::Opcode` is implemented except the handful
listed under "Deliberate gaps" below, which answer
`HalStatusCode::UnsupportedBaseline` with a precise message instead of silently
doing nothing. A textured, tinted triangle drawn into a 64x64 image and read back
per pixel has been validated on NVIDIA and lavapipe, together with buffers, the
default swapchain path, compute dispatch and the HLSL -> SPIR-V -> pipeline route.

## Descriptor sets

Vulkan has one binding namespace per descriptor set, while Kaldi numbers each
binding kind independently. The executor maps each kind to its own set and keeps
the portable slot number as the binding:

| Set | Kaldi binding kind | HLSL declaration |
|---|---|---|
| 0 | sampled textures (`BindTexture`) | `[[vk::binding(slot, 0)]] Texture2D` |
| 1 | samplers (`BindSampler`) | `[[vk::binding(slot, 1)]] SamplerState` |
| 2 | uniform buffers (`BindUniformBuffer`) | `[[vk::binding(slot, 2)]] cbuffer` |
| 3 | storage buffers (`BindStorageBuffer`) | `[[vk::binding(slot, 3)]] RWStructuredBuffer` |
| 4 | storage images (`BindStorageImage`) | `[[vk::binding(slot, 4)]] RWTexture2D` |

That is exactly what glslang's HLSL front-end emits for explicitly bound resources,
so the shader author's slot numbers need no renumbering. DXC honours the same
explicit decorations (`-fvk-auto-shift-bindings` only renumbers *register-derived*
bindings), so the mapping survives a compiler switch; see
[`docs/SHADERS.md`](../docs/SHADERS.md) for the current compiler, its deprecation
and the deferred DXC migration. A pipeline that declares a
binding slot requires a matching bound resource before it draws; otherwise the
draw fails with `InvalidResourceState` (a Vulkan draw with an unwritten
descriptor is undefined behaviour, not an ignored binding).

## Deliberate gaps

Explicit swapchain objects, base-instance draws (Vulkan 1.0 has no such draw
parameter without `VK_KHR_shader_draw_parameters`), vertex instance divisors
above 1, per-sampler LOD bias (a Vulkan 1.1 feature), mip generation for
depth/stencil and block-compressed images, blits/readback of depth/stencil
images, readback of block-compressed images, framebuffers with multisampled
attachments, block-compressed uploads whose extents are not multiples of four,
and debug labels (no debug-marker extension is enabled).

Vulkan consumes SPIR-V directly, so this backend needs no shader cross-compiler: it takes
the words straight out of `Kld::ShaderBinaryDescriptor`.

## Build

| Item | Value |
|---|---|
| qmake target | `KaldiVK` (`TEMPLATE = lib`, shared, C++20, no Qt) |
| Export macro | `MH_KALDI_IMPL_EXPORT` while building, `MH_KALDI_IMPL_API` for consumers |
| Files | `KaldiVulkanDevice.hpp` / `KaldiVulkanDevice.cpp` / `VkPlatform.hpp` / `VkContext.hpp` / `VkCapabilities.hpp` / `VkUtil.hpp` (+ `.cpp`) / `KldImplLib.h` |
| Depends on | Kaldi headers via `include(../Kaldi.pri)`; the glad Vulkan 1.0 loader from `../glad/src/vulkan.c`; `libEuphemy`/`libElvavena` at link time, because `CreateTexture2DFromDecodeTarget` consumes a Euphemy `DecodeTarget` |

## What exists today

| File | State |
|---|---|
| `KldImplLib.h` | Declares the backend C ABI: `kld_create_rendering_device`, `kld_destroy_rendering_device`, `kld_get_renderer_name`, `kld_get_version`, `kld_get_last_error_message` |
| `KaldiVulkanDevice.hpp` / `.cpp` | `Vk::Device : Kld::Device`; owns the `Vk::Context` and a `CommandExecutor`, implements `capabilities()` and `processCommandQueue()`, exposes `context()` and `executor()`; the C ABI creates/destroys a real device and keeps a thread-local error message |
| `VkCommandExecutor.hpp` / `.cpp` | The submission core: command pool/buffer, synchronous submit-and-wait, deferred destruction, handle registries, state tracking, barriers, fences and the swapchain operations |
| `VkExecutorFrames.cpp` | Default swapchain creation, render pass/framebuffers, acquire/present, render-pass scoping and clears |
| `VkExecutorResources.cpp` | Buffers, 1D/2D/3D/array/cube textures, 2D and multisampled images, samplers, framebuffers (one render pass per attachment set), staging uploads, mip chains and blits |
| `VkExecutorPipelines.cpp` | SPIR-V shader modules, descriptor set layouts per binding kind, graphics pipeline variants, compute pipelines, resource bindings, every draw form and compute dispatch |
| `VkExecutorTransfer.cpp` | Occlusion/timestamp queries, buffer and image copies, and buffer/image readback through staging |
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
- [x] Create instance/device/queues and a swapchain from the native surface (`VK_KHR_surface` + platform surface extension).
- [x] Port the resource handlers: 1D/2D/3D/array/cube textures, 2D and multisampled images, create/update/readback, mip chains, samplers and user framebuffers.
- [x] Port the pipeline half: descriptor set layouts from `Kld::PipelineBindingLayout`, graphics pipeline variants (topology/blend/depth/stencil/cull/depth bias), compute pipelines, bindings and every draw/indirect form.
- [x] Port transfers and queries: buffer/image copies, readback through staging, occlusion/timestamp queries and indirect dispatch.
- [x] Implement `Device::processCommandQueue(std::span<const GfxOp>)` for the whole Kaldi opcode set.
- [x] Validate a textured, tinted triangle with per-pixel readback on NVIDIA and lavapipe.
- [ ] Implement frames in flight and replace the synchronous submit-and-wait with fences and a deferred retirement queue (the hard part of a production submission model).
- [ ] Replace the descriptor-set-per-draw allocation with a ring of sets - the current design is correct but allocates per draw.
- [ ] Implement explicit swapchain objects (`OpCreateSwapchain` and friends) on top of `Vk::Context`'s surface.
- [ ] Enable and use a debug-marker extension so `supportsDebugLabels` can become true.
- [ ] Move static resources to device-local memory through a staging ring; buffers are currently host-visible for simplicity.
- [ ] Support multisampled framebuffer attachments (the descriptor, not the image, is the missing piece) and depth/stencil readback.
