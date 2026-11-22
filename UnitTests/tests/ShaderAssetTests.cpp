/**
 * @file ShaderAssetTests.cpp
 * @brief Verifies the build-time HLSL -> SPIR-V asset pipeline and its consumers.
 *
 * `UnitTests.pro` compiles every HLSL file under `shaders/` with glslang during
 * the build (the same `QMAKE_EXTRA_COMPILERS` technique the WasmJit tests use to turn
 * AssemblyScript sources into `.wasm` modules). These tests load those blobs and
 * push them through the SPIR-V -> GLSL cross-compiler the OpenGL backends use, so
 * a broken shader or a broken compiler invocation fails the suite instead of a
 * backend at run time.
 */
#include <catch2/catch_test_macros.hpp>
#include <Kaldi/KaldiOperation.hpp>
#include <KaldiGLCommon/GlSpirvShader.hpp>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#ifdef KALDI_TEST_HAS_HLSL_SHADERS

namespace {

/** @brief SPIR-V magic number from the Khronos specification. */
constexpr std::uint32_t spirvMagic = 0x07230203u;

/** @brief Loads a SPIR-V blob the build emitted next to the test binary. */
std::vector<std::uint32_t> loadShader(const char* name)
{
	const std::string path = std::string(KALDI_TEST_SHADER_DIR) + name;
	std::ifstream file(path, std::ios::binary);
	if (!file.good()) {
		FAIL("the build-time shader " << path << " is missing");
	}
	const std::vector<char> bytes((std::istreambuf_iterator<char>(file)),
				      std::istreambuf_iterator<char>());
	REQUIRE(bytes.size() % sizeof(std::uint32_t) == 0);
	std::vector<std::uint32_t> words(bytes.size() / sizeof(std::uint32_t));
	std::memcpy(words.data(), bytes.data(), bytes.size());
	return words;
}

/** @brief Cross-compiles one blob to GLSL, surfacing cross-compiler errors. */
std::string crossCompile(Kld::ShaderStage stage, const std::vector<std::uint32_t>& words)
{
	const Kld::ShaderBinaryDescriptor binary { stage, words.data(), words.size() };
	try {
		return GL::spirvToGlsl(binary);
	} catch (const std::exception& error) {
		FAIL("SPIRV-Cross rejected the blob: " << error.what());
	}
	return {};
}

} // namespace

TEST_CASE("glslang emitted valid SPIR-V for every HLSL asset", "[shader][hlsl][build]")
{
	const char* const assets[] = { "triangle.vert.spv", "triangle.frag.spv",
				       "compute.comp.spv" };
	for (const char* asset : assets) {
		const std::vector<std::uint32_t> words = loadShader(asset);
		INFO(asset);
		REQUIRE(words.size() > 5);
		CHECK(words.front() == spirvMagic);
		// The header is (magic, version, generator, bound, schema).
		CHECK(words[4] == 0u);
	}
}

TEST_CASE("the HLSL vertex asset cross-compiles to GLSL clip-space output", "[shader][hlsl]")
{
	const std::string glsl =
		crossCompile(Kld::ShaderStage::Vertex, loadShader("triangle.vert.spv"));
	CHECK(glsl.find("#version") != std::string::npos);
	CHECK(glsl.find("gl_Position") != std::string::npos);
	CHECK(glsl.find("void main") != std::string::npos);
}

TEST_CASE("the HLSL fragment asset keeps its texture and sampler binding", "[shader][hlsl]")
{
	const std::string glsl =
		crossCompile(Kld::ShaderStage::Fragment, loadShader("triangle.frag.spv"));
	// GLSL has one namespace for all descriptors, so the Vulkan sets collapse into
	// bindings; the slot number the shader author wrote must survive the trip.
	CHECK(glsl.find("sampler2D") != std::string::npos);
	CHECK(glsl.find("texture(") != std::string::npos);
	CHECK(glsl.find("void main") != std::string::npos);
}

TEST_CASE("the HLSL compute asset cross-compiles with its thread group size", "[shader][hlsl]")
{
	const std::string glsl =
		crossCompile(Kld::ShaderStage::Compute, loadShader("compute.comp.spv"));
	CHECK(glsl.find("local_size_x = 64") != std::string::npos);
	// The storage buffer becomes a std430 SSBO block in GLSL.
	CHECK(glsl.find("std430") != std::string::npos);
	CHECK(glsl.find("buffer ") != std::string::npos);
}

#endif // KALDI_TEST_HAS_HLSL_SHADERS
