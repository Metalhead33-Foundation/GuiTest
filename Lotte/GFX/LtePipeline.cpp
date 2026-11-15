#include "LtePipeline.hpp"
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/Include/ResourceLimits.h>
#include <map>

namespace Lotte {
namespace Gfx {

Pipeline::Pipeline() {}

// TODO: Complete refactor. (and possible reconsider if this even belongs in Lotte)
// Also, if we keep it, it would be great to do some C/C++ header preprocessor stuff. inside the shader source.
class GlslangIncluder : public glslang::TShader::Includer
{
private:
	Elv::Io::System* iosys;
	std::map<std::string, std::unique_ptr<IncludeResult>> mIncludes;
	std::map<std::string, std::vector<std::byte>> mSources;
public:
	GlslangIncluder(Elv::Io::System* iosys);

	// Includer interface
public:
	/*
		// For the "system" or <>-style includes; search the "system" paths.
		virtual IncludeResult* includeSystem(const char* headerName,
											 const char* includerName,
											 size_t inclusionDepth) { return nullptr; }

		// For the "local"-only aspect of a "" include. Should not search in the
		// "system" paths, because on returning a failure, the parser will
		// call includeSystem() to look in the "system" locations.
		virtual IncludeResult* includeLocal(const char* headerName,
											const char* includerName,
											size_t inclusionDepth) { return nullptr; }
*/

	IncludeResult* includeSystem(const char* headerName, const char* includerName, size_t inclusionDepth) override;
	IncludeResult* includeLocal(const char* headerName, const char* includerName, size_t inclusionDepth) override;
	void releaseInclude(IncludeResult* res) override;
};

GlslangIncluder::GlslangIncluder(Elv::Io::System* iosys)
	: iosys(iosys)
{

}

glslang::TShader::Includer::IncludeResult* GlslangIncluder::includeSystem(const char* headerName, const char* includerName, size_t inclusionDepth)
{
	std::string path = "/shaders/" + std::string(headerName);
	{
		auto it = mIncludes.find(path);
		if(it != std::end(mIncludes)) {
			return it->second.get();
		}
	}
	if(!iosys->exists(path)) {
		return nullptr;
	}
	auto iodev(iosys->open(path,Elv::Io::Mode::READ));
	std::string tmpStr;
	mSources[path] = iodev->readAllAsVector();
	auto result = std::make_unique<IncludeResult>(
		path, reinterpret_cast<const char*>(mSources[path].data()),
		mSources[path].size(), nullptr);
	auto [it, b] = mIncludes.emplace(std::make_pair(path, std::move(result)));
	return it->second.get();
}

glslang::TShader::Includer::IncludeResult* GlslangIncluder::includeLocal(const char* headerName, const char* includerName, size_t inclusionDepth)
{
	return nullptr;
}

void GlslangIncluder::releaseInclude(IncludeResult*res)
{
	{
		auto it = mSources.find(res->headerName);
		if (it != mSources.end())
		{
			mSources.erase(it);
		}
	}
	{
		auto it = mIncludes.find(res->headerName);
		if (it != mIncludes.end())
		{
			it->second = nullptr;
			mIncludes.erase(it);
		}
	}
}

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

// TODO: Complete refactor. (and possible reconsider if this even belongs in Lotte)
void Pipeline::prepareShaderModuleFor(Elv::Io::System& iosys, const std::span<ShaderModuleCreateInfo>& output, const std::span<const std::string>& input)
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
			case Kld::ShaderStage::Vertex: stage = EShLangVertex; break;
			case Kld::ShaderStage::Geometry: stage = EShLangGeometry; break;
			case Kld::ShaderStage::TessControl: stage = EShLangTessControl; break;
			case Kld::ShaderStage::TessEvaluation: stage = EShLangTessEvaluation; break;
			case Kld::ShaderStage::Fragment: stage = EShLangFragment; break;
			case Kld::ShaderStage::Compute: stage = EShLangCompute; break;
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
		shader.setEnvInput(glslang::EShSource::EShSourceHlsl, stage, glslang::EShClient::EShClientVulkan, 100);
		shader.setEnvClient(glslang::EShClient::EShClientVulkan, glslang::EshTargetClientVersion::EShTargetVulkan_1_0);
		shader.setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_0);
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
	glslang::SpvOptions spvOpts;
	spvOpts.disableOptimizer = false;
	spvOpts.disassemble = false;
	spvOpts.generateDebugInfo = false;
	spvOpts.disassemble = false;
	spvOpts.compileOnly = false;
	spvOpts.optimizeSize = false;
	spvOpts.validate = true;
	for(size_t i = 0; i < output.size(); ++i) {
		auto& out = output[i];
		const EShLanguage& stage = stages[i];
		std::vector<uint32_t> spirv;
		glslang::GlslangToSpv(*program.getIntermediate(stage), spirv, &spvOpts);
		// convertSpirvBackToGlsl(out,std::move(spirv));
		// Nope. We don't convert to GLSL or HLSL. That is outside Lotte's jurisdiction.
		// Lotte hands SPIRV to Kaldi and that is final.
	}
}

}
}