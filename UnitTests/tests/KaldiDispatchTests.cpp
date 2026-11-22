/**
 * @file KaldiDispatchTests.cpp
 * @brief Verifies the command dispatch layer used by Kaldi backend implementations.
 *
 * This translation unit compiles `Kaldi/KldDispatch.hpp` in its default mode,
 * which is direct threading on GCC/Clang. `KaldiDispatchSwitchTests.cpp`
 * compiles the same probe against the portable `switch` structure, so both code
 * paths are covered by the suite regardless of the compiler building it.
 */
#include <catch2/catch_test_macros.hpp>
#include "support/DispatchProbe.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <vector>

#if defined(__GNUC__) || defined(__clang__)
static_assert(KLD_DISPATCH_COMPUTED_GOTO == 1,
	      "the default structure should be direct threading on GCC/Clang");
#endif

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

TEST_CASE("KLD_OPCODE_LIST mirrors the Opcode enum", "[kaldi][dispatch]")
{
	const std::vector<const char*> names = allOpcodeNames();
	REQUIRE(names.size() == Kld::kOpcodeCount);
	for (const char* name : names) {
		CHECK(name != nullptr);
		CHECK(std::strcmp(name, "Unknown") != 0);
	}

	CHECK(std::strcmp(Kld::opcodeName(Kld::Opcode::CreateBufferObject), "CreateBufferObject") == 0);
	CHECK(std::strcmp(Kld::opcodeName(Kld::Opcode::SetViewport), "SetViewport") == 0);
	CHECK(std::strcmp(Kld::opcodeName(Kld::Opcode::EndRenderPass), "EndRenderPass") == 0);
	CHECK(std::strcmp(Kld::opcodeName(static_cast<std::uint16_t>(0xFFFFu)), "Unknown") == 0);

	CHECK(Kld::isKnownOpcode(0));
	CHECK(Kld::isKnownOpcode(static_cast<std::uint16_t>(Kld::kOpcodeCount - 1)));
	CHECK_FALSE(Kld::isKnownOpcode(static_cast<std::uint16_t>(Kld::kOpcodeCount)));
	CHECK_FALSE(Kld::isKnownOpcode(0xFFFFu));

	// Names are unique, so diagnostics can be matched by spelling.
	std::vector<std::string> sorted;
	sorted.reserve(names.size());
	for (const char* name : names) {
		sorted.emplace_back(name);
	}
	std::sort(sorted.begin(), sorted.end());
	CHECK(std::adjacent_find(sorted.begin(), sorted.end()) == sorted.end());
}

TEST_CASE("Dispatch runs a command span in append order", "[kaldi][dispatch]")
{
	const std::vector<Kld::GfxOp> commands {
		beginFrame(), setViewport(3.5f), drawTriangles(), setScissor(-4), endFrame(77)
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
	CHECK(trace.viewportX == 3.5f);
	CHECK(trace.scissorX == -4);
	CHECK(trace.endFrameFramebuffer == 77);
}

TEST_CASE("Dispatch treats an empty span as a no-op", "[kaldi][dispatch]")
{
	const Trace trace = run(std::span<const Kld::GfxOp>());
	CHECK(trace.status.ok());
	CHECK(trace.visited.empty());
	CHECK_FALSE(trace.sawUnknown);
}

TEST_CASE("Unknown and out-of-range tags abort the stream", "[kaldi][dispatch]")
{
	SECTION("raw tag outside the Opcode range")
	{
		const std::vector<Kld::GfxOp> commands { beginFrame(), outOfRangeOpcode(), endFrame(9) };
		const Trace trace = run(commands);
		REQUIRE_FALSE(trace.status.ok());
		CHECK(trace.status.code == Kld::HalStatusCode::InvalidCommand);
		REQUIRE(trace.sawUnknown);
		REQUIRE(trace.visited.size() == 2);
		CHECK(std::strcmp(trace.visited[1], "Unknown") == 0);
		// The probe aborts, so the trailing EndFrame must not have run.
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
