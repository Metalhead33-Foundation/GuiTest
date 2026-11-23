# Using Kaldi

This is a hands-on guide to the [Kaldi](../Kaldi/README.md) graphics HAL: how to
submit work to a backend, what the contracts are, and which of the two client APIs
to pick. It assumes you already know what a framebuffer and a vertex buffer are and
that the project renders through [Lotte](../Lotte/README.md); it does not assume any
Vulkan, OpenGL or Direct3D experience.

Kaldi has **two ways to say the same thing**:

| API | Type | What you write |
|---|---|---|
| Raw | tagged union `Kld::GfxOp` | an array of commands |
| Object-oriented | `Kaldi/KldCommandBuffer.hpp` | calls on RAII proxies |

Both produce the *same* command stream, and a stream recorded with one can be
submitted next to a stream recorded with the other.

---

## 1. The model in one page

A Kaldi backend is a **command interpreter**. Client code does not call the graphics
API directly; it fills an array of command records and hands the array to the
backend, which translates each record into its native API:

```cpp
const Kld::HalStatus status = device->processCommandQueue(commands);
```

That is the entire backend contract (`Kaldi/KldDevice.hpp`). Everything else in this
tutorial is about producing that array well.

Five rules explain most of the API:

1. **The span order is execution order.** Re-submitting is just recording again.
2. **Handles are words, not pointers.** A `Kld::HandleId` names a resource the
   backend owns. One numeric namespace is shared by every resource kind, so a
   texture id never collides with a buffer id (the facade's `HandleStorage`
   guarantees this for you - see §3.2).
3. **Destroy commands are retirement requests.** `DestroyTexture` means "the client
   will not use this handle again", not "the memory is free now". When the backend
   actually releases the resource differs per API (§5).
4. **Payload pointers stay yours.** If a command points at pixels, shader words, a
   descriptor or a callback, that memory must stay valid until
   `processCommandQueue` returns. Backends are allowed to copy, but the contract
   never requires it.
5. **A `Device` is not required to be thread-safe.** Recording may happen on any
   number of threads (one dispatcher each), but submission is one call on one thread.

Three handles are reserved and never come from any pool:

| Handle | Meaning |
|---|---|
| `Kld::DefaultFramebuffer` | the window's back buffer (swapchain image) |
| `Kld::DefaultSwapchain` | the lazily created default swapchain |
| `Kld::BackendHandleBase` and up | backend-owned framebuffers reported back to you |

`Kld::HalCapabilities` tells you what the backend can do *before* you record
anything that needs it:

```cpp
const Kld::HalCapabilities& caps = device->capabilities();
if (!caps.supportsCompute) { /* no DispatchCompute */ }
if (!caps.supportsReadback) { /* no ReadBuffer/ReadImage2D */ }
if (!caps.supportsExplicitSwapchains) { /* stay on BeginFrame/Present */ }
```

A backend that cannot provide the baseline answers `UnsupportedBaseline` instead of
silently doing nothing, and the message says which command and why.

---

## 2. The raw API: tagged unions

### 2.1 `GfxOp` layout

`Kld::GfxOp` is an opcode plus a union of payload structs:

```cpp
struct GfxOp {
	Opcode opcode;
	union { ... } opt;   // the active member is selected by opcode
};
```

You **write** it with designated initializers - the initializer names the union
member, so the `opt.` prefix never appears in recording code:

```cpp
ops.push_back({ Kld::Opcode::DestroyBufferObject, { .opDestroy = { bufferId } } });
```

You **read** it back with the member name in front:

```cpp
if (ops[0].opcode == Kld::Opcode::DestroyBufferObject) {
	const Kld::HandleId id = ops[0].opt.opDestroy.id;
}
```

### 2.2 First program: clear the window

