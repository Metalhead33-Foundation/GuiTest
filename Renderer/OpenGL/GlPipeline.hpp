#ifndef GLPIPELINE_HPP
#define GLPIPELINE_HPP
#include <MhLib/Media/GFX/MhPipeline.hpp>
#include <MhLib/Media/GFX/MhGFXResourceFactory.hpp>
#include <GL/GlShader.hpp>
#include <GL/GlShaderProgram.hpp>
#include <MhLib/Io/MhIoDevice.hpp>
namespace GL {

class Pipeline : public MH33::GFX::Pipeline
{
private:
	ShaderProgram shader_program;
public:
	Pipeline(const MH33::GFX::ShaderBinary& binaries);
	~Pipeline() = default;
	MH33::GFX::Handle getNativeHandle() override;
	MH33::GFX::ConstHandle getNativeHandle() const override;
	void setTexture(int id, const MH33::GFX::Texture2D &texture) override;
	void setTexture(int id, const MH33::GFX::Texture3D &texture) override;
	void setTexture(int id, const MH33::GFX::ArrayTexture2D &texture) override;
	void setUniformBuffer(int id, const MH33::GFX::UniformBuffer &texture) override;
	int getId(const std::string &uniformName) override;
	void drawTriangles(MH33::GFX::UnindexedMesh &mesh) override;
	void drawTriangles(MH33::GFX::IndexedMesh &mesh) override;
	void drawLines(MH33::GFX::UnindexedMesh &mesh, float thickness) override;
	void drawLines(MH33::GFX::IndexedMesh &mesh, float thickness) override;
};

}
#endif // GLPIPELINE_HPP
