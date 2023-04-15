#include "SoftGuiPipeline.hpp"
#include <cstring>
namespace Renderer {
namespace Software {
/*
 * Pipeline<ColouredWidgetVert,BasicVertOut,ColouredWidgetUniform>
	typedef std::function<VertexOutType(const UniformType&, const VertexInType&)> VertexShader;
	typedef std::function<void(RenderingContext&, const glm::ivec2&, const UniformType&, const VertexOutType&)> FragmentShader;
*/

static BasicVertOut colouredWidgetVertexShader(const ColouredWidgetUniform& u, const ColouredWidgetVert& v) {
	return { .POS = glm::fvec4(v.POS,0.0f,1.0f) };
}
static void colouredWidgetFragmentShader(RenderingContext& c, const glm::ivec2& p, const ColouredWidgetUniform& u, const BasicVertOut& v)
{
	if(p.x < 0 || p.y < 0) return;
	c.framebuffer->setPixelWithBlending(p,u.colour,c.blending);
}

ColouredWidgetPipeline::ColouredWidgetPipeline()
{
	_pipeline.context = &CONTEXT;
	_pipeline.vert = colouredWidgetVertexShader;
	_pipeline.frag = colouredWidgetFragmentShader;
}

void ColouredWidgetPipeline::setTexture(int id, const  MH33::GFX::Texture2D& texture)
{
	//if(id >= 0) CONTEXT.texture_units[id] = dynamic_cast<Texture*>(&texture);
	(void)id;
	(void)texture;
}

void ColouredWidgetPipeline::setTexture(int id, const MH33::GFX::Texture3D& texture)
{
	(void)id;
	(void)texture;
}

void ColouredWidgetPipeline::setTexture(int id, const MH33::GFX::ArrayTexture2D& texture)
{
	(void)id;
	(void)texture;
}

void ColouredWidgetPipeline::setUniformBuffer(int id, const MH33::GFX::UniformBuffer& texture)
{
	if(id == 1) {
		_pipeline.uniform = reinterpret_cast<const ColouredWidgetUniform*>(texture.getNativeHandle().ptr);
	}
}

int ColouredWidgetPipeline::getId(const std::string& uniformName)
{
	/*if(!uniformName.compare("texture")) {
		return 0;
	} else*/ if(!uniformName.compare("uniforms")) {
		return 1;
	}
	return -1;
}

// <TexturedWidgetVert,TexturedVertexOut,int>
static TexturedVertexOut texturedWidgetVertexShader(const int& u, const TexturedWidgetVert& v) {
	(void)u;
	return { .POS = glm::fvec4(v.POS,0.0f,1.0f), .TEXCOORD = v.TEXCOORD };
}
static void texturedWidgetFragmentShader(RenderingContext& c, const glm::ivec2& p, const int& u, const TexturedVertexOut& v)
{
	(void)u;
	if(p.x < 0 || p.y < 0) return;
	if(!c.texture_units[0]) return;
	c.framebuffer->setPixelWithBlending(p,c.texture_units[0]->sample(v.TEXCOORD,p,c.filtering),c.blending);
}

TexturedWidgetPipeline::TexturedWidgetPipeline()
{
	_pipeline.context = &CONTEXT;
	_pipeline.vert = texturedWidgetVertexShader;
	_pipeline.frag = texturedWidgetFragmentShader;
}

void TexturedWidgetPipeline::setTexture(int id, const MH33::GFX::Texture2D& texture)
{
	if(id >= 0) CONTEXT.texture_units[id] = dynamic_cast<const Texture*>(&texture);
}

void TexturedWidgetPipeline::setTexture(int id, const MH33::GFX::Texture3D& texture)
{
	(void)id;
	(void)texture;
}

void TexturedWidgetPipeline::setTexture(int id,const  MH33::GFX::ArrayTexture2D& texture)
{
	(void)id;
	(void)texture;
}

void TexturedWidgetPipeline::setUniformBuffer(int id, const MH33::GFX::UniformBuffer& texture)
{
	if(id == 1) {
		_pipeline.uniform = reinterpret_cast<const int*>(texture.getNativeHandle().ptr);
	}
}

int TexturedWidgetPipeline::getId(const std::string& uniformName)
{
	if(!uniformName.compare("texture")) {
		return 0;
	}
	return -1;
}

static TexturedVertexOut clrdTexturedWidgetVertexShader(const ColouredWidgetUniform& u, const TexturedWidgetVert& v) {
	(void)u;
	return { .POS = glm::fvec4(v.POS,0.0f,1.0f), .TEXCOORD = v.TEXCOORD };
}
static void clrdTexturedWidgetFragmentShader(RenderingContext& c, const glm::ivec2& p, const ColouredWidgetUniform& u, const TexturedVertexOut& v)
{
	if(p.x < 0 || p.y < 0) return;
	if(!c.texture_units[0]) return;
	c.framebuffer->setPixelWithBlending(p,c.texture_units[0]->sample(v.TEXCOORD,p,c.filtering) * u.colour,c.blending);
}
static void clrdTexturedWidgetFragmentShaderAlt(RenderingContext& c, const glm::ivec2& p, const ColouredWidgetUniform& u, const TexturedVertexOut& v)
{
	if(p.x < 0 || p.y < 0) return;
	if(!c.texture_units[0]) return;
	auto a = c.texture_units[0]->sample(v.TEXCOORD,p,c.filtering).r;
	c.framebuffer->setPixelWithBlending(p,glm::fvec4(u.colour.r,u.colour.g,u.colour.b,a),c.blending);
}

ColouredTexturedWidgetPipeline::ColouredTexturedWidgetPipeline(bool rAsAlpha)
{
	_pipeline.context = &CONTEXT;
	_pipeline.vert = clrdTexturedWidgetVertexShader;
	if(rAsAlpha) _pipeline.frag = clrdTexturedWidgetFragmentShaderAlt;
	else _pipeline.frag = clrdTexturedWidgetFragmentShader;
}

void ColouredTexturedWidgetPipeline::setTexture(int id, const MH33::GFX::Texture2D& texture)
{
	if(id >= 0) CONTEXT.texture_units[id] = dynamic_cast<const Texture*>(&texture);
}

void ColouredTexturedWidgetPipeline::setTexture(int id, const MH33::GFX::Texture3D& texture)
{
	(void)id;
	(void)texture;
}

void ColouredTexturedWidgetPipeline::setTexture(int id, const MH33::GFX::ArrayTexture2D& texture)
{
	(void)id;
	(void)texture;
}

void ColouredTexturedWidgetPipeline::setUniformBuffer(int id, const MH33::GFX::UniformBuffer& texture)
{
	if(id == 1) {
		_pipeline.uniform = reinterpret_cast<const ColouredWidgetUniform*>(texture.getNativeHandle().ptr);
	}
}

int ColouredTexturedWidgetPipeline::getId(const std::string& uniformName)
{
	if(!uniformName.compare("texture")) {
		return 0;
	} else if(!uniformName.compare("uniforms")) {
		return 1;
	}
	return -1;
}

}
}
