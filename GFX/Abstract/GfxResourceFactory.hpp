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
	virtual pPipeline createPipeline(Io::System& iosys, const std::string& shaderName, const VertexDescriptor* vertexDescriptor) = 0;
	virtual sComputeShader createComputeShader(Io::System& iosys, const std::string& shaderName) = 0;
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
	virtual pTexture2D createTexture2D(const Image::DecodeTarget& decodeTarget) = 0;
	virtual pTexture2D createTexture2D(const Image::Image2D& decodeTarget) = 0;
	virtual pTexture3D createTexture3D(const Image::DecodeTarget& decodeTarget) = 0;
	virtual pTextureArray2D createTextureArray2D(const Image::DecodeTarget& decodeTarget) = 0;
	virtual pCubemap createCubemap(const Image::DecodeTarget& decodeTarget) = 0;
	virtual pWriteableTexture2D createWriteableTexture2D(Image::Format format, int width, int height) = 0;
	virtual pFramebuffer createFramebuffer(std::span<Image::Format> attachmentFormat, int width, int height) = 0;
	virtual pCubemapFramebuffer createCubemapFramebuffer(std::span<Image::Format> attachmentFormat, int width, int height) = 0;
	// Utilities
	virtual void makeCurrent() = 0;
	virtual void beginFrame() = 0;
	virtual void endFrame() = 0;
};

}
}
#endif // GFXRESOURCEFACTORY_HPP
