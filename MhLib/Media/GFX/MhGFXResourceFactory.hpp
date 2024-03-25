#ifndef MHGFXRESOURCEFACTORY_HPP
#define MHGFXRESOURCEFACTORY_HPP
#include <MhLib/Media/Image/MhDecodeTarget.hpp>
#include <MhLib/Media/GFX/MhMesh.hpp>
#include <MhLib/Media/GFX/MhTexture.hpp>
#include <MhLib/Media/GFX/MhUniformBuffer.hpp>
#include <MhLib/Media/GFX/MhPipeline.hpp>
#include <vector>
#include <span>
#include <string>
#include <map>
namespace MH33 {
namespace GFX {

typedef std::map<std::string,std::vector<std::byte>> ShaderBinary;

DEFINE_CLASS(ResourceFactory)
class ResourceFactory {
public:
	virtual ~ResourceFactory() = default;
	virtual void beginFrame() = 0;
	virtual void endFrame() = 0;
	// Textures
	virtual sTexture2D createTexture2D(const Image::DecodeTarget& source) = 0;
	virtual sTexture2D createTexture2D(const Image::DecodeTarget& source, std::vector<bool>& bitmap, glm::ivec2& dimensions) = 0;
	virtual sWriteableTexture2D createWriteableTexture2D(const Image::DecodeTarget& source) = 0;
	virtual sWriteableTexture2D createWriteableTexture2D(TextureFormat format, const glm::ivec2& dimensions) = 0;
	// Meshes
	virtual sUnindexedMesh createUnindexedMesh(const VertexDescriptor* vertexDescriptor, bool streaming = false) = 0;
	virtual sUnindexedMesh createUnindexedMesh(const VertexDescriptor* vertexDescriptor, const std::span<const std::byte>& vertexData, bool streaming = false) = 0;
	virtual sIndexedMesh createIndexedMesh(const VertexDescriptor* vertexDescriptor, bool streaming = false) = 0;
	virtual sIndexedMesh createIndexedMesh(const VertexDescriptor* vertexDescriptor, const std::span<const std::byte>& vertexData, const std::span<const uint32_t>& indices, bool streaming = false) = 0;
	virtual sUniformBuffer createUniformBuffer(size_t size) = 0;
	// Shaders
	virtual bool supportsCustomShaders() const = 0;
	virtual sPipeline createPipeline(const std::string& name) = 0;
	virtual sPipeline createPipeline(const ShaderBinary& binaries, VertexDescriptor* vertexInput) = 0;
};

}
}
#endif // MHGFXRESOURCEFACTORY_HPP
