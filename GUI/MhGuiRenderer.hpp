#ifndef MHGUIRENDERER_HPP
#define MHGUIRENDERER_HPP
#include <glm/glm.hpp>
#include <span>
namespace MH33 {
namespace GUI {

struct ColouredQuad {
	glm::fvec2 topLeft;
	glm::fvec2 bottomRight;
	glm::fvec4 clr;
};

struct GradientQuad {
	glm::fvec2 topLeft;
	glm::fvec2 bottomRight;
	glm::fvec4 clr[4];
};

struct ColouredTriangle {
	glm::fvec2 pos[3];
	glm::fvec4 clr;
};

struct GradientTriangle {
	glm::fvec2 pos[3];
	glm::fvec4 clr[3];
};

struct ColouredLine {
	glm::fvec2 pos[2];
	glm::fvec4 clr;
};

struct GradientLine {
	glm::fvec2 pos[2];
	glm::fvec4 clr[2];
};

struct TexturedQuad {
	glm::fvec2 topLeft;
	glm::fvec2 bottomRight;
	glm::fvec2 tcoord[2];
	uintptr_t texturePtr;
};

struct ColouredTexturedQuad {
	glm::fvec2 topLeft;
	glm::fvec2 bottomRight;
	glm::fvec2 tcoord[2];
	uintptr_t texturePtr;
	glm::fvec4 clr;
};

struct GradientTexturedQuad {
	glm::fvec2 topLeft;
	glm::fvec2 bottomRight;
	glm::fvec2 tcoord[2];
	uintptr_t texturePtr;
	glm::fvec4 clr[4];
};

class Renderer
{
public:
	virtual ~Renderer() = default;
	// Pure virtual
	virtual void render(const ColouredQuad& target) = 0;
	virtual void render(const GradientQuad& target) = 0;
	virtual void render(const ColouredTriangle& target) = 0;
	virtual void render(const GradientTriangle& target) = 0;
	virtual void render(const ColouredLine& target) = 0;
	virtual void render(const GradientLine& target) = 0;
	virtual void render(const TexturedQuad& target) = 0;
	virtual void render(const ColouredTexturedQuad& target) = 0;
	virtual void render(const GradientTexturedQuad& target) = 0;
	// Regular virtual
	virtual void render(const std::span<const ColouredQuad>& target);
	virtual void render(const std::span<const GradientQuad>& target);
	virtual void render(const std::span<const ColouredTriangle>& target);
	virtual void render(const std::span<const GradientTriangle>& target);
	virtual void render(const std::span<const ColouredLine>& target);
	virtual void render(const std::span<const GradientLine>& target);
	virtual void render(const std::span<const TexturedQuad>& target);
	virtual void render(const std::span<const ColouredTexturedQuad>& target);
	virtual void render(const std::span<const GradientTexturedQuad>& target);
	// End Frame?
	virtual void flush() = 0;
};
}
}

#endif // MHGUIRENDERER_HPP
