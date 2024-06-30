#include "GlHlslConversion.hpp"
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/Include/ResourceLimits.h>
#include <spirv-tools/libspirv.hpp>
#include <spirv-tools/optimizer.hpp>
#include "GlslangIncluder.hpp"
#include <spirv_cross/spirv_glsl.hpp>
#include <iostream>
namespace GL {
namespace HLSL {

class HlslParseError : public std::exception {
private:
	std::string errStr;
public:
	HlslParseError(glslang::TShader& shdr) {
		std::stringstream stringstream;
		stringstream << "HLSL Parsing failed: " << shdr.getInfoLog() << std::endl;
		stringstream << "Debug log: " << shdr.getInfoDebugLog() << std::endl;
		errStr = stringstream.str();
	}
	const char* what() const noexcept override {
		return errStr.c_str();
	}
};

class GlslangProgramError : public std::exception {
private:
	std::string errStr;
public:
	GlslangProgramError(glslang::TProgram& program) {
		std::stringstream stringstream;
		stringstream << "glSlang linking: " << program.getInfoLog() << std::endl;
		stringstream << "Debug log: " << program.getInfoDebugLog() << std::endl;
		errStr = stringstream.str();
	}
	const char* what() const noexcept override {
		return errStr.c_str();
	}
};

void CLIMessageConsumer(spv_message_level_t level, const char*,
						const spv_position_t& position, const char* message) {
  switch (level) {
	case SPV_MSG_FATAL:
	case SPV_MSG_INTERNAL_ERROR:
	case SPV_MSG_ERROR:
	  std::cerr << "error: line " << position.index << ": " << message
				<< std::endl;
	  break;
	case SPV_MSG_WARNING:
	  std::cout << "warning: line " << position.index << ": " << message
				<< std::endl;
	  break;
	case SPV_MSG_INFO:
	  std::cout << "info: line " << position.index << ": " << message
				<< std::endl;
	  break;
	default:
	  break;
  }
}

static TBuiltInResource GlslangResources;
static void initResources() {
	GlslangResources.maxLights                                 = 32;
	   GlslangResources.maxClipPlanes                             = 6;
	   GlslangResources.maxTextureUnits                           = 32;
	   GlslangResources.maxTextureCoords                          = 32;
	   GlslangResources.maxVertexAttribs                          = 64;
	   GlslangResources.maxVertexUniformComponents                = 4096;
	   GlslangResources.maxVaryingFloats                          = 64;
	   GlslangResources.maxVertexTextureImageUnits                = 32;
	   GlslangResources.maxCombinedTextureImageUnits              = 80;
	   GlslangResources.maxTextureImageUnits                      = 32;
	   GlslangResources.maxFragmentUniformComponents              = 4096;
	   GlslangResources.maxDrawBuffers                            = 32;
	   GlslangResources.maxVertexUniformVectors                   = 128;
	   GlslangResources.maxVaryingVectors                         = 8;
	   GlslangResources.maxFragmentUniformVectors                 = 16;
	   GlslangResources.maxVertexOutputVectors                    = 16;
	   GlslangResources.maxFragmentInputVectors                   = 15;
	   GlslangResources.minProgramTexelOffset                     = -8;
	   GlslangResources.maxProgramTexelOffset                     = 7;
	   GlslangResources.maxClipDistances                          = 8;
	   GlslangResources.maxComputeWorkGroupCountX                 = 65535;
	   GlslangResources.maxComputeWorkGroupCountY                 = 65535;
	   GlslangResources.maxComputeWorkGroupCountZ                 = 65535;
	   GlslangResources.maxComputeWorkGroupSizeX                  = 1024;
	   GlslangResources.maxComputeWorkGroupSizeY                  = 1024;
	   GlslangResources.maxComputeWorkGroupSizeZ                  = 64;
	   GlslangResources.maxComputeUniformComponents               = 1024;
	   GlslangResources.maxComputeTextureImageUnits               = 16;
	   GlslangResources.maxComputeImageUniforms                   = 8;
	   GlslangResources.maxComputeAtomicCounters                  = 8;
	   GlslangResources.maxComputeAtomicCounterBuffers            = 1;
	   GlslangResources.maxVaryingComponents                      = 60;
	   GlslangResources.maxVertexOutputComponents                 = 64;
	   GlslangResources.maxGeometryInputComponents                = 64;
	   GlslangResources.maxGeometryOutputComponents               = 128;
	   GlslangResources.maxFragmentInputComponents                = 128;
	   GlslangResources.maxImageUnits                             = 8;
	   GlslangResources.maxCombinedImageUnitsAndFragmentOutputs   = 8;
	   GlslangResources.maxCombinedShaderOutputResources          = 8;
	   GlslangResources.maxImageSamples                           = 0;
	   GlslangResources.maxVertexImageUniforms                    = 0;
	   GlslangResources.maxTessControlImageUniforms               = 0;
	   GlslangResources.maxTessEvaluationImageUniforms            = 0;
	   GlslangResources.maxGeometryImageUniforms                  = 0;
	   GlslangResources.maxFragmentImageUniforms                  = 8;
	   GlslangResources.maxCombinedImageUniforms                  = 8;
	   GlslangResources.maxGeometryTextureImageUnits              = 16;
	   GlslangResources.maxGeometryOutputVertices                 = 256;
	   GlslangResources.maxGeometryTotalOutputComponents          = 1024;
	   GlslangResources.maxGeometryUniformComponents              = 1024;
	   GlslangResources.maxGeometryVaryingComponents              = 64;
	   GlslangResources.maxTessControlInputComponents             = 128;
	   GlslangResources.maxTessControlOutputComponents            = 128;
	   GlslangResources.maxTessControlTextureImageUnits           = 16;
	   GlslangResources.maxTessControlUniformComponents           = 1024;
	   GlslangResources.maxTessControlTotalOutputComponents       = 4096;
	   GlslangResources.maxTessEvaluationInputComponents          = 128;
	   GlslangResources.maxTessEvaluationOutputComponents         = 128;
	   GlslangResources.maxTessEvaluationTextureImageUnits        = 16;
	   GlslangResources.maxTessEvaluationUniformComponents        = 1024;
	   GlslangResources.maxTessPatchComponents                    = 120;
	   GlslangResources.maxPatchVertices                          = 32;
	   GlslangResources.maxTessGenLevel                           = 64;
	   GlslangResources.maxViewports                              = 16;
	   GlslangResources.maxVertexAtomicCounters                   = 0;
	   GlslangResources.maxTessControlAtomicCounters              = 0;
	   GlslangResources.maxTessEvaluationAtomicCounters           = 0;
	   GlslangResources.maxGeometryAtomicCounters                 = 0;
	   GlslangResources.maxFragmentAtomicCounters                 = 8;
	   GlslangResources.maxCombinedAtomicCounters                 = 8;
	   GlslangResources.maxAtomicCounterBindings                  = 1;
	   GlslangResources.maxVertexAtomicCounterBuffers             = 0;
	   GlslangResources.maxTessControlAtomicCounterBuffers        = 0;
	   GlslangResources.maxTessEvaluationAtomicCounterBuffers     = 0;
	   GlslangResources.maxGeometryAtomicCounterBuffers           = 0;
	   GlslangResources.maxFragmentAtomicCounterBuffers           = 1;
	   GlslangResources.maxCombinedAtomicCounterBuffers           = 1;
	   GlslangResources.maxAtomicCounterBufferSize                = 16384;
	   GlslangResources.maxTransformFeedbackBuffers               = 4;
	   GlslangResources.maxTransformFeedbackInterleavedComponents = 64;
	   GlslangResources.maxCullDistances                          = 8;
	   GlslangResources.maxCombinedClipAndCullDistances           = 8;
	   GlslangResources.maxSamples                                = 4;
	   GlslangResources.maxMeshOutputVerticesNV                   = 256;
	   GlslangResources.maxMeshOutputPrimitivesNV                 = 512;
	   GlslangResources.maxMeshWorkGroupSizeX_NV                  = 32;
	   GlslangResources.maxMeshWorkGroupSizeY_NV                  = 1;
	   GlslangResources.maxMeshWorkGroupSizeZ_NV                  = 1;
	   GlslangResources.maxTaskWorkGroupSizeX_NV                  = 32;
	   GlslangResources.maxTaskWorkGroupSizeY_NV                  = 1;
	   GlslangResources.maxTaskWorkGroupSizeZ_NV                  = 1;
	   GlslangResources.maxMeshViewCountNV                        = 4;

	   GlslangResources.limits.nonInductiveForLoops                 = 1;
	   GlslangResources.limits.whileLoops                           = 1;
	   GlslangResources.limits.doWhileLoops                         = 1;
	   GlslangResources.limits.generalUniformIndexing               = 1;
	   GlslangResources.limits.generalAttributeMatrixVectorIndexing = 1;
	   GlslangResources.limits.generalVaryingIndexing               = 1;
	   GlslangResources.limits.generalSamplerIndexing               = 1;
	   GlslangResources.limits.generalVariableIndexing              = 1;
	   GlslangResources.limits.generalConstantMatrixVectorIndexing  = 1;
}

void prepareShaderModuleFor(MH33::Io::System& iosys, const std::span<MH33::GFX::ShaderModuleCreateInfo>& output, const std::span<const std::string>& input)
{
	GlslangIncluder includer(&iosys);
	std::vector<EShLanguage> stages(output.size());
	std::vector<std::string> preprocessedShaderStrings(output.size());
	std::vector<std::unique_ptr<glslang::TShader>> tempShaders;
	tempShaders.reserve(output.size());
	glslang::TProgram program;
	for(size_t i = 0; i < output.size(); ++i) {
		EShLanguage& stage = stages[i];
		auto& out = output[i];
		switch (out.shaderType) {
			case MH33::GFX::ShaderModuleType::VERTEX_SHADER: stage = EShLangVertex; break;
			case MH33::GFX::ShaderModuleType::GEOMETRY_SHADER: stage = EShLangGeometry; break;
			case MH33::GFX::ShaderModuleType::TESSELLATION_CONTROL_SHADER: stage = EShLangTessControl; break;
			case MH33::GFX::ShaderModuleType::TESSELLATION_EVALUATION_SHADER: stage = EShLangTessEvaluation; break;
			case MH33::GFX::ShaderModuleType::PIXEL_SHADER: stage = EShLangFragment; break;
			case MH33::GFX::ShaderModuleType::COMPUTE_SHADER: stage = EShLangCompute; break;
			default: throw std::runtime_error("Unsupported shader type.");
		}
		tempShaders.push_back(std::make_unique<glslang::TShader>(stage));
	}
	for(size_t i = 0; i < output.size(); ++i) {
		auto& out = output[i];
		const auto& in = input[i];
		const EShLanguage& stage = stages[i];
		const char* shaderSource = in.c_str();
		auto& shader = *tempShaders[i];
		//glslang::TShader shader(stage);
		shader.setStrings(&shaderSource, 1);
		shader.setEnvInput(glslang::EShSourceHlsl, stage, glslang::EShClientVulkan, 1);
		shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
		shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);
		shader.setEntryPoint("main");

		EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

		std::string& preprocessedStr = preprocessedShaderStrings[i];
		if(!shader.preprocess(&GlslangResources,100, ENoProfile, false, false, messages, &preprocessedStr, includer)) {
			throw HlslParseError(shader);
		}
		const char* preprocessedSources[1] = { preprocessedStr.c_str() };
		shader.setStrings(preprocessedSources, 1);

		if (!shader.parse(&GlslangResources, 100, false, messages, includer)) {
			throw HlslParseError(shader);
		}
		program.addShader(&shader);
	}
	EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
	if (!program.link(messages)) {
		throw GlslangProgramError(program);
	}
	spvtools::SpirvTools spvTool(SPV_ENV_VULKAN_1_0);
	spvtools::Optimizer optimizer(SPV_ENV_VULKAN_1_0);
	spvTool.SetMessageConsumer(CLIMessageConsumer);
	optimizer.SetMessageConsumer(CLIMessageConsumer);
	optimizer.RegisterPass(spvtools::CreateNullPass());
	//optimizer.RegisterLegalizationPasses();
	//optimizer.RegisterPerformancePasses();
	//optimizer.RegisterSizePasses();
	for(size_t i = 0; i < output.size(); ++i) {
		auto& out = output[i];
		const EShLanguage& stage = stages[i];
		std::vector<uint32_t> spirv;
		glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);
		auto spirvSize = spirv.size();
		/*if (spvTool.Validate(spirv) != SPV_SUCCESS) {
				throw std::runtime_error("SPIR-V validation failed");
		}
		std::vector<uint32_t> optimizedSpirv;
		if(!optimizer.Run(spirv.data(),spirv.size(),&optimizedSpirv)) {
			throw std::runtime_error("SPIR-V optimization failed!");
		}*/
		/*out.source.resize(spirv.size() * sizeof(uint32_t));
		auto outSize = out.source.size();
		std::memcpy(out.source.data(), spirv.data(), out.source.size());
		out.isBinary = true;*/
		convertSpirvBackToGlsl(out,std::move(spirv));
	}
}

