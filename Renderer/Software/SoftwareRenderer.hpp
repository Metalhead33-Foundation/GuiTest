#ifndef SOFTWARERENDERER_HPP
#define SOFTWARERENDERER_HPP
#include <MhLib/Media/GFX/MhGFXResourceFactory.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

namespace Renderer {
namespace Software {
DEFINE_CLASS(SoftwareRenderer)
class SoftwareRenderer : public MH33::GFX::ResourceFactory
{
private:
	SDL_Window* window;
	SDL_SysWMinfo* winfo;
public:
	SoftwareRenderer(SDL_Window* window, SDL_SysWMinfo* winfo);
	MH33::GFX::sTexture2D createTexture2D(const MH33::Image::DecodeTarget& source) override;
	MH33::GFX::sTexture2D createTexture2D(const MH33::Image::DecodeTarget& source, std::vector<bool>& bitmap, glm::ivec2& dimensions) override;
	MH33::GFX::sWriteableTexture2D createWriteableTexture2D(const MH33::Image::DecodeTarget& source) override;
	MH33::GFX::sWriteableTexture2D createWriteableTexture2D(MH33::GFX::TextureFormat format, const glm::ivec2& dimensions) override;
	MH33::GFX::sUnindexedMesh createUnindexedMesh(const MH33::GFX::VertexDescriptor* vertexDescriptor, bool streaming) override;
	MH33::GFX::sUnindexedMesh createUnindexedMesh(const MH33::GFX::VertexDescriptor* vertexDescriptor, const std::span<const std::byte>& vertexData, bool streaming) override;
	MH33::GFX::sIndexedMesh createIndexedMesh(const MH33::GFX::VertexDescriptor* vertexDescriptor, bool streaming) override;
	MH33::GFX::sIndexedMesh createIndexedMesh(const MH33::GFX::VertexDescriptor* vertexDescriptor, const std::span<const std::byte>& vertexData, const std::span<const uint32_t>& indices, bool streaming) override;
	MH33::GFX::sUniformBuffer createUniformBuffer(size_t size) override;
	bool supportsCustomShaders() const override;
	MH33::GFX::sPipeline createPipeline(const std::string& name) override;
	MH33::GFX::sPipeline createPipeline(const MH33::GFX::ShaderBinary& binaries, MH33::GFX::VertexDescriptor* vertexInput) override;
	void beginFrame() override;
	void endFrame() override;
};
}
}

#endif // SOFTWARERENDERER_HPP
