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

}
