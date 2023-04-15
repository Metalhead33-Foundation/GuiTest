#include <GL/GlVAO.hpp>
#include <stdexcept>
namespace GL {

std::recursive_mutex VaoEssentials::vaomutex;
typedef std::lock_guard<std::recursive_mutex> RecursiveLock;
GLuint VaoEssentials::lastBound = 0;
void VaoEssentials::ensureBound(GLuint id)
{
	if(lastBound != id) {
		glBindVertexArray(id);
		lastBound = id;
	}
}

GLuint Vao::getVaoId() const
{
	return vaoId;
}

VaoRef Vao::toRef() const
{
	return VaoRef(vaoId);
}

Vao::Vao()
{
	glGenVertexArrays(1,&vaoId);
}

Vao::~Vao()
{
	if(vaoId) glDeleteVertexArrays(1,&vaoId);
}

Vao::Vao(Vao&& mov)
	: vaoId(mov.vaoId)
{
	mov.vaoId = 0;
}

Vao& Vao::operator=(Vao&& mov)
{
	this->vaoId = mov.vaoId;
	mov.vaoId = 0;
	return *this;
}

VaoRef::VaoRef(GLuint vaoId)
	: vaoId(vaoId)
{

}

VaoRef::VaoRef(const VaoRef& cpy)
	: vaoId(cpy.vaoId)
{

}

VaoRef& VaoRef::operator=(const VaoRef& cpy)
{
	this->vaoId = cpy.vaoId;
	return *this;
}

VaoRef::~VaoRef()
{

}

GLuint VaoRef::getVaoId() const
{
	return vaoId;
}

void IVao::bind()
{
	VaoEssentials::ensureBound(getVaoId());
}

void IVao::enableVertexAttribArray(GLuint index)
{
	bind();
	glEnableVertexAttribArray(index);
}

void IVao::vertexAttribPointer(GLuint index, GLint size, GLenum type, bool normalized, GLsizei stride, uintptr_t offset)
{
	bind();
	glVertexAttribPointer(index,size,type,normalized,stride,reinterpret_cast<void*>(offset));
}

void IVao::vertexAttribPointer(GLuint index, MH33::GFX::PrimitiveType type, GLsizei stride, uintptr_t offset)
{
	const uint8_t dataFormat = static_cast<uint8_t>(type) & MH33::GFX::PrimitiveType::TYPE_MASK;
	const uint8_t dataSize = static_cast<uint8_t>(type) & MH33::GFX::PrimitiveType::SIZE_MASK;
	const bool isNormalized = (static_cast<uint8_t>(type) & MH33::GFX::PrimitiveType::IS_NORMALIZED) != 0;
	if(!dataSize) throw std::runtime_error("Invalid Vertex format!");
	bind();
	switch (dataFormat) {
		case MH33::GFX::PrimitiveType::TYPE_HALF_FLOAT: glVertexAttribPointer(index,dataSize,GL_HALF_FLOAT,isNormalized,stride,reinterpret_cast<void*>(offset)); break;
		case MH33::GFX::PrimitiveType::TYPE_FLOAT: glVertexAttribPointer(index,dataSize,GL_FLOAT,isNormalized,stride,reinterpret_cast<void*>(offset)); break;
		case MH33::GFX::PrimitiveType::TYPE_UBYTE: glVertexAttribPointer(index,dataSize,GL_UNSIGNED_BYTE,isNormalized,stride,reinterpret_cast<void*>(offset)); break;
		case MH33::GFX::PrimitiveType::TYPE_SBYTE: glVertexAttribPointer(index,dataSize,GL_BYTE,isNormalized,stride,reinterpret_cast<void*>(offset)); break;
		case MH33::GFX::PrimitiveType::TYPE_USHORT: glVertexAttribPointer(index,dataSize,GL_UNSIGNED_SHORT,isNormalized,stride,reinterpret_cast<void*>(offset)); break;
		case MH33::GFX::PrimitiveType::TYPE_SSHORT: glVertexAttribPointer(index,dataSize,GL_SHORT,isNormalized,stride,reinterpret_cast<void*>(offset)); break;
		case MH33::GFX::PrimitiveType::TYPE_UINT: glVertexAttribPointer(index,dataSize,GL_UNSIGNED_INT,isNormalized,stride,reinterpret_cast<void*>(offset)); break;
		case MH33::GFX::PrimitiveType::TYPE_SINT: glVertexAttribPointer(index,dataSize,GL_INT,isNormalized,stride,reinterpret_cast<void*>(offset)); break;
		default: throw std::runtime_error("Unsupported Vertex format!");
	}
}

void IVao::enableAttributes(const MH33::GFX::VertexDescriptor& descriptor)
{
	for(size_t i = 0; i < descriptor.descriptors.size(); ++i)
	{
		const auto& it = descriptor.descriptors[i];
		vertexAttribPointer(i,it.type,descriptor.stride,it.offset);
		glEnableVertexAttribArray(i);
	}
}

void IVao::doActions(const VaoAccessor& fun)
{
	RecursiveLock lock(VaoEssentials::vaomutex);
	VaoEssentials::ensureBound(getVaoId());
	fun(*this);
}

void IVao::doActions(const VaoConstAccessor& fun) const
{
	RecursiveLock lock(VaoEssentials::vaomutex);
	VaoEssentials::ensureBound(getVaoId());
	fun(*this);
}

void IVao::doActions(const VaoIdAccessor& fun) const
{
	RecursiveLock lock(VaoEssentials::vaomutex);
	VaoEssentials::ensureBound(getVaoId());
	fun(getVaoId());
}

void IVao::draw(GLenum mode, GLint first, GLsizei count)
{
	bind();
	glDrawArrays(mode,first,count);
}

}
