#include "ModelPipeline.hpp"
#include <glm/gtc/matrix_transform.hpp>

ModelVertexOut ModelVertexShader(const ModelUniform &uniform, const ModelVertexIn &vertex)
{
	glm::fvec4 tmp = glm::fvec4(vertex.POS,1.0f);
	//tmp.z *= -1.0;
	tmp = uniform.model * tmp;
	tmp = uniform.projection * tmp;
	ModelVertexOut out{ tmp, vertex.TEXCOORD, vertex.COLOUR };
	return out;
}

void ModelFragmentShader(Texture &framebuffer, const glm::ivec2 &point, const ModelUniform &uniform, const ModelVertexOut &v0)
{
	if(!uniform.tex) return;
	if(point.x < 0 || point.y < 0) return;
	float& zbuffpoint = uniform.zbuff->get(point.x,point.y);
	if(v0.POS.z >= 0.0f && v0.POS.z <= zbuffpoint) {
		glm::fvec4 colourKernel = uniform.tex->sample(v0.TEXCOORD,point,uniform.samplerState) * v0.COLOUR;
		if(framebuffer.setPixelWithBlending(point,colourKernel,uniform.blending)) zbuffpoint = v0.POS.z;
		}
}