```cpp
#include <Kaldi/KaldiOperation.hpp>
#include <Kaldi/KldDevice.hpp>
#include <cstdio>
#include <vector>

void clearTheWindow(Kld::Device& device)
{
	std::vector<Kld::GfxOp> ops;
	ops.push_back({ Kld::Opcode::BeginFrame,
			{ .opBeginFrame = { Kld::DefaultFramebuffer } } });
	ops.push_back({ Kld::Opcode::ClearColorAttachment,
			{ .opClearColorAttachment = { Kld::DefaultFramebuffer, 0,
						      { 0.1f, 0.2f, 0.3f, 1.0f } } } });
	ops.push_back({ Kld::Opcode::EndFrame,
			{ .opEndFrame = { Kld::DefaultFramebuffer } } });
	ops.push_back({ Kld::Opcode::Present,
			{ .opPresent = { Kld::DefaultFramebuffer } } });

	const Kld::HalStatus status = device.processCommandQueue(ops);
	if (!status.ok()) {
		// `message` is backend-owned and valid until the next call on this device.
		std::fprintf(stderr, "clear failed (%d): %s\n", static_cast<int>(status.code),
			     status.message != nullptr ? status.message : "(no message)");
	}
}
```

Notes:

- `BeginFrame`/`EndFrame` bracket work that draws into the default framebuffer.
  `Present` is what flips it in OpenGL and presents the acquired image in Vulkan.
- `status.message` is a *view* into backend storage. Copy it if you need it later.
- Nothing here allocates GPU memory, so there is no destroy command.


### 2.3 A textured, tinted triangle

Everything below is portable across the GL, GL ES and Vulkan backends; the unit
numbers are arbitrary but must stay unique across *all* resource kinds.

```cpp
using namespace Kld;   // only for brevity inside this example

const float vertices[9] = { -0.8f, -0.8f, 0.0f, 0.8f, -0.8f, 0.0f, 0.0f, 0.8f, 0.0f };
const std::uint8_t white[4] = { 255, 255, 255, 255 };
const float tint[4] = { 1.0f, 0.5f, 0.5f, 1.0f };
const AttributeDescriptor attributes[1] = { { "POSITION", 0, PrimitiveType::F32x3, 0 } };
const VertexDescriptor vertexInput { 12, attributes };
const SampledTextureBindingLayout textures[1] = {
	{ 0, TextureBindingType::Texture2D, 1, ShaderStage::Fragment }
};
const SamplerBindingLayout samplers[1] = { { 0, 1, ShaderStage::Fragment } };
const UniformBufferBindingLayout uniforms[1] = { { 0, 16, ShaderStage::Fragment } };
const PipelineBindingLayout bindings { textures, 1, samplers, 1, uniforms, 1,
				       nullptr, 0, nullptr, 0 };
const ShaderBinaryDescriptor shaders[2] = {
	{ ShaderStage::Vertex, spirvVert.data(), spirvVert.size() },
	{ ShaderStage::Fragment, spirvFrag.data(), spirvFrag.size() }
};
const FramebufferAttachment attachments[1] = { { 2, FramebufferAttachmentRole::Color, 0 } };

std::vector<GfxOp> ops;
ops.push_back({ Opcode::CreateBufferObject,
		{ .opCreateBufferObject = { 1, sizeof(vertices), BUFFER_USAGE_VERTEX,
					    vertices } } });
ops.push_back({ Opcode::CreateBufferObject,
		{ .opCreateBufferObject = { 5, sizeof(tint), BUFFER_USAGE_UNIFORM, tint } } });
ops.push_back({ Opcode::CreateTexture2D,
		{ .opCreateTexture2D = { 4, TextureFormat::RGBA8U, 1, false, 1, 1, white } } });
ops.push_back({ Opcode::CreateSampler,
		{ .opCreateSampler = { 6, SamplerFiltering::LINEAR,
				       { SamplerWrapping::REPEAT, SamplerWrapping::REPEAT,
					 SamplerWrapping::REPEAT },
				       1.0f, 1000.0f, 0.0f, 0.0f } } });
ops.push_back({ Opcode::CreateImage2D,
		{ .opCreateImage2D = { 2, TextureFormat::RGBA8U, 64, 64, 1,
				       IMAGE_USAGE_COLOR_ATTACHMENT } } });
ops.push_back({ Opcode::CreateFramebuffer,
		{ .opCreateFramebuffer = { 3, 64, 64, attachments, 1 } } });
ops.push_back({ Opcode::CreatePipeline,
		{ .opCreatePipeline = { 7, "triangle", shaders, 2, &vertexInput, &bindings } } });

ops.push_back({ Opcode::BindFramebuffer, { .opBindFramebuffer = { 3 } } });
ops.push_back({ Opcode::SetViewport,
		{ .opSetViewport = { 0.0f, 0.0f, 64.0f, 64.0f, 0.0f, 1.0f } } });
ops.push_back({ Opcode::SetScissor, { .opSetScissor = { 0, 0, 64, 64 } } });
ops.push_back({ Opcode::ClearColorAttachment,
		{ .opClearColorAttachment = { 3, 0, { 0.0f, 0.0f, 0.0f, 1.0f } } } });
ops.push_back({ Opcode::BindPipeline, { .opBindPipeline = { 7 } } });
ops.push_back({ Opcode::BindTexture,
		{ .opBindTexture = { 0, 4, TextureBindingType::Texture2D } } });
ops.push_back({ Opcode::BindSampler, { .opBindSampler = { 0, 6 } } });
ops.push_back({ Opcode::BindUniformBuffer,
		{ .opBindUniformBuffer = { 0, 5, 0, sizeof(tint) } } });
ops.push_back({ Opcode::BindVertexBuffer,
		{ .opBindVertexBuffer = { 0, 1, 0, 12, 0 } } });
ops.push_back({ Opcode::DrawTriangles, { .opDrawTriangles = { 0, 3 } } });

const HalStatus status = device->processCommandQueue(ops);
```

