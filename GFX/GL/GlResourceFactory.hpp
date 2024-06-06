#ifndef RESOURCEFACTORY_HPP
#define RESOURCEFACTORY_HPP
#include <GFX/Abstract/GfxResourceFactory.hpp>
#include <GFX/GL/GlContext.hpp>

namespace GL {
class ResourceFactory : public MH33::GFX::ResourceFactory
{
public:
	virtual Context& getContext() = 0;
	MH33::GFX::pPipeline createPipeline(const std::span<const MH33::GFX::ShaderModuleCreateInfo>& createInfo, const MH33::GFX::VertexDescriptor* vertexDescriptor) override;
	MH33::GFX::pPipeline createPipeline(const std::span<const MH33::GFX::ShaderModuleCreateInfoRef>& createInfo, const MH33::GFX::VertexDescriptor* vertexDescriptor) override;
	//MH33::GFX::pPipeline createPipeline(MH33::Io::System& iosys, const std::string& shaderName, const MH33::GFX::VertexDescriptor* vertexDescriptor) override;
	MH33::GFX::pComputeShader createComputeShader(const std::span<const MH33::GFX::ShaderModuleCreateInfo>& createInfo) override;
	MH33::GFX::pComputeShader createComputeShader(const std::span<const MH33::GFX::ShaderModuleCreateInfoRef>& createInfo) override;
	MH33::GFX::pStorageBuffer createStorageBuffer(MH33::GFX::StorageBufferType type, size_t size) override;
	MH33::GFX::pUnindexedVertexBuffer createUnindexedVertexBuffer(MH33::GFX::VertexBufferUsageClass storageClass, const MH33::GFX::VertexDescriptor* vertexDescriptor, size_t sizeInBytes) override;
	MH33::GFX::pIndexedVertexBuffer createIndexedVertexBuffer(MH33::GFX::VertexBufferUsageClass storageClass, const MH33::GFX::VertexDescriptor* vertexDescriptor, size_t sizeInBytes, size_t indexCount) override;
	MH33::GFX::pTexture2D createTexture2D(const MH33::Image::DecodeTarget& decodeTarget) override;
	MH33::GFX::pTexture2D createTexture2D(const MH33::Image::Image2D& decodeTarget) override;
	MH33::GFX::pTexture3D createTexture3D(const MH33::Image::DecodeTarget& decodeTarget) override;
	MH33::GFX::pTextureArray2D createTextureArray2D(const MH33::Image::DecodeTarget& decodeTarget) override;
	MH33::GFX::pCubemap createCubemap(const MH33::Image::DecodeTarget& decodeTarget) override;
	MH33::GFX::pWriteableTexture2D createWriteableTexture2D(MH33::Image::Format format, int width, int height) override;
	MH33::GFX::pFramebuffer createFramebuffer(const std::span<const MH33::Image::Format>& attachmentFormat, int width, int height) override;
	MH33::GFX::pCubemapFramebuffer createCubemapFramebuffer(const std::span<const MH33::Image::Format>& attachmentFormat, int width, int height) override;
	void makeCurrent() override;
	void beginFrame() override;
	void endFrame() override;
};
}

#endif // RESOURCEFACTORY_HPP
