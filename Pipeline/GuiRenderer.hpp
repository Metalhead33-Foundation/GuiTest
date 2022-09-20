#ifndef GUIRENDERER_H
#define GUIRENDERER_H
#include <Pipeline/ITexture.hpp>
#include <glm/glm.hpp>
#include <span>
#include <memory>

namespace SYS {
class GuiRenderer {
public:
	virtual ~GuiRenderer() = default;
	virtual void renderCLine(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec4& colour, int thickness = 1) = 0;
	virtual void renderCLines(const std::span<glm::fvec2>& points, const glm::fvec4& colour, int thickness = 1);
	virtual void renderCRect(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec4& colour) = 0;
	virtual void renderCTriang(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec2& p2, const glm::fvec4& colour) = 0;
	virtual void renderTex(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec2 t0, const glm::fvec2& t1, const ITexture& tex) = 0;
	virtual void renderTex(const glm::fvec2& p0, const glm::fvec2& p1, const ITexture& tex) = 0;
	virtual void renderTex(const ITexture&) = 0;
	virtual void renderCTex(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec2 t0, const glm::fvec2& t1, const glm::vec4& colour, const ITexture& tex) = 0;
	virtual void renderCTex(const glm::fvec2& p0, const glm::fvec2& p1, const glm::vec4& colour, const ITexture& tex) = 0;
	virtual void renderCTex(const glm::vec4& colour, const ITexture& tex) = 0;
	virtual void renderTiltedCTex(float tilt, const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec2 t0, const glm::fvec2& t1, const glm::vec4& colour, const ITexture& tex) = 0;
};
}

#endif // GUIRENDERER_H
