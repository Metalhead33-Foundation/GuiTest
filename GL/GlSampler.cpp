#include <GL/GlSampler.hpp>
namespace GL {

std::recursive_mutex SamplerEssentials::shadermutex;
typedef std::lock_guard<std::recursive_mutex> RecursiveLock;
GLuint SamplerEssentials::lastBound = 0;
void SamplerEssentials::ensureBound(GLuint id, GLenum target)
{
	if(lastBound != id) {
		glBindSampler(target,id);
		lastBound = id;
	}
}

Sampler::Sampler()
{
	glGenSamplers(1,&samplerId);
}

GL::Sampler::~Sampler()
{
	if(samplerId) {
		glDeleteSamplers(1,&samplerId);
	}
}

Sampler::Sampler(Sampler&& mov)
	: samplerId(mov.samplerId)
{
	mov.samplerId = 0;
}

Sampler& Sampler::operator=(Sampler&& mov)
{
	if(samplerId) {
		glDeleteSamplers(1,&samplerId);
	}
	this->samplerId = mov.samplerId;
	mov.samplerId = 0;
	return *this;
}

GLuint Sampler::getSamplerId() const
{
	return samplerId;
}

SamplerRef Sampler::toRef()
{
	return SamplerRef(samplerId);
}

GLint ISampler::getValueI(GLenum pname) const
{
	GLint toReturn;
	glGetSamplerParameteriv(getSamplerId(),pname,&toReturn);
	return toReturn;
}

void ISampler::setValueI(GLenum pname, GLint value)
{
	glSamplerParameteri(getSamplerId(),pname,value);
}

float ISampler::getValueF(GLenum pname) const
{
	float toReturn;
	glGetSamplerParameterfv(getSamplerId(),pname,&toReturn);
	return toReturn;
}

void ISampler::setValueF(GLenum pname, float value)
{
	glSamplerParameterf(getSamplerId(),pname,value);
}

GLenum ISampler::getMagFilter() const
{
	return static_cast<GLenum>(getValueI(GL_TEXTURE_MAG_FILTER));
}

void ISampler::setMagFilter(GLenum mode)
{
	setValueI(GL_TEXTURE_MAG_FILTER,static_cast<GLint>(mode));
}

GLenum ISampler::getMinFilter() const
{
	return static_cast<GLenum>(getValueI(GL_TEXTURE_MIN_FILTER));
}

void ISampler::setMinFilter(GLenum mode)
{
	setValueI(GL_TEXTURE_MIN_FILTER,static_cast<GLint>(mode));
}

float ISampler::getMinLod() const
{
	return getValueF(GL_TEXTURE_MIN_LOD);
}

void ISampler::setMinLod(float val)
{
	setValueF(GL_TEXTURE_MIN_LOD,val);
}

float ISampler::getMaxLod() const
{
	return getValueF(GL_TEXTURE_MAX_LOD);
}

void ISampler::setMaxLod(float val)
{
	setValueF(GL_TEXTURE_MAX_LOD,val);
}

GLenum ISampler::getWrapS() const
{
	return static_cast<GLenum>(getValueI(GL_TEXTURE_WRAP_S));
}

void ISampler::setWrapS(GLenum mode)
{
	setValueI(GL_TEXTURE_WRAP_S,static_cast<GLint>(mode));
}

GLenum ISampler::getWrapT() const
{
	return static_cast<GLenum>(getValueI(GL_TEXTURE_WRAP_T));
}

void ISampler::setWrapT(GLenum mode)
{
	setValueI(GL_TEXTURE_WRAP_T,static_cast<GLint>(mode));
}

GLenum ISampler::getWrapR() const
{
	return static_cast<GLenum>(getValueI(GL_TEXTURE_WRAP_R));
}

void ISampler::setWrapR(GLenum mode)
{
	setValueI(GL_TEXTURE_WRAP_R,static_cast<GLint>(mode));
}

GLenum ISampler::getCompareMode() const {
	return static_cast<GLenum>(getValueI(GL_TEXTURE_COMPARE_MODE));
}

void ISampler::setCompareMode(GLenum mode) {
	setValueI(GL_TEXTURE_COMPARE_MODE,static_cast<GLint>(mode));
}

GLenum ISampler::getCompareFunc() const {
	return static_cast<GLenum>(getValueI(GL_TEXTURE_COMPARE_FUNC));
}

void ISampler::setCompareFunc(GLenum mode) {
	setValueI(GL_TEXTURE_COMPARE_FUNC,static_cast<GLint>(mode));
}

void ISampler::bind(GLuint textureUnit)
{
	//RecursiveLock lock(SamplerEs::buffermutex);
	glBindSampler(textureUnit,getSamplerId());
}

void ISampler::doActions(const SamplerUser& fun) {
	fun(*this);
}
void ISampler::doActions(const SamplerIdUser& fun) const {
	fun(getSamplerId());
}
void ISampler::doActions(const SamplerConstUser& fun) const {
	fun(*this);
}

SamplerRef::SamplerRef(GLuint samplerId)
	: samplerId(samplerId)
{

}

SamplerRef::~SamplerRef()
{

}

GLuint SamplerRef::getSamplerId() const
{
	return samplerId;
}

SamplerRef::SamplerRef(const SamplerRef& cpy)
	: samplerId(cpy.samplerId)
{

}

SamplerRef& SamplerRef::operator=(const SamplerRef& cpy)
{
	this->samplerId = cpy.samplerId;
	return *this;
}

}
