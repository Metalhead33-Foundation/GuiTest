#include "TexturedPipeline.hpp"

namespace SoftwareRenderer {
TexturedVertexOut TexturedVertexShader(const TexturedUniform &uniform, const TexturedVertexIn &vertex)
{
	TexturedVertexOut out = { glm::fvec4(vertex.POS,0.5f, 1.0f) , vertex.TEXCOORD };
	//if(perspectiveCorrection) out.TEXCOORD /= out.POS.z;
	return out;
}

void TexturedFragmentShader(Texture &framebuffer, const glm::ivec2 &point, const TexturedUniform &uniform,
						 const TexturedVertexOut &v0)
{
	if(point.x < 0 || point.y < 0) return;
	if(!uniform.tex) return;
	framebuffer.setPixelWithBlending(point,uniform.tex->sample(v0.TEXCOORD,point,uniform.samplerState),uniform.blending);
}
}