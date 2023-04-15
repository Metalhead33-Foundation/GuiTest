#include "GuiRenderer.hpp"
namespace MH33 {
namespace GFX {

ColouredGuiTechnique::ColouredGuiTechnique(ResourceFactory& factory)
	: pipeline(factory.createPipeline("single_colour_widget")), ubuffer(factory.createUniformBuffer(sizeof(glm::fvec4)) )
{
	ubuffer_id = pipeline->getId("uniforms");
}

void ColouredGuiTechnique::renderTriangles(UnindexedMesh& mesh, const glm::vec4& clr)
{
	ubuffer->access([&clr] (void* ptr, size_t) {
		std::memcpy(ptr,&clr,sizeof(glm::vec4));
	});
	pipeline->setUniformBuffer(ubuffer_id,*ubuffer);
	pipeline->drawTriangles(mesh);
}

void ColouredGuiTechnique::renderTriangles(IndexedMesh& mesh, const glm::vec4& clr)
{
	ubuffer->access([&clr] (void* ptr, size_t) {
		std::memcpy(ptr,&clr,sizeof(glm::vec4));
	});
	pipeline->setUniformBuffer(ubuffer_id,*ubuffer);
	pipeline->drawTriangles(mesh);
}

void ColouredGuiTechnique::renderLines(UnindexedMesh& mesh, const glm::vec4& clr, float thickness)
{
	ubuffer->access([&clr] (void* ptr, size_t) {
		std::memcpy(ptr,&clr,sizeof(glm::vec4));
	});
	pipeline->setUniformBuffer(ubuffer_id,*ubuffer);
	pipeline->drawLines(mesh,thickness);
}

void ColouredGuiTechnique::renderLines(IndexedMesh& mesh, const glm::vec4& clr, float thickness)
{
	ubuffer->access([&clr] (void* ptr, size_t) {
		std::memcpy(ptr,&clr,sizeof(glm::vec4));
	});
	pipeline->setUniformBuffer(ubuffer_id,*ubuffer);
	pipeline->drawLines(mesh,thickness);
}

TexturedGuiTechnique::TexturedGuiTechnique(ResourceFactory& factory)
	: pipeline(factory.createPipeline("textured_widget"))
{
	texture_id = pipeline->getId("texture");
}

void TexturedGuiTechnique::renderTriangles(UnindexedMesh& mesh, const Texture2D& texture)
{
	pipeline->setTexture(texture_id,texture);
	pipeline->drawTriangles(mesh);
}

void TexturedGuiTechnique::renderTriangles(IndexedMesh& mesh, const Texture2D& texture)
{
	pipeline->setTexture(texture_id,texture);
	pipeline->drawTriangles(mesh);
}

void TexturedGuiTechnique::renderLines(UnindexedMesh& mesh, const Texture2D& texture, float thickness)
{
	pipeline->setTexture(texture_id,texture);
	pipeline->drawLines(mesh,thickness);
}

void TexturedGuiTechnique::renderLines(IndexedMesh& mesh, const Texture2D& texture, float thickness)
{
	pipeline->setTexture(texture_id,texture);
	pipeline->drawLines(mesh,thickness);
}

ColouredTexturedGuiTechnique::ColouredTexturedGuiTechnique(ResourceFactory& factory, bool text)
	: pipeline(factory.createPipeline(text ? "coloured_text" : "coloured_textured_widget")), ubuffer(factory.createUniformBuffer(sizeof(glm::vec4)))
{
	ubuffer_id = pipeline->getId("uniforms");
	texture_id = pipeline->getId("texture");
}

void ColouredTexturedGuiTechnique::renderTriangles(UnindexedMesh& mesh, const glm::vec4& clr, const Texture2D& texture)
{
	ubuffer->access([&clr] (void* ptr, size_t) {
		std::memcpy(ptr,&clr,sizeof(glm::vec4));
	});
	pipeline->setUniformBuffer(ubuffer_id,*ubuffer);
	pipeline->setTexture(texture_id,texture);
	pipeline->drawTriangles(mesh);
}

void ColouredTexturedGuiTechnique::renderTriangles(IndexedMesh& mesh, const glm::vec4& clr, const Texture2D& texture)
{
	ubuffer->access([&clr] (void* ptr, size_t) {
		std::memcpy(ptr,&clr,sizeof(glm::vec4));
	});
	pipeline->setUniformBuffer(ubuffer_id,*ubuffer);
	pipeline->setTexture(texture_id,texture);
	pipeline->drawTriangles(mesh);
}

void ColouredTexturedGuiTechnique::renderLines(UnindexedMesh& mesh, const glm::vec4& clr, const Texture2D& texture, float thickness)
{
	ubuffer->access([&clr] (void* ptr, size_t) {
		std::memcpy(ptr,&clr,sizeof(glm::vec4));
	});
	pipeline->setUniformBuffer(ubuffer_id,*ubuffer);
	pipeline->setTexture(texture_id,texture);
	pipeline->drawLines(mesh,thickness);
}

void ColouredTexturedGuiTechnique::renderLines(IndexedMesh& mesh, const glm::vec4& clr, const Texture2D& texture, float thickness)
{
	ubuffer->access([&clr] (void* ptr, size_t) {
		std::memcpy(ptr,&clr,sizeof(glm::vec4));
	});
	pipeline->setUniformBuffer(ubuffer_id,*ubuffer);
	pipeline->setTexture(texture_id,texture);
	pipeline->drawLines(mesh,thickness);
}

/*
	ColouredGuiTechnique colouredPipeline;
	TexturedGuiTechnique texturedPipeline;
	sIndexedMesh colouredIMesh;
	sIndexedMesh texturedIMesh;
	sUnindexedMesh colouredUMesh;
*/

static const unsigned quadIds[] = { 0, 1, 2, 1, 2, 3 };
GuiRenderingContext::GuiRenderingContext(ResourceFactory& factory)
	: colouredPipeline(factory), texturedPipeline(factory),
	  colouredIMesh(factory.createIndexedMesh(&Renderer::ColouredWidgetVert::DESCRIPTOR)),
	  texturedIMesh(factory.createIndexedMesh(&Renderer::TexturedWidgetVert::DESCRIPTOR)),
	  colouredUMesh(factory.createUnindexedMesh(&Renderer::ColouredWidgetVert::DESCRIPTOR))
{
	colouredIMesh->ensureVertexCount(4);
	colouredIMesh->ensureIndexCount(6);
	texturedIMesh->ensureVertexCount(4);
	texturedIMesh->ensureIndexCount(6);
	colouredIMesh->accessIndices([](std::span<uint32_t>& span) {
		for(int i = 0; i < 6; ++i) {
			span[i] = quadIds[i];
		}
	});
	texturedIMesh->accessIndices([](std::span<uint32_t>& span) {
		for(int i = 0; i < 6; ++i) {
			span[i] = quadIds[i];
		}
	});
}


}
}