Five things worth internalising:

- **Ids are yours.** Nothing picks ids for you in the raw API; anything unique and
  non-zero works, but keep it unique across *all* kinds.
- **Binding slots are shader slots.** `BindTexture(0, ...)` feeds slot `0` of the
  pipeline's `SampledTextureBindingLayout`, which each backend maps onto its native
  namespace (Vulkan: descriptor set 0, binding 0).
- **The pipeline owns its shaders.** `CreatePipeline` compiles once and caches
  variants; nothing is recompiled per draw.
- **The backends validate.** A draw without a bound pipeline answers
  "no graphics pipeline is bound" instead of guessing.
- **Destroy order does not matter.** Destroy commands are retirement requests, so
  they never have to wait for the last draw; the backend owns the GPU lifetime (5).

### 2.4 Readback

To get pixels or bytes back to the host (for tests and tooling, not for a game
loop):

```cpp
struct Readback { std::vector<std::uint8_t> bytes; std::uint32_t pitch = 0; };

void onPixels(const void* data, std::uint32_t size, std::uint16_t width,
	      std::uint16_t height, std::uint32_t rowPitch, void* userData)
{
	auto* out = static_cast<Readback*>(userData);
	out->bytes.assign(static_cast<const std::uint8_t*>(data),
			  static_cast<const std::uint8_t*>(data) + size);
	out->pitch = rowPitch;
}

// ... after the draw, still in the same stream:
ops.push_back({ Opcode::ReadImage2D,
		{ .opReadImage2D = { 2, 0, 0, 0, 0, 64, 64, onPixels, &readback } } });
```

The callback runs *during* `processCommandQueue`, and the backend may synchronise
the GPU to make it possible - Vulkan submits and waits, GL calls `glReadPixels`.
`readback` therefore has to outlive the call, and `rowPitch` is authoritative:
never assume `width * texelSize`.

Depth/stencil images work too. The portable texel is the depth bytes followed by the
stencil byte (when the format has one), which is exactly what `pixelByteSize`
reports:

| Format | Portable texel |
|---|---|
| `DEPTH_COMPONENT16` | 2 bytes |
| `DEPTH_COMPONENT24` | 3 bytes, low 24 bits of the depth value |
| `DEPTH_COMPONENT32` | 4 bytes, depth in the low 24 bits |
| `DEPTH_COMPONENT32F` | 4 bytes, IEEE float |
| `DEPTH24_STENCIL8` | 3 depth bytes + 1 stencil byte |
| `DEPTH32F_STENCIL8` | 4 float bytes + 1 stencil byte |
| `STENCIL_INDEX8` | 1 stencil byte |

### 2.5 The C ABI

A backend builds into a shared library that exports a tiny C surface
(`Kld/KldImplLib.h`):

```cpp
MhRenderingDeviceHandle kld_create_rendering_device(const SDL_SysWMinfo* window,
						   const MhRenderingDeviceCreationInfo* info);
void        kld_destroy_rendering_device(MhRenderingDeviceHandle device);
const char* kld_get_renderer_name(void);
const char* kld_get_version(void);
const char* kld_get_last_error_message(void);
```

