/**
 * @file KaldiHalTests.cpp
 * @brief Verifies the portable pieces of `Kaldi/KldHal.hpp`.
 *
 * The per-backend probes need a live GL/Vulkan/D3D device, so they are exercised
 * by smoke programs rather than this suite. What *is* testable without an API
 * object is the contract every backend falls back to: the empty capability
 * snapshot, the version tag and the status helpers.
 */
#include <catch2/catch_test_macros.hpp>
#include <Kaldi/KldHal.hpp>
#include <cstring>

static_assert(Kld::halCapabilitiesNone().maxTexture2DSize == 0,
	      "halCapabilitiesNone() must be usable in constant expressions");

TEST_CASE("Detached capability snapshots report no usable limits", "[kaldi][capabilities]")
{
	const Kld::HalCapabilities capabilities = Kld::halCapabilitiesNone();

	CHECK(capabilities.version.major == Kld::KaldiHalVersion1_0.major);
	CHECK(capabilities.version.minor == Kld::KaldiHalVersion1_0.minor);
	CHECK(capabilities.version.patch == Kld::KaldiHalVersion1_0.patch);

	CHECK(capabilities.maxTexture1DSize == 0);
	CHECK(capabilities.maxTexture2DSize == 0);
	CHECK(capabilities.maxTexture3DSize == 0);
	CHECK(capabilities.maxTextureArrayLayers == 0);
	CHECK(capabilities.maxCubeMapSize == 0);
	CHECK(capabilities.maxSamples == 0);
	CHECK(capabilities.maxColorAttachments == 0);
	CHECK(capabilities.maxVertexAttributes == 0);
	CHECK(capabilities.maxVertexBuffers == 0);
	CHECK(capabilities.maxSampledTextureBindings == 0);
	CHECK(capabilities.maxSamplerBindings == 0);
	CHECK(capabilities.maxUniformBufferBindings == 0);
	CHECK(capabilities.maxStorageBufferBindings == 0);
	CHECK(capabilities.maxStorageImageBindings == 0);
	CHECK(capabilities.maxFramesInFlight == 0);

	CHECK_FALSE(capabilities.supportsCompute);
	CHECK_FALSE(capabilities.supportsIndirectDraw);
	CHECK_FALSE(capabilities.supportsQueries);
	CHECK_FALSE(capabilities.supportsDebugLabels);
	CHECK_FALSE(capabilities.supportsTextureCompression);
	CHECK_FALSE(capabilities.supportsReadback);
	CHECK_FALSE(capabilities.supportsStorageResources);
	CHECK_FALSE(capabilities.supportsMultisampleResolve);

	CHECK(Kld::kDefaultMaxFramesInFlight > 0);
}

TEST_CASE("HalStatus distinguishes success from failure", "[kaldi][capabilities]")
{
	const Kld::HalStatus ok = Kld::HalOk;
	CHECK(ok.ok());
	CHECK(ok.code == Kld::HalStatusCode::Succeeded);
	CHECK(ok.message == nullptr);

	const Kld::HalStatus failed { Kld::HalStatusCode::InvalidCommand, "bad command" };
	CHECK_FALSE(failed.ok());
	CHECK(failed.code == Kld::HalStatusCode::InvalidCommand);
	REQUIRE(failed.message != nullptr);
	CHECK(std::strcmp(failed.message, "bad command") == 0);
}
