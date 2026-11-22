# KaldiVK

Vulkan backend for the Kaldi graphics HAL (see [Kaldi](../Kaldi/README.md)).

**Status: complete executor, HAL 1.1.** The library builds and exports the backend C ABI,
`Vk::Device` owns the context and a `CommandExecutor`, and `processCommandQueue()` records
into one of three frame slots and submits **without waiting**: a slot is only waited on when
it is reused, or when a command needs host-visible results. Every opcode in `Kld::Opcode` is
implemented except the handful listed under "Deliberate gaps" below, which answer
`HalStatusCode::UnsupportedBaseline` with a precise message instead of silently doing
nothing. A textured, tinted triangle drawn into a 64x64 image and read back per pixel has
been validated on NVIDIA and lavapipe, together with buffers, the default swapchain path,
an explicit swapchain (create, acquire, draw, resize, present, destroy), a 4x multisampled
attachment with resolve, compute dispatch and the HLSL -> SPIR-V -> pipeline route. The
whole flow runs clean under `VK_LAYER_KHRONOS_validation`.

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

The sets come from a **per-frame-slot descriptor ring**. Each frame slot owns one
descriptor pool (plus extra pools if a frame needs more than a pool holds), the
pool is reset together with the slot's command pool once that slot's fence has
signalled, and every draw still writes its own set. A set therefore never has to
survive longer than the submission that reads it, and nothing accumulates across
frames.

## Explicit swapchains

`OpCreateSwapchain` and friends are implemented on top of the context surface
(`SwapchainDescriptor::nativeSurface` may also carry a `VkSurfaceKHR` as an opaque
pointer). Vulkan owns the presentable images, so they cannot be wrapped as `Image`
handles; instead **every swapchain image gets a framebuffer handle from the
backend-reserved range** (`Kld::BackendHandleBase` upwards), which
`AcquireSwapchainImage` reports through `SwapchainAcquireInfo::framebuffer`. The
whole draw path is `AcquireSwapchainImage` -> `BindFramebuffer(reported)` -> draw ->
`PresentSwapchain`. Those handles are retired by `DestroySwapchain`, and
`DestroyFramebuffer` rejects them.

Constraints worth knowing:

- A `VkSurfaceKHR` backs one swapchain at a time, so an explicit swapchain on the
  context's own surface conflicts with the default path. The default swapchain is
  created lazily on the first `DefaultFramebuffer` use for that reason; using both
  on one window reports `VK_ERROR_NATIVE_WINDOW_IN_USE_KHR` with an explanation.
  Pass a different surface to run two swapchains.
- Explicit swapchains are color-only here: `depthStencilFormat` must be
  `Format::INVALID`, and `SwapchainImageBinding` entries must carry zero image
  handles (they are accepted and validated, not honoured).
- Acquire transitions the image to `COLOR_ATTACHMENT_OPTIMAL` and present
  transitions it back to `PRESENT_SRC_KHR`; the render pass itself stays in the
  attachment layout so a re-begun pass is valid.
- Recreating a swapchain hands the old handle to the driver as `oldSwapchain`, so it
  retires as part of the replacement. That is the portable way to resize on a native
  window that can back only one swapchain, and it keeps the framebuffer handles
  reported to the caller stable.
- **Nothing in the swapchain lifecycle blocks on the presentation queue.**
  `vkQueuePresentKHR` is not covered by the submit fences, so a present arms a
  *marker*: an empty submission on the presentation queue, whose fence proves that
  the queue got past the present. Views, framebuffers and the render pass are retired
  through the frame slot's fence (a later submission, so its fence covers them), the
  swapchain handle is destroyed once the marker signals, and a handle that is still
  busy is parked and released by later non-blocking polls - or at teardown, after the
  device went idle. A presentation engine that never completes a present therefore
  stalls only the work that genuinely needs the queue, never a resize or destroy call.

## Memory

