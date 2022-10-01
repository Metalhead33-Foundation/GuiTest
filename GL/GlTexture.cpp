#include <GL/GlTexture.hpp>
#include <GL/GlValidate.hpp>
namespace GL {

std::recursive_mutex TextureEssentials::texturemutex;
typedef std::lock_guard<std::recursive_mutex> RecursiveLock;
GLuint TextureEssentials::lastBound = 0;
void TextureEssentials::ensureBound(GLuint id, GLenum target)
{
	if(lastBound != id) {
		glBindTexture(target,id);
		lastBound = id;
	}
}

void TextureEssentials::forceBound(GLuint id, GLenum target)
{
	glBindTexture(target,id);
	lastBound = id;
}

void TextureEssentials::activate(GLuint texnum)
{
	glActiveTexture(GL_TEXTURE0 + texnum);
}

GLuint Texture::getTexId() const
{
	return texId;
}

GLenum Texture::getTarget() const
{
	return target;
}

TextureRef Texture::toRef()
{
	return TextureRef(texId,target);
}

Texture::Texture(GLenum target) : target(target)
{
	glGenTextures(1,&texId);
}

Texture::~Texture()
{
	if(texId) {
		glDeleteTextures(1,&texId);
	}
}

Texture::Texture(Texture&& mov)
	: texId(mov.texId), target(mov.target)
{
	mov.texId = 0;
}

Texture& Texture::operator=(Texture&& mov)
{
	if(texId) {
		glDeleteTextures(1,&texId);
	}
	this->texId = mov.texId;
	this->target = mov.target;
	mov.texId = 0;
	return *this;
}

GLint ITexture::getValueI(GLenum pname) const
{
	GLint toReturn;
	RecursiveLock lock(TextureEssentials::texturemutex);
	TextureEssentials::ensureBound(getTexId(),getTarget());
	glGetTexParameteriv(getTarget(),pname,&toReturn);
	Validate::validate();
	return toReturn;
}

void ITexture::setValueI(GLenum pname, GLint value)
{
	RecursiveLock lock(TextureEssentials::texturemutex);
	TextureEssentials::ensureBound(getTexId(),getTarget());
	glTexParameteri(getTarget(),pname,value);
	Validate::validate();
}

float ITexture::getValueF(GLenum pname) const
{
	float toReturn;
	RecursiveLock lock(TextureEssentials::texturemutex);
	TextureEssentials::ensureBound(getTexId(),getTarget());
	glGetTexParameterfv(getTarget(),pname,&toReturn);
	Validate::validate();
	return toReturn;
}

void ITexture::setValueF(GLenum pname, float value)
{
	RecursiveLock lock(TextureEssentials::texturemutex);
	TextureEssentials::ensureBound(getTexId(),getTarget());
	glTexParameterf(getTarget(),pname,value);
	Validate::validate();
}

GLint ITexture::getLevelValueI(GLenum pname, GLint level) const
{
	GLint toReturn;
	RecursiveLock lock(TextureEssentials::texturemutex);
	TextureEssentials::ensureBound(getTexId(),getTarget());
	glGetTexLevelParameteriv(getTarget(),level,pname,&toReturn);
	Validate::validate();
	return toReturn;
}

float ITexture::getLevelValueF(GLenum pname, GLint level) const
{
	float toReturn;
	RecursiveLock lock(TextureEssentials::texturemutex);
	TextureEssentials::ensureBound(getTexId(),getTarget());
	glGetTexLevelParameterfv(getTarget(),level,pname,&toReturn);
	Validate::validate();
	return toReturn;
}

void GL::ITexture::activate(GLuint index)
{
	glActiveTexture(GL_TEXTURE0 + index);
	Validate::validate();
}

void ITexture::bind() const
{
	RecursiveLock lock(TextureEssentials::texturemutex);
	TextureEssentials::forceBound(getTexId(),getTarget());
	Validate::validate();
}

void ITexture::bind(GLenum target) const
{
	RecursiveLock lock(TextureEssentials::texturemutex);
	TextureEssentials::forceBound(getTexId(),target);

}

void ITexture::doActions(const TextureAccessor& fun)
{
	RecursiveLock lock(TextureEssentials::texturemutex);
	TextureEssentials::ensureBound(getTexId(),getTarget());
	fun(*this);
}

void ITexture::doActions(const TextureConstAccessor& fun) const
{
	RecursiveLock lock(TextureEssentials::texturemutex);
	TextureEssentials::ensureBound(getTexId(),getTarget());
	fun(*this);
}

void ITexture::doActions(const TextureIdAccessor1& fun) const
{
	RecursiveLock lock(TextureEssentials::texturemutex);
	TextureEssentials::ensureBound(getTexId(),getTarget());
	fun(getTexId());
}

void ITexture::doActions(const TextureIdAccessor2& fun) const
{
	RecursiveLock lock(TextureEssentials::texturemutex);
	TextureEssentials::ensureBound(getTexId(),getTarget());
	fun(getTexId(),getTarget());
}


GLenum ITexture::getCompareMode() const {
	return static_cast<GLenum>(getValueI(GL_TEXTURE_COMPARE_MODE));
}

void ITexture::setCompareMode(GLenum mode) {
	setValueI(GL_TEXTURE_COMPARE_MODE,static_cast<GLint>(mode));
	Validate::validate();
}

GLenum ITexture::getCompareFunc() const {
	return static_cast<GLenum>(getValueI(GL_TEXTURE_COMPARE_FUNC));
}

void ITexture::setCompareFunc(GLenum mode) {
	setValueI(GL_TEXTURE_COMPARE_FUNC,static_cast<GLint>(mode));
	Validate::validate();
}

void ITexture::pixelStorei(GLenum pname, GLint param)
{
	RecursiveLock lock(TextureEssentials::texturemutex);
	TextureEssentials::ensureBound(getTexId(),getTarget());
	glPixelStorei(pname,param);
	Validate::validate();
}

GLint ITexture::getBaseLevel() const
{
	return getValueI(GL_TEXTURE_BASE_LEVEL);
}

void ITexture::setBaseLevel(GLint level)
{
	setValueI(GL_TEXTURE_BASE_LEVEL,level);
}

GLenum ITexture::getDepthStencilTextureMode() const
{
	return static_cast<GLenum>(getValueI(GL_DEPTH_STENCIL_TEXTURE_MODE));
}

void ITexture::setDepthStencilTextureMode(GLenum mode)
{
	setValueI(GL_DEPTH_STENCIL_TEXTURE_MODE,static_cast<GLint>(mode));
}

GLenum ITexture::getMagFilter() const
{
	return static_cast<GLenum>(getValueI(GL_TEXTURE_MAG_FILTER));
}

void ITexture::setMagFilter(GLenum mode)
{
	setValueI(GL_TEXTURE_MAG_FILTER,static_cast<GLint>(mode));
}

GLenum ITexture::getMinFilter() const
{
	return static_cast<GLenum>(getValueI(GL_TEXTURE_MIN_FILTER));
}

void ITexture::setMinFilter(GLenum mode)
{
	setValueI(GL_TEXTURE_MIN_FILTER,static_cast<GLint>(mode));
}

float ITexture::getMinLod() const
{
	return getValueF(GL_TEXTURE_MIN_LOD);
}

void ITexture::setMinLod(float val)
{
	setValueF(GL_TEXTURE_MIN_LOD,val);
}

float ITexture::getMaxLod() const
{
	return getValueF(GL_TEXTURE_MAX_LOD);
}

void ITexture::setMaxLod(float val)
{
	setValueF(GL_TEXTURE_MAX_LOD,val);
}

GLenum ITexture::getWrapS() const
{
	return static_cast<GLenum>(getValueI(GL_TEXTURE_WRAP_S));
}

void ITexture::setWrapS(GLenum mode)
{
	setValueI(GL_TEXTURE_WRAP_S,static_cast<GLint>(mode));
}

GLenum ITexture::getWrapT() const
{
	return static_cast<GLenum>(getValueI(GL_TEXTURE_WRAP_T));
}

void ITexture::setWrapT(GLenum mode)
{
	setValueI(GL_TEXTURE_WRAP_T,static_cast<GLint>(mode));
}

GLenum ITexture::getWrapR() const
{
	return static_cast<GLenum>(getValueI(GL_TEXTURE_WRAP_R));
}

void ITexture::setWrapR(GLenum mode)
{
	setValueI(GL_TEXTURE_WRAP_R,static_cast<GLint>(mode));
}

GLint ITexture::getWidth(GLint level) const
{
	return getLevelValueI(GL_TEXTURE_WIDTH,level);
}

GLint ITexture::getHeight(GLint level) const
{
	return getLevelValueI(GL_TEXTURE_HEIGHT,level);
}

GLint ITexture::getDepth(GLint level) const
{
	return getLevelValueI(GL_TEXTURE_DEPTH,level);
}

void ITexture::storage2D(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
	RecursiveLock lock(TextureEssentials::texturemutex);
	TextureEssentials::ensureBound(getTexId(),getTarget());
	glTexStorage2D(getTarget(),levels,internalformat,width,height);
	Validate::validate();
}

void ITexture::image2D(GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* data)
{
	RecursiveLock lock(TextureEssentials::texturemutex);
	TextureEssentials::ensureBound(getTexId(),getTarget());
	glTexImage2D(getTarget(),level,internalFormat,width,height,border,format,type,data);
	Validate::validate();
}

void ITexture::subImage2D(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* data)
{
	RecursiveLock lock(TextureEssentials::texturemutex);
	TextureEssentials::ensureBound(getTexId(),getTarget());
	glTexSubImage2D(getTarget(),level,xoffset,yoffset,width,height,format,type,data);
	Validate::validate();
}

void ITexture::compressedImage2D(GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data)
{
	RecursiveLock lock(TextureEssentials::texturemutex);
	TextureEssentials::ensureBound(getTexId(),getTarget());
	glCompressedTexImage2D(getTarget(),level,internalformat,width,height,border,imageSize,data);
	Validate::validate();
}

void ITexture::compressedSubImage2D(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data)
{
	RecursiveLock lock(TextureEssentials::texturemutex);
	TextureEssentials::ensureBound(getTexId(),getTarget());
	glCompressedTexSubImage2D(getTarget(),level,xoffset,yoffset,width,height,format,imageSize,data);
	Validate::validate();
}

void ITexture::storage3D(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
	RecursiveLock lock(TextureEssentials::texturemutex);
	TextureEssentials::ensureBound(getTexId(),getTarget());
	glTexStorage3D(getTarget(),levels,internalformat,width,height,depth);
	Validate::validate();
}

void ITexture::image3D(GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* data)
{
	RecursiveLock lock(TextureEssentials::texturemutex);
	TextureEssentials::ensureBound(getTexId(),getTarget());
	glTexImage3D(getTarget(),level,internalFormat,width,height,depth,border,format,type,data);
	Validate::validate();
}

void ITexture::subImage3D(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei depth, GLsizei height, GLenum format, GLenum type, const void* data)
{
	glTexSubImage3D(getTarget(),level,xoffset,yoffset,zoffset,width,height,depth,format,type,data);
	Validate::validate();
}

void ITexture::compressedImage3D(GLint level, GLenum internalformat, GLsizei width, GLsizei depth, GLsizei height, GLint border, GLsizei imageSize, const void* data)
{
	RecursiveLock lock(TextureEssentials::texturemutex);
	TextureEssentials::ensureBound(getTexId(),getTarget());
	glCompressedTexImage3D(getTarget(),level,internalformat,width,height,depth,border,imageSize,data);
	Validate::validate();
}

void ITexture::compressedSubImage3D(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei depth, GLsizei height, GLenum format, GLsizei imageSize, const void* data)
{
	RecursiveLock lock(TextureEssentials::texturemutex);
	TextureEssentials::ensureBound(getTexId(),getTarget());
	glCompressedTexSubImage3D(getTarget(),level,xoffset,yoffset,zoffset,width,height,depth,format,imageSize,data);
	Validate::validate();
}

void ITexture::generateMipmaps(int mipCount)
{
	RecursiveLock lock(TextureEssentials::texturemutex);
	TextureEssentials::ensureBound(getTexId(),getTarget());
	for(int i = 0; i < mipCount; ++i) glGenerateMipmap(getTarget());
	Validate::validate();
}

void ITexture::generateMipmap()
{
	RecursiveLock lock(TextureEssentials::texturemutex);
	TextureEssentials::ensureBound(getTexId(),getTarget());
	glGenerateMipmap(getTarget());
	Validate::validate();
}

TextureRef::TextureRef(GLuint texId, GLenum target)
	: texId(texId), target(target)
{

}

TextureRef::~TextureRef()
{

}

TextureRef::TextureRef(const TextureRef& cpy)
	: texId(cpy.texId), target(cpy.target)
{

}

TextureRef& TextureRef::operator=(const TextureRef& cpy)
{
	this->texId = cpy.texId;
	this->target = cpy.target;
	return *this;
}

GLuint TextureRef::getTexId() const
{
	return texId;
}

GLenum TextureRef::getTarget() const
{
	return target;
}

}
