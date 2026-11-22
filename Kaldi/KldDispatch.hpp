#ifndef KLDDISPATCH_HPP
#define KLDDISPATCH_HPP
/**
 * @file KldDispatch.hpp
 * @brief Compiler-portable command dispatch primitives for Kaldi backend implementations.
 *
 * A Kaldi backend consumes an externally owned `std::span<const Kld::GfxOp>` and
 * has to run one handler per tagged union. That is exactly the inner loop of a
 * bytecode interpreter, so the usual interpreter dispatch trade-offs apply:
 * every extra branch in front of a handler competes for branch-predictor history
 * with all the other commands.
 *
 * This header packages the two dispatch structures that make sense for Kaldi
 * backends behind one set of macros:
 *
 *  - **Direct threading** (a.k.a. *computed goto*) on GCC and Clang, using the
 *    `&&label` / `goto *pointer` extensions. The dispatch jump is duplicated at
 *    the end of every handler, so the CPU sees one indirect branch per opcode
 *    instead of one shared branch for the whole stream. The indirect-branch
 *    predictor can then learn real per-handler successor patterns.
 *  - **A plain `switch`** everywhere else (most importantly MSVC), which also
 *    keeps the API usable on compilers without the GNU label extension.
 *
 * Both structures are generated from the same handler bodies, so a backend is
 * written once and the compiler picks the better structure. Define
 * `KLD_DISPATCH_FORCE_SWITCH` to force the portable path (used by the unit tests
 * to exercise both).
 *
 * @section klddispatch_why Why two structures at all?
 *
 * Direct threading only pays off when the dispatch branch itself is a measurable
 * part of the cost. For a Kaldi command stream each handler usually issues one
 * or more driver calls, and those dwarf the dispatch branch; the two structures
 * then perform identically. The reason to keep the computed-goto variant is that
 * the same macros also serve cheap state-only opcodes (viewports, scissors,
 * debug labels, debug markers, bind and barrier bookkeeping) and any future
 * interpreter-shaped work, where the duplicated dispatch jump is exactly what
 * you want.
 *
 * A dense `switch` over an enum is already compiled to a jump table by GCC,
 * Clang and MSVC, so the portable path is not a linear comparison chain either.
 * Its remaining weakness is that all opcodes share a single dispatch branch;
 * that is the aliasing problem direct threading removes.
 *
 * Guaranteed tail calls (`[[clang::musttail]]`) would distribute the indirect
 * branches in the same way, but they require every handler to become its own
 * function with the whole execution state threaded through its parameters, the
 * attribute is not standard C++ yet (Clang only), and MSVC offers neither the
 * attribute nor a guaranteed tail call for mutually recursive handlers - it
 * would eventually overflow the stack. Direct threading reaches the same
 * per-handler branch distribution inside a single stack frame, which is why it
 * is the better default for a library that has to build with GCC, Clang and
 * MSVC from one source tree.
 *
 * @section klddispatch_usage Usage
 *
 * @code
 * Kld::HalStatus GL::Device::processCommandQueue(std::span<const Kld::GfxOp> commands)
 * {
 *     KLD_DISPATCH_RUN(commands, KLD_OPCODE_LIST);
 *     KLD_DISPATCH_ON(CreateBufferObject)
 *     {
 *         // `kldIp` points at the current command, `kldIp->opt.opCreateBufferObject`
 *         // is the payload.
 *         createBuffer(kldIp->opt.opCreateBufferObject);
 *         KLD_DISPATCH_NEXT();
 *     }
 *     KLD_DISPATCH_ON(BeginFrame)
 *     {
 *         kldStatus = beginFrame(kldIp->opt.opBeginFrame);
 *         KLD_DISPATCH_NEXT();
 *     }
 *     // ... one KLD_DISPATCH_ON per opcode in the list ...
 *     KLD_DISPATCH_UNKNOWN()
 *     {
 *         kldStatus = { Kld::HalStatusCode::InvalidCommand, Kld::opcodeName(kldIp->opcode) };
 *         KLD_DISPATCH_ABORT();
 *     }
 *     KLD_DISPATCH_END();
 *     return kldStatus;
 * }
 * @endcode
 *
 * The second argument is a function-like X-macro list of opcode enumerator
 * names. Production backends pass @ref KLD_OPCODE_LIST; tests and incomplete
 * backends may pass a shorter list (for example only the opcodes they implement
 * so far). Using the full @ref KLD_OPCODE_LIST makes every handler mandatory:
 * the computed-goto table takes the address of every listed handler label, so a
 * missing `KLD_DISPATCH_ON` is a compile error rather than a silent no-op.
 *
 * @section klddispatch_rules Rules
 *
 *  - `KLD_DISPATCH_RUN()` declares the locals `kldCommands`, `kldIp`, `kldEnd`,
 *    `kldStatus` and - on the computed-goto path - `kldDispatchTable`. Use it
 *    once per function; two dispatchers in one function would collide on the
 *    handler labels.
 *  - Every handler ends with exactly one of `KLD_DISPATCH_NEXT()` (continue with
 *    the following command) or `KLD_DISPATCH_ABORT()` (stop the stream; the
 *    status the caller observes is @c kldStatus). A handler may also return
 *    directly if the function returns `Kld::HalStatus`.
 *  - `KLD_DISPATCH_UNKNOWN()` is mandatory. On the computed-goto path its label
 *    is the table's fallback target, so omitting it does not compile; on the
 *    `switch` path it is the `default` label that catches out-of-range values.
 *  - Do **not** call `KLD_DISPATCH_NEXT()`/`KLD_DISPATCH_ABORT()` from inside a
 *    nested loop or `switch`: the portable structure terminates a handler with
 *    `break`, which would target the inner construct. Call a helper function
 *    instead.
 *  - In the portable structure omitting `KLD_DISPATCH_NEXT()` falls through into
 *    the next `case`, exactly like a forgotten `break`; in the computed-goto
 *    structure it falls into the next handler body. Both are bugs, and both are
 *    caught by the same review rule: every handler ends in a terminating macro.
 *
 * @section klddispatch_scope Scope and ABI
 *
 * Nothing here crosses the `Kld::Device` virtual boundary: the dispatch table is
 * a function-local `static` inside the backend's own `processCommandQueue`
 * implementation, built from labels of that same function. Choosing the computed
 * goto path therefore has no effect on the client, the DLL ABI, or the meaning of
 * `Kld::Device`. It is purely an implementation detail of one translation unit.
 *
 * @section klddispatch_extensions Extension points
 *
 *  - `KLD_DISPATCH_TRACE(commandPointer)` is invoked once per dispatched command
 *    when the stream is entered or advanced. It expands to nothing by default;
 *    a backend can define it before using the dispatch macros (or before
 *    including this header) to hook up a tracer.
 *  - `Opcode` is a 16-bit enum while only `Kld::kOpcodeCount` values are used.
 *    The spare range is available for *superinstructions* - fused opcodes emitted
 *    by the client for hot command pairs - if a real workload ever shows the
 *    dispatch/driver ratio justifying them. A fused opcode only needs a new
 *    enumerator, an @ref KLD_OPCODE_LIST entry and a handler that runs both
 *    payloads; the dispatch machinery below does not have to change.
 */
