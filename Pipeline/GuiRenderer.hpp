#ifndef GUIRENDERER_HPP
#define GUIRENDERER_HPP
#include "../Texture/Texture.hpp"
#include <glm/glm.hpp>
#include <span>
#include <memory>

class GuiRenderer {
public:
	virtual ~GuiRenderer() = default;
	virtual void renderCLine(const glm::vec2& p0, const glm::vec2& p1, const glm::vec4& colour) = 0;
	void renderCLines(const std::span<glm::vec2>& points, const glm::vec4& colour);
	virtual void renderCRect(const glm::vec2& p0, const glm::vec2& p1, const glm::vec4& colour) = 0;
	virtual void renderCTriang(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, const glm::vec4& colour) = 0;
	virtual void renderTex(const glm::vec2& p0, const glm::vec2& p1, const std::shared_ptr<Texture> tex) = 0;
};

#endif // GUIRENDERER_HPP
