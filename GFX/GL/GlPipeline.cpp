#include "GlPipeline.hpp"
#include "GlShaderModule.hpp"
//#include <MhLib/Util/MhStaticStackAllocator.hpp>
#include <MhLib/Util/MhStackAllocator.hpp>
/*typedef MH33::Util::StaticStackAllocator<GL::ShaderModule,16,1> GlShaderModuleAllocator;
template<> size_t GlShaderModuleAllocator::offset = 0;
template<> std::array<GL::ShaderModule, 16> GlShaderModuleAllocator::buffer = {};*/
typedef MH33::Util::StackAllocator<GL::ShaderModule, 16> GlShaderModuleAllocator;

namespace GL {


Pipeline::Pipeline(const std::span<const MH33::GFX::ShaderModuleCreateInfo>& createInfo)
	: shaderProgram(0)
{
	if(!createInfo.size()) throw std::runtime_error("No shader source files!");
	if(createInfo.size() > 16) throw std::runtime_error("Too many shader stages!");
	std::vector<ShaderModule, GlShaderModuleAllocator> modules;
	for(const auto& it : createInfo) {
		modules.push_back(ShaderModule(it));
	}
	shaderProgram = glCreateProgram();
	GLint success;
	for(const auto& it : modules) {
		glAttachShader(shaderProgram, it.getId());
	}
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if(!success) {
		throw ShaderError(shaderProgram, true);
	}
}

Pipeline::Pipeline(const std::span<const MH33::GFX::ShaderModuleCreateInfoRef>& createInfo)
	: shaderProgram(0)
{
	if(!createInfo.size()) throw std::runtime_error("No shader source files!");
	if(createInfo.size() > 16) throw std::runtime_error("Too many shader stages!");
	std::vector<ShaderModule, GlShaderModuleAllocator> modules;
	for(const auto& it : createInfo) {
		modules.push_back(ShaderModule(it));
	}
	shaderProgram = glCreateProgram();
	GLint success;
	for(const auto& it : modules) {
		glAttachShader(shaderProgram, it.getId());
	}
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if(!success) {
		throw ShaderError(shaderProgram, true);
	}
}

Pipeline::~Pipeline()
{
	if(shaderProgram) glDeleteProgram(shaderProgram);
}
MH33::GFX::Handle Pipeline::getNativeHandle()
{
	return { .oglUint = shaderProgram };
}

MH33::GFX::ConstHandle Pipeline::getNativeHandle() const
{
	return { .oglUint = shaderProgram };
}

void Pipeline::bind() const
{
	glUseProgram(shaderProgram);
}

unsigned Pipeline::getBindingPoint(const std::string& uniformName) const
{
	return glGetUniformLocation(shaderProgram, uniformName.c_str());
}

unsigned Pipeline::getUniformBlockIndex(const std::string& uniformName) const
{
	return glGetUniformBlockIndex(shaderProgram, uniformName.c_str());
}

void Pipeline::setUniform(unsigned bindingPoint, MH33::GFX::StorageBuffer& buffer, uint8_t unit) const
{
	glUniformBlockBinding(shaderProgram, bindingPoint, unit);
}

void Pipeline::setUniform(unsigned bindingPoint, MH33::GFX::Texture2D& texture, uint8_t unit) const
{
	texture.bind(unit);
	glUniform1i(bindingPoint, unit);
}

void Pipeline::setUniform(unsigned bindingPoint, MH33::GFX::TextureArray2D& texture, uint8_t unit) const
{
	texture.bind(unit);
	glUniform1i(bindingPoint, unit);
}

void Pipeline::setUniform(unsigned bindingPoint, MH33::GFX::Texture3D& texture, uint8_t unit) const
{
	texture.bind(unit);
	glUniform1i(bindingPoint, unit);
}

void Pipeline::setUniform(unsigned bindingPoint, MH33::GFX::Cubemap& texture, uint8_t unit) const
{
	texture.bind(unit);
	glUniform1i(bindingPoint, unit);
}

constexpr GLenum translateGeometryType(MH33::GFX::RenderType geometryType) {
	switch (geometryType) {
		case MH33::GFX::RenderType::POINTS:
			return GL_POINTS;
		case MH33::GFX::RenderType::LINES:
			return GL_LINES;
		case MH33::GFX::RenderType::LINES_ADJACENCY:
			return GL_LINES_ADJACENCY;
		case MH33::GFX::RenderType::LINE_STRIPS:
			return GL_LINE_STRIP;
		case MH33::GFX::RenderType::LINE_STRIP_ADJACENCY:
			return GL_LINE_STRIP_ADJACENCY;
		case MH33::GFX::RenderType::TRIANGLES:
			return GL_TRIANGLES;
		case MH33::GFX::RenderType::TRIANGLES_ADJACENCY:
			return GL_TRIANGLES_ADJACENCY;
		case MH33::GFX::RenderType::TRIANGLE_STRIP:
			return GL_TRIANGLE_STRIP;
		case MH33::GFX::RenderType::TRIANGLE_STRIP_ADJACENCY:
			return GL_TRIANGLE_STRIP_ADJACENCY;
		case MH33::GFX::RenderType::TRIANGLE_FAN:
			return GL_TRIANGLE_FAN;
		case MH33::GFX::RenderType::PATCHES:
			return GL_INVALID_ENUM;
		default: return GL_INVALID_ENUM;

	}
}

void Pipeline::draw(MH33::GFX::UnindexedVertexBuffer& vertices, MH33::GFX::RenderType geometryType, size_t offset, size_t count)
{
	vertices.bind();
	glDrawArrays( translateGeometryType(geometryType), offset, count);
}

void Pipeline::draw(MH33::GFX::IndexedVertexBuffer& vertices, MH33::GFX::RenderType geometryType, size_t offset, size_t count)
{
	GLuint mode = translateGeometryType(geometryType);
	if(mode == GL_INVALID_ENUM) return;
	vertices.bindData();
	vertices.bindIndices();
	if(offset) {
		if(count) glDrawRangeElements(mode, offset, offset + count, count, GL_UNSIGNED_INT, nullptr);
		else {
			count = vertices.getIndexCount() - offset;
			if(count) glDrawRangeElements(mode, offset, offset + count, count, GL_UNSIGNED_INT, nullptr);
		}
	} else {
		if(count) glDrawElements(mode, count,  GL_UNSIGNED_INT, nullptr);
		else glDrawElements(mode, vertices.getIndexCount(),  GL_UNSIGNED_INT, nullptr);
	}
}

void Pipeline::drawInstanced(uint32_t instances, MH33::GFX::UnindexedVertexBuffer& vertices, MH33::GFX::RenderType geometryType, size_t offset, size_t count)
{
	vertices.bind();
	glDrawArraysInstanced( translateGeometryType(geometryType), offset, count, instances);
}

void Pipeline::drawInstanced(uint32_t instances, MH33::GFX::IndexedVertexBuffer& vertices, MH33::GFX::RenderType geometryType, size_t count)
{
	GLuint mode = translateGeometryType(geometryType);
	if(mode == GL_INVALID_ENUM) return;
	vertices.bindData();
	vertices.bindIndices();
	if(count) glDrawElementsInstanced(mode, count, GL_UNSIGNED_INT, nullptr, instances);
	else {
		if(count) glDrawElementsInstanced(mode, vertices.getIndexCount(), GL_UNSIGNED_INT, nullptr, instances);
	}
}

}
