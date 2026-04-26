# Kaldi

It is intended to become a graphics library abstraction layer that Lotte will be built on top of.

When development starts, this folder will host the Lotte library implementation and supporting project files.

## Current command model

`KaldiOperation.hpp` is the canonical example for the new VM-like API shape. It uses a single `HandleId` namespace for backend-preallocated resources, an `Opcode` enum to identify commands, and small POD payload structs inside `GfxOp`.

`KldHal.hpp` defines the public HAL contract: `HalVersion`, `HalCapabilities`, `HalStatusCode`, and `HalStatus`. Backend-specific probing stays inside backend initialization. Higher layers consume the resulting portable capabilities and status codes instead of depending on backend-specific extension or feature details.

Command payloads may reference externally owned data by pointer. As with the existing texture upload commands, callers must keep pointed-to data alive until the backend has processed the command queue. That rule covers upload/readback memory, shader words, vertex descriptors, framebuffer attachment arrays, pipeline binding layouts, swapchain descriptors/image bindings/native surface pointers, render-pass descriptors/attachment arrays, debug strings, optional pipeline names, callbacks, and callback `userData`.

Readback commands use POD callbacks plus `void* userData`, matching the buffer mapping command style. Backend-provided callback data is valid only for the duration of the callback; callers must copy anything they need after the callback returns. The callback pointer and `userData` must remain valid until the backend has processed the command queue.

Pipeline shader binaries are SPIR-V word arrays. `ShaderBinaryDescriptor` stores the shader stage, a `const uint32_t*` pointer, and a word count; the pointed-to words must outlive command processing. `VertexDescriptor` remains the vertex input contract. `PipelineBindingLayout` can optionally describe sampled texture, sampler, uniform buffer, storage buffer, and storage image slots when a backend needs explicit shader resource layout metadata. There are still no built-in pipelines; engines provide their own shader binaries, vertex input, and binding layout declarations.

Compressed texture commands treat payload bytes as opaque backend-facing block-compressed data. The command includes explicit block width, block height, and bytes-per-block metadata so DDS-style mip payloads can be passed through without CPU decompression.

Texture creation from `const Euph::Media::Image::DecodeTarget*` treats frames as mip levels: frame 0 is mip 0, frame 1 is mip 1, and so on. If `generateMipmaps` is true, the backend uses frame 0 and generates the rest.

`ResourceFactory::beginFrame()` maps to submitting a short command queue that begins the frame and binds the default framebuffer. `ResourceFactory::endFrame()` maps to submitting a short command queue that ends the frame and presents the default framebuffer. The device remains the submission boundary through `Device::processCommandQueue(std::span<const GfxOp>)`, which returns `HalStatus`. The old misspelled `processCommandQeue` name remains as a compatibility wrapper.

## Kaldi HAL 1.0 contract

Kaldi HAL 1.0 is identified by `KaldiHalVersion1_0`. A conforming backend reports a `HalCapabilities` snapshot after its private initialization/probing step. The baseline is the command vocabulary already present in `KaldiOperation.hpp`: buffers, sampled textures, images, framebuffers, samplers, graphics and compute pipelines, render state, synchronization, transfer/readback, cube maps, texture arrays, multisample resolves, storage resources, indirect work, queries, debug labels, and instanced draws. Capability limits tell higher layers how much of that vocabulary can be used portably on a specific backend instance.

Occlusion queries use the common denominator shared by OpenGL 4.x, Direct3D 11, Vulkan, Direct3D 12, and WebGPU: portable callers may depend only on binary zero/nonzero visibility. A readback value of `0` means no samples passed the active per-fragment tests; any nonzero value means at least one sample passed. Backends that naturally produce exact sample counts may return those counts through the existing `uint64_t` query result path, but portable engine code must treat `QueryType::Occlusion` as binary unless a future precise query type or explicit capability says otherwise.

Backend failures are reported through `HalStatus`. Initialization can fail because the baseline is unsupported. Submission can report invalid commands, invalid handles, invalid resource states, allocation failure, swapchain loss, shader creation failure, pipeline creation failure, or a generic backend error. Optional status messages are backend-owned text; callers copy the message if they need it later.

The default framebuffer is backend-owned and addressed as `DefaultFramebuffer`. Version 1.0 keeps swapchain creation, resize, default color/depth formats, and present mode inside the backend. `BeginFrame`, `EndFrame`, and `Present` report failures through submission status, with `SwapchainLost` covering resize/recreation cases until a future explicit swapchain API exists. Default framebuffer images are not directly referenced as ordinary `Image` handles in v1.0.