#include <Kaldi/KaldiOperation.hpp>
#include <Kaldi/KldHal.hpp>
#include <cstddef>
#include <cstdint>

/**
 * @brief X-macro list of every `Kld::Opcode`, in declaration order.
 *
 * @param X Macro invoked as `X(EnumeratorName)` once per opcode.
 *
 * The list exists so that generated tables, diagnostics and dispatch tables have
 * one source of truth that is not the enum body itself (the enum stays a clean
 * public data contract). Keep the order identical to `Kld::Opcode`; the dispatch
 * table uses the enumerator value as a C++20 designated-initializer index, and
 * designated initializers must be listed in increasing order. When a new opcode
 * is appended to `Kld::Opcode`, append it here as well - the `static_assert`
 * below turns a forgotten entry into a build error.
 */
#define KLD_OPCODE_LIST(X) \
	X(CreateBufferObject) \
	X(DestroyBufferObject) \
	X(CreateVertexArrayObject) \
	X(CreateTexture1D) \
	X(CreateTexture2D) \
	X(CreateCompressedTexture2D) \
	X(CreateTexture2DFromDecodeTarget) \
	X(CreateTexture3D) \
	X(DestroyTexture) \
	X(CreateSampler) \
	X(DestroySampler) \
	X(CreateImage2D) \
	X(DestroyImage) \
	X(CreateFramebuffer) \
	X(DestroyFramebuffer) \
	X(BindFramebuffer) \
	X(BeginFrame) \
	X(EndFrame) \
	X(ClearColorAttachment) \
	X(ClearDepthStencilAttachment) \
	X(Present) \
	X(CreatePipeline) \
	X(DestroyPipeline) \
	X(BindPipeline) \
	X(BindTexture) \
	X(BindSampler) \
	X(BindUniformBuffer) \
	X(BindVertexBuffer) \
	X(BindIndexBuffer) \
	X(DrawTriangles) \
	X(DrawIndexedTriangles) \
	X(DrawLines) \
	X(DrawIndexedLines) \
	X(ResizeBufferObject) \
	X(UpdateBufferObject) \
	X(MapBufferObject) \
	X(ResizeTexture2D) \
	X(UpdateTexture2D) \
	X(UpdateCompressedTexture2D) \
	X(BlitTexture2D) \
	X(GenerateTextureMipmaps) \
	X(ResizeImage2D) \
	X(UpdateImage2D) \
	X(BlitImage2D) \
	X(SetViewport) \
	X(SetScissor) \
	X(SetBlendState) \
	X(SetDepthStencilState) \
	X(SetRasterizerState) \
	X(TransitionResource) \
	X(BufferBarrier) \
	X(ImageBarrier) \
	X(CreateFence) \
	X(DestroyFence) \
	X(SignalFence) \
	X(WaitFence) \
	X(CreateComputePipeline) \
	X(DestroyComputePipeline) \
	X(BindComputePipeline) \
	X(DispatchCompute) \
	X(DispatchComputeIndirect) \
	X(DrawIndirect) \
	X(DrawIndexedIndirect) \
	X(CreateQueryPool) \
	X(DestroyQueryPool) \
	X(ResetQueryPool) \
	X(BeginQuery) \
	X(EndQuery) \
	X(WriteTimestamp) \
	X(DebugLabelBegin) \
	X(DebugLabelEnd) \
	X(DebugMarker) \
	X(CopyBuffer) \
	X(CopyBufferToImage2D) \
	X(CopyImage2DToBuffer) \
	X(CopyImage2D) \
	X(ReadBuffer) \
	X(ReadImage2D) \
	X(ReadQueryResults) \
	X(CreateTextureCube) \
	X(UpdateTextureCubeFace) \
	X(CreateTexture2DArray) \
	X(UpdateTexture2DArray) \
	X(CreateMultisampledImage2D) \
	X(ResolveImage2D) \
	X(BindStorageBuffer) \
	X(BindStorageImage) \
	X(DrawTrianglesInstanced) \
	X(DrawIndexedTrianglesInstanced) \
	X(DrawLinesInstanced) \
	X(DrawIndexedLinesInstanced) \
	X(CreateSwapchain) \
	X(DestroySwapchain) \
	X(ResizeSwapchain) \
	X(AcquireSwapchainImage) \
	X(PresentSwapchain) \
	X(BeginRenderPass) \
	X(EndRenderPass)

