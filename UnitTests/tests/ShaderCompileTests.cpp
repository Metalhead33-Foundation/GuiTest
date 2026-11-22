/**
 * @file ShaderCompileTests.cpp
 * @brief Verifies that Lotte turns authored shader sources into SPIR-V modules.
 *
 * These tests exercise the Lotte -> Kaldi shader contract only: no graphics
 * device, no context, no backend. They are the runnable proof that the
 * `ShaderModuleCreateInfo` removal did not leave a hole: the compiler emits
 * `Lotte::Gfx::ShaderBinary` blobs whose `Kld::ShaderBinaryDescriptor` views are
 * consistent with the stored words.
 */
#include <catch2/catch_test_macros.hpp>
#include <KaldiGLCommon/GlSpirvShader.hpp>
#include <Lotte/GFX/LtePipeline.hpp>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

/** @brief SPIR-V magic number from the Khronos specification. */
constexpr uint32_t spirvMagic = 0x07230203u;

/** @brief Minimal HLSL vertex stage using stage-io semantics. */
constexpr std::string_view hlslVertexSource = R"(
struct VsInput {
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
};

struct VsOutput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VsOutput main(VsInput input) {
    VsOutput output;
    output.position = float4(input.position, 1.0);
    output.uv = input.uv;
    return output;
}
)";

/** @brief Minimal HLSL fragment stage matching the vertex stage interface. */
constexpr std::string_view hlslFragmentSource = R"(
struct PsInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(PsInput input) : SV_TARGET {
    return float4(input.uv, 0.0, 1.0);
}
)";

/** @brief Minimal GLSL vertex stage. */
constexpr std::string_view glslVertexSource = R"(#version 450
layout(location = 0) in vec2 position;
void main() {
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

/** @brief Minimal GLSL fragment stage. */
constexpr std::string_view glslFragmentSource = R"(#version 450
layout(location = 0) out vec4 color;
void main() {
    color = vec4(1.0);
}
)";

/** @brief Builds one attribute descriptor matching Kaldi's vertex layout model. */
Kld::AttributeDescriptor makeAttribute(std::string_view semantic, unsigned index, Kld::PrimitiveType type, uintptr_t offset)
{
	Kld::AttributeDescriptor descriptor{};
	descriptor.SemanticName = semantic;
	descriptor.SemanticIndex = index;
	descriptor.type = type;
	descriptor.offset = offset;
	return descriptor;
}

}

TEST_CASE("Lotte compiles an HLSL program into one SPIR-V blob per stage", "[shader][lotte]")
{
	using namespace Lotte::Gfx;

	const std::vector<ShaderStageSource> stages = {
		{ Kld::ShaderStage::Vertex, hlslVertexSource, "test.vs.hlsl" },
		{ Kld::ShaderStage::Fragment, hlslFragmentSource, "test.ps.hlsl" },
	};

	ShaderCompileOptions options;
	options.language = ShaderSourceLanguage::Hlsl;

	const std::vector<ShaderBinary> binaries = compileShaderProgram(nullptr, stages, options);
	REQUIRE(binaries.size() == 2);
	CHECK(binaries[0].stage() == Kld::ShaderStage::Vertex);
	CHECK(binaries[1].stage() == Kld::ShaderStage::Fragment);

	for (const ShaderBinary& binary : binaries) {
		REQUIRE_FALSE(binary.empty());
		REQUIRE(binary.words().size() > 5);
		CHECK(binary.words()[0] == spirvMagic);
		CHECK(binary.byteSize() == binary.words().size() * sizeof(uint32_t));

		const Kld::ShaderBinaryDescriptor descriptor = binary.descriptor();
		CHECK(descriptor.stage == binary.stage());
		CHECK(descriptor.spirv == binary.words().data());
		CHECK(descriptor.wordCount == binary.words().size());
	}
}

// The source stays Vulkan GLSL: Lotte always emits Vulkan SPIR-V, because that single
// blob is consumed unchanged by the Vulkan backend and cross-compiled by the GL/GLES and
// D3D backends through SPIRV-Cross. An OpenGL-flavoured module (`-G`: OriginLowerLeft,
// extra gl_VertexID/gl_InstanceID inputs) is not legal Vulkan SPIR-V, and glslang cannot
// even compile HLSL under OpenGL semantics.
TEST_CASE("Lotte compiles GLSL sources into Vulkan SPIR-V", "[shader][lotte]")
{
	using namespace Lotte::Gfx;

	const std::vector<ShaderStageSource> stages = {
		{ Kld::ShaderStage::Vertex, glslVertexSource, "test.vs.glsl" },
		{ Kld::ShaderStage::Fragment, glslFragmentSource, "test.ps.glsl" },
	};

	ShaderCompileOptions options;
	options.language = ShaderSourceLanguage::Glsl;

	const std::vector<ShaderBinary> binaries = compileShaderProgram(nullptr, stages, options);
	REQUIRE(binaries.size() == 2);
	for (const ShaderBinary& binary : binaries) {
		REQUIRE_FALSE(binary.empty());
		CHECK(binary.words()[0] == spirvMagic);
	}
}

