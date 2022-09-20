#ifndef COLOUREDTEXTUREDPIPELINE_HPP
#define COLOUREDTEXTUREDPIPELINE_HPP
#include <glm/glm.hpp>
#include "../Texture/Texture.hpp"
#include "RenderingPipeline.hpp"
#include <memory>

namespace SoftwareRenderer {
struct ColouredTexturedUniform {
	const Texture* tex;
	Sampler samplerState;
	AlphaBlending blending;
};
struct ColouredTexturedVertexIn {
	glm::fvec2 POS;
	glm::fvec2 TEXCOORD;
	glm::fvec4 COLOUR;
};
struct ColouredTexturedVertexOut {
	glm::fvec4 POS;
	glm::fvec2 TEXCOORD;
	glm::fvec4 COLOUR;
	inline static ColouredTexturedVertexOut split(const ColouredTexturedVertexOut& t, const ColouredTexturedVertexOut& m, const ColouredTexturedVertexOut& b, float dy, float iy) {
		return { glm::fvec4(
						t.POS.x + ((b.POS.x - t.POS.x) / dy) * iy,
						m.POS.y,
						t.POS.z + ((b.POS.z - t.POS.z) / dy) * iy,
						t.POS.w + ((b.POS.w - t.POS.w) / dy) * iy
						),
					glm::fvec2(
						t.TEXCOORD.r + ((b.TEXCOORD.r - t.TEXCOORD.r) / dy) * iy,
						t.TEXCOORD.g + ((b.TEXCOORD.g - t.TEXCOORD.g) / dy) * iy
						),
					glm::fvec4(
						t.COLOUR.r + ((b.COLOUR.r - t.COLOUR.r) / dy) * iy,
						t.COLOUR.g + ((b.COLOUR.g - t.COLOUR.g) / dy) * iy,
						t.COLOUR.b + ((b.COLOUR.b - t.COLOUR.b) / dy) * iy,
						t.COLOUR.a + ((b.COLOUR.a - t.COLOUR.a) / dy) * iy
						) };
	}
	inline static ColouredTexturedVertexOut interpolate(const ColouredTexturedVertexOut& v0, const ColouredTexturedVertexOut& v1, float w0, float w1, bool perspectiveCorrect) {
		ColouredTexturedVertexOut out = {
							glm::fvec4(  // POS
							(v0.POS.x * w0) + (v1.POS.x * w1), // X
							(v0.POS.y * w0) + (v1.POS.y * w1), // Y
							(v0.POS.z * w0) + (v1.POS.z * w1), // Z
							(v0.POS.w * w0) + (v1.POS.w * w1) // Z
							),
							glm::fvec2( // TEXCOORD
							(v0.TEXCOORD.x * w0) + (v1.TEXCOORD.x * w1), // X
							(v0.TEXCOORD.y * w0) + (v1.TEXCOORD.y * w1) // Y
							),
							glm::fvec4( // COLOUR
							(v0.COLOUR.x * w0) + (v1.COLOUR.x * w1), // X
							(v0.COLOUR.y * w0) + (v1.COLOUR.y * w1), // Y
							(v0.COLOUR.z * w0) + (v1.COLOUR.z * w1), // Z
							(v0.COLOUR.w * w0) + (v1.COLOUR.w * w1) // W
							)
					};
		if(perspectiveCorrect) out.TEXCOORD *= out.POS.w; // We're assuming that the TEXCOORD attribute of each vertex was divided by POS.z prior to calling this function
		return out;
	}
	inline static ColouredTexturedVertexOut interpolate2D(const ColouredTexturedVertexOut& v0, const ColouredTexturedVertexOut& v1, float rowWeight, float columnWeight, bool perspectiveCorrect) {
		const float w1 = rowWeight * columnWeight;
		const float w0 = 1.0f - w1;
		ColouredTexturedVertexOut out = {
							glm::fvec4(  // POS
							(v0.POS.x * (1.0f - columnWeight)) + (v1.POS.x * columnWeight), // X
							(v0.POS.y * (1.0f - rowWeight)) + (v1.POS.y * rowWeight), // Y
							(v0.POS.z * w0) + (v1.POS.z * w1), // Z
							(v0.POS.w * w0) + (v1.POS.w * w1) // W
							),
							glm::fvec2( // TEXCOORD
							(v0.TEXCOORD.x * (1.0f - columnWeight)) + (v1.TEXCOORD.x * columnWeight), // X
							(v0.TEXCOORD.y * (1.0f - rowWeight)) + (v1.TEXCOORD.y * rowWeight) // Y
							),
							glm::fvec4( // COLOUR
							(v0.COLOUR.x * w0) + (v1.COLOUR.x * w1), // X
							(v0.COLOUR.y * w0) + (v1.COLOUR.y * w1), // Y
							(v0.COLOUR.z * w0) + (v1.COLOUR.z * w1), // Z
							(v0.COLOUR.w * w0) + (v1.COLOUR.w * w1) // W
							)
					};
		if(perspectiveCorrect) out.TEXCOORD *= out.POS.w; // We're assuming that the TEXCOORD attribute of each vertex was divided by POS.z prior to calling this function
		return out;
	}
	inline static ColouredTexturedVertexOut interpolate(const ColouredTexturedVertexOut& v0, const ColouredTexturedVertexOut& v1, const ColouredTexturedVertexOut& v2, float w0, float w1, float w2, bool perspectiveCorrect) {
		ColouredTexturedVertexOut out = {
							glm::fvec4(  // POS
							(v0.POS.x * w0) + (v1.POS.x * w1) + (v2.POS.x * w2), // X
							(v0.POS.y * w0) + (v1.POS.y * w1) + (v2.POS.y * w2), // Y
							(v0.POS.z * w0) + (v1.POS.z * w1) + (v2.POS.z * w2), // Z
							(v0.POS.w * w0) + (v1.POS.w * w1) + (v2.POS.w * w2) // Z
							),
							glm::fvec2( // TEXCOORD
							(v0.TEXCOORD.x * w0) + (v1.TEXCOORD.x * w1) + (v2.TEXCOORD.x * w2), // X
							(v0.TEXCOORD.y * w0) + (v1.TEXCOORD.y * w1) + (v2.TEXCOORD.y * w2) // Y
							),
							glm::fvec4( // COLOUR
							(v0.COLOUR.x * w0) + (v1.COLOUR.x * w1) + (v2.COLOUR.x * w2), // X
							(v0.COLOUR.y * w0) + (v1.COLOUR.y * w1) + (v2.COLOUR.y * w2), // Y
							(v0.COLOUR.z * w0) + (v1.COLOUR.z * w1) + (v2.COLOUR.z * w2), // Z
							(v0.COLOUR.w * w0) + (v1.COLOUR.w * w1) + (v2.COLOUR.w * w2) // W
							)
					};
		if(perspectiveCorrect) out.TEXCOORD *= out.POS.w; // We're assuming that the TEXCOORD attribute of each vertex was divided by POS.z prior to calling this function
		return out;
	}
	void perspectiveCorrect() {
		TEXCOORD /= POS.z;
	}
};

typedef RenderingPipeline<ColouredTexturedVertexIn,ColouredTexturedVertexOut,ColouredTexturedUniform> ColouredTexturedPipeline;
ColouredTexturedVertexOut ColouredTexturedVertexShader(const ColouredTexturedUniform& uniform, const ColouredTexturedVertexIn& vertex);
void ColouredTexturedFragmentShader(Texture& framebuffer, const glm::ivec2& point, const ColouredTexturedUniform& uniform,
						 const ColouredTexturedVertexOut& v0);

}
#endif // COLOUREDTEXTUREDPIPELINE_HPP