The handle is the backend's `Kld::Device` in disguise, so an engine that loads
backends at run time either forwards a C++ `processCommandQueue` pointer or exposes
its own `submit` shim. `kld_get_last_error_message()` returns the last failure
recorded on the calling thread - creation failures *and* command-queue failures -
and is `nullptr` only before the first failure.

---

## 3. The object-oriented facade

`Kaldi/KldCommandBuffer.hpp` wraps the tagged union in RAII proxies and a
double-buffered command vector. It records the same commands, so you can mix the two
styles freely.

### 3.1 Why bother

Three things the raw API makes you do by hand:

- pick and hand out handle ids;
- remember to record a destroy command for everything you created;
- keep the `Opcode`/payload pair consistent.

The facade does all three: `createXxx()` allocates an id and records the create,
the returned proxy records the destroy when it dies, and each proxy method pushes
exactly one well-formed command.

### 3.2 The pieces

```cpp
Kld::CommandDispatcher<> dispatcher;              // handle pool + double-buffered vector
auto vbo = Kld::OopFacade::createBuffer(dispatcher, size, Kld::BUFFER_USAGE_VERTEX, data);
vbo.bindVertex(0, 0, 12);                          // records BindVertexBuffer
dispatcher.getCommandBuffer().swapBuffers();       // write buffer -> read buffer
const Kld::HalStatus status = dispatcher.submit(*device);
```

| Type | Role |
|---|---|
| `Kld::HandleStorage<>` | one `IdPool` for **all** kinds, so ids never collide |
| `Kld::CommandDispatcher<>` | `HandleStorage` + `DoubleBufferedVector<GfxOp>` |
| `Kld::OopFacade` | `oop_detail<>`, the default instantiation with the recording helpers |
| `Kld::DeferredDestroyQueue` | explicit retirement list, flushed into a live dispatcher |
| `Kld::mergeCommandBuffers` | appends another dispatcher's read buffer, in order |

**The protocol is swap-then-submit.** `push`/`append` write the *write* buffer;
`swapBuffers()` moves it to the read buffer and clears the write side; `submit()`
hands the read buffer to a device. Recording and submitting never race because they
touch different halves.

### 3.3 Proxies

`createXxx()` returns a move-only proxy. It owns the handle: moving transfers the
destroy command, and destroying the last owner records it.

| Proxy | Commands it can record |
|---|---|
| `Buffer` | `resize`, `update`, `map`, `read`, `copyTo`, `barrier`, `transition`, `createVertexArray`, `bindUniform`, `bindStorage`, `bindVertex`, `bindIndex`, `drawIndirect`, `drawIndexedIndirect`, `dispatchComputeIndirect` |
| `Texture` | `resize2D`, `update2D`, `updateCompressed2D`, `updateCubeFace`, `update2DArray`, `generateMipmaps`, `blitTo`, `bind`, `transition` |
| `Image` | `resize2D`, `update2D`, `blitTo`, `resolveTo`, `copyTo`, `copyFromBuffer`, `copyToBuffer`, `read2D`, `bindStorage`, `barrier`, `transition` |
| `Sampler` | `bind` |
| `Framebuffer` | `bind`, `clearColor`, `clearDepthStencil` |
| `Pipeline` / `ComputePipeline` | `bind` |
| `Fence` | `signal`, `wait` |
| `QueryPool` | `reset`, `begin`, `end`, `writeTimestamp`, `readResults` |

Commands that name no resource stay on the facade as static helpers: `beginFrame`,
`endFrame`, `present`, `beginRenderPass`, `endRenderPass`, every draw form,
`setViewport`, `setScissor`, the three state setters, `dispatchCompute`,
`dispatchComputeIndirect`, `debugLabelBegin`, `debugLabelEnd`, `debugMarker`,
`mergeCommandBuffers`, and the swapchain trio (`resizeSwapchain`,
`acquireSwapchainImage`, `presentSwapchain`).

Ids are still plain words: a proxy converts to `Kld::HandleId`
(`operator HandleId()`), so it can go anywhere the raw API expects one, including
`FramebufferAttachment { image.id(), ... }`.

### 3.4 The same triangle, through the facade