// End-to-end proof of the intended pipeline: Lotte compiles the HLSL once into Vulkan
// SPIR-V, and the GL backend turns that same blob into GLSL text with SPIRV-Cross. If the
// blob were OpenGL-flavoured (`-G`), SPIRV-Cross would be consuming input it does not
// target, and the Vulkan backend would not be able to use it at all.
TEST_CASE("Lotte's Vulkan SPIR-V cross-compiles to GLSL through SPIRV-Cross", "[shader][lotte][spirv-cross]")
{
	using namespace Lotte::Gfx;

	const std::vector<ShaderStageSource> stages = {
		{ Kld::ShaderStage::Vertex, hlslVertexSource, "test.vs.hlsl" },
		{ Kld::ShaderStage::Fragment, hlslFragmentSource, "test.ps.hlsl" },
	};
	ShaderCompileOptions options;
	options.language = ShaderSourceLanguage::Hlsl;

	const std::vector<ShaderBinary> binaries = compileShaderProgram(nullptr, stages, options);
	REQUIRE(binaries.size() == 2);

	const std::string vertexGlsl = GL::spirvToGlsl(binaries[0].descriptor());
	const std::string fragmentGlsl = GL::spirvToGlsl(binaries[1].descriptor());

	REQUIRE_FALSE(vertexGlsl.empty());
	REQUIRE_FALSE(fragmentGlsl.empty());
	// Desktop defaults from GL::defaultGlslOptions().
	CHECK(vertexGlsl.find("#version 430") != std::string::npos);
	CHECK(fragmentGlsl.find("#version 430") != std::string::npos);
	// The stage I/O of the HLSL sources has to survive the round trip.
	CHECK(vertexGlsl.find("gl_Position") != std::string::npos);
	CHECK(fragmentGlsl.find("main") != std::string::npos);
}

TEST_CASE("Lotte rejects empty stage lists, empty sources and duplicate stages", "[shader][lotte]")
{
	using namespace Lotte::Gfx;

	CHECK_THROWS_AS(compileShaderProgram(nullptr, std::span<const ShaderStageSource>()), std::invalid_argument);

	const std::vector<ShaderStageSource> emptySource = {
		{ Kld::ShaderStage::Vertex, std::string_view(), "empty.vs.hlsl" },
	};
	CHECK_THROWS_AS(compileShaderProgram(nullptr, emptySource), std::invalid_argument);

	const std::vector<ShaderStageSource> duplicate = {
		{ Kld::ShaderStage::Vertex, hlslVertexSource, "a.vs.hlsl" },
		{ Kld::ShaderStage::Vertex, hlslVertexSource, "b.vs.hlsl" },
	};
	CHECK_THROWS_AS(compileShaderProgram(nullptr, duplicate), std::invalid_argument);
}

TEST_CASE("Lotte surfaces glslang diagnostics for broken sources", "[shader][lotte]")
{
	using namespace Lotte::Gfx;

	const std::vector<ShaderStageSource> stages = {
		{ Kld::ShaderStage::Fragment, "float4 main() : SV_TARGET { return undefinedSymbol(); }", "broken.ps.hlsl" },
	};

	CHECK_THROWS_AS(compileShaderProgram(nullptr, stages), std::runtime_error);
}

TEST_CASE("Lotte rejects include directives when no resolver is supplied", "[shader][lotte]")
{
	using namespace Lotte::Gfx;

	const std::vector<ShaderStageSource> stages = {
		{ Kld::ShaderStage::Vertex, "#include \"missing.hlsl\"\nfloat4 main() : SV_POSITION { return 0; }", "include.vs.hlsl" },
	};

	CHECK_THROWS_AS(compileShaderProgram(nullptr, stages), std::runtime_error);
}

TEST_CASE("Lotte pipelines own their binaries and expose Kaldi views", "[shader][lotte]")
{
	using namespace Lotte::Gfx;

	std::vector<ShaderBinary> binaries;
	binaries.emplace_back(Kld::ShaderStage::Vertex, std::vector<uint32_t>{ spirvMagic, 0, 0, 0, 0 });

	std::vector<Kld::AttributeDescriptor> attributes = {
		makeAttribute("POSITION", 0, Kld::PrimitiveType::F32x3, 0),
		makeAttribute("TEXCOORD", 0, Kld::PrimitiveType::F32x2, 12),
	};

	Pipeline pipeline(std::move(binaries), std::move(attributes), 20);
	REQUIRE(pipeline.shaderBinaries().size() == 1);
	REQUIRE(pipeline.findStage(Kld::ShaderStage::Vertex) != nullptr);
	CHECK(pipeline.findStage(Kld::ShaderStage::Fragment) == nullptr);

	const Kld::VertexDescriptor vertexInput = pipeline.vertexInput();
	CHECK(vertexInput.stride == 20);
	REQUIRE(vertexInput.descriptors.size() == 2);
	CHECK(vertexInput.descriptors[1].offset == 12);
	CHECK(vertexInput.descriptors[1].type == Kld::PrimitiveType::F32x2);

	const std::vector<Kld::ShaderBinaryDescriptor> descriptors = pipeline.shaderDescriptors();
	REQUIRE(descriptors.size() == 1);
	CHECK(descriptors[0].wordCount == 5);
	CHECK(descriptors[0].stage == Kld::ShaderStage::Vertex);

	pipeline.setVertexAttributes({ makeAttribute("POSITION", 0, Kld::PrimitiveType::F32x2, 0) }, 8);
	CHECK(pipeline.vertexInput().stride == 8);
	CHECK(pipeline.vertexInput().descriptors.size() == 1);
}