namespace Kld {

/** @brief Number of enumerator names in @ref KLD_OPCODE_LIST. */
#define KLD_DISPATCH_COUNT_ENTRY(name) + 1
inline constexpr std::size_t kOpcodeListSize = 0 KLD_OPCODE_LIST(KLD_DISPATCH_COUNT_ENTRY);
#undef KLD_DISPATCH_COUNT_ENTRY

/**
 * @brief One past the highest enumerator of @ref Opcode.
 *
 * `Opcode` is a dense enum starting at zero, so this is also the number of
 * opcodes and the valid range for raw command tags. The expression is derived
 * from the last enumerator, which means appending an opcode updates it
 * automatically.
 */
inline constexpr std::size_t kOpcodeCount = static_cast<std::size_t>(Opcode::EndRenderPass) + 1;

static_assert(kOpcodeListSize == kOpcodeCount,
	      "KLD_OPCODE_LIST must contain every Kld::Opcode exactly once, in declaration order");

// Internal expansion helpers: defined here because the function doc comments
// have to sit directly above the functions themselves.
#define KLD_DISPATCH_NAME_CASE(name) \
	case ::Kld::Opcode::name: return #name;

/**
 * @brief Stable display name of an opcode enumerator.
 * @return The enumerator spelling, or `"Unknown"` for values outside @ref Opcode.
 * @note The returned pointer refers to a string literal; it never has to be freed.
 */
inline const char* opcodeName(Opcode opcode) noexcept
{
	switch (opcode) {
		KLD_OPCODE_LIST(KLD_DISPATCH_NAME_CASE)
		default: return "Unknown";
	}
}
#undef KLD_DISPATCH_NAME_CASE

#define KLD_DISPATCH_KNOWN_CASE(name) \
	case static_cast<std::uint16_t>(::Kld::Opcode::name): return true;

/**
 * @brief True when @p opcode is one of the declared @ref Opcode enumerators.
 *
 * Backends use this before casting an untrusted raw tag to @ref Opcode, so that
 * an out-of-range value can be reported as `HalStatusCode::InvalidCommand`
 * without narrowing it first.
 */
inline bool isKnownOpcode(std::uint16_t opcode) noexcept
{
	switch (opcode) {
		KLD_OPCODE_LIST(KLD_DISPATCH_KNOWN_CASE)
		default: return false;
	}
}
#undef KLD_DISPATCH_KNOWN_CASE

/** @brief Stable display name of a raw command tag, or `"Unknown"`. */
inline const char* opcodeName(std::uint16_t opcode) noexcept
{
	return isKnownOpcode(opcode) ? opcodeName(static_cast<Opcode>(opcode)) : "Unknown";
}

} // namespace Kld

