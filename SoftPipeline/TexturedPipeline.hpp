#ifndef TEXTUREDPIPELINE_HPP
#define TEXTUREDPIPELINE_HPP
#include <glm/glm.hpp>
#include "../Texture/Texture.hpp"
#include "RenderingPipeline.hpp"
#include <memory>

namespace SoftwareRenderer {
struct TexturedUniform {
	const Texture* tex;
	Sampler samplerState;
	AlphaBlending blending;
};
struct TexturedVertexIn {
	glm::fvec2 POS;
	glm::fvec2 TEXCOORD;
};
struct TexturedVertexOut {
	glm::fvec4 POS;
	glm::fvec2 TEXCOORD;
	inline static TexturedVertexOut split(const TexturedVertexOut& t, const TexturedVertexOut& m, const TexturedVertexOut& b, float dy, float iy) {
		return { glm::fvec4(
						t.POS.x + ((b.POS.x - t.POS.x) / dy) * iy,
						m.POS.y,
						t.POS.z + ((b.POS.z - t.POS.z) / dy) * iy,
						t.POS.w + ((b.POS.w - t.POS.w) / dy) * iy
						),
					glm::fvec2(
						t.TEXCOORD.r + ((b.TEXCOORD.r - t.TEXCOORD.r) / dy) * iy,
						t.TEXCOORD.g + ((b.TEXCOORD.g - t.TEXCOORD.g) / dy) * iy
						) };
	}
	inline static TexturedVertexOut interpolate(const TexturedVertexOut& v0, const TexturedVertexOut& v1, float w0, float w1, bool perspectiveCorrect) {
		TexturedVertexOut out = {
							glm::fvec4(  // POS
							(v0.POS.x * w0) + (v1.POS.x * w1), // X
							(v0.POS.y * w0) + (v1.POS.y * w1), // Y
							(v0.POS.z * w0) + (v1.POS.z * w1), // Z
							(v0.POS.w * w0) + (v1.POS.w * w1) // Z
							),
							glm::fvec2( // TEXCOORD
							(v0.TEXCOORD.x * w0) + (v1.TEXCOORD.x * w1), // X
							(v0.TEXCOORD.y * w0) + (v1.TEXCOORD.y * w1) // Y
							)
					};
		if(perspectiveCorrect) out.TEXCOORD *= out.POS.w; // We're assuming that the TEXCOORD attribute of each vertex was divided by POS.z prior to calling this function
		return out;
	}
	inline static TexturedVertexOut interpolate2D(const TexturedVertexOut& v0, const TexturedVertexOut& v1, float rowWeight, float columnWeight, bool perspectiveCorrect) {
		const float w1 = rowWeight * columnWeight;
		const float w0 = 1.0f - w1;
		TexturedVertexOut out = {
							glm::fvec4(  // POS
							(v0.POS.x * (1.0f - columnWeight)) + (v1.POS.x * columnWeight), // X
							(v0.POS.y * (1.0f - rowWeight)) + (v1.POS.y * rowWeight), // Y
							(v0.POS.z * w0) + (v1.POS.z * w1), // Z
							(v0.POS.w * w0) + (v1.POS.w * w1) // W
							),
							glm::fvec2( // TEXCOORD
							(v0.TEXCOORD.x * (1.0f - columnWeight)) + (v1.TEXCOORD.x * columnWeight), // X
							(v0.TEXCOORD.y * (1.0f - rowWeight)) + (v1.TEXCOORD.y * rowWeight) // Y
							)
					};
		if(perspectiveCorrect) out.TEXCOORD *= out.POS.w; // We're assuming that the TEXCOORD attribute of each vertex was divided by POS.z prior to calling this function
		return out;
	}
	inline static TexturedVertexOut interpolate(const TexturedVertexOut& v0, const TexturedVertexOut& v1, const TexturedVertexOut& v2, float w0, float w1, float w2, bool perspectiveCorrect) {
		TexturedVertexOut out = {
							glm::fvec4(  // POS
							(v0.POS.x * w0) + (v1.POS.x * w1) + (v2.POS.x * w2), // X
							(v0.POS.y * w0) + (v1.POS.y * w1) + (v2.POS.y * w2), // Y
							(v0.POS.z * w0) + (v1.POS.z * w1) + (v2.POS.z * w2), // Z
							(v0.POS.w * w0) + (v1.POS.w * w1) + (v2.POS.w * w2) // Z
							),
							glm::fvec2( // TEXCOORD
							(v0.TEXCOORD.x * w0) + (v1.TEXCOORD.x * w1) + (v2.TEXCOORD.x * w2), // X
							(v0.TEXCOORD.y * w0) + (v1.TEXCOORD.y * w1) + (v2.TEXCOORD.y * w2) // Y
							)
					};
		if(perspectiveCorrect) out.TEXCOORD *= out.POS.w; // We're assuming that the TEXCOORD attribute of each vertex was divided by POS.z prior to calling this function
		return out;
	}
	void perspectiveCorrect() {
		TEXCOORD /= POS.z;
	}
};

typedef RenderingPipeline<TexturedVertexIn,TexturedVertexOut,TexturedUniform> TexturedPipeline;
TexturedVertexOut TexturedVertexShader(const TexturedUniform& uniform, const TexturedVertexIn& vertex);
void TexturedFragmentShader(Texture& framebuffer, const glm::ivec2& point, const TexturedUniform& uniform,
						 const TexturedVertexOut& v0);
}
#endif // TEXTUREDPIPELINE_HPP
