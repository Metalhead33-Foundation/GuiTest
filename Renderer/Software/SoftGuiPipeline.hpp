#ifndef SOFTGUIPIPELINE_HPP
#define SOFTGUIPIPELINE_HPP
#include <Renderer/Shared/GuiVertexTypes.hpp>
#include <Renderer/Software/RenderingPipeline.hpp>
#include <MhLib/Media/GFX/MhPipeline.hpp>
namespace Renderer {
namespace Software {

struct BasicVertOut {
	glm::fvec4 POS;
	inline static BasicVertOut split(const BasicVertOut& t, const BasicVertOut& m, const BasicVertOut& b, float dy, float iy) {
		return { glm::fvec4(
						t.POS.x + ((b.POS.x - t.POS.x) / dy) * iy,
						m.POS.y,
						t.POS.z + ((b.POS.z - t.POS.z) / dy) * iy,
						t.POS.w + ((b.POS.w - t.POS.w) / dy) * iy
						) };
	}
	inline static BasicVertOut interpolate(const BasicVertOut& v0, const BasicVertOut& v1, float w0, float w1, bool perspectiveCorrect) {
		BasicVertOut out = {
							glm::fvec4(  // POS
							(v0.POS.x * w0) + (v1.POS.x * w1), // X
							(v0.POS.y * w0) + (v1.POS.y * w1), // Y
							(v0.POS.z * w0) + (v1.POS.z * w1), // Z
							(v0.POS.w * w0) + (v1.POS.w * w1) // Z
							)
					};
		return out;
	}
	inline static BasicVertOut interpolate2D(const BasicVertOut& v0, const BasicVertOut& v1, float rowWeight, float columnWeight, bool perspectiveCorrect) {
		const float w1 = rowWeight * columnWeight;
		const float w0 = 1.0f - w1;
		BasicVertOut out = {
							glm::fvec4(  // POS
							(v0.POS.x * (1.0f - columnWeight)) + (v1.POS.x * columnWeight), // X
							(v0.POS.y * (1.0f - rowWeight)) + (v1.POS.y * rowWeight), // Y
							(v0.POS.z * w0) + (v1.POS.z * w1), // Z
							(v0.POS.w * w0) + (v1.POS.w * w1) // W
							)
					};
		return out;
	}
	inline static BasicVertOut interpolate(const BasicVertOut& v0, const BasicVertOut& v1, const BasicVertOut& v2, float w0, float w1, float w2, bool perspectiveCorrect) {
		BasicVertOut out = {
							glm::fvec4(  // POS
							(v0.POS.x * w0) + (v1.POS.x * w1) + (v2.POS.x * w2), // X
							(v0.POS.y * w0) + (v1.POS.y * w1) + (v2.POS.y * w2), // Y
							(v0.POS.z * w0) + (v1.POS.z * w1) + (v2.POS.z * w2), // Z
							(v0.POS.w * w0) + (v1.POS.w * w1) + (v2.POS.w * w2) // Z
							)
					};
		return out;
	}
	void perspectiveCorrect() {
		void();
	}
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

struct STD140 ColouredWidgetUniform {
	glm::fvec4 colour;
};
// template<typename VertexInType, typename VertexOutType, typename UniformType>

class ColouredWidgetPipeline : public Pipeline<ColouredWidgetVert,BasicVertOut,ColouredWidgetUniform> {
public:
	ColouredWidgetPipeline();
	void setTexture(int id, const MH33::GFX::Texture2D& texture) override;
	void setTexture(int id, const MH33::GFX::Texture3D& texture) override;
	void setTexture(int id, const MH33::GFX::ArrayTexture2D& texture) override;
	void setUniformBuffer(int id, const MH33::GFX::UniformBuffer& texture) override;
	int getId(const std::string& uniformName) override;
};
class TexturedWidgetPipeline : public Pipeline<TexturedWidgetVert,TexturedVertexOut,int> {
public:
	TexturedWidgetPipeline();
	void setTexture(int id, const MH33::GFX::Texture2D& texture) override;
	void setTexture(int id, const MH33::GFX::Texture3D& texture) override;
	void setTexture(int id, const MH33::GFX::ArrayTexture2D& texture) override;
	void setUniformBuffer(int id, const MH33::GFX::UniformBuffer& texture) override;
	int getId(const std::string& uniformName) override;
};
class ColouredTexturedWidgetPipeline : public Pipeline<TexturedWidgetVert,TexturedVertexOut,ColouredWidgetUniform> {
public:
	ColouredTexturedWidgetPipeline(bool rAsAlpha = false);
	void setTexture(int id, const MH33::GFX::Texture2D& texture) override;
	void setTexture(int id, const MH33::GFX::Texture3D& texture) override;
	void setTexture(int id, const MH33::GFX::ArrayTexture2D& texture) override;
	void setUniformBuffer(int id, const MH33::GFX::UniformBuffer& texture) override;
	int getId(const std::string& uniformName) override;
};

}
}
#endif // SOFTGUIPIPELINE_HPP