// ─────────────────────────────────────────────────────────────────────────────
// Dispatch structure selection
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief 1 when the dispatch macros use computed goto, 0 when they expand to a `switch`.
 *
 * GCC and Clang (including clang-cl) support the GNU label-address extension;
 * MSVC does not. Override by defining `KLD_DISPATCH_COMPUTED_GOTO` to `0` or `1`
 * before including this header. `KLD_DISPATCH_FORCE_SWITCH` always selects the
 * portable path and exists so the unit tests can compile both structures.
 */
#ifndef KLD_DISPATCH_COMPUTED_GOTO
#	if !defined(KLD_DISPATCH_FORCE_SWITCH) && (defined(__GNUC__) || defined(__clang__))
#		define KLD_DISPATCH_COMPUTED_GOTO 1
#	else
#		define KLD_DISPATCH_COMPUTED_GOTO 0
#	endif
#endif

/**
 * @brief Optional per-command trace hook; expands to nothing by default.
 * @param commandPointer `const Kld::GfxOp*` of the command about to be dispatched.
 *
 * Define this macro before the dispatch macros are expanded to observe the
 * stream from inside the backend (for example to print opcode names). It is
 * deliberately *not* called from the handler bodies, so it cannot be confused
 * with a debug label.
 */
#ifndef KLD_DISPATCH_TRACE
#	define KLD_DISPATCH_TRACE(commandPointer) ((void)0)
#endif

#if KLD_DISPATCH_COMPUTED_GOTO

/**
 * @brief Appends one handler address to the computed-goto table initializer.
 *
 * Expanded by @ref KLD_DISPATCH_RUN through the caller's opcode list. It is part
 * of the public macro surface because the list macro has to name it.
 *
 * The entry is stored at the enumerator's numeric value, so the table works for
 * any subset list: slots of opcodes the list does not mention stay null and are
 * treated as unknown. (A C++20 designated initializer would express the same
 * thing more directly, but GCC does not accept label addresses in designated
 * initializers, so the table is filled once from a function-local static
 * initializer instead.)
 */
#define KLD_DISPATCH_TABLE_ENTRY(name) \
	kldDispatchTable[static_cast<std::size_t>(::Kld::Opcode::name)] = &&kldOp_##name,

/**
 * @brief Internal: validates the current tag and jumps to its handler.
 *
 * Values outside @ref Kld::kOpcodeCount, and values whose slot stayed null
 * because the caller's list does not mention them, go to `KLD_DISPATCH_UNKNOWN()`
 * instead of dereferencing a null handler pointer. In a production dispatcher
 * built from @ref KLD_OPCODE_LIST both checks are always false and are predicted
 * perfectly.
 */
#define KLD_DISPATCH_IMPL_JUMP() \
	do { \
		KLD_DISPATCH_TRACE(kldIp); \
		const std::size_t kldIndex = static_cast<std::size_t>(kldIp->opcode); \
		if (kldIndex < ::Kld::kOpcodeCount && kldDispatchTable[kldIndex] != nullptr) { \
			goto *kldDispatchTable[kldIndex]; \
		} \
		goto kldOpUnknown; \
	} while (false)

/**
 * @brief Declares the dispatch state for @p commands and jumps to the first handler.
 * @param commands A `std::span<const Kld::GfxOp>` (or any type with `data()` and `size()`).
 * @param opcodeList Function-like X-macro list of the opcodes the handlers cover.
 *
 * Expands into the locals `kldCommands`, `kldIp`, `kldEnd`, `kldStatus`,
 * `kldDispatchTable` and `kldDispatchTableReady`. The table is filled on the
 * first call only; its initializer is a function-local static, so the
 * initialization is thread-safe. An empty span jumps straight to the finish
 * label, so the caller's post-dispatch code observes `kldStatus == Kld::HalOk`.
 */
