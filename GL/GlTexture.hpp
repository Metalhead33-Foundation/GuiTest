#ifndef TEXTURE_HPP
#define TEXTURE_HPP
#include <GL/gles2.h>
#include <mutex>
#include <span>
#include <vector>
#include <functional>

namespace GL {

struct TextureEssentials {
	static std::recursive_mutex texturemutex;
	static GLuint lastBound;
	static void ensureBound(GLuint id, GLenum target);
	static void activate(GLuint texnum);
};

class ITexture
{
private:
	GLint getValueI(GLenum pname) const;
	void setValueI(GLenum pname, GLint value);
	float getValueF(GLenum pname) const;
	void setValueF(GLenum pname, float value);
	GLint getLevelValueI(GLenum pname, GLint level) const;
	float getLevelValueF(GLenum pname, GLint level) const;
public:
	typedef std::function<void(ITexture&)> TextureAccessor;
	typedef std::function<void(const ITexture&)> TextureConstAccessor;
	typedef std::function<void(GLuint)> TextureIdAccessor1;
	typedef std::function<void(GLuint,GLenum)> TextureIdAccessor2;
	virtual ~ITexture() = default;
	virtual GLuint getTexId() const = 0;
	virtual GLenum getTarget() const = 0;
	// Bind
	static void activate(GLuint index);
	void bind();
	void bind(GLenum target);
	void doActions(const TextureAccessor& fun);
	void doActions(const TextureConstAccessor& fun) const;
	void doActions(const TextureIdAccessor1& fun) const;
	void doActions(const TextureIdAccessor2& fun) const;
	// Getters, setters
	GLint getBaseLevel() const; // GL_TEXTURE_BASE_LEVEL
	void setBaseLevel(GLint level); // GL_TEXTURE_BASE_LEVEL

	GLenum getDepthStencilTextureMode() const; // GL_DEPTH_STENCIL_TEXTURE_MODE
	void setDepthStencilTextureMode(GLenum mode); // GL_DEPTH_STENCIL_TEXTURE_MODE
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

	GLint getWidth(GLint level = 0) const; // GL_TEXTURE_WIDTH
	GLint getHeight(GLint level = 0) const; // GL_TEXTURE_HEIGHT
	GLint getDepth(GLint level = 0) const; // GL_TEXTURE_DEPTH

	GLenum getCompareMode() const; // GL_TEXTURE_COMPARE_MODE
	void setCompareMode(GLenum mode); // GL_TEXTURE_COMPARE_MODE
	GLenum getCompareFunc() const; // GL_TEXTURE_COMPARE_FUNC
	void setCompareFunc(GLenum mode); // GL_TEXTURE_COMPARE_FUNC

	// Storage - 2D
	void storage2D(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
	void image2D(GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void * data);
	void subImage2D(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void * data);
	void compressedImage2D(GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void * data);
	void compressedSubImage2D(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void * data);
	// Storage - 3D
	void storage3D(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
	void image3D(GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void * data);
	void subImage3D(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei depth, GLsizei height, GLenum format, GLenum type, const void * data);
	void compressedImage3D(GLint level, GLenum internalformat, GLsizei width, GLsizei depth, GLsizei height, GLint border, GLsizei imageSize, const void * data);
	void compressedSubImage3D(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei depth, GLsizei height, GLenum format, GLsizei imageSize, const void * data);
	// Mipmap
	void generateMipmap();
	void generateMipmaps(int mipCount);
};
class TextureRef;
class Texture : public ITexture
{
private:
	GLuint texId;
	GLenum target;
	Texture(const Texture& cpy) = delete;
	Texture& operator=(const Texture& cpy) = delete;
public:
	friend class TextureRef;
	explicit Texture(GLenum target);
	~Texture() override;
	Texture(Texture&& mov);
	Texture& operator=(Texture&& mov);
	GLuint getTexId() const override;
	GLenum getTarget() const override;
	TextureRef toRef();
};
class TextureRef : public ITexture
{
private:
	GLuint texId;
	GLenum target;
public:
	explicit TextureRef(GLuint texId, GLenum target);
	~TextureRef() override;
	TextureRef(const TextureRef& cpy);
	TextureRef& operator=(const TextureRef& cpy);
	GLuint getTexId() const override;
	GLenum getTarget() const override;
};

}

#endif // TEXTURE_HPP
