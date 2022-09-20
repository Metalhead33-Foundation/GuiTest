#ifndef GLSAMPLER_HPP
#define GLSAMPLER_HPP
#include <GL/gles2.h>
#include <mutex>
#include <span>
#include <vector>
#include <functional>
namespace GL {


struct SamplerEssentials {
	static std::recursive_mutex shadermutex;
	static GLuint lastBound;
	static void ensureBound(GLuint id, GLenum target);
};

class ISampler
{
private:
	GLint getValueI(GLenum pname) const;
	void setValueI(GLenum pname, GLint value);
	float getValueF(GLenum pname) const;
	void setValueF(GLenum pname, float value);
public:
	virtual ~ISampler() = default;
	virtual GLuint getSamplerId() const = 0;

	GLenum getMagFilter() const; // GL_TEXTURE_MAG_FILTER
	void setMagFilter(GLenum mode); // GL_TEXTURE_MAG_FILTER
	GLenum getMinFilter() const; // GL_TEXTURE_MIN_FILTER
	void setMinFilter(GLenum mode); // GL_TEXTURE_MIN_FILTER

	float getMinLod() const; // GL_TEXTURE_MIN_LOD
	void setMinLod(float val); // GL_TEXTURE_MIN_LOD
	float getMaxLod() const; // GL_TEXTURE_MAX_LOD
	void setMaxLod(float val); // GL_TEXTURE_MAX_LOD

	GLenum getWrapS() const; // GL_TEXTURE_WRAP_S
	void setWrapS(GLenum mode); // GL_TEXTURE_WRAP_S
	GLenum getWrapT() const; // GL_TEXTURE_WRAP_T
	void setWrapT(GLenum mode); // GL_TEXTURE_WRAP_T
	GLenum getWrapR() const; // GL_TEXTURE_WRAP_R
	void setWrapR(GLenum mode); // GL_TEXTURE_WRAP_R

	GLenum getCompareMode() const; // GL_TEXTURE_COMPARE_MODE
	void setCompareMode(GLenum mode); // GL_TEXTURE_COMPARE_MODE
	GLenum getCompareFunc() const; // GL_TEXTURE_COMPARE_FUNC
	void setCompareFunc(GLenum mode); // GL_TEXTURE_COMPARE_FUNC

	void bind(GLuint textureUnit);

	typedef std::function<void(ISampler&)> SamplerUser;
	typedef std::function<void(const ISampler&)> SamplerConstUser;
	typedef std::function<void(GLuint)> SamplerIdUser;

	void doActions(const SamplerUser& fun);
	void doActions(const SamplerIdUser& fun) const;
	void doActions(const SamplerConstUser& fun) const;
};

class SamplerRef;
class Sampler : public ISampler
{
private:
	GLuint samplerId;
	Sampler(const Sampler& cpy) = delete;
	Sampler& operator=(const Sampler& cpy) = delete;
public:
	Sampler();
	~Sampler() override;
	Sampler(Sampler&& mov);
	Sampler& operator=(Sampler&& mov);
	GLuint getSamplerId() const override;
	SamplerRef toRef();
};
class SamplerRef : public ISampler
{
private:
	GLuint samplerId;
public:
	explicit SamplerRef(GLuint samplerId);
	~SamplerRef() override;
	SamplerRef(const SamplerRef& cpy);
	SamplerRef& operator=(const SamplerRef& cpy);
	GLuint getSamplerId() const override;
};

}
#endif // GLSAMPLER_HPP
