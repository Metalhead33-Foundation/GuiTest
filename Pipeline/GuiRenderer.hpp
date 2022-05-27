#ifndef GUIRENDERER_HPP
#define GUIRENDERER_HPP
#include "../Texture/Texture.hpp"
#include <glm/glm.hpp>
#include <span>
#include <memory>

class GuiRenderer {
public:
	virtual ~GuiRenderer() = default;
	virtual void renderCLine(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec4& colour, int thickness = 1) = 0;
	void renderCLines(const std::span<glm::fvec2>& points, const glm::fvec4& colour, int thickness = 1);
	virtual void renderCRect(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec4& colour) = 0;
	virtual void renderCTriang(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec2& p2, const glm::fvec4& colour) = 0;
	virtual void renderTex(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec2 t0, const glm::fvec2& t1, const Texture& tex) = 0;
	virtual void renderTex(const glm::fvec2& p0, const glm::fvec2& p1, const Texture& tex) = 0;
	virtual void renderTex(const Texture&) = 0;
};

#endif // GUIRENDERER_HPP
