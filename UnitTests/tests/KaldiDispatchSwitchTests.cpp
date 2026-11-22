/**
 * @file KaldiDispatchSwitchTests.cpp
 * @brief Verifies the portable `switch` structure of `Kaldi/KldDispatch.hpp`.
 *
 * `KLD_DISPATCH_FORCE_SWITCH` is defined before the shared probe header is
 * included, so this translation unit exercises the structure MSVC uses even
 * though it runs on a compiler that supports computed goto. The scenarios match
 * `KaldiDispatchTests.cpp`; together the two units pin the behavioural
 * equivalence of the two structures.
 */
#define KLD_DISPATCH_FORCE_SWITCH
#include "support/DispatchProbe.hpp"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <vector>

static_assert(KLD_DISPATCH_COMPUTED_GOTO == 0,
	      "this unit must exercise the portable switch structure");

namespace {

using namespace UnitTests::DispatchProbe;

/** @brief Collects the `opcodeName()` result of every enumerator in KLD_OPCODE_LIST. */
std::vector<const char*> allOpcodeNames()
{
	std::vector<const char*> names;
	names.reserve(Kld::kOpcodeCount);
#define KLD_TEST_COLLECT_OPCODE_NAME(name) names.push_back(Kld::opcodeName(Kld::Opcode::name));
	KLD_OPCODE_LIST(KLD_TEST_COLLECT_OPCODE_NAME)
#undef KLD_TEST_COLLECT_OPCODE_NAME
	return names;
}

}

TEST_CASE("Switch fallback mirrors the Opcode enum", "[kaldi][dispatch][switch]")
{
	const std::vector<const char*> names = allOpcodeNames();
	REQUIRE(names.size() == Kld::kOpcodeCount);
	for (const char* name : names) {
		CHECK(name != nullptr);
		CHECK(std::strcmp(name, "Unknown") != 0);
	}
	CHECK(std::strcmp(Kld::opcodeName(Kld::Opcode::DrawIndexedLinesInstanced), "DrawIndexedLinesInstanced") == 0);
	CHECK(Kld::isKnownOpcode(static_cast<std::uint16_t>(Kld::Opcode::PresentSwapchain)));
	CHECK_FALSE(Kld::isKnownOpcode(static_cast<std::uint16_t>(Kld::kOpcodeCount)));
}

TEST_CASE("Switch fallback runs a command span in append order", "[kaldi][dispatch][switch]")
{
	const std::vector<Kld::GfxOp> commands {
		beginFrame(), setViewport(6.25f), drawTriangles(), setScissor(12), endFrame(88)
	};
	const Trace trace = run(commands);

	REQUIRE(trace.status.ok());
	CHECK_FALSE(trace.sawUnknown);
	REQUIRE(trace.visited.size() == 5);
	CHECK(std::strcmp(trace.visited[0], "BeginFrame") == 0);
	CHECK(std::strcmp(trace.visited[1], "SetViewport") == 0);
	CHECK(std::strcmp(trace.visited[2], "DrawTriangles") == 0);
	CHECK(std::strcmp(trace.visited[3], "SetScissor") == 0);
	CHECK(std::strcmp(trace.visited[4], "EndFrame") == 0);
	CHECK(trace.viewportX == 6.25f);
	CHECK(trace.scissorX == 12);
	CHECK(trace.endFrameFramebuffer == 88);
}

TEST_CASE("Switch fallback treats an empty span as a no-op", "[kaldi][dispatch][switch]")
{
	const Trace trace = run(std::span<const Kld::GfxOp>());
	CHECK(trace.status.ok());
	CHECK(trace.visited.empty());
	CHECK_FALSE(trace.sawUnknown);
}

TEST_CASE("Switch fallback aborts on unhandled tags", "[kaldi][dispatch][switch]")
{
	SECTION("raw tag outside the Opcode range")
	{
		const std::vector<Kld::GfxOp> commands { beginFrame(), outOfRangeOpcode(), endFrame(9) };
		const Trace trace = run(commands);
		REQUIRE_FALSE(trace.status.ok());
		CHECK(trace.status.code == Kld::HalStatusCode::InvalidCommand);
		REQUIRE(trace.sawUnknown);
		CHECK(trace.endFrameFramebuffer == 0xFFFFFFFFu);
	}
	SECTION("valid opcode that the probe does not implement")
	{
		const std::vector<Kld::GfxOp> commands { beginFrame(), unlistedOpcode(), endFrame(9) };
		const Trace trace = run(commands);
		REQUIRE_FALSE(trace.status.ok());
		CHECK(trace.status.code == Kld::HalStatusCode::InvalidCommand);
		CHECK(trace.sawUnknown);
		CHECK(trace.endFrameFramebuffer == 0xFFFFFFFFu);
	}
}
