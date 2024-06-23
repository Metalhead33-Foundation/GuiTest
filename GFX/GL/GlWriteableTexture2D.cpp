#include "GlWriteableTexture2D.hpp"
#include "GlUtil.hpp"

namespace GL {
WriteableTexture2D::WriteableTexture2D(const SoftwareSideImageCreator& imageCreator)
	: softTexture(imageCreator()),
	  topLeftUpdated(std::numeric_limits<int>::max() , std::numeric_limits<int>::max()),
	  bottomRightUpdated(std::numeric_limits<int>::min(), std::numeric_limits<int>::min()), wasResized(false)
{
	glGenTextures(1,&textureVar);
	glBindTexture(GL_TEXTURE_2D,textureVar);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GLenum glinternalFormat, glFormat, gltype;
	prevalidateGlTexture(*softTexture,glinternalFormat,glFormat,gltype);
	glTexImage2D(GL_TEXTURE_2D,0,glinternalFormat,softTexture->getWidth(),softTexture->getHeight(),0,glFormat,gltype,softTexture->getRawPixels());
}

WriteableTexture2D::~WriteableTexture2D()
{
	if(textureVar) glDeleteTextures(1,&textureVar);
}

WriteableTexture2D::WriteableTexture2D(WriteableTexture2D&& mov)
	: textureVar(mov.textureVar), softTexture(std::move(mov.softTexture)), topLeftUpdated(mov.topLeftUpdated), bottomRightUpdated(mov.bottomRightUpdated), wasResized(mov.wasResized)
{
	mov.textureVar = 0;
}

WriteableTexture2D& WriteableTexture2D::operator=(WriteableTexture2D&& mov)
{
	this->textureVar = mov.textureVar;
	this->softTexture = std::move(mov.softTexture);
	this->wasResized = mov.wasResized;
	this->topLeftUpdated = mov.topLeftUpdated;
	this->bottomRightUpdated = mov.bottomRightUpdated;
	mov.textureVar = 0;
}

unsigned WriteableTexture2D::clampXWitthinBounds(unsigned x) const
{
	return std::min(x, softTexture->getWidth());
}

unsigned WriteableTexture2D::clampYWitthinBounds(unsigned y) const
{
	return std::min(y, softTexture->getHeight());
}

void WriteableTexture2D::onRegionUpdate(const glm::ivec2& topleft, const glm::ivec2& bottomright)
{
	if(!wasResized) {
		topLeftUpdated.x = std::min(topleft.x, topLeftUpdated.x);
		topLeftUpdated.y = std::min(topleft.y, topLeftUpdated.y);
		bottomRightUpdated.x = std::max(bottomright.x, bottomRightUpdated.x);
		bottomRightUpdated.y = std::max(bottomright.y, bottomRightUpdated.y);
	}
}

void WriteableTexture2D::clearUpdatedRegion()
{
	topLeftUpdated.x = std::numeric_limits<int>::max();
	topLeftUpdated.y = std::numeric_limits<int>::max();
	bottomRightUpdated.x = std::numeric_limits<int>::min();
	bottomRightUpdated.y = std::numeric_limits<int>::min();
}

MH33::GFX::Handle WriteableTexture2D::getNativeHandle()
{
	return { .oglUint = textureVar };
}

MH33::GFX::ConstHandle WriteableTexture2D::getNativeHandle() const
{
	return { .oglUint = textureVar };
}

MH33::GFX::TextureFormat WriteableTexture2D::getFormat() const
{
	return softTexture->getFormat();
}

void WriteableTexture2D::bind(uint8_t unit) const
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, textureVar);
}

void WriteableTexture2D::bindAsImage(uint8_t unit, MH33::GFX::ImageBindingType bindingType) const
{
	GLenum access = translateBindingType(bindingType);
	glBindImageTexture(unit, textureVar, 0, false, 0, access, translateToInternalFormat(softTexture->getFormat()));
}

unsigned WriteableTexture2D::getWidth() const
{
	return softTexture->getWidth();
}

float WriteableTexture2D::getWidthF() const
{
	return softTexture->getWidthF();
}

float WriteableTexture2D::getWidthR() const
{
	return softTexture->getWidthR();
}

unsigned WriteableTexture2D::getHeight() const
{
	return softTexture->getHeight();
}

float WriteableTexture2D::getHeightF() const
{
	return softTexture->getHeightF();
}

float WriteableTexture2D::getHeightR() const
{
	return softTexture->getHeightR();
}

unsigned WriteableTexture2D::getStride() const
{
	return softTexture->getStride();
}

void* WriteableTexture2D::getPixels()
{
	return softTexture->getRawPixels();
}

const void* WriteableTexture2D::getPixels() const
{
	return softTexture->getRawPixels();
}

bool WriteableTexture2D::resize(int newWidth, int newHeight)
{
	topLeftUpdated = { 0, 0 };
	bottomRightUpdated = { newWidth, newHeight };
	wasResized = true;
	return softTexture->resize(newWidth, newHeight);
}

void WriteableTexture2D::iterateOverPixels(const ColourIterator& program) const
{
	softTexture->iterateOverPixels(program);
}

void WriteableTexture2D::iterateOverPixels(const ColourIterator2& program) const
{
	softTexture->iterateOverPixels(program);
}

void WriteableTexture2D::iterateOverPixels(const ColourIterator& program, const glm::ivec2 offset, const glm::ivec2& dimensions) const
{
	softTexture->iterateOverPixels(program, offset, dimensions);
}

void WriteableTexture2D::iterateOverPixels(const ColourIterator2& program, const glm::ivec2 offset, const glm::ivec2& dimensions) const
{
	softTexture->iterateOverPixels(program, offset, dimensions);
}