```cpp
Kld::CommandDispatcher<> dispatcher;
auto vbo = Kld::OopFacade::createBuffer(dispatcher, sizeof(vertices),
					Kld::BUFFER_USAGE_VERTEX, vertices);
auto tintBuffer = Kld::OopFacade::createBuffer(dispatcher, sizeof(tint),
					       Kld::BUFFER_USAGE_UNIFORM, tint);
auto texture = Kld::OopFacade::createTexture2D(dispatcher, Kld::TextureFormat::RGBA8U, 1,
					       false, 1, 1, white);
auto sampler = Kld::OopFacade::createSampler(dispatcher, Kld::SamplerFiltering::LINEAR,
					     wrapping, 1.0f, 1000.0f, 0.0f, 0.0f);
auto target = Kld::OopFacade::createImage2D(dispatcher, Kld::TextureFormat::RGBA8U, 64, 64,
					    1, Kld::IMAGE_USAGE_COLOR_ATTACHMENT);
const Kld::FramebufferAttachment attachments[1] = {
	{ target.id(), Kld::FramebufferAttachmentRole::Color, 0 }
};
auto framebuffer = Kld::OopFacade::createFramebuffer(dispatcher, 64, 64, attachments, 1);
auto pipeline = Kld::OopFacade::createPipeline(dispatcher, "triangle", shaders, 2,
					       &vertexInput, &bindings);

Kld::OopFacade::setViewport(dispatcher, 0.0f, 0.0f, 64.0f, 64.0f, 0.0f, 1.0f);
Kld::OopFacade::setScissor(dispatcher, 0, 0, 64, 64);
framebuffer.bind();
framebuffer.clearColor(0, { 0.0f, 0.0f, 0.0f, 1.0f });
pipeline.bind();
texture.bind(0, Kld::TextureBindingType::Texture2D);
sampler.bind(0);
tintBuffer.bindUniform(0, 0, sizeof(tint));
vbo.bindVertex(0, 0, 12);
Kld::OopFacade::drawTriangles(dispatcher, 0, 3);
target.read2D(0, 0, 0, 0, 64, 64, onPixels, &readback);

dispatcher.getCommandBuffer().swapBuffers();
const Kld::HalStatus status = dispatcher.submit(device);   // device is a Kld::Device&
```

When the proxies go out of scope they record their destroys into the *write*
buffer. Recording those and submitting them is a normal cycle:

```cpp
}   // proxies die here
dispatcher.getCommandBuffer().swapBuffers();
dispatcher.submit(device);        // DestroyFramebuffer, DestroyPipeline, ...
```

### 3.5 Streams, merging and deferred retirement

`CommandDispatcher` is single-threaded for recording, so the pattern for worker
threads is *one dispatcher per thread* plus an explicit merge:

```cpp
Kld::CommandDispatcher<> mainThread;      // owns the device submission
Kld::CommandDispatcher<> streamA;         // filled by worker A
Kld::CommandDispatcher<> streamB;         // filled by worker B
// ... workers record ...
streamA.getCommandBuffer().swapBuffers();  // merge reads the read buffer
streamB.getCommandBuffer().swapBuffers();
Kld::mergeCommandBuffers(mainThread, { &streamA, &streamB });   // A's commands first
mainThread.getCommandBuffer().swapBuffers();
mainThread.submit(device);
```

When ownership crosses threads, or when a worker dispatcher has already been
swapped and must not receive more commands, use the explicit retirement list:

```cpp
Kld::DeferredDestroyQueue retired;
retired.retire(Kld::ResourceHandleKind::Texture, textureId);   // any thread
// ... later, on the submission thread:
retired.flushInto(mainThread);   // records DestroyTexture + frees the id
```

## 4. Choosing between the two

| Situation | Use |
|---|---|
| Engine code that owns resources for a long time | the facade - RAII is exactly the right fit |
| A hot loop that re-records every frame | raw - one `push_back` per command, no proxies to construct |
| Tooling, tests, one-off throws | raw - the whole program is one array |
| Anything multi-threaded | facade on the worker, merged into a submission dispatcher |
| A code generator or a serialized stream | raw - it is a POD array |

They mix: a facade dispatcher's `getCommandBuffer().onWriteBuffer(...)` and
`CommandDispatcher::append()` take raw `GfxOp`s, so a legacy recorder can feed the
same stream. Reading a stream back (`onReadBuffer`) gives the raw union - which is
why the `opt.` prefix matters (§2.1).

---

## 5. Backend notes

The command vocabulary is portable; when resources are actually freed is not.

