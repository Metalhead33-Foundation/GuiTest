#ifndef ITEXTURE_HPP
#define ITEXTURE_HPP

class ITexture {
public:
	virtual ~ITexture() = default;
	virtual int getWidth() const = 0;
	virtual int getHeight() const = 0;
	virtual int getStride() const = 0;
};

#endif // ITEXTURE_HPP
