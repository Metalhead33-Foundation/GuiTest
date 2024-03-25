#include "GlTexture.hpp"

namespace GL {

const Texture& Texture2D::getTexHndl() const
{
	return texHndl;
}

Texture& Texture2D::getTexHndl()
{
	return texHndl;
}
const Texture& WriteableTexture2D::getTexHndl() const
{
	return texHndl;
}

Texture& WriteableTexture2D::getTexHndl()
{
	return texHndl;
}
const Texture& ArrayTexture2D::getTexHndl() const
{
	return texHndl;
}

Texture& ArrayTexture2D::getTexHndl()
{
	return texHndl;
}
const Texture& Texture3D::getTexHndl() const
{
	return texHndl;
}

Texture& Texture3D::getTexHndl()
{
	return texHndl;
}

void Texture2D::updateDim(int nwidth, int nheight, int npixxisze)
{
	// Width
	width = nwidth;
	widthF = static_cast<float>(nwidth - 1);
	widthR = 1.0f / widthF;
	// Height
	height = nheight;
	heightF = static_cast<float>(nheight - 1);
	heightR = 1.0f / heightF;
	// Stride
	stride = width * npixxisze;
}

Texture2D::Texture2D()
	: texHndl(GL_TEXTURE_2D), format(MH33::Image::Format::INVALID)
{
	updateDim(0,0,0);
}

void WriteableTexture2D::updateDim(int nwidth, int nheight, int npixxisze)
{
	// Width
	width = nwidth;
	widthF = static_cast<float>(nwidth - 1);
	widthR = 1.0f / widthF;
	// Height
	height = nheight;
	heightF = static_cast<float>(nheight - 1);
	heightR = 1.0f / heightF;
	// Stride
	stride = width * npixxisze;
}

WriteableTexture2D::WriteableTexture2D()
	: texHndl(GL_TEXTURE_2D), format(MH33::Image::Format::INVALID)
{
	updateDim(0,0,0);
}

void ArrayTexture2D::updateDim(int nwidth, int nheight, int ndepth, int npixxisze)
{
	// Width
	width = nwidth;
	widthF = static_cast<float>(nwidth - 1);
	widthR = 1.0f / widthF;
	// Height
	height = nheight;
	heightF = static_cast<float>(nheight - 1);
	heightR = 1.0f / heightF;
	// Depth
	depth = ndepth;
	depthF = static_cast<float>(ndepth - 1);
	depthR = 1.0f / depthF;
	// Stride
	stride = width * npixxisze;
}

ArrayTexture2D::ArrayTexture2D()
	: texHndl(GL_TEXTURE_2D_ARRAY), format(MH33::Image::Format::INVALID)
{
	updateDim(0,0,0,0);
}

void Texture3D::updateDim(int nwidth, int nheight, int ndepth, int npixxisze)
{
	// Width
	width = nwidth;
	widthF = static_cast<float>(nwidth - 1);
	widthR = 1.0f / widthF;
	// Height
	height = nheight;
	heightF = static_cast<float>(nheight - 1);
	heightR = 1.0f / heightF;
	// Depth
	depth = ndepth;
	depthF = static_cast<float>(ndepth - 1);
	depthR = 1.0f / depthF;
	// Stride
	stride = width * npixxisze;
}

Texture3D::Texture3D()
	: texHndl(GL_TEXTURE_3D), format(MH33::Image::Format::INVALID)
{
	updateDim(0,0,0,0);
}

MH33::GFX::Handle Texture2D::getNativeHandle()
{
	return { .oglUint = texHndl.getTexId() };
}

MH33::GFX::ConstHandle Texture2D::getNativeHandle() const
{
	return { .oglUint = texHndl.getTexId() };
}

MH33::GFX::TextureFormat Texture2D::getFormat() const
{
	return format;
}

int Texture2D::getWidth() const
{
	return width;
}

float Texture2D::getWidthF() const
{
	return widthF;
}

float Texture2D::getWidthR() const
{
	return widthR;
}

int Texture2D::getHeight() const
{
	return height;
}

float Texture2D::getHeightF() const
{
	return heightF;
}

float Texture2D::getHeightR() const
{
	return heightR;
}

int Texture2D::getStride() const
{
	return stride;
}

MH33::GFX::Handle WriteableTexture2D::getNativeHandle()
{
	return { .oglUint = texHndl.getTexId() };
}

MH33::GFX::ConstHandle WriteableTexture2D::getNativeHandle() const
{
	return { .oglUint = texHndl.getTexId() };
}

MH33::GFX::TextureFormat WriteableTexture2D::getFormat() const
{
	return format;
}

int WriteableTexture2D::getWidth() const
{
	return width;
}

float WriteableTexture2D::getWidthF() const
{
	return widthF;
}

float WriteableTexture2D::getWidthR() const
{
	return widthR;
}

int WriteableTexture2D::getHeight() const
{
	return height;
}

float WriteableTexture2D::getHeightF() const
{
	return heightF;
}

float WriteableTexture2D::getHeightR() const
{
	return heightR;
}

int WriteableTexture2D::getStride() const
{
	return stride;
}

bool WriteableTexture2D::resize(int newWidth, int newHeight)
{
	return false;
}

void WriteableTexture2D::blit(const MH33::GFX::WriteableTexture2D& cpy, const glm::ivec2 offset, const glm::ivec2& dimensions)
{
}

void WriteableTexture2D::blit(const MH33::GFX::WriteableTexture2D& cpy, const glm::ivec2 offset)
{
}

void WriteableTexture2D::blit(const std::span<const std::byte>& data, MH33::GFX::TextureFormat format, const glm::ivec2 offset, const glm::ivec2& dimensions)
{
}

void WriteableTexture2D::update()
{
}

MH33::GFX::Handle ArrayTexture2D::getNativeHandle()
{
	return { .oglUint = texHndl.getTexId() };
}

MH33::GFX::ConstHandle ArrayTexture2D::getNativeHandle() const
{
	return { .oglUint = texHndl.getTexId() };
}

MH33::GFX::TextureFormat ArrayTexture2D::getFormat() const
{
	return format;
}

int ArrayTexture2D::getWidth() const
{
	return width;
}

float ArrayTexture2D::getWidthF() const
{
	return widthF;
}

float ArrayTexture2D::getWidthR() const
{
	return widthR;
}

int ArrayTexture2D::getHeight() const
{
	return height;
}

float ArrayTexture2D::getHeightF() const
{
	return heightF;
}

float ArrayTexture2D::getHeightR() const
{
	return heightR;
}

int ArrayTexture2D::getDepth() const
{
	return depth;
}

float ArrayTexture2D::getDepthF() const
{
	return depthF;
}

float ArrayTexture2D::getDepthR() const
{
	return depthR;
}

int ArrayTexture2D::getStride() const
{
	return stride;
}

MH33::GFX::Handle Texture3D::getNativeHandle()
{
	return { .oglUint = texHndl.getTexId() };
}

MH33::GFX::ConstHandle Texture3D::getNativeHandle() const
{
	return { .oglUint = texHndl.getTexId() };
}

MH33::GFX::TextureFormat Texture3D::getFormat() const
{
	return format;
}

int Texture3D::getWidth() const
{
	return width;
}

float Texture3D::getWidthF() const
{
	return widthF;
}

float Texture3D::getWidthR() const
{
	return widthR;
}

int Texture3D::getHeight() const
{
	return height;
}

float Texture3D::getHeightF() const
{
	return heightF;
}

float Texture3D::getHeightR() const
{
	return heightR;
}

int Texture3D::getDepth() const
{
	return depth;
}

float Texture3D::getDepthF() const
{
	return depthF;
}

float Texture3D::getDepthR() const
{
	return depthR;
}

int Texture3D::getStride() const
{
	return stride;
}

}
