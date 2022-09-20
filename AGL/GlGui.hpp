#ifndef GLGUI_H
#define GLGUI_H
#include <Pipeline/GuiRenderer.hpp>
#include <GL/GlBuffer.hpp>
#include <GL/GlVAO.hpp>
#include <GL/GlShader.hpp>
#include <GL/GlShaderProgram.hpp>

namespace GL {

class Gui : public SYS::GuiRenderer
{
private:
	Buffer quad;
	Buffer quadI;
	Vao quadVao;
	Buffer quadTex;
	Buffer quadTexI;
	Vao quadTexVao;
	Buffer triang;
	Vao triangVao;
	Buffer line;
	Vao lineVao;
	Shader justColourVert;
	Shader justColourFrag;
	Shader justTexVert;
	Shader justTexFrag;
	Shader clrPTexVert;
	Shader clrPTexFrag;
	ShaderProgram justColour;
	ShaderProgram justTex;
	ShaderProgram clrPTex;
	int justColourAttrib;
	int justTexAttrib;
	int clrPTexAttribTex;
	int clrPTexAttribColour;
public:
	Gui();
	~Gui() override;
	void renderCLine(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec4& colour, int thickness) override;
	void renderCRect(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec4& colour) override;
	void renderCTriang(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec2& p2, const glm::fvec4& colour) override;
	void renderTex(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec2 t0, const glm::fvec2& t1, const SYS::ITexture& tex) override;
	void renderTex(const glm::fvec2& p0, const glm::fvec2& p1, const SYS::ITexture& tex) override;
	void renderTex(const SYS::ITexture& tex) override;
	void renderCTex(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec2 t0, const glm::fvec2& t1, const glm::vec4& colour, const SYS::ITexture& tex) override;
	void renderCTex(const glm::fvec2& p0, const glm::fvec2& p1, const glm::vec4& colour, const SYS::ITexture& tex) override;
	void renderCTex(const glm::vec4& colour, const SYS::ITexture& tex) override;
	void renderTiltedCTex(float tilt, const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec2 t0, const glm::fvec2& t1, const glm::vec4& colour, const SYS::ITexture& tex) override;
};

}

#endif // GLGUI_H