Destroy commands are logical retirement requests. Backends must delay physical deletion until all previously submitted work that can reference the resource has completed. RAII proxy destruction records the destroy command; it does not imply immediate backend deletion. This is especially important when multiple frames are in flight.

`CommandBuffer` remains the single-threaded v1.0 recording primitive. Append order is execution order. Per-thread merge helpers, shared handle allocators, deferred destruction queues, and opt-in thread-safe facades remain higher-level or future additions.

## Kaldi HAL 1.1 contract additions

Explicit swapchains are optional caller-visible presentation objects addressed by `HandleId`, with `DefaultSwapchain` reserved as `0` for the backend-owned default target. `SwapchainDescriptor` carries an opaque backend-facing native surface pointer, dimensions, color/depth formats, present mode, image count, and optional `SwapchainImageBinding` entries that associate swapchain image indices with predeclared image/framebuffer handles. `AcquireSwapchainImage` reports `SwapchainStatus` plus the active image through a POD callback, and `PresentSwapchain` presents an image by index. Existing `BeginFrame`, `EndFrame`, and `Present` remain the simple default-framebuffer path. Swapchain loss is still reported through `HalStatusCode::SwapchainLost`.

Render-pass metadata is available through `RenderPassDescriptor`, `RenderPassColorAttachment`, and `RenderPassDepthStencilAttachment`. `BeginRenderPass` implicitly establishes its framebuffer for following draw commands and describes each attachment's load, clear, store, or don't-care policy. `EndRenderPass` closes that scope. The older `BindFramebuffer` plus explicit clear commands remain valid for immediate-style backends and simple command streams.

Parallel recording support is deliberately conservative. A shared `HandleAllocator` gives worker command buffers disjoint handle IDs, but it does not make a single `CommandBuffer` thread-safe. `CommandBuffer::append` and `mergeCommandBuffers` concatenate already-recorded command spans in caller-specified order, which is the execution order. Cross-thread resource ownership uses `release()` plus a coordinator-owned `DeferredDestroyQueue`; flushing that queue records typed destroy commands into the coordinator command buffer.

## Facade model

`KldCommandBuffer.hpp` provides a thin client-side object-oriented facade over the VM command interface. `Kld::CommandBuffer` owns command storage with `std::pmr::vector<GfxOp>`, allocates local monotonic `HandleId` values starting at `1`, and submits spans to `Kld::Device`. Resource proxy classes are move-only RAII wrappers; when an owning wrapper is destroyed, it appends the matching destroy operation to its command buffer.

`Kld::CommandBuffer` is not internally synchronized. Record from one thread at a time, use separate command buffers per worker thread, or provide external synchronization. Making a shared command buffer thread-safe requires protecting both ID allocation and command storage, not just making the ID counter atomic.

## Thread-safe command recording ruminations

The current `CommandBuffer` is deliberately single-threaded. That is a valid long-term option, not merely an omission: command recording is cheap, command ordering is explicit, the facade stays header-only and small, and users who care about parallel recording can already give each worker its own command buffer. The downside is that the RAII proxies are only as safe as their owning command buffer. Destroying a `Texture`, `Buffer`, `Pipeline`, or other owning proxy appends a destroy command, so moving or destroying proxies on another thread is also a recording operation. With the current design, "do not share a command buffer across threads" must include everything owned by it.

A coarse-locking `ThreadSafeCommandBuffer` would be the most conservative future facade. It could wrap `nextId`, `commandStorage`, `push`, `allocateId`, `clear`, `span`, `submit`, and every RAII-destruction path behind one mutex. This is easy to reason about and preserves a single total command order matching lock acquisition order. It would also make proxy destruction from different threads mechanically safe. The cost is that every tiny inline helper becomes a lock/unlock pair, `span()` cannot safely expose storage while another thread records, and `submit()` or `clear()` would need clear rules: either hold the lock while the backend consumes the span, or snapshot/copy the queue first. Holding the lock through backend processing is simple but may block destructors and producers for too long; copying preserves responsiveness but undermines the current externally owned, no-extra-ownership model.

Making only `nextId` atomic is insufficient. It prevents duplicate handles, but `std::pmr::vector<GfxOp>` mutation remains unsafe, `clear()` can race with `push()`, and RAII destructors can append destroy commands while another thread is recording or submitting. An atomic-ID plus locked-storage design is possible, but once storage is locked, the atomic counter buys little except narrower critical sections around resource creation. It also does not solve command ordering: two threads can allocate IDs in one order and append creation commands in another. That may be acceptable, but it should be documented as "append order is execution order; ID order is not semantic."

