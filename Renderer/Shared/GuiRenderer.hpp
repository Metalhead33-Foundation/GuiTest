#ifndef GUIRENDERER_HPP
#define GUIRENDERER_HPP
#include <Renderer/Shared/GuiVertexTypes.hpp>
#include <MhLib/Media/GFX/MhGFXResourceFactory.hpp>
namespace MH33 {
namespace GFX {

DEFINE_CLASS(ColouredGuiTechnique)
class ColouredGuiTechnique
{
private:
	sPipeline pipeline;
	sUniformBuffer ubuffer;
	int ubuffer_id;
public:
	ColouredGuiTechnique(MH33::GFX::ResourceFactory& factory);
	void renderTriangles(UnindexedMesh& mesh, const glm::vec4& clr);
	void renderTriangles(IndexedMesh& mesh, const glm::vec4& clr);
	void renderLines(UnindexedMesh& mesh, const glm::vec4& clr, float thickness = 1.0f);
	void renderLines(IndexedMesh& mesh, const glm::vec4& clr, float thickness = 1.0f);

};
DEFINE_CLASS(TexturedGuiTechnique)
class TexturedGuiTechnique
{
private:
	sPipeline pipeline;
	int texture_id;
public:
	TexturedGuiTechnique(MH33::GFX::ResourceFactory& factory);
	void renderTriangles(UnindexedMesh& mesh, const Texture2D& texture);
	void renderTriangles(IndexedMesh& mesh, const Texture2D& texture);
	void renderLines(UnindexedMesh& mesh, const Texture2D& texture, float thickness = 1.0f);
	void renderLines(IndexedMesh& mesh, const Texture2D& texture, float thickness = 1.0f);
};
DEFINE_CLASS(ColouredTexturedGuiTechnique)
class ColouredTexturedGuiTechnique
{
private:
	sPipeline pipeline;
	sUniformBuffer ubuffer;
	int ubuffer_id;
	int texture_id;
public:
	ColouredTexturedGuiTechnique(MH33::GFX::ResourceFactory& factory, bool text = false);
	void renderTriangles(UnindexedMesh& mesh, const glm::vec4& clr, const Texture2D& texture);
	void renderTriangles(IndexedMesh& mesh, const glm::vec4& clr, const Texture2D& texture);
	void renderLines(UnindexedMesh& mesh, const glm::vec4& clr, const Texture2D& texture, float thickness = 1.0f);
	void renderLines(IndexedMesh& mesh, const glm::vec4& clr, const Texture2D& texture, float thickness = 1.0f);
};

DEFINE_STRUCT(GuiRenderingContext)
struct GuiRenderingContext {
	ColouredGuiTechnique colouredPipeline;
	TexturedGuiTechnique texturedPipeline;
	sIndexedMesh colouredIMesh;
	sIndexedMesh texturedIMesh;
	sUnindexedMesh colouredUMesh;
	std::vector<uint32_t> indexCache;
	std::vector<Renderer::TexturedWidgetVert> texturedVertCache;
	std::vector<Renderer::ColouredWidgetVert> colouredVertCache;
	GuiRenderingContext(MH33::GFX::ResourceFactory& factory);
};

}
}

#endif // GUIRENDERER_HPP
