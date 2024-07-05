#ifndef MHHARDWAREACCELERATEDGUIRENDERER_HPP
#define MHHARDWAREACCELERATEDGUIRENDERER_HPP
#include <GUI/MhGuiRenderer.hpp>
#include <GFX/Advanced/MhHardwareAcceleratedRenderCache.hpp>
namespace MH33 {
namespace GFX {

struct ColouredGuiVertex {
	glm::vec2 POS;
	glm::vec4 CLR;
	static const MH33::GFX::AttributeDescriptor attributes[];
	static const MH33::GFX::VertexDescriptor vertexDescriptor;
};
struct TexturedGuiVertex {
	glm::vec2 POS;
	glm::vec2 TEX;
	static const MH33::GFX::AttributeDescriptor attributes[];
	static const MH33::GFX::VertexDescriptor vertexDescriptor;
};
struct ColouredTexturedGuiVertex {
	glm::vec2 POS;
	glm::vec2 TEX;
	glm::vec4 CLR;
	static const MH33::GFX::AttributeDescriptor attributes[];
	static const MH33::GFX::VertexDescriptor vertexDescriptor;
};

class GuiRenderer : public GUI::Renderer
{
private:
	CachedPipeline<ColouredGuiVertex, RenderType::LINES> ColouredLineCache;
	IndexedCachedPipeline<ColouredGuiVertex, RenderType::TRIANGLES> ColouredTriQuadCache;
	IndexedCachedPipelineWithTexture<TexturedGuiVertex, RenderType::TRIANGLES> TexturedQuadCache;
	IndexedCachedPipelineWithTexture<ColouredTexturedGuiVertex, RenderType::TRIANGLES> ColouredTexturedQuadCache;
	uintptr_t lastTexture;
	uint8_t lastRenderType;
	bool needsToFlush;
public:
	GuiRenderer(ResourceFactory& resFac, const sPipeline& colouredPipeline, const sPipeline& texturedPipeline, const sPipeline& colouredTexturedPipeline);
	// Renderer interface
public:
	void render(const GUI::ColouredQuad& target) override;
	void render(const GUI::GradientQuad& target) override;
	void render(const GUI::ColouredTriangle& target) override;
	void render(const GUI::GradientTriangle& target) override;
	void render(const GUI::ColouredLine& target) override;
	void render(const GUI::GradientLine& target) override;
	void render(const GUI::TexturedQuad& target) override;
	void render(const GUI::ColouredTexturedQuad& target) override;
	void render(const GUI::GradientTexturedQuad& target) override;
	void flush() override;
};

}
}

#endif // MHHARDWAREACCELERATEDGUIRENDERER_HPP