void WriteableTexture2D::clearToColour(const glm::fvec4& colourKernel)
{
	softTexture->clearToColour(colourKernel);
	onRegionUpdate( { 0, 0 }, glm::ivec2(softTexture->getWidth(), softTexture->getHeight() ) );
}

void WriteableTexture2D::clearToColour(const ColourProgrammer& program)
{
	softTexture->clearToColour(program);
	onRegionUpdate( { 0, 0 }, glm::ivec2(softTexture->getWidth(), softTexture->getHeight() ) );
}

void WriteableTexture2D::clearToColour(const ColourProgrammer2& program)
{
	softTexture->clearToColour(program);
	onRegionUpdate( { 0, 0 }, glm::ivec2(softTexture->getWidth(), softTexture->getHeight() ) );
}

void WriteableTexture2D::clearToColour(const ColourProgrammer3& program)
{
	softTexture->clearToColour(program);
	onRegionUpdate( { 0, 0 }, glm::ivec2(softTexture->getWidth(), softTexture->getHeight() ) );
}

void WriteableTexture2D::clearToColour(const ColourProgrammer4& program)
{
	softTexture->clearToColour(program);
	onRegionUpdate( { 0, 0 }, glm::ivec2(softTexture->getWidth(), softTexture->getHeight() ) );
}

void WriteableTexture2D::clearToColour(const glm::fvec4& colourKernel, const glm::ivec2 offset, const glm::ivec2& dimensions)
{
	softTexture->clearToColour(colourKernel, offset, dimensions);
	onRegionUpdate( offset, offset + dimensions );
}

void WriteableTexture2D::clearToColour(const ColourProgrammer& program, const glm::ivec2 offset, const glm::ivec2& dimensions)
{
	softTexture->clearToColour(program, offset, dimensions);
	onRegionUpdate( offset, offset + dimensions );
}

void WriteableTexture2D::clearToColour(const ColourProgrammer2& program, const glm::ivec2 offset, const glm::ivec2& dimensions)
{
	softTexture->clearToColour(program, offset, dimensions);
	onRegionUpdate( offset, offset + dimensions );
}

void WriteableTexture2D::clearToColour(const ColourProgrammer3& program, const glm::ivec2 offset, const glm::ivec2& dimensions)
{
	softTexture->clearToColour(program, offset, dimensions);
	onRegionUpdate( offset, offset + dimensions );
}

void WriteableTexture2D::clearToColour(const ColourProgrammer4& program, const glm::ivec2 offset, const glm::ivec2& dimensions)
{
	softTexture->clearToColour(program, offset, dimensions);
	onRegionUpdate( offset, offset + dimensions );
}

void WriteableTexture2D::blit(const MH33::Image::Image2D& cpy, const glm::ivec2 offset, const glm::ivec2& dimensions)
{
	softTexture->blit(cpy,offset,dimensions);
	onRegionUpdate( offset, offset + dimensions );
}

void WriteableTexture2D::blit(const MH33::Image::Image2D& cpy, const glm::ivec2 offset)
{
	softTexture->blit(cpy,offset);
	onRegionUpdate( offset, glm::ivec2(softTexture->getWidth(), softTexture->getHeight()) );
}

void WriteableTexture2D::blit(const std::span<const std::byte>& data, MH33::GFX::TextureFormat format, const glm::ivec2 offset, const glm::ivec2& dimensions)
{
	softTexture->blit(data, format, offset, dimensions);
	onRegionUpdate( offset, offset + dimensions );
}

void WriteableTexture2D::update()
{
	GLenum glinternalFormat, glFormat, gltype;
	prevalidateGlTexture(*softTexture,glinternalFormat,glFormat,gltype);
	glBindTexture(GL_TEXTURE_2D,textureVar);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	if(wasResized) {
		glTexImage2D(GL_TEXTURE_2D,0,glinternalFormat,softTexture->getWidth(),softTexture->getHeight(),0,glFormat,gltype,softTexture->getRawPixels());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		clearUpdatedRegion();
		wasResized = false;
	}else {
		// Edge case in case the entire texture was updated.
		glTexSubImage2D(GL_TEXTURE_2D,0,0,0,softTexture->getWidth(),softTexture->getHeight(),glFormat,gltype,softTexture->getRawPixels());
		clearUpdatedRegion();
	}/* else if(!topLeftUpdated.x && !topLeftUpdated.y && bottomRightUpdated.x == softTexture->getWidth() && bottomRightUpdated.y == softTexture->getHeight()) {
		// Edge case in case the entire texture was updated.
		glTexSubImage2D(GL_TEXTURE_2D,0,0,0,softTexture->getWidth(),softTexture->getHeight(),glFormat,gltype,softTexture->getRawPixels());
		clearUpdatedRegion();
	} else {
		// We're gonna update line by line. Ouch.
		const int lineLength = clampXWitthinBounds(bottomRightUpdated.x) - clampXWitthinBounds(topLeftUpdated.x);
		for(int y = topLeftUpdated.y; y < bottomRightUpdated.y; ++y) {
			const std::byte* const lineStart = &static_cast<const std::byte*>(softTexture->getRawPixels())[softTexture->getStride() * y];
			const std::byte* const offsetLine = &lineStart[topLeftUpdated.x * MH33::Image::byteSize(softTexture->getFormat())];
			glTexSubImage2D(GL_TEXTURE_2D, 0, topLeftUpdated.x, y, lineLength, 1, glFormat, gltype, offsetLine);
		}
		clearUpdatedRegion();
	}*/
}

void WriteableTexture2D::saveTo(MH33::Io::Device& iodev)
{
	softTexture->save(iodev);
}

}
