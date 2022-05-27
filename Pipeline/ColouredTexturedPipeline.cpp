#include "ColouredTexturedPipeline.hpp"

ColouredTexturedVertexOut ColouredTexturedVertexShader(const ColouredTexturedUniform& uniform, const ColouredTexturedVertexIn& vertex)
{
	ColouredTexturedVertexOut out = { glm::fvec4(vertex.POS,0.5f, 1.0f) , vertex.TEXCOORD, vertex.COLOUR };
	//if(perspectiveCorrection) out.TEXCOORD /= out.POS.z;
	return out;
}

void ColouredTexturedFragmentShader(Texture& framebuffer, const glm::ivec2& point, const ColouredTexturedUniform& uniform, const ColouredTexturedVertexOut& v0)
{
	if(point.x < 0 || point.y < 0) return;
	if(!uniform.tex) return;
	framebuffer.setPixelWithBlending(point,uniform.tex->sample(v0.TEXCOORD,point,uniform.samplerState) * v0.COLOUR,uniform.blending);
}