#define KLD_DISPATCH_RUN(commands, opcodeList) \
	const auto kldCommands = (commands); \
	const ::Kld::GfxOp* kldIp = kldCommands.data(); \
	const ::Kld::GfxOp* const kldEnd = kldIp + kldCommands.size(); \
	::Kld::HalStatus kldStatus = ::Kld::HalOk; \
	static const void* kldDispatchTable[::Kld::kOpcodeCount]; \
	static const bool kldDispatchTableReady = (opcodeList(KLD_DISPATCH_TABLE_ENTRY) true); \
	(void)kldCommands; \
	(void)kldStatus; \
	(void)kldDispatchTableReady; \
	if (kldIp == kldEnd) goto kldDispatchFinish; \
	KLD_DISPATCH_IMPL_JUMP()

/** @brief Declares the handler for one opcode enumerator; expands to its dispatch label. */
#define KLD_DISPATCH_ON(name) kldOp_##name:

/**
 * @brief Advances to the next command and dispatches it, or finishes the stream.
 *
 * Each handler must end with exactly one terminating macro. In this structure the
 * dispatch jump lives here, which is what gives every opcode its own indirect
 * branch site.
 */
#define KLD_DISPATCH_NEXT() \
	do { \
		++kldIp; \
		if (kldIp == kldEnd) goto kldDispatchFinish; \
		KLD_DISPATCH_IMPL_JUMP(); \
	} while (false)

/** @brief Stops the stream; control resumes after `KLD_DISPATCH_END()`. */
#define KLD_DISPATCH_ABORT() \
	do { goto kldDispatchFinish; } while (false)

/** @brief Declares the handler that receives unknown and out-of-range command tags. */
#define KLD_DISPATCH_UNKNOWN() kldOpUnknown:

/**
 * @brief Closes the dispatch structure and defines the finish label.
 *
 * Place the handler epilogue (returning @c kldStatus or mapping it onto the
 * backend's richer result) directly after this macro.
 */
#define KLD_DISPATCH_END() kldDispatchFinish: ;

#else // KLD_DISPATCH_COMPUTED_GOTO

/** @brief Unused on the portable path; present so opcode lists stay uniform. */
#define KLD_DISPATCH_TABLE_ENTRY(name) /* only expanded by the computed-goto structure */

/**
 * @brief Declares the dispatch state for @p commands and starts the loop.
 * @param commands A `std::span<const Kld::GfxOp>` (or any type with `data()` and `size()`).
 * @param opcodeList Function-like X-macro list of the opcodes the handlers cover.
 *
 * The list is not expanded here: the `switch` only needs the `case` labels that
 * `KLD_DISPATCH_ON()` produces. Unlisted tags fall into `KLD_DISPATCH_UNKNOWN()`.
 */
#define KLD_DISPATCH_RUN(commands, opcodeList) \
	const auto kldCommands = (commands); \
	const ::Kld::GfxOp* kldIp = kldCommands.data(); \
	const ::Kld::GfxOp* const kldEnd = kldIp + kldCommands.size(); \
	::Kld::HalStatus kldStatus = ::Kld::HalOk; \
	(void)kldCommands; \
	(void)kldStatus; \
	while (kldIp != kldEnd) { \
		KLD_DISPATCH_TRACE(kldIp); \
		switch (kldIp->opcode) {

/** @brief Declares the handler for one opcode enumerator; expands to its `case` label. */
#define KLD_DISPATCH_ON(name) case ::Kld::Opcode::name:

/**
 * @brief Advances to the next command; the enclosing loop performs the dispatch.
 *
 * Expands to a `break` that leaves the `switch`; the `while` then re-enters it
 * with the next command. Do not use this from a nested loop or `switch`.
 */
#define KLD_DISPATCH_NEXT() \
	++kldIp; \
	break

/** @brief Stops the stream; the `while` condition then ends the loop. */
#define KLD_DISPATCH_ABORT() \
	kldIp = kldEnd; \
	break

/** @brief Declares the handler that receives unknown and out-of-range command tags. */
#define KLD_DISPATCH_UNKNOWN() default:

/** @brief Closes the `switch` and the loop opened by `KLD_DISPATCH_RUN()`. */
#define KLD_DISPATCH_END() \
	} \
	}

#endif // KLD_DISPATCH_COMPUTED_GOTO

#endif // KLDDISPATCH_HPP