Per-thread command buffers with an explicit merge step probably fit the VM model best. Each worker records into its own unsynchronized `CommandBuffer`, using either disjoint handle ranges or a shared thread-safe `HandleAllocator`. At the frame boundary, a coordinator concatenates command spans in an explicit order and submits one combined queue or multiple ordered queues. This keeps normal recording fast, avoids locking around every command, and makes ordering a caller-visible decision. The price is that resource ownership becomes less local: an RAII proxy cannot safely append its destroy command into a worker buffer that has already been merged, cleared, or destroyed. This model likely wants non-owning lightweight handles for cross-thread use, plus an explicit retirement/deferred-destroy queue owned by the coordinator.

Another possible design is a multi-producer command sink: workers reserve slots in append-only chunks, write POD `GfxOp`s into those chunks, and a submitter drains completed chunks. This can reduce lock contention while still sharing one logical command stream. It is also much more complex. The sink needs stable memory for chunks, a way to publish fully written commands, a deterministic ordering rule, and careful handling of pointer payload lifetimes. Because many commands reference externally owned data, "command recorded" is not enough; the pointed-to bytes, shader words, names, descriptors, and callbacks must outlive backend processing.

An actor/owner-thread model is the safest high-level ownership story. All owning proxies would be destroyed on the command-buffer owner thread, or their destructors would enqueue lightweight destroy requests to that owner. This avoids shared mutation of command storage and makes lifetime rules crisp. It is less convenient for users who expect C++ RAII objects to be freely movable between threads, and it introduces a queue or mailbox anyway. It also means destruction is no longer simply "append now"; it becomes "request destruction, owner records later."

Recommended future direction: keep `CommandBuffer` unsynchronized and documented as the zero-overhead primitive; add a separate parallel-recording layer if real examples need it. The likely first step should be a shared `HandleAllocator` plus per-thread command buffers and an explicit merge/submission helper. If RAII proxies must cross threads, add a coordinator-owned deferred destruction queue rather than making every proxy destructor lock a shared command vector. A coarse-locking wrapper is still useful as a debug or convenience facade, but it should be opt-in so the default API keeps its current predictable cost model.

## Modern graphics HAL foundation ruminations

Kaldi is not a game engine and should not grow engine-level concepts such as worlds, entities, visibility systems, materials, asset databases, animation graphs, physics, networking, editor tooling, or gameplay-facing scene ownership. Its job is lower-level: provide a small, stable command vocabulary that can abstract backend APIs such as OpenGL 4.x, Direct3D 11, Vulkan, Direct3D 12, and future APIs with different execution and synchronization models.

The current command vocabulary is already enough to describe a useful amount of GPU work: static and dynamic buffers, textures, cube maps, texture arrays, samplers, graphics pipelines, render targets, clears, draws, indexed draws, instanced draws, explicit updates, transfer/readback work, multisampled resolves, storage buffers/images, frame lifecycle, render state, barriers, compute, indirect work, queries, and debug labels. Query portability should favor the shared subset first: for example, occlusion queries are visibility tests with zero/nonzero results, not guaranteed precise sample counters. That is enough for a higher layer to start building renderers in the spirit of Quake 3, Unreal Engine 1998, or many 2000s-era engines. Those engines' higher-level systems should live above Kaldi and compile their decisions down into existing buffer, texture, pipeline, bind, and draw commands.

The gap is not "more engine." The gap is contract clarity. A low-level abstraction must tell callers what the device can do, what limits and formats are supported, how command submission and resource lifetime work, how swapchains/default framebuffers are represented, how binding layouts map onto very different backend APIs, and how errors are reported. Without those contracts, the command stream can express work, but a renderer cannot confidently plan portable work.

OpenGL 4.x and Direct3D 11 can hide many synchronization and lifetime details behind driver-managed state. Vulkan and Direct3D 12 make those details explicit. Kaldi should avoid becoming a direct clone of either family, but it needs enough vocabulary that both styles can be implemented faithfully: explicit submission boundaries, predictable resource state transitions, clear ownership of command payload memory, and a defined policy for work that a backend cannot support directly.

Compatibility with Quake 3-like or early-Unreal-like renderers is still a useful pressure test, but only as a workload shape: lots of static geometry, lightmaps, dynamic meshes, render-state sorting, texture streaming, visibility-driven batches, and tool readback. Kaldi should not implement those systems. It should make sure a layer above can implement them without relying on backend-specific escape hatches for basic graphics work.

The strongest next move is probably not to add every modern GPU concept. It is to define the contracts that make the existing commands dependable across old immediate-style APIs, modern explicit APIs, and future backends: capabilities, submission lifetime, default framebuffer ownership, binding layout conventions, synchronization semantics, and backend error reporting. After that, add missing resource kinds only when a concrete backend or renderer-shaped smoke test exposes a real abstraction gap.