void prepareShaderModuleFor(MH33::Io::System& iosys, MH33::GFX::ShaderModuleCreateInfo& output, const std::string& input)
{
	std::span<MH33::GFX::ShaderModuleCreateInfo> out1(&output,1);
	std::span<const std::string> in1(&input,1);
	prepareShaderModuleFor(iosys,out1,in1);
}

static void testWholeShader(glslang::TProgram& program) {
	std::vector<uint32_t> spriv;
}

void convertSpirvBackToGlsl(MH33::GFX::ShaderModuleCreateInfo& output, std::vector<uint32_t>&& vec)
{
	spirv_cross::CompilerGLSL glsl(std::move(vec));
	spirv_cross::ShaderResources resources = glsl.get_shader_resources();
	// Get all sampled images in the shader.
	/*for (auto &resource : resources.sampled_images)
	{
		unsigned set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
		unsigned binding = glsl.get_decoration(resource.id, spv::DecorationBinding);
		printf("Image %s at set = %u, binding = %u\n", resource.name.c_str(), set, binding);

		// Modify the decoration to prepare it for GLSL.
		glsl.unset_decoration(resource.id, spv::DecorationDescriptorSet);

		// Some arbitrary remapping if we want.
		glsl.set_decoration(resource.id, spv::DecorationBinding, set * 16 + binding);
	}*/

	// Set some options.
	spirv_cross::CompilerGLSL::Options options;
	options.version = 410;
	options.es = false;
	options.fragment.default_float_precision = spirv_cross::CompilerGLSL::Options::Lowp;
	options.fragment.default_int_precision = spirv_cross::CompilerGLSL::Options::Mediump;
	glsl.set_common_options(options);
	glsl.build_combined_image_samplers();
	for (auto &remap : glsl.get_combined_image_samplers())
	{
		glsl.set_name(remap.combined_id, glsl.get_name(remap.image_id));
	}

	// Compile to GLSL, ready to give to GL driver.
	std::string source = glsl.compile();
	// Debug
	std::cout << source << std::endl;
	output.isBinary = false;
	output.source.resize(source.size() + 1);
	std::memcpy(output.source.data(),source.data(),source.size() + 1);
}

void initialize()
{
	glslang::InitializeProcess();
	initResources();
}

void finalize()
{
	glslang::FinalizeProcess();
}



}
}