| Backend | Retirement | Notes |
|---|---|---|
| OpenGL 4.x | `glDelete*` immediately: GL flags the object and frees it once nothing references it | the executor additionally clears its own vertex/index binding trackers, the bound framebuffer and the bound program, which GL cannot do for it |
| OpenGL ES 3.2 | same rule as GL | same handling |
| Vulkan 1.0 | frames in flight: destroys are queued per frame slot and run when that slot's fence signals; swapchain handles wait for a present marker | point payloads are copied or consumed when the command is recorded; `vkQueueSubmit` happens at the end of `process()` |
| Direct3D 11 / 12 | not implemented yet; `processCommandQueue` answers `UnsupportedBaseline` | capability probing works, so versions and limits are available |

Practical consequences:

- **A destroyed handle is dead immediately in the API sense.** Do not keep using it,
  even though the memory may still be alive.
- **Readback can synchronise.** `ReadBuffer`, `ReadImage2D` and `ReadQueryResults`
  are the only commands whose callbacks the client observes inline; everything else
  is asynchronous.
- **Explicit swapchains are optional.** `caps.supportsExplicitSwapchains` tells you
  whether `CreateSwapchain` is honoured; without it, stay on
  `BeginFrame`/`EndFrame`/`Present` with `DefaultFramebuffer`.
- **GL and GL ES share one executor** (`KaldiGLCommon/GlCommandExecutor.hpp`), so
  a difference between them is a dialect limitation, not a design choice: 1D
  textures, `PolygonMode::Line`/`Point`, depth clamp, base-instance draws and
  timestamp queries need GL-only features.

---

## 6. Things that bite

1. **`swapBuffers()` before `submit()`.** Submitting without swapping hands the
   device the *previous* read buffer - usually an empty one, which silently does
   nothing.
2. **Ids are unique across kinds.** Two pools that both start at 1 will collide;
   the facade's `HandleStorage` uses a single pool for exactly this reason.
3. **Payload pointers are borrowed.** Pixels, descriptor arrays, shader words and
   callbacks must outlive the `processCommandQueue` call.
4. **`status.message` is borrowed too.** Copy it before the next call on the same
   device; `kld_get_last_error_message()` keeps the last *failure*, not the last call.
5. **Read the union through `opt.`.** `op.opSetViewport` does not exist; it is
   `op.opt.opSetViewport`. Writing uses the designated initializer form instead.
6. **`rowPitch` is authoritative.** Backends align rows; never recompute the pitch
   from `width`.
7. **Binding slots are per-kind.** Texture slot 0 and uniform slot 0 are different
   bindings; Vulkan puts them in different descriptor sets.
8. **One dispatcher per recording thread**, merged explicitly. A `Device` is not
   required to be thread-safe.
9. **Depth/stencil readback returns the portable texel**, not the driver's internal
   layout: depth bytes first, stencil last (§2.4).
10. **Clip-space y is not portable.** The viewport *rectangle* uses the OpenGL
    convention everywhere (origin bottom-left), but the NDC y direction differs:
    Vulkan's `y = +1` is the bottom of the framebuffer, OpenGL's and Direct3D's is
    the top. Read `HalCapabilities::clipSpaceYAxis` (HAL 1.2) before building a UI
    transform or a projection matrix - otherwise the same vertices render
    vertically flipped on one backend. This is the classic reason a Vulkan port of
    an OpenGL renderer shows everything upside down. Lotte bundles the whole family
    of consequences (pixel transform, projection, winding, rectangle origin) into
    `Gfx::ClipSpaceConventions`, which `ResourceFactory::conventions()` hands to
    renderers, so Lotte code never reads the field itself.
11. **Hiding the system pointer is the host's job.** SDL keeps drawing its own
    cursor over your rendered one; `SDL_ShowCursor(SDL_DISABLE)` (and restoring it
    on focus loss) is not something a renderer can do for you.

---

## See also

- [Kaldi README](../Kaldi/README.md) - the HAL contract, opcode list and capability table
- [Shaders](../docs/SHADERS.md) - HLSL -> SPIR-V, and how each backend consumes it
- [Lotte](../Lotte/README.md) - the renderer built on this API
- `Kaldi/KldCommandBuffer.hpp` - every helper, documented in place
- `UnitTests/tests/KaldiCommandBufferTests.cpp` - the facade's behaviour as executable spec

```