## Completed migration ledger

- [x] Complete the missing framebuffer/render-target vocabulary in the old model.
- [x] Add Kaldi image/framebuffer/bind/clear/present commands.
- [x] Translate old pipeline resource binding and draw calls into Kaldi VM commands.
- [x] Translate mesh and uniform-buffer mutation into explicit buffer resize/upload/update commands.
- [x] Support legacy mesh and uniform-buffer mutation via memory-mapping commands that temporarily map the data into memory, execute a function pointer callback, and avoid `std::function` so ops stay PODs.
- [x] Translate writable texture resize/blit/update into explicit texture/image mutation commands.
- [x] Define the final `ResourceFactory::beginFrame` / `endFrame` mapping onto bind, submit, and present command queues.
- [x] Decide whether shader binaries need a richer typed stage model than the current externally owned descriptors. Shaders are SPIR-V `const uint32_t*` plus `size_t` word-count pairs with explicit shader stages.
- [x] Brainstorm other commands that one would expect from a modern-ish 3D API abstraction / HAL.
- [x] Implement render state commands from the Future HAL Command Backlog.
- [x] Implement synchronization and resource transition commands from the Future HAL Command Backlog.
- [x] Implement compute, indirect work, query, and debug commands from the Future HAL Command Backlog.
- [x] Create a thin, header-only, zero-overhead / minimal-overhead (via use of inline functions) object-oriented facade around the ID, command buffer, client-server, VM-like interface to reduce boilerplate, consisting of classes that provide object-oriented, RAII-oriented (Resource acquisition is initialization) wrappers around this command buffer interface. This will require a class that actually owns the command buffer via a `std::pmr::vector<GfxOp>` (implementations of `Kld::Device` will NOT own the data - we are decoupling ownership of the data from operations on the data!) These object-oriented proxies shall only contain the dumb IDs of their respective resources (e.g. TextureId) and a pointer to the class that owns the `std::pmr::vector<GfxOp>`.
- [x] Document everything Doxygen-style.
- [x] Support compressed texture creation/update with explicit block metadata.
- [x] Support creating 2D textures directly from `const Euph::Media::Image::DecodeTarget*` pointers, interpreting frames as mip levels.
- [x] Add command-buffer facade helpers for compressed textures and `DecodeTarget*` texture creation.
- [x] Explore and document future thread-safe command recording options.
- [x] Add low-level transfer and readback commands with POD callback payloads.
- [x] Add cube maps, 2D texture arrays, multisampled images, resolve commands, storage bindings, and instanced draws as HAL primitives.
- [x] Add command-buffer facade helpers for transfer/readback and advanced resource commands.
- [x] Define Kaldi HAL versioning and backend-reported v1.0 capability limits.
- [x] Define portable backend status reporting for initialization, unsupported baselines, invalid commands/handles/states, allocation failures, swapchain loss, and shader/pipeline creation failures.
- [x] Specify command payload, callback/user-data, shader word, descriptor, debug string, upload, and readback lifetime rules.
- [x] Define the v1.0 default framebuffer/swapchain contract around backend-owned `DefaultFramebuffer` and status-returning frame/present submission.
- [x] Define resource retirement as backend-deferred physical deletion after logical destroy commands.
- [x] Keep `CommandBuffer` single-threaded for v1.0 and document future parallel-recording directions separately.
- [x] Add explicit optional shader resource binding layout metadata while keeping engine-owned shaders and `VertexDescriptor` as the vertex input contract.
- [x] Decide that the current bind/clear/draw model is sufficient for v1.0; render-pass/load-store metadata can wait for a concrete backend need.
- [x] Keep facade ergonomics in parity with the raw command vocabulary for the current opcode set.
- [x] Add explicit swapchain objects, present modes, image/framebuffer bindings, acquire callbacks, and present commands.
- [x] Add render-pass/load-store metadata while keeping the older bind/clear/draw model valid.
- [x] Add conservative parallel recording helpers: shared handle allocation, ordered merge/append, and coordinator-owned deferred destruction.

## Future graphics HAL TODOs

- [ ] Validate the swapchain contract against real OpenGL, Vulkan, Direct3D 11, and Direct3D 12 backends.
- [ ] Validate render-pass/load-store metadata against tile-based and explicit APIs, then add subpasses or dependency metadata only if real backend work needs it.
- [ ] Build a renderer-shaped parallel recording example to pressure-test merge ordering, shared allocation, and deferred destruction ergonomics.
