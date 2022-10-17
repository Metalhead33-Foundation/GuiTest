#ifndef STANDARDTEXTURE_IPP
#define STANDARDTEXTURE_IPP

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline StandardTexture<PixelType,fmt>::StandardTexture(const StandardTexture& cpy)
	: pixels(cpy.pixels), width(cpy.width), height(cpy.height), stride(cpy.stride),
	  widthF(cpy.widthF), heightF(cpy.heightF), widthR(cpy.widthR), heightR(cpy.heightR) {

}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline StandardTexture<PixelType,fmt>::StandardTexture(StandardTexture&& mov)
	: pixels(std::move(mov.pixels)), width(mov.width), height(mov.height), stride(mov.stride),
	  widthF(mov.widthF), heightF(mov.heightF), widthR(mov.widthR), heightR(mov.heightR) {
	mov.width = 0;
	mov.height = 0;
	mov.stride = 0;
	mov.widthF = 0.0f;
	mov.heightF = 0.0f;
	mov.widthR = 0.0f;
	mov.heightR = 0.0f;
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline StandardTexture<PixelType,fmt>& StandardTexture<PixelType,fmt>::operator=(const StandardTexture& cpy) {
	this->pixels = cpy.pixels;
	this->width = cpy.width;
	this->height = cpy.height;
	this->stride = cpy.stride;
	this->widthF = cpy.widthF;
	this->heightF = cpy.heightF;
	this->widthR = cpy.widthR;
	this->heightR = cpy.heightR;
	return *this;
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline StandardTexture<PixelType,fmt>& StandardTexture<PixelType,fmt>::operator=(StandardTexture&& mov) {
	this->pixels = std::move(mov.pixels);
	this->width = mov.width;
	this->height = mov.height;
	this->stride = mov.stride;
	this->widthF = mov.widthF;
	this->heightF = mov.heightF;
	this->widthR = mov.widthR;
	this->heightR = mov.heightR;
	mov.width = 0;
	mov.height = 0;
	mov.stride = 0;
	mov.widthF = 0.0f;
	mov.heightF = 0.0f;
	mov.widthR = 0.0f;
	mov.heightR = 0.0f;
	return *this;
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline StandardTexture<PixelType,fmt>::StandardTexture(int width, int height)
	: pixels(width*height), width(width), height(height), stride(width*sizeof(PixelType)),
	  widthF(width-1), heightF(height-1), widthR(1.0f / static_cast<float>(width)), heightR(1.0f / static_cast<float>(height))
{
	std::memset(pixels.data(),0,sizeof(PixelType)*pixels.size());
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline StandardTexture<PixelType,fmt>::StandardTexture(const PixelType* pixelsToCopy, int width, int height)
	: pixels(width*height), width(width), height(height), stride(width*sizeof(PixelType)),
	  widthF(width-1), heightF(height-1), widthR(1.0f / static_cast<float>(width)), heightR(1.0f / static_cast<float>(height))
{
	std::memcpy(pixels.data(),pixelsToCopy,sizeof(PixelType)*pixels.size());
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline StandardTexture<PixelType,fmt>::StandardTexture(const std::span<PixelType> pixelsToCopy, int width, int height)
	: pixels(width*height), width(width), height(height), stride(width*sizeof(PixelType)),
	  widthF(width-1), heightF(height-1), widthR(1.0f / static_cast<float>(width)), heightR(1.0f / static_cast<float>(height))
{
	std::memcpy(pixels.data(),pixelsToCopy.data(),sizeof(PixelType)*pixels.size());
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline StandardTexture<PixelType,fmt>::StandardTexture(std::vector<PixelType>&& mov, int width, int height)
	: pixels(std::move(mov)), width(width), height(height), stride(width*sizeof(PixelType)),
	  widthF(width-1), heightF(height-1), widthR(1.0f / static_cast<float>(width)), heightR(1.0f / static_cast<float>(height))
{

}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline bool StandardTexture<PixelType,fmt>::resize(int newWidth, int newHeight) {
	const int rowsToRunThrough = std::min(this->height,newHeight);
	const size_t bytesPerRow = std::min(this->width,newWidth) * sizeof(PixelType);
	std::vector<PixelType> newPixels(newWidth * newHeight);
	std::memset(newPixels.data(),0,sizeof(PixelType) * newPixels.size());
	for(int y = 0; y < rowsToRunThrough; ++y) {
		const PixelType* const originalRow = &pixels[y*width];
		PixelType* const newRow = &newPixels[y*newWidth];
		memcpy(newRow,originalRow,bytesPerRow);
	}
	this->pixels = std::move(newPixels);
	this->width = newWidth;
	this->height = newHeight;
	this->widthF = static_cast<float>(newWidth - 1);
	this->heightF = static_cast<float>(newHeight - 1);
	this->widthR = 1.0f / static_cast<float>(newWidth);
	this->heightR = 1.0f / static_cast<float>(newHeight);
	return true;
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::blit(const PixelType* cpy, const glm::ivec2 offset, const glm::ivec2& dimensions) {
	if(offset.x >= width || offset.y >= height) return;
	const int rowsToCopy = std::min(dimensions.y,height - offset.y);
	const int columnsToCopy = std::min(dimensions.x,width - offset.x);
	const size_t bytesPerRow = columnsToCopy * sizeof(PixelType);
	for(int y = 0; y < rowsToCopy; ++y) {
		const PixelType* const originalRow = cpy + (y*dimensions.x);
		PixelType* const newRow = pixels.data() + (((offset.y+y)*width)+offset.x);
		memcpy(newRow,originalRow,bytesPerRow);
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::blit(const Texture& cpy, const glm::ivec2 offset, const glm::ivec2& dimensions) {
	const StandardTexture* standardTexture = dynamic_cast<const StandardTexture*>(&cpy);
	if(standardTexture) blit(standardTexture->pixels.data(),offset,dimensions);
	else {
		const ReferenceTexture<PixelType,fmt>* referenceTexture = dynamic_cast<const ReferenceTexture<PixelType,fmt>*>(&cpy);
		if(referenceTexture) blit(referenceTexture->pixels.data(),offset,dimensions);
		else Texture::blit(cpy,offset,dimensions);
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::blit(const Texture& cpy, const glm::ivec2 offset) {
	const StandardTexture* standardTexture = dynamic_cast<const StandardTexture*>(&cpy);
	if(standardTexture) blit(standardTexture->pixels.data(),offset,glm::ivec2(standardTexture->width,standardTexture->height));
	else {
		const ReferenceTexture<PixelType,fmt>* referenceTexture = dynamic_cast<const ReferenceTexture<PixelType,fmt>*>(&cpy);
		if(referenceTexture) blit(referenceTexture->pixels.data(),offset,glm::ivec2(referenceTexture->width,referenceTexture->height));
		else Texture::blit(cpy,offset);
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::getPixel(const glm::ivec2& pos, glm::fvec4& colourKernel, Wrap wrap) const {
	int x;
	int y;
	switch (wrap) {
	case Wrap::REPEAT:
		x = (pos.x >= 0) ? (pos.x % width) : ((pos.x % width) + width);
		y = (pos.y >= 0) ? (pos.y % height) : ((pos.y % height) + height);
		break;
	case MIRRORED_REPEAT:
		x = std::abs(pos.x % width);
		y = std::abs(pos.y % height);
		break;
	case CLAMP_TO_EDGE:
		x = std::clamp(pos.x,0,width);
		y = std::clamp(pos.y,0,height);
		break;
	case CLAMP_TO_BORDER:
		if(pos.x > 0 && pos.x < width && pos.y > 0 && pos.y < height) { x = pos.x; y = pos.y; }
		else { std::memset(&colourKernel,0,sizeof(glm::fvec4)); return; }
		break;
	}
	pixels[ (y * width) + x ].toKernel(colourKernel);
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::setPixel(const glm::ivec2& pos, const glm::fvec4& colourKernel) {
	pixels[ (pos.y * width) + pos.x ].fromKernel(colourKernel);
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::setPixelDithered(const glm::ivec2& pos, const glm::fvec4& colourKernel) {
	pixels[ (pos.y * width) + pos.x ].fromKernelDithered(colourKernel,pos);
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void* StandardTexture<PixelType,fmt>::getRawPixels() {
	return pixels.data();
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline const void* StandardTexture<PixelType,fmt>::getRawPixels() const {
	return pixels.data();
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::clearToColour(const glm::fvec4& colourKernel) {
	PixelType p;
	p.fromKernel(colourKernel);
	for(int y = 0; y < height; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			scanline[x] = p;
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::clearToColour(const ColourProgrammer& program) {
	for(int y = 0; y < height; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			scanline[x].fromKernelDithered(program(pos),pos);
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::clearToColour(const ColourProgrammer2& program) {
	for(int y = 0; y < height; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			glm::fvec4 kernel;
			scanline[x].toKernel(kernel);
			scanline[x].fromKernelDithered(program(pos,kernel),pos);
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::clearToColour(const ColourProgrammer3& program)
{
	const glm::fvec2 reciprocal = glm::fvec2( 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height) );
	for(int y = 0; y < height; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			glm::fvec2 fpos = glm::fvec2(static_cast<float>(x) * reciprocal.x, static_cast<float>(y) * reciprocal.y);
			scanline[x].fromKernelDithered(program(fpos),pos);
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::clearToColour(const ColourProgrammer4& program)
{
	const glm::fvec2 reciprocal = glm::fvec2( 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height) );
	for(int y = 0; y < height; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			glm::fvec2 fpos = glm::fvec2(static_cast<float>(x) * reciprocal.x, static_cast<float>(y) * reciprocal.y);
			glm::fvec4 kernel;
			scanline[x].toKernel(kernel);
			scanline[x].fromKernelDithered(program(fpos,kernel),pos);
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::iterateOverPixels(const ColourIterator& program) const
{
	for(int y = 0; y < height; ++y) {
		const PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			glm::fvec4 kernel;
			scanline[x].toKernel(kernel);
			program(pos,kernel);
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::iterateOverPixels(const ColourIterator2& program) const
{
	const glm::fvec2 reciprocal = glm::fvec2( 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height) );
	for(int y = 0; y < height; ++y) {
		const PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::fvec2 fpos = glm::fvec2(static_cast<float>(x) * reciprocal.x, static_cast<float>(y) * reciprocal.y);
			glm::fvec4 kernel;
			scanline[x].toKernel(kernel);
			program(fpos,kernel);
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline ReferenceTexture<PixelType,fmt>::ReferenceTexture(const ReferenceTexture& cpy)
	: pixels(cpy.pixels), width(cpy.width), height(cpy.height), stride(cpy.stride),
	  widthF(cpy.widthF), heightF(cpy.heightF), widthR(cpy.widthR), heightR(cpy.heightR)  {

}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline ReferenceTexture<PixelType,fmt>& ReferenceTexture<PixelType,fmt>::operator=(const ReferenceTexture& cpy) {
	this->pixels = cpy.pixels;
	this->width = cpy.width;
	this->height = cpy.height;
	this->stride = cpy.stride;
	this->widthF = cpy.widthF;
	this->heightF = cpy.heightF;
	this->widthR = cpy.widthR;
	this->heightR = cpy.heightR;
	return *this;
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline ReferenceTexture<PixelType,fmt>::ReferenceTexture(PixelType* pixelsPointing, int width, int height)
	: pixels(pixelsPointing,width*height), width(width), height(height), stride(width*sizeof(PixelType)),
	  widthF(width-1), heightF(height-1), widthR(1.0f / static_cast<float>(width)), heightR(1.0f / static_cast<float>(height))
{

}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline ReferenceTexture<PixelType,fmt>::ReferenceTexture(const std::span<PixelType> pixelsToCopy, int width, int height)
	: pixels(pixelsToCopy), width(width), height(height), stride(width*sizeof(PixelType)),
	  widthF(width-1), heightF(height-1), widthR(1.0f / static_cast<float>(width)), heightR(1.0f / static_cast<float>(height))
{

}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline ReferenceTexture<PixelType,fmt>::ReferenceTexture(std::span<PixelType>&& mov, int width, int height)
	: pixels(std::move(mov)), width(width), height(height), stride(width*sizeof(PixelType)),
	  widthF(width-1), heightF(height-1), widthR(1.0f / static_cast<float>(width)), heightR(1.0f / static_cast<float>(height))
{

}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline bool ReferenceTexture<PixelType,fmt>::resize(int newWidth, int newHeight) {
	(void)newWidth;
	(void)newHeight;
	return false;
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::blit(const PixelType* cpy, const glm::ivec2 offset, const glm::ivec2& dimensions) {
	if(offset.x >= width || offset.y >= height) return;
	const int rowsToCopy = std::min(dimensions.y,height - offset.y);
	const int columnsToCopy = std::min(dimensions.x,width - offset.x);
	const size_t bytesPerRow = columnsToCopy * sizeof(PixelType);
	for(int y = 0; y < rowsToCopy; ++y) {
		const PixelType* const originalRow = cpy + (y*dimensions.x);
		PixelType* const newRow = pixels.data() + (((offset.y+y)*width)+offset.x);
		memcpy(newRow,originalRow,bytesPerRow);
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::blit(const Texture& cpy, const glm::ivec2 offset, const glm::ivec2& dimensions) {
	const ReferenceTexture* refTxt = dynamic_cast<const ReferenceTexture*>(&cpy);
	if(refTxt) blit(refTxt->pixels.data(),offset,dimensions);
	else {
		const StandardTexture<PixelType,fmt>* standardTexture = dynamic_cast<const StandardTexture<PixelType,fmt>*>(&cpy);
		if(standardTexture) blit(standardTexture->pixels.data(),offset,dimensions);
		else Texture::blit(cpy,offset,dimensions);
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::blit(const Texture& cpy, const glm::ivec2 offset) {
	const ReferenceTexture* refTxt = dynamic_cast<const ReferenceTexture*>(&cpy);
	if(refTxt) blit(refTxt->pixels.data(),offset,glm::ivec2(refTxt->width,refTxt->height));
	else {
		const StandardTexture<PixelType,fmt>* standardTexture = dynamic_cast<const StandardTexture<PixelType,fmt>*>(&cpy);
		if(standardTexture) blit(standardTexture->pixels.data(),offset,glm::ivec2(standardTexture->width,standardTexture->height));
		else Texture::blit(cpy,offset);
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::getPixel(const glm::ivec2& pos, glm::fvec4& colourKernel, Wrap wrap) const {
	int x;
	int y;
	switch (wrap) {
	case Wrap::REPEAT:
		x = (pos.x >= 0) ? (pos.x % width) : ((pos.x % width) + width);
		y = (pos.y >= 0) ? (pos.y % height) : ((pos.y % height) + height);
		break;
	case MIRRORED_REPEAT:
		x = std::abs(pos.x % width);
		y = std::abs(pos.y % height);
		break;
	case CLAMP_TO_EDGE:
		x = std::clamp(pos.x,0,width);
		y = std::clamp(pos.y,0,height);
		break;
	case CLAMP_TO_BORDER:
		if(pos.x > 0 && pos.x < width && pos.y > 0 && pos.y < height) { x = pos.x; y = pos.y; }
		else { std::memset(&colourKernel,0,sizeof(glm::fvec4)); return; }
		break;
	}
	pixels[ (y * width) + x ].toKernel(colourKernel);
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::setPixel(const glm::ivec2& pos, const glm::fvec4& colourKernel) {
	pixels[ (pos.y * width) + pos.x ].fromKernel(colourKernel);
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::setPixelDithered(const glm::ivec2& pos, const glm::fvec4& colourKernel) {
	pixels[ (pos.y * width) + pos.x ].fromKernelDithered(colourKernel,pos);
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void* ReferenceTexture<PixelType,fmt>::getRawPixels() {
	return pixels.data();
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline const void* ReferenceTexture<PixelType,fmt>::getRawPixels() const {
	return pixels.data();
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::clearToColour(const glm::fvec4& colourKernel) {
	PixelType p;
	p.fromKernel(colourKernel);
	for(int y = 0; y < height; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			scanline[x] = p;
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::clearToColour(const ColourProgrammer& program) {
	for(int y = 0; y < height; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			scanline[x].fromKernelDithered(program(pos),pos);
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::clearToColour(const ColourProgrammer2& program) {
	for(int y = 0; y < height; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			glm::fvec4 kernel;
			scanline[x].toKernel(kernel);
			scanline[x].fromKernelDithered(program(pos,kernel),pos);
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::clearToColour(const ColourProgrammer3& program)
{
	const glm::fvec2 reciprocal = glm::fvec2( 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height) );
	for(int y = 0; y < height; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			glm::fvec2 fpos = glm::fvec2(static_cast<float>(x) * reciprocal.x, static_cast<float>(y) * reciprocal.y);
			scanline[x].fromKernelDithered(program(fpos),pos);
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::clearToColour(const ColourProgrammer4& program)
{
	const glm::fvec2 reciprocal = glm::fvec2( 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height) );
	for(int y = 0; y < height; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			glm::fvec2 fpos = glm::fvec2(static_cast<float>(x) * reciprocal.x, static_cast<float>(y) * reciprocal.y);
			glm::fvec4 kernel;
			scanline[x].toKernel(kernel);
			scanline[x].fromKernelDithered(program(fpos,kernel),pos);
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::iterateOverPixels(const ColourIterator& program) const
{
	for(int y = 0; y < height; ++y) {
		const PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			glm::fvec4 kernel;
			scanline[x].toKernel(kernel);
			program(pos,kernel);
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::iterateOverPixels(const ColourIterator2& program) const
{
	const glm::fvec2 reciprocal = glm::fvec2( 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height) );
	for(int y = 0; y < height; ++y) {
		const PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::fvec2 fpos = glm::fvec2(static_cast<float>(x) * reciprocal.x, static_cast<float>(y) * reciprocal.y);
			glm::fvec4 kernel;
			scanline[x].toKernel(kernel);
			program(fpos,kernel);
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt> StandardTexture<PixelType,fmt>::StandardTexture(const ReferenceTexture<PixelType,fmt>& cpy) {
	this->pixels.resize(cpy.pixels.size());
	std::memcpy(this->pixels.data(),cpy.pixels.data(),cpy.pixels.size() * sizeof(PixelType));
	this->width = cpy.width;
	this->height = cpy.height;
	this->stride = cpy.stride;
	this->widthF = cpy.widthF;
	this->heightF = cpy.heightF;
	this->widthR = cpy.widthR;
	this->heightR = cpy.heightR;
}
template <typename PixelType, MH33::GFX::TextureFormat fmt> StandardTexture<PixelType,fmt>& StandardTexture<PixelType,fmt>::operator=(const ReferenceTexture<PixelType,fmt>& cpy) {
	this->pixels.resize(cpy.pixels.size());
	std::memcpy(this->pixels.data(),cpy.pixels.data(),cpy.pixels.size() * sizeof(PixelType));
	this->width = cpy.width;
	this->height = cpy.height;
	this->stride = cpy.stride;
	this->widthF = cpy.widthF;
	this->heightF = cpy.heightF;
	this->widthR = cpy.widthR;
	this->heightR = cpy.heightR;
	return *this;
}
template <typename PixelType, MH33::GFX::TextureFormat fmt> ReferenceTexture<PixelType,fmt>::ReferenceTexture(const StandardTexture<PixelType,fmt>& cpy) {
	this->pixels = cpy.pixels;
	this->width = cpy.width;
	this->height = cpy.height;
	this->stride = cpy.stride;
	this->widthF = cpy.widthF;
	this->heightF = cpy.heightF;
	this->widthR = cpy.widthR;
	this->heightR = cpy.heightR;
}
template <typename PixelType, MH33::GFX::TextureFormat fmt> ReferenceTexture<PixelType,fmt>& ReferenceTexture<PixelType,fmt>::operator=(const StandardTexture<PixelType,fmt>& cpy) {
	this->pixels = cpy.pixels;
	this->width = cpy.width;
	this->height = cpy.height;
	this->stride = cpy.stride;
	this->widthF = cpy.widthF;
	this->heightF = cpy.heightF;
	this->widthR = cpy.widthR;
	this->heightR = cpy.heightR;
	return *this;
}

#endif // STANDARDTEXTURE_IPP
