#include "GlGui.hpp"
#include <glm/glm.hpp>
#include "GlTexture2D.hpp"
#include <GL/GlValidate.hpp>
#include <Util/Dither.hpp>
#include <Util/NormDenorm.hpp>

namespace GL {


struct CVert {
	glm::fvec2 pos;
	static const VertexDescriptor descriptor;
};
static const AttributeDescriptor CVertAttributes[] = {
	{ .SemanticName = "POS", .SemanticIndex = 0, .type = PrimitiveType::F32x2, .offset = offsetof(CVert,pos) }
};
const VertexDescriptor CVert::descriptor = {
	.stride = sizeof(CVert),
	.descriptors = CVertAttributes
};
struct CVertTex {
	glm::fvec2 pos;
	glm::fvec2 tex;
	static const VertexDescriptor descriptor;
};
static const AttributeDescriptor CVertTexAttributes[] = {
	{ .SemanticName = "POS", .SemanticIndex = 0, .type = PrimitiveType::F32x2, .offset = offsetof(CVertTex,pos) },
	{ .SemanticName = "TEX", .SemanticIndex = 0, .type = PrimitiveType::F32x2, .offset = offsetof(CVertTex,tex) }
};
const VertexDescriptor CVertTex::descriptor = {
	.stride = sizeof(CVertTex),
	.descriptors = CVertTexAttributes
};

static const unsigned quadIds[] = { 0, 1, 2, 1, 2, 3 };

static const std::string justColourVertexShader = "#version 300 es\n"
												  "precision highp float;\n"
												  "attribute vec2 position;\n"
												  "uniform vec2 win_size;\n"
												  "varying vec2 screen_coord;\n"
												  "void main()\n"
												  "{\n"
												  "screen_coord = ((vec2(position.x,position.y * -1.0) + 1.0)*0.5) * win_size;\n"
												  "    gl_Position = vec4(position,0.0,1.0);\n"
												  "}";
static const std::string justColourFragmentShader = "#version 300 es\n"
													"precision highp float;\n"
													"varying vec2 screen_coord;\n"
													"uniform vec4 colour;\n"
													"uniform sampler2D threshold_texture;\n"
													"void main()\n"
													"{\n"
													"ivec2 wincoords = ivec2( floor( mod(screen_coord,vec2(4.0)) ) );\n"
													"vec4 threshold = texelFetch(threshold_texture, wincoords, 0)\n;"
													"if(colour.a < threshold.r)\n"
													"{\n"
													"discard;\n"
													"}\n"
													"else gl_FragColor = vec4(colour.xyz,1.0);\n"
													"//gl_FragColor = vec4(threshold.r,threshold.r,threshold.r,1.0);\n"
													"//gl_FragColor = vec4(wincoords,0.0,1.0);\n"
													"}\n";

static const std::string justTexVertexShader = "#version 300 es\n"
											   "precision highp float;\n"
											   "attribute vec2 position;\n"
											   "attribute vec2 texture_coordinates;\n"
											   "uniform vec2 win_size;\n"
											   "varying vec2 screen_coord;\n"
											   "varying vec2 interpolated_texture_coordinates;\n"
											   "void main()\n"
											   "{\n"
											   "screen_coord = ((vec2(position.x,position.y * -1.0) + 1.0)*0.5) * win_size;\n"
											   "    interpolated_texture_coordinates = texture_coordinates;\n"
											   "    gl_Position = vec4(position,0.0,1.0);\n"
											   "}\n";

static const std::string justTexFragmentShader = "#version 300 es\n"
												 "precision highp float;\n"
												 "varying vec2 screen_coord;\n"
												 "varying vec2 interpolated_texture_coordinates;\n"
												 "uniform sampler2D threshold_texture;\n"
												 "uniform sampler2D texture_sampler;\n"
												 "void main()\n"
												 "{\n"
												 "vec4 texClr = texture2D(texture_sampler, interpolated_texture_coordinates);"
												 "ivec2 wincoords = ivec2( floor( mod(screen_coord,vec2(4.0)) ) );\n"
												 "vec4 threshold = texelFetch(threshold_texture, wincoords, 0);\n"
												 "if(texClr.a < threshold.r)\n"
												 "{\n"
												 "discard;\n"
												 "}\n"
												 "else gl_FragColor = vec4(texClr.xyz,1.0);\n"
												 "//else gl_FragColor = vec4(threshold.r,threshold.r,threshold.r,1.0);\n"
												 "}\n";

static const std::string clrPTexVertexShader = justTexVertexShader;
static const std::string clrPTexFragmentShader = "#version 300 es\n"
												 "precision highp float;\n"
												 "varying vec2 screen_coord;\n"
												 "varying vec2 interpolated_texture_coordinates;\n"
												 "uniform sampler2D threshold_texture;\n"
												 "uniform sampler2D texture_sampler;\n"
												 "uniform vec4 colour;\n"
												 "void main()\n"
												 "{\n"
												 "ivec2 wincoords = ivec2( floor( mod(screen_coord,vec2(4.0)) ) );\n"
												 "vec4 threshold = texelFetch(threshold_texture, wincoords, 0);\n"
												 "vec4 texclr = texture2D(texture_sampler, interpolated_texture_coordinates);\n"
												 "if(texclr.a < threshold.r)\n"
												 "{\n"
												 "discard;\n"
												 "}\n"
												 "else gl_FragColor = vec4(colour.rgb,1.0);\n"
												 "}\n";
/*
	Buffer quad;
	Vao quadVao;
	Buffer quadTex;
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
*/

/*
	int justColourAttrib;
	int justTexAttrib;
	int clrPTexAttribTex;
	int clrPTexAttribColour;
*/

const glm::vec2& Gui::getScreensize() const
{
	return screensize;
}

void Gui::setScreensize(const glm::vec2& newScreensize)
{
	screensize = newScreensize;
}

Gui::Gui(glm::vec2 screensize)
	: screensize(screensize),
	  quad(GL_ARRAY_BUFFER,GL_DYNAMIC_DRAW),
	  quadI(GL_ELEMENT_ARRAY_BUFFER,GL_STATIC_DRAW),
	  quadTex(GL_ARRAY_BUFFER,GL_DYNAMIC_DRAW),
	  quadTexI(GL_ELEMENT_ARRAY_BUFFER,GL_STATIC_DRAW),
	  triang(GL_ARRAY_BUFFER,GL_DYNAMIC_DRAW),
	  line(GL_ARRAY_BUFFER,GL_DYNAMIC_DRAW),
	  justColourVert(GL_VERTEX_SHADER),
	  justColourFrag(GL_FRAGMENT_SHADER),
	  justTexVert(GL_VERTEX_SHADER),
	  justTexFrag(GL_FRAGMENT_SHADER),
	  clrPTexVert(GL_VERTEX_SHADER),
	  clrPTexFrag(GL_FRAGMENT_SHADER),
	  threshold(GL_TEXTURE_2D)
{
	// Threshold
	{
		/*std::vector<uint8_t> thresholdVector;
		thresholdVector.reserve(16);
		for(int x = 0; x < 4; ++x)
		{
			for(int y = 0; y < 4; ++y)
			{
				thresholdVector.push_back(denormalize<half>(thresholdMatrix[x][y]));
			}
		}*/
		threshold.bind();
		threshold.pixelStorei(GL_UNPACK_ALIGNMENT,1);
		threshold.pixelStorei(GL_PACK_ALIGNMENT,1);
		threshold.image2D(0, GL_RED,4,4,0, GL_RED, GL_FLOAT,&thresholdMatrix);
		//threshold.image2D(0, GL_ALPHA,4,4,0, GL_ALPHA, GL_UNSIGNED_BYTE,thresholdVector.data());
		threshold.setWrapS(GL_REPEAT);
		threshold.setWrapT(GL_REPEAT);
		threshold.setMinFilter(GL_NEAREST);
		threshold.setMagFilter(GL_NEAREST);
		Validate::validate();
	}
	// Shaders
	justColourVert.source(justColourVertexShader);
	justColourVert.compile();
	justColourFrag.source(justColourFragmentShader);
	justColourFrag.compile();
	justColour.attachShader(justColourVert);
	justColour.attachShader(justColourFrag);
	justColour.link();
	GL::Validate();
	thresholdCol = justColour.getUniformLocation("threshold_texture");
	justColourAttrib = justColour.getUniformLocation("colour");
	screenSizeCol = justColour.getUniformLocation("win_size");
	justTexVert.source(justTexVertexShader);
	justTexVert.compile();
	justTexFrag.source(justTexFragmentShader);
	justTexFrag.compile();
	justTex.attachShader(justTexVert);
	justTex.attachShader(justTexFrag);
	justTex.link();
	GL::Validate();
	threshHoldTex = justTex.getUniformLocation("threshold_texture");
	justTexAttrib = justTex.getUniformLocation("texture_sampler");
	screenSizeTex = justTex.getUniformLocation("win_size");
	clrPTexVert.source(clrPTexVertexShader);
	clrPTexVert.compile();
	clrPTexFrag.source(clrPTexFragmentShader);
	clrPTexFrag.compile();
	clrPTex.attachShader(clrPTexVert);
	clrPTex.attachShader(clrPTexFrag);
	clrPTex.link();
	GL::Validate();
	threshHoldColTEx = clrPTex.getUniformLocation("threshold_texture");
	clrPTexAttribTex = clrPTex.getUniformLocation("texture_sampler");
	clrPTexAttribColour = clrPTex.getUniformLocation("colour");
	screenSizeColTex = clrPTex.getUniformLocation("win_size");
	// Regular quad
	quad.bind();
	quad.bufferData(nullptr,sizeof(CVert)*4);
	quadI.bind();
	quadI.bufferData(quadIds,sizeof(quadIds));
	quadVao.bind();
	quadVao.enableAttributes(CVert::descriptor);
	GL::Validate();
	// Textured quad
	quadTex.bind();
	quadTex.bufferData(nullptr,sizeof(CVertTex)*4);
	quadTexI.bind();
	quadTexI.bufferData(quadIds,sizeof(quadIds));
	quadTexVao.bind();
	quadTexVao.enableAttributes(CVertTex::descriptor);
	GL::Validate();
	// Triang
	triang.bind();
	triang.bufferData(nullptr,sizeof(CVert)*3);
	triangVao.bind();
	triangVao.enableAttributes(CVert::descriptor);
	GL::Validate();
	// Line
	line.bind();
	line.bufferData(nullptr,sizeof(CVert)*2);
	lineVao.bind();
	lineVao.enableAttributes(CVert::descriptor);
	GL::Validate();
}

Gui::~Gui()
{

}

void Gui::renderCLine(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec4& colour, int thickness)
{
	const CVert tmpVert[2] = { { glm::fvec2(p0.x, -1.0f * p0.y) }, { glm::fvec2(p1.x, -1.0f * p1.y) } };
	line.bufferSubData(0,tmpVert,sizeof(tmpVert));
	justColour.bind();
	justColour.uniform4f(justColourAttrib,colour);
	glLineWidth(static_cast<float>(thickness));
	lineVao.draw(GL_LINES,0,2);
}

void Gui::renderCRect(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec4& colour)
{
	const CVert tmpVert[4] = {
		{ glm::fvec2(std::min(p0.x,p1.x),-1.0f * std::min(p0.y,p1.y)) },
		{ glm::fvec2(std::max(p0.x,p1.x),-1.0f * std::min(p0.y,p1.y)) },
		{ glm::fvec2(std::min(p0.x,p1.x),-1.0f * std::max(p0.y,p1.y)) },
		{ glm::fvec2(std::max(p0.x,p1.x),-1.0f * std::max(p0.y,p1.y)) }
	};
	quad.bufferSubData(0,tmpVert,sizeof(tmpVert));
	justColour.bind();
	justColour.uniform2f(screenSizeCol,screensize);
	justColour.uniform4f(justColourAttrib,colour);
	threshold.activate(0);
	threshold.bind();
	justColour.uniform1i(thresholdCol,0);
	quadTexVao.bind();
	quadVao.bind();
	quadI.draw(GL_TRIANGLES,6,GL_UNSIGNED_INT,nullptr);
}

void Gui::renderCTriang(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec2& p2, const glm::fvec4& colour)
{
	const CVert tmpVert[3] = {
		{ glm::fvec2(p0.x, -1.0f * p0.y) },
		{ glm::fvec2(p1.x, -1.0f * p1.y) },
		{ glm::fvec2(p2.x, -1.0f * p2.y) }
	};
	triang.bufferSubData(0,tmpVert,sizeof(tmpVert));
	justColour.bind();
	justColour.uniform2f(screenSizeCol,screensize);
	justColour.uniform4f(justColourAttrib,colour);
	threshold.activate(0);
	threshold.bind();
	justColour.uniform1i(thresholdCol,0);
	triangVao.draw(GL_TRIANGLES,0,3);
}

void Gui::renderTex(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec2 t0, const glm::fvec2& t1, const SYS::ITexture& tex)
{
	const AcceleratedTexture* accelTex = dynamic_cast<const AcceleratedTexture*>(&tex);
	if(!accelTex) return;
	const CVertTex tmpVert[4] = {
		{ glm::fvec2(std::min(p0.x,p1.x),-1.0f * std::min(p0.y,p1.y)), glm::fvec2(std::min(t0.x,t1.x),std::min(t0.y,t1.y)) },
		{ glm::fvec2(std::max(p0.x,p1.x),-1.0f * std::min(p0.y,p1.y)), glm::fvec2(std::max(t0.x,t1.x),std::min(t0.y,t1.y)) },
		{ glm::fvec2(std::min(p0.x,p1.x),-1.0f * std::max(p0.y,p1.y)), glm::fvec2(std::min(t0.x,t1.x),std::max(t0.y,t1.y)) },
		{ glm::fvec2(std::max(p0.x,p1.x),-1.0f * std::max(p0.y,p1.y)), glm::fvec2(std::max(t0.x,t1.x),std::max(t0.y,t1.y)) }
	};
	quadTex.bufferSubData(0,tmpVert,sizeof(tmpVert));
	justTex.bind();
	justTex.uniform2f(screenSizeTex,screensize);
	threshold.activate(0);
	threshold.bind();
	accelTex->getTex().activate(1);
	accelTex->getTex().bind();
	justTex.uniform1i(threshHoldTex,0);
	justTex.uniform1i(justTexAttrib,0);
	quadTexVao.bind();
	quadTexI.draw(GL_TRIANGLES,6,GL_UNSIGNED_INT,nullptr);
}

void Gui::renderTex(const glm::fvec2& p0, const glm::fvec2& p1, const SYS::ITexture& tex)
{
	const AcceleratedTexture* accelTex = dynamic_cast<const AcceleratedTexture*>(&tex);
	if(!accelTex) return;
	const CVertTex tmpVert[4] = {
		{ glm::fvec2(std::min(p0.x,p1.x),-1.0f * std::min(p0.y,p1.y)), glm::fvec2(0.0f, 0.0f) },
		{ glm::fvec2(std::max(p0.x,p1.x),-1.0f * std::min(p0.y,p1.y)), glm::fvec2(1.0f, 0.0f) },
		{ glm::fvec2(std::min(p0.x,p1.x),-1.0f * std::max(p0.y,p1.y)), glm::fvec2(0.0f, 1.0f) },
		{ glm::fvec2(std::max(p0.x,p1.x),-1.0f * std::max(p0.y,p1.y)), glm::fvec2(1.0f, 1.0f) }
	};
	quadTex.bufferSubData(0,tmpVert,sizeof(tmpVert));
	justTex.bind();
	justTex.uniform2f(screenSizeTex,screensize);
	threshold.activate(0);
	threshold.bind();
	accelTex->getTex().activate(1);
	accelTex->getTex().bind();
	justTex.uniform1i(threshHoldTex,0);
	justTex.uniform1i(justTexAttrib,1);
	quadTexVao.bind();
	quadTexI.draw(GL_TRIANGLES,6,GL_UNSIGNED_INT,nullptr);
}

void Gui::renderTex(const SYS::ITexture& tex)
{
	const AcceleratedTexture* accelTex = dynamic_cast<const AcceleratedTexture*>(&tex);
	if(!accelTex) return;
	const CVertTex tmpVert[4] = {
		{ glm::fvec2(-1.0f, -1.0f), glm::fvec2(0.0f, 0.0f) },
		{ glm::fvec2(1.0f, -1.0f), glm::fvec2(1.0f, 0.0f) },
		{ glm::fvec2(-1.0f, 1.0f), glm::fvec2(0.0f, 1.0f) },
		{ glm::fvec2(1.0f, 1.0f), glm::fvec2(1.0f, 1.0f) }
	};
	quadTex.bufferSubData(0,tmpVert,sizeof(tmpVert));
	justTex.bind();
	justTex.uniform2f(screenSizeTex,screensize);
	threshold.activate(0);
	threshold.bind();
	accelTex->getTex().activate(1);
	accelTex->getTex().bind();
	justTex.uniform1i(threshHoldTex,0);
	justTex.uniform1i(justTexAttrib,1);
	quadTexVao.bind();
	quadTexI.draw(GL_TRIANGLES,6,GL_UNSIGNED_INT,nullptr);
}

void Gui::renderCTex(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec2 t0, const glm::fvec2& t1, const glm::vec4& colour, const SYS::ITexture& tex)
{
	const AcceleratedTexture* accelTex = dynamic_cast<const AcceleratedTexture*>(&tex);
	if(!accelTex) return;
	const CVertTex tmpVert[4] = {
		{ glm::fvec2(std::min(p0.x,p1.x),-1.0f * std::min(p0.y,p1.y)), glm::fvec2(std::min(t0.x,t1.x),std::min(t0.y,t1.y)) },
		{ glm::fvec2(std::max(p0.x,p1.x),-1.0f * std::min(p0.y,p1.y)), glm::fvec2(std::max(t0.x,t1.x),std::min(t0.y,t1.y)) },
		{ glm::fvec2(std::min(p0.x,p1.x),-1.0f * std::max(p0.y,p1.y)), glm::fvec2(std::min(t0.x,t1.x),std::max(t0.y,t1.y)) },
		{ glm::fvec2(std::max(p0.x,p1.x),-1.0f * std::max(p0.y,p1.y)), glm::fvec2(std::max(t0.x,t1.x),std::max(t0.y,t1.y)) }
	};
	quadTex.bufferSubData(0,tmpVert,sizeof(tmpVert));
	clrPTex.bind();
	clrPTex.uniform2f(screenSizeColTex,screensize);
	clrPTex.uniform4f(clrPTexAttribColour,colour);
	threshold.activate(0);
	threshold.bind();
	accelTex->getTex().activate(1);
	accelTex->getTex().bind();
	clrPTex.uniform1i(threshHoldColTEx,0);
	clrPTex.uniform1i(clrPTexAttribTex,1);
	quadTexVao.bind();
	quadTexI.draw(GL_TRIANGLES,6,GL_UNSIGNED_INT,nullptr);
}

void Gui::renderCTex(const glm::fvec2& p0, const glm::fvec2& p1, const glm::vec4& colour, const SYS::ITexture& tex)
{
	const AcceleratedTexture* accelTex = dynamic_cast<const AcceleratedTexture*>(&tex);
	if(!accelTex) return;
	const CVertTex tmpVert[4] = {
		{ glm::fvec2(std::min(p0.x,p1.x),-1.0f * std::min(p0.y,p1.y)), glm::fvec2(0.0f, 0.0f) },
		{ glm::fvec2(std::max(p0.x,p1.x),-1.0f * std::min(p0.y,p1.y)), glm::fvec2(1.0f, 0.0f) },
		{ glm::fvec2(std::min(p0.x,p1.x),-1.0f * std::max(p0.y,p1.y)), glm::fvec2(0.0f, 1.0f) },
		{ glm::fvec2(std::max(p0.x,p1.x),-1.0f * std::max(p0.y,p1.y)), glm::fvec2(1.0f, 1.0f) }
	};
	quadTex.bufferSubData(0,tmpVert,sizeof(tmpVert));
	clrPTex.bind();
	clrPTex.uniform2f(screenSizeColTex,screensize);
	clrPTex.uniform4f(clrPTexAttribColour,colour);
	threshold.activate(0);
	threshold.bind();
	accelTex->getTex().activate(1);
	accelTex->getTex().bind();
	clrPTex.uniform1i(threshHoldColTEx,0);
	clrPTex.uniform1i(clrPTexAttribTex,1);
	quadTexVao.bind();
	quadTexI.draw(GL_TRIANGLES,6,GL_UNSIGNED_INT,nullptr);
}

void Gui::renderCTex(const glm::vec4& colour, const SYS::ITexture& tex)
{
	const AcceleratedTexture* accelTex = dynamic_cast<const AcceleratedTexture*>(&tex);
	if(!accelTex) return;
	const CVertTex tmpVert[4] = {
		{ glm::fvec2(-1.0f, -1.0f), glm::fvec2(0.0f, 0.0f) },
		{ glm::fvec2(1.0f, -1.0f), glm::fvec2(1.0f, 0.0f) },
		{ glm::fvec2(-1.0f, 1.0f), glm::fvec2(0.0f, 1.0f) },
		{ glm::fvec2(1.0f, 1.0f), glm::fvec2(1.0f, 1.0f) }
	};
	quadTex.bufferSubData(0,tmpVert,sizeof(tmpVert));
	clrPTex.bind();
	clrPTex.uniform2f(screenSizeColTex,screensize);
	clrPTex.uniform4f(clrPTexAttribColour,colour);
	threshold.activate(0);
	threshold.bind();
	accelTex->getTex().activate(1);
	accelTex->getTex().bind();
	clrPTex.uniform1i(threshHoldColTEx,0);
	clrPTex.uniform1i(clrPTexAttribTex,1);
	quadTexVao.bind();
	quadTexI.draw(GL_TRIANGLES,6,GL_UNSIGNED_INT,nullptr);
}

void Gui::renderTiltedCTex(float tilt, const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec2 t0, const glm::fvec2& t1, const glm::vec4& colour, const SYS::ITexture& tex)
{
	const AcceleratedTexture* accelTex = dynamic_cast<const AcceleratedTexture*>(&tex);
	if(!accelTex) return;
	const float xdiff = std::abs(std::max(p0.y,p1.y) - std::min(p0.y,p1.y)) * tilt;
	const CVertTex tmpVert[4] = {
		{ glm::fvec2(std::min(p0.x,p1.x)+xdiff,-1.0f * std::min(p0.y,p1.y)), glm::fvec2(std::min(t0.x,t1.x),std::min(t0.y,t1.y))  },
		{ glm::fvec2(std::max(p0.x,p1.x)+xdiff,-1.0f * std::min(p0.y,p1.y)), glm::fvec2(std::max(t0.x,t1.x),std::min(t0.y,t1.y))  },
		{ glm::fvec2(std::min(p0.x,p1.x),-1.0f * std::max(p0.y,p1.y)), glm::fvec2(std::min(t0.x,t1.x),std::max(t0.y,t1.y))  },
		{ glm::fvec2(std::max(p0.x,p1.x),-1.0f * std::max(p0.y,p1.y)), glm::fvec2(std::max(t0.x,t1.x),std::max(t0.y,t1.y))  }
	};
	quadTex.bufferSubData(0,tmpVert,sizeof(tmpVert));
	clrPTex.bind();
	clrPTex.uniform2f(screenSizeColTex,screensize);
	clrPTex.uniform4f(clrPTexAttribColour,colour);
	threshold.activate(0);
	threshold.bind();
	accelTex->getTex().activate(1);
	accelTex->getTex().bind();
	clrPTex.uniform1i(threshHoldColTEx,0);
	clrPTex.uniform1i(clrPTexAttribTex,1);
	quadTexVao.bind();
	quadTexI.draw(GL_TRIANGLES,6,GL_UNSIGNED_INT,nullptr);
}

}
