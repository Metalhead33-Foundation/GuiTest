#ifndef GFXRESOURCEFACTORY_HPP
#define GFXRESOURCEFACTORY_HPP
#include "Pipeline.hpp"
#include "ComputeShader.hpp"
#include "Texture.hpp"
#include "UniformBuffer.hpp"
#include "VertexBuffer.hpp"
#include "VertexBufferAdapter.hpp"
#include "VertexFormat.hpp"
#include "Framebuffer.hpp"
#include <MhLib/IoSys/MhIoSystem.hpp>
#include <MhLib/Media/Image/MhDecodeTarget.hpp>
namespace MH33 {
namespace GFX {

DEFINE_CLASS(ResourceFactory)
class ResourceFactory {
public:
	virtual ~ResourceFactory() = default;
	virtual pPipeline createPipeline(const std::span<const ShaderModuleCreateInfo>& createInfo, const VertexDescriptor* vertexDescriptor) = 0;
	virtual pPipeline createPipeline(const std::span<const ShaderModuleCreateInfoRef>& createInfo, const VertexDescriptor* vertexDescriptor) = 0;
	//virtual pPipeline createPipeline(Io::System& iosys, const std::string& shaderName, const VertexDescriptor* vertexDescriptor) = 0;
	virtual pComputeShader createComputeShader(const std::span<const ShaderModuleCreateInfo>& createInfo) = 0;
	virtual pComputeShader createComputeShader(const std::span<const ShaderModuleCreateInfoRef>& createInfo) = 0;
	virtual pStorageBuffer createStorageBuffer(StorageBufferType type, size_t size) = 0;
	virtual pUnindexedVertexBuffer createUnindexedVertexBuffer(VertexBufferUsageClass storageClass, const VertexDescriptor* vertexDescriptor, size_t sizeInBytes) = 0;
	template <typename T> inline TypedVertexBuffer<T> createTypedVertexBuffer(VertexBufferUsageClass storageClass, const VertexDescriptor* vertexDescriptor, size_t elementCount) {
		return TypedVertexBuffer<T>(
					[this](VertexBufferUsageClass storageClass, const VertexDescriptor* desc,size_t sizeInBytes) { return createUnindexedVertexBuffer(storageClass, desc,sizeInBytes); },
					storageClass,vertexDescriptor,elementCount
		);
	}
	virtual pIndexedVertexBuffer createIndexedVertexBuffer(VertexBufferUsageClass storageClass, const VertexDescriptor* vertexDescriptor, size_t sizeInBytes, size_t indexCount) = 0;
	template <typename T> inline IndexedTypedVertexBuffer<T> createTypedIndexedVertexBuffer(VertexBufferUsageClass storageClass, const VertexDescriptor* vertexDescriptor, size_t elementCount, size_t indexCount) {
		return IndexedTypedVertexBuffer<T>(
					[this](VertexBufferUsageClass storageClass, const VertexDescriptor* desc,size_t sizeInBytes, size_t indexCount) { return createIndexedVertexBuffer(storageClass,desc,sizeInBytes,indexCount); },
					storageClass,vertexDescriptor,elementCount, indexCount
		);
	}
	virtual pTexture2D createTexture2D(TextureFormat format, unsigned width, unsigned height) = 0;
	virtual pTexture2D createTexture2D(const Image::DecodeTarget& decodeTarget) = 0;
	virtual pTexture2D createTexture2D(const Image::Image2D& decodeTarget) = 0;
	virtual pTexture3D createTexture3D(const Image::DecodeTarget& decodeTarget) = 0;
	virtual pTextureArray2D createTextureArray2D(const Image::DecodeTarget& decodeTarget) = 0;
	virtual pCubemap createCubemap(const Image::DecodeTarget& decodeTarget) = 0;
	virtual pWriteableTexture2D createWriteableTexture2D(Image::Format format, int width, int height) = 0;
	virtual pFramebuffer createFramebuffer(const std::span<const MH33::Image::Format>& attachmentFormat, int width, int height) = 0;
	virtual pCubemapFramebuffer createCubemapFramebuffer(const std::span<const MH33::Image::Format>& attachmentFormat, int width, int height) = 0;
	// Shader compilation stuff
	virtual bool supportsBinaryShaders() const = 0; // Only really relevant for OpenGL
	virtual void prepareShaderModuleFor(Io::System& iosys, ShaderModuleCreateInfo& output, const std::string& input) = 0;
	virtual void prepareShaderModuleFor(Io::System& iosys, const std::span<ShaderModuleCreateInfo>& output, const std::span<const std::string>& input) = 0;
	// Utilities
	virtual void makeCurrent() = 0;
	virtual void beginFrame() = 0;
	virtual void endFrame() = 0;
};

}
}
#endif // GFXRESOURCEFACTORY_HPP
