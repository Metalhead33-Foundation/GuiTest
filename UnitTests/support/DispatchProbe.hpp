#ifndef UNITTESTS_DISPATCHPROBE_HPP
#define UNITTESTS_DISPATCHPROBE_HPP
/**
 * @file DispatchProbe.hpp
 * @brief Minimal Kaldi command executor used to exercise `Kaldi/KldDispatch.hpp`.
 *
 * The probe deliberately implements a *subset* of the Kaldi vocabulary. That is
 * the interesting case for the dispatch layer: it proves that
 *
 *  - listed opcodes reach their handler,
 *  - unlisted opcodes that are valid enumeration values reach the unknown
 *    handler instead of a null table slot,
 *  - raw tags outside `Kld::kOpcodeCount` reach the unknown handler as well,
 *  - payloads stay readable through `kldIp->opt`.
 *
 * `KALDIGL_*`/`GL_*` are not involved: the probe is plain C++ so the tests can
 * compile it twice, once with the computed-goto structure and once with the
 * portable `switch` fallback.
 */
#include <Kaldi/KldDispatch.hpp>
#include <span>
#include <vector>

namespace UnitTests {
namespace DispatchProbe {

/** @brief Opcodes the probe implements; intentionally not the full vocabulary. */
#define KLD_PROBE_OPCODE_LIST(X) \
	X(BeginFrame) \
	X(SetViewport) \
	X(SetScissor) \
	X(DrawTriangles) \
	X(EndFrame)

/** @brief Everything one probe run observed. */
struct Trace {
	/** @brief Opcode names in the order the handlers ran; `"Unknown"` for rejected tags. */
	std::vector<const char*> visited;
	/** @brief Accumulated status; starts as `Kld::HalOk`. */
	Kld::HalStatus status = Kld::HalOk;
	/** @brief True when the unknown handler ran. */
	bool sawUnknown = false;
	/** @brief X component of the last `SetViewport` payload. */
	float viewportX = 0.0f;
	/** @brief Number of commands the last `SetScissor` handler saw. */
	int32_t scissorX = 0;
	/** @brief `framebuffer` of the last `EndFrame`; the sentinel means it never ran. */
	Kld::HandleId endFrameFramebuffer = 0xFFFFFFFFu;
};

/**
 * @brief Executes @p commands through the Kaldi dispatch macros.
 * @param commands Command span, in execution order.
 * @return What the handlers observed.
 *
 * @note This is `static`, not `inline`, on purpose: both dispatch test
 * translation units include this header with a different structure selected, and
 * the two executor copies must not be merged by the linker.
 */
static Trace run(std::span<const Kld::GfxOp> commands)
{
	Trace trace;
	KLD_DISPATCH_RUN(commands, KLD_PROBE_OPCODE_LIST);
	KLD_DISPATCH_ON(BeginFrame)
	{
		trace.visited.push_back(Kld::opcodeName(kldIp->opcode));
		KLD_DISPATCH_NEXT();
	}
	KLD_DISPATCH_ON(SetViewport)
	{
		trace.visited.push_back(Kld::opcodeName(kldIp->opcode));
		trace.viewportX = kldIp->opt.opSetViewport.x;
		KLD_DISPATCH_NEXT();
	}
	KLD_DISPATCH_ON(SetScissor)
	{
		trace.visited.push_back(Kld::opcodeName(kldIp->opcode));
		trace.scissorX = kldIp->opt.opSetScissor.x;
		KLD_DISPATCH_NEXT();
	}
	KLD_DISPATCH_ON(DrawTriangles)
	{
		trace.visited.push_back(Kld::opcodeName(kldIp->opcode));
		KLD_DISPATCH_NEXT();
	}
	KLD_DISPATCH_ON(EndFrame)
	{
		trace.visited.push_back(Kld::opcodeName(kldIp->opcode));
		trace.endFrameFramebuffer = kldIp->opt.opEndFrame.framebuffer;
		KLD_DISPATCH_NEXT();
	}
	KLD_DISPATCH_UNKNOWN()
	{
		trace.sawUnknown = true;
		trace.visited.push_back("Unknown");
		// A backend-owned string literal keeps the HalStatus message alive for
		// exactly as long as the contract requires (the enclosing call).
		trace.status = { Kld::HalStatusCode::InvalidCommand, Kld::opcodeName(kldIp->opcode) };
		KLD_DISPATCH_ABORT();
	}
	KLD_DISPATCH_END();
	return trace;
}

/** @brief Builds a `BeginFrame` command targeting the default framebuffer. */
inline Kld::GfxOp beginFrame()
{
	return { Kld::Opcode::BeginFrame, { .opBeginFrame = { Kld::DefaultFramebuffer } } };
}

/** @brief Builds an `EndFrame` command with a recognizable framebuffer payload. */
inline Kld::GfxOp endFrame(Kld::HandleId framebuffer = Kld::DefaultFramebuffer)
{
	return { Kld::Opcode::EndFrame, { .opEndFrame = { framebuffer } } };
}

/** @brief Builds a `SetViewport` command whose X component encodes @p x. */
inline Kld::GfxOp setViewport(float x)
{
	return { Kld::Opcode::SetViewport, { .opSetViewport = { x, 0.0f, 16.0f, 16.0f, 0.0f, 1.0f } } };
}

/** @brief Builds a `SetScissor` command whose X component encodes @p x. */
inline Kld::GfxOp setScissor(int32_t x)
{
	return { Kld::Opcode::SetScissor, { .opSetScissor = { x, 0, 16, 16 } } };
}

/** @brief Builds a three-vertex `DrawTriangles` command. */
inline Kld::GfxOp drawTriangles()
{
	return { Kld::Opcode::DrawTriangles, { .opDrawTriangles = { 0, 3 } } };
}

/** @brief Builds a command whose tag is a valid but unlisted opcode. */
inline Kld::GfxOp unlistedOpcode()
{
	Kld::GfxOp command{};
	command.opcode = Kld::Opcode::SetBlendState;
	return command;
}

/** @brief Builds a command whose tag is outside `Kld::kOpcodeCount`. */
inline Kld::GfxOp outOfRangeOpcode()
{
	Kld::GfxOp command{};
	command.opcode = static_cast<Kld::Opcode>(0xFFFFu);
	return command;
}

} // namespace DispatchProbe
} // namespace UnitTests
#endif