Allocation goes through [VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
(vendored as the system header `vk_mem_alloc.h`; see [VkMemory.hpp](VkMemory.hpp)),
so the backend never picks a memory type, tracks `memoryTypeBits` or owns a
suballocator itself. What it does decide is **where a resource lives**:

| Buffer policy | Memory | Why |
|---|---|---|
| `Vertex`, `Index`, plain transfer | device-local, filled through the staging ring | Static data: read by the GPU at full bandwidth, never by the host |
| `Uniform`, `Storage`, `Indirect`, `Readback` | host-visible, persistently mapped | Rewritten every frame (or read by the host); a GPU round-trip would be pure overhead |
| Images | device-local | The classic textured/attachment resource |

The **staging ring** is one host-visible buffer per frame slot, handed out in
aligned slices. A slice is only reused after the slot's fence signalled, so a copy
never reads bytes the host already replaced, and the ring grows (doubling, with the
old buffer retired through the slot's fence) rather than wrapping mid-frame. Every
staged copy ends with a `transferBarrier`, so a vertex fetch or sample later in the
same frame is ordered against it.

`MapBuffer`, `UpdateBuffer` and `ReadBuffer` therefore work on any buffer: the
host-visible path maps the allocation, and the device-local path round-trips
through the ring, keeping the portable map/modify semantics intact.

## Deliberate gaps

Base-instance draws (Vulkan 1.0 has no such draw parameter without
`VK_KHR_shader_draw_parameters`), vertex instance divisors above 1
(`VK_EXT_vertex_attribute_divisor`), per-sampler LOD bias (a Vulkan 1.1 feature),
mip generation for depth/stencil and block-compressed images, blits and readback
of depth/stencil images, readback of block-compressed images, and block-compressed
uploads whose extents are not multiples of four.

Vulkan consumes SPIR-V directly, so this backend needs no shader cross-compiler: it takes
the words straight out of `Kld::ShaderBinaryDescriptor`.

## Build

| Item | Value |
|---|---|
| qmake target | `KaldiVK` (`TEMPLATE = lib`, shared, C++20, no Qt) |
| Export macro | `MH_KALDI_IMPL_EXPORT` while building, `MH_KALDI_IMPL_API` for consumers |
| Files | `KaldiVulkanDevice.hpp` / `KaldiVulkanDevice.cpp` / `VkPlatform.hpp` / `VkContext.hpp` / `VkMemory.hpp` / `VkCapabilities.hpp` / `VkUtil.hpp` (+ `.cpp`) / `KldImplLib.h`; the executor is split across `VkCommandExecutor.cpp`, `VkExecutorFrames.cpp`, `VkExecutorSwapchains.cpp`, `VkExecutorResources.cpp`, `VkExecutorPipelines.cpp` and `VkExecutorTransfer.cpp` |
| Depends on | Kaldi headers via `include(../Kaldi.pri)`; the glad Vulkan 1.0 loader from `../glad/src/vulkan.c`; the VulkanMemoryAllocator header `vk_mem_alloc.h` (expected in the compiler's default include path, normally `/usr/local/include`); `libEuphemy`/`libElvavena` at link time, because `CreateTexture2DFromDecodeTarget` consumes a Euphemy `DecodeTarget` |

## What exists today

| File | State |
|---|---|
| `KldImplLib.h` | Declares the backend C ABI: `kld_create_rendering_device`, `kld_destroy_rendering_device`, `kld_get_renderer_name`, `kld_get_version`, `kld_get_last_error_message` |
| `KaldiVulkanDevice.hpp` / `.cpp` | `Vk::Device : Kld::Device`; owns the `Vk::Context` and a `CommandExecutor`, implements `capabilities()` and `processCommandQueue()`, exposes `context()` and `executor()`; the C ABI creates/destroys a real device and keeps a thread-local error message |
| `VkCommandExecutor.hpp` / `.cpp` | The submission core: three frame slots with their own command pool, command buffer and fence, a per-slot descriptor ring and retirement list, handle registries, state tracking, barriers, fences and debug labels |
| `VkExecutorFrames.cpp` | Default swapchain creation, render pass/framebuffers, acquire/present (with the layout transitions), render-pass scoping and clears |
| `VkExecutorSwapchains.cpp` | Explicit swapchain objects: create/destroy/resize/acquire/present, backend-owned framebuffer handles from the reserved range, per-image views and render pass |
| `VkExecutorResources.cpp` | Buffers, 1D/2D/3D/array/cube textures, 2D and multisampled images, samplers, framebuffers (one render pass per attachment set), the per-slot staging ring, mip chains and blits |
| `VkExecutorPipelines.cpp` | SPIR-V shader modules, descriptor set layouts per binding kind, graphics pipeline variants, compute pipelines, resource bindings, every draw form and compute dispatch |
| `VkExecutorTransfer.cpp` | Occlusion/timestamp queries, buffer and image copies, and buffer/image readback through staging |
| `VkCapabilities.hpp` / `.cpp` | `Vk::probeCapabilities(const Context&)`: maps `VkPhysicalDeviceLimits`, queue-family compute support and BC1/BC3 format support onto `Kld::HalCapabilities`; `supportsDebugLabels` stays false until the debug-marker extensions are enabled |
| `VkPlatform.hpp` / `.cpp` | glad/Vulkan bootstrap and `VkSurfaceKHR` creation from `Kld::NativeWindowHandles` (`VK_KHR_xlib_surface`, `VK_KHR_wayland_surface`, `VK_KHR_win32_surface`, `VK_KHR_android_surface`), plus `resultName()` |
| `VkContext.hpp` / `.cpp` | Instance creation (with optional `VK_LAYER_KHRONOS_validation`), physical-device scoring, graphics/present queue selection, logical device and queue retrieval |
| `VkMemory.hpp` / `.cpp` | The `VmaAllocator` owner (`Vk::MemoryAllocator`) plus the single `VMA_IMPLEMENTATION` translation unit; documents why VMA is pinned to Vulkan 1.0 and to glad's function loading |
| `KaldiVulkanDevice.cpp` | Stub exports: create returns `nullptr`, destroy is a no-op, renderer name is `"Vulkan Renderer"`, version is `"0.0.1"`, last error is `nullptr` |
| `VkUtil.hpp` / `VkUtil.cpp` | The Vulkan counterpart of `KaldiGLCommon/GlUtil`: Euphemy `Format` and `Kld::PrimitiveType` -> `VkFormat`, vertex binding/attribute descriptions, image/view types, sampler state, image usage, image layouts and access/stage masks |

`VkUtil` is a pure translation layer - no `VkDevice`, no allocation, no command buffer - so
it is unit-testable and reusable by any tooling that needs the same mappings.

## TODO

- [x] Implement `Device::capabilities()` and report the probed HAL version with limits from `VkPhysicalDevice`.
- [x] Add the command executor core: frame slots, fence-driven retirement, host-visible buffers, the default swapchain lifecycle, render state, clears, render passes and barriers.
- [x] Create instance/device/queues and a swapchain from the native surface (`VK_KHR_surface` + platform surface extension).
- [x] Port the resource handlers: 1D/2D/3D/array/cube textures, 2D and multisampled images, create/update/readback, mip chains, samplers and user framebuffers.
- [x] Port the pipeline half: descriptor set layouts from `Kld::PipelineBindingLayout`, graphics pipeline variants (topology/blend/depth/stencil/cull/depth bias), compute pipelines, bindings and every draw/indirect form.
- [x] Port transfers and queries: buffer/image copies, readback through staging, occlusion/timestamp queries and indirect dispatch.
- [x] Implement `Device::processCommandQueue(std::span<const GfxOp>)` for the whole Kaldi opcode set.
- [x] Validate a textured, tinted triangle with per-pixel readback on NVIDIA and lavapipe.
- [x] Implement frames in flight and replace the synchronous submit-and-wait with fences and a deferred retirement queue.
- [x] Replace the descriptor-set-per-draw allocation with a ring of sets (per frame slot).
- [x] Implement explicit swapchain objects (`OpCreateSwapchain` and friends) on top of `Vk::Context`'s surface.
- [x] Enable and use a debug-marker extension (`VK_EXT_debug_utils`) so `supportsDebugLabels` becomes true.
- [x] Support multisampled framebuffer attachments and `vkCmdResolveImage`-backed resolves.
- [x] Move static resources to device-local memory through a staging ring, with allocation routed through VulkanMemoryAllocator.
- [ ] Support depth/stencil readback (the image formats are already created; the copy paths reject them).
- [ ] Validate the explicit-swapchain resize/destroy flow on lavapipe: on this host its X11 WSI aborts the process with `BadDrawable` on the first present (a pixmap it owns is freed before `XPutImage` completes), so that flow is validated on NVIDIA only.
