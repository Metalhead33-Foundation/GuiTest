#include "GlPipeline.hpp"
#include "GlTexture.hpp"
#include "GlMesh.hpp"
namespace GL {

Pipeline::Pipeline(const MH33::GFX::ShaderBinary &binaries)
{
	std::vector<GL::Shader> shaders;
	shaders.reserve(binaries.size());
	for(auto it = std::begin(binaries); it != std::end(binaries); ++it) {
		if(it->first == "vert") {
			GL::Shader shaderModule(GL_VERTEX_SHADER);
			shaderModule.source(reinterpret_cast<const GLchar*>(it->second.data()),it->second.size());
			shaders.push_back(std::move(shaderModule));
		}
		else if(it->first == "frag") {
			GL::Shader shaderModule(GL_FRAGMENT_SHADER);
			shaderModule.source(reinterpret_cast<const GLchar*>(it->second.data()),it->second.size());
			shaders.push_back(std::move(shaderModule));
		}
	}
	for(auto& it : shaders) {
		it.compile();
		shader_program.attachShader(it);
	}
	shader_program.link();
}

}


MH33::GFX::Handle GL::Pipeline::getNativeHandle()
{
	return { .oglUint = shader_program.getProgramId() };
}

MH33::GFX::ConstHandle GL::Pipeline::getNativeHandle() const
{
	return { .oglUint = shader_program.getProgramId() };
}

void GL::Pipeline::setTexture(int id, const MH33::GFX::Texture2D &texture)
{
	shader_program.uniform1i(id,dynamic_cast<const GL::Texture2D*>(&texture)->getNativeHandle().oglInt );
}

void GL::Pipeline::setTexture(int id, const MH33::GFX::Texture3D &texture)
{
}

void GL::Pipeline::setTexture(int id, const MH33::GFX::ArrayTexture2D &texture)
{
}

void GL::Pipeline::setUniformBuffer(int id, const MH33::GFX::UniformBuffer &texture)
{

}

int GL::Pipeline::getId(const std::string &uniformName)
{
	//shader_program.bind();
	return glGetUniformLocation(shader_program.getProgramId(), uniformName.c_str());
}

void GL::Pipeline::drawTriangles(MH33::GFX::UnindexedMesh &mesh)
{
	GL::UnindexedMesh* cmesh = dynamic_cast<GL::UnindexedMesh*>(&mesh);
	if(cmesh) {
		shader_program.bind();
		cmesh->getBuffer().bind();
		cmesh->getVao().bind();
		cmesh->getVao().draw(GL_ACTIVE_ATTRIBUTES,0,cmesh->getBuffer().getSize());
	}
}

void GL::Pipeline::drawTriangles(MH33::GFX::IndexedMesh &mesh)
{
}

void GL::Pipeline::drawLines(MH33::GFX::UnindexedMesh &mesh, float thickness)
{
}

void GL::Pipeline::drawLines(MH33::GFX::IndexedMesh &mesh, float thickness)
{
}
