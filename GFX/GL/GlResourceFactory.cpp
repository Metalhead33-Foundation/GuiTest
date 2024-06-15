#include "GlResourceFactory.hpp"
#include "GlTexture.hpp"
#include "GlVertexBuffer.hpp"
#include "GlPipeline.hpp"
#include "GlComputeShader.hpp"
#include "GlStorageBuffer.hpp"
#include "GlWriteableTexture2D.hpp"
#include "GlFramebuffer.hpp"
#define MH33_IMG_LEAN_AND_MEAN
#include <MhLib/Media/Image/MhStandardImage2D.hpp>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/Include/ResourceLimits.h>

namespace GL {

#ifdef DESKTOP_GL
#define GL_SHADER_PATH(TYPE) CONCAT("/Shaders/OpenGL/",STRINGIFY(TYPE))
#else
#define GL_SHADER_PATH(TYPE) CONCAT("/Shaders/OpenGLES/",STRINGIFY(TYPE))
#endif

const std::array<std::string, 5> GlShaderPaths = {
	GL_SHADER_PATH(Vert/),
	GL_SHADER_PATH(TessControl/),
	GL_SHADER_PATH(TessEval/),
	GL_SHADER_PATH(Geom/),
	GL_SHADER_PATH(Frag/)
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

ResourceFactory::ResourceFactory()
{
	glslang::InitializeProcess();
	initResources();
}

ResourceFactory::~ResourceFactory()
{
	glslang::FinalizeProcess();
}

MH33::GFX::pPipeline ResourceFactory::createPipeline(const std::span<const MH33::GFX::ShaderModuleCreateInfo>& createInfo, const MH33::GFX::VertexDescriptor* vertexDescriptor)
{
	return new Pipeline(createInfo);
}

MH33::GFX::pPipeline ResourceFactory::createPipeline(const std::span<const MH33::GFX::ShaderModuleCreateInfoRef>& createInfo, const MH33::GFX::VertexDescriptor* vertexDescriptor)
{
	return new Pipeline(createInfo);
}

/*MH33::GFX::pPipeline ResourceFactory::createPipeline(MH33::Io::System& iosys, const std::string& shaderName, const MH33::GFX::VertexDescriptor* vertexDescriptor)
{
	std::vector<ShaderModuleCreateInfo> createInfo;
	for(size_t i = 0; i < GlShaderPaths.size(); ++i) {
		std::string path = GlShaderPaths[0] + shaderName + ".glsl";
		if(!iosys.exists(path)) continue;
		GLenum shaderType;
		switch (i) {
			case 0: shaderType = GL_VERTEX_SHADER; break;
			case 1: shaderType = GL_TESS_CONTROL_SHADER; break;
			case 2: shaderType = GL_TESS_EVALUATION_SHADER; break;
			case 3: shaderType = GL_GEOMETRY_SHADER; break;
			case 4: shaderType = GL_FRAGMENT_SHADER; break;
			default: shaderType = GL_INVALID_ENUM;
		}
		std::unique_ptr<MH33::Io::Device> reader(iosys.open(path,MH33::Io::Mode::READ));
		std::string source = reader->readAllAsString();
		createInfo.push_back({ .shaderType = shaderType, .sourceCode = std::move(source) });
	}
	return new Pipeline(createInfo);
}*/

MH33::GFX::pComputeShader ResourceFactory::createComputeShader(const std::span<const MH33::GFX::ShaderModuleCreateInfo>& createInfo)
{
	return new ComputeShader(createInfo);
}

MH33::GFX::pComputeShader ResourceFactory::createComputeShader(const std::span<const MH33::GFX::ShaderModuleCreateInfoRef>& createInfo)
{
	return new ComputeShader(createInfo);
}

MH33::GFX::pStorageBuffer  ResourceFactory::createStorageBuffer(MH33::GFX::StorageBufferType type, size_t size)
{
	switch (type) {
		case MH33::GFX::UNIFORM_BUFFER:
			return new UniformBuffer(size);
		case MH33::GFX::SHADER_STORAGE_BUFFER:
			return new ShaderStorageBuffer(size);
		default: return nullptr;
	}
}

MH33::GFX::pUnindexedVertexBuffer  ResourceFactory::createUnindexedVertexBuffer(MH33::GFX::VertexBufferUsageClass storageClass, const MH33::GFX::VertexDescriptor* vertexDescriptor, size_t sizeInBytes)
{
	switch (storageClass) {
		case MH33::GFX::VertexBufferUsageClass::Static:
			return new UnindexedVertexBuffer(GL_STATIC_DRAW,vertexDescriptor,sizeInBytes);
		case MH33::GFX::VertexBufferUsageClass::Dynamic:
			return new UnindexedVertexBuffer(GL_DYNAMIC_DRAW,vertexDescriptor,sizeInBytes);
	}
}

MH33::GFX::pIndexedVertexBuffer  ResourceFactory::createIndexedVertexBuffer(MH33::GFX::VertexBufferUsageClass storageClass, const MH33::GFX::VertexDescriptor* vertexDescriptor, size_t sizeInBytes, size_t indexCount)
{
	switch (storageClass) {
		case MH33::GFX::VertexBufferUsageClass::Static:
			return new IndexedVertexBuffer(GL_STATIC_DRAW,vertexDescriptor,sizeInBytes,indexCount);
		case MH33::GFX::VertexBufferUsageClass::Dynamic:
			return new IndexedVertexBuffer(GL_DYNAMIC_DRAW,vertexDescriptor,sizeInBytes,indexCount);
	}
}

MH33::GFX::pTexture2D ResourceFactory::createTexture2D(MH33::GFX::TextureFormat format, unsigned width, unsigned height)
{
	return new Texture2D(format, width, height);
}

MH33::GFX::pTexture2D  ResourceFactory::createTexture2D(const MH33::Image::DecodeTarget& decodeTarget)
{
	return new Texture2D(decodeTarget);
}

MH33::GFX::pTexture2D ResourceFactory::createTexture2D(const MH33::Image::Image2D& decodeTarget)
{
	return new Texture2D(decodeTarget);
}

MH33::GFX::pTexture3D  ResourceFactory::createTexture3D(const MH33::Image::DecodeTarget& decodeTarget)
{
	return new Texture3D(decodeTarget);
}

MH33::GFX::pTextureArray2D  ResourceFactory::createTextureArray2D(const MH33::Image::DecodeTarget& decodeTarget)
{
	return new TextureArray2D(decodeTarget);
}

MH33::GFX::pCubemap  ResourceFactory::createCubemap(const MH33::Image::DecodeTarget& decodeTarget)
{
	return new Cubemap(decodeTarget);
}

MH33::GFX::pWriteableTexture2D  ResourceFactory::createWriteableTexture2D(MH33::Image::Format format, int width, int height)
{
	return new WriteableTexture2D([format, width, height]() {
		return MH33::Image::createImage2D(format,width,height);
	});
}

MH33::GFX::pFramebuffer  ResourceFactory::createFramebuffer(const std::span<const MH33::Image::Format>& attachmentFormat, int width, int height)
{
	return new Framebuffer(attachmentFormat, width, height);
}

MH33::GFX::pCubemapFramebuffer  ResourceFactory::createCubemapFramebuffer(const std::span<const MH33::Image::Format>& attachmentFormat, int width, int height)
{
	return nullptr;
}

void ResourceFactory::makeCurrent()
{
	getContext().makeCurrent();
}

void ResourceFactory::beginFrame()
{
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void ResourceFactory::endFrame()
{
	getContext().swapBuffers();
}

bool ResourceFactory::supportsBinaryShaders() const
{
	return GLAD_GL_ARB_gl_spirv != 0;
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

void ResourceFactory::prepareShaderModuleFor(MH33::GFX::ShaderModuleCreateInfo& output, const std::span<const std::byte>& input)
{
	EShLanguage stage;
	switch (output.shaderType) {
		case MH33::GFX::ShaderModuleType::VERTEX_SHADER: stage = EShLangVertex; break;
		case MH33::GFX::ShaderModuleType::GEOMETRY_SHADER: stage = EShLangGeometry; break;
		case MH33::GFX::ShaderModuleType::TESSELLATION_CONTROL_SHADER: stage = EShLangTessControl; break;
		case MH33::GFX::ShaderModuleType::TESSELLATION_EVALUATION_SHADER: stage = EShLangTessEvaluation; break;
		case MH33::GFX::ShaderModuleType::PIXEL_SHADER: stage = EShLangFragment; break;
		case MH33::GFX::ShaderModuleType::COMPUTE_SHADER: stage = EShLangCompute; break;
		default: throw std::runtime_error("Unsupported shader type.");
	}
	const char* shaderSource = reinterpret_cast<const char*>(input.data());
	glslang::TShader shader(stage);
	shader.setStrings(&shaderSource, 1);
	shader.setEnvInput(glslang::EShSourceHlsl, stage, glslang::EShClientOpenGL, 460);
	shader.setEnvClient(glslang::EShClientOpenGL, glslang::EShTargetOpenGL_450);
	shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);

	EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

	if (!shader.parse(&GlslangResources, 460, false, messages)) {
		throw HlslParseError(shader);
	}

	std::vector<unsigned int> spirv;
	glslang::GlslangToSpv(*shader.getIntermediate(), spirv);

	output.source.resize(spirv.size() * sizeof(unsigned int));
	std::memcpy(output.source.data(), spirv.data(), output.source.size());
	output.isBinary = true;
}

void ResourceFactory::prepareShaderModuleFor(std::span<MH33::GFX::ShaderModuleCreateInfo>& output, const std::span<const std::vector<std::byte> >& input)
{
	std::vector<EShLanguage> stages(output.size());
	std::vector<glslang::TShader> tempShaders;
	tempShaders.reserve(output.size());
	glslang::TProgram program;
	for(size_t i = 0; i < output.size(); ++i) {
		auto& out = output[i];
		const auto& in = input[i];
		EShLanguage& stage = stages[i];
		switch (out.shaderType) {
			case MH33::GFX::ShaderModuleType::VERTEX_SHADER: stage = EShLangVertex; break;
			case MH33::GFX::ShaderModuleType::GEOMETRY_SHADER: stage = EShLangGeometry; break;
			case MH33::GFX::ShaderModuleType::TESSELLATION_CONTROL_SHADER: stage = EShLangTessControl; break;
			case MH33::GFX::ShaderModuleType::TESSELLATION_EVALUATION_SHADER: stage = EShLangTessEvaluation; break;
			case MH33::GFX::ShaderModuleType::PIXEL_SHADER: stage = EShLangFragment; break;
			case MH33::GFX::ShaderModuleType::COMPUTE_SHADER: stage = EShLangCompute; break;
			default: throw std::runtime_error("Unsupported shader type.");
		}
		const char* shaderSource = reinterpret_cast<const char*>(input.data());
		tempShaders.push_back(glslang::TShader(stage));
		auto& shader = tempShaders.back();
		shader.setStrings(&shaderSource, 1);
		shader.setEnvInput(glslang::EShSourceHlsl, stage, glslang::EShClientOpenGL, 460);
		shader.setEnvClient(glslang::EShClientOpenGL, glslang::EShTargetOpenGL_450);
		shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);

		EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

		if (!shader.parse(&GlslangResources, 460, false, messages)) {
			throw HlslParseError(shader);
		}
		program.addShader(&shader);
	}
	EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
	if (!program.link(messages)) {
		throw GlslangProgramError(program);
	}
	for(size_t i = 0; i < output.size(); ++i) {
		auto& out = output[i];
		const EShLanguage& stage = stages[i];
		std::vector<unsigned int> spirv;
		glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);
		out.source.resize(spirv.size() * sizeof(unsigned int));
		std::memcpy(out.source.data(), spirv.data(), out.source.size());
		out.isBinary = true;
	}
}

}
