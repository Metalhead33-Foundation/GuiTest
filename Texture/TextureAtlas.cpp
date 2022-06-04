#include "TextureAtlas.hpp"

Texture*TextureAtlas::getTexture() const
{
	return texture.get();
}

void TextureAtlas::expand(const glm::ivec2& byHowManyBlocks)
{
	if(!byHowManyBlocks.x || !byHowManyBlocks.y) return;
	glm::ivec2 newSize = blockCount + byHowManyBlocks;
	std::vector<bool> newOccupancy(newSize.x * newSize.y);
	std::fill(std::begin(newOccupancy),std::end(newOccupancy),false);
	for(int y = 0; y < blockCount.y; ++y) {
		const int originalLineStart = blockCount.x * y;
		const int newLineStart = newSize.x * y;
		for(int x = 0; x < blockCount.x; ++x) {
			newOccupancy[newLineStart + x] = blockOccupancy[originalLineStart + x];
		}
	}
	texture->resize(newSize.x * blockSize.x, newSize.y * blockSize.y);
	blockCount = newSize;
	blockOccupancy = std::move(newOccupancy);

}

void TextureAtlas::freeBlock(const glm::ivec2& blockIndex)
{
	blockOccupancy[(blockCount.x * blockIndex.y) + blockIndex.x] = false;
}

void TextureAtlas::occupyBlock(const glm::ivec2& blockIndex)
{
	blockOccupancy[(blockCount.x * blockIndex.y) + blockIndex.x] = true;
}

void TextureAtlas::freeBlocks(const glm::ivec2& offset, const glm::ivec2& size)
{
	for(int y = 0; y < size.y; ++y) {
		const int lineStart = (blockCount.x * ( offset.y + y )) + offset.x;
		for(int x = 0; x < size.x; ++x) {
			blockOccupancy[lineStart + x] = false;
		}
	}
}

void TextureAtlas::occupyBlocks(const glm::ivec2& blockIndex, const glm::ivec2& size)
{
	for(int y = 0; y < size.y; ++y) {
		const int lineStart = (blockCount.x * ( blockIndex.y + y )) + blockIndex.x;
		for(int x = 0; x < size.x; ++x) {
			blockOccupancy[lineStart + x] = true;
		}
	}
}

glm::ivec2 TextureAtlas::blockTopLeft(const glm::ivec2& offset) const
{
	return offset * blockSize;
}

glm::ivec2 TextureAtlas::calculateBlockCount(const glm::ivec2& size) const
{
	return glm::ivec2(
				(size.x % blockSize.x) ? ((size.x / blockSize.x) + 1) : (size.x / blockSize.x),
				(size.y % blockSize.y) ? ((size.y / blockSize.y) + 1) : (size.y / blockSize.y)
										 );
}

glm::ivec2 TextureAtlas::findFirstFreeBlock(const glm::ivec2& blockToAllocate)
{
	// Early return, if we need way more blocks than this.
	if(blockToAllocate.x > blockCount.x || blockToAllocate.y > blockCount.y) return glm::ivec2(-1,-1);
	// Early return, if we need way more blocks than this.
	for(int y = 0; y < blockCount.y; ++y) {
		const int lineStart = blockCount.x * y;
		for(int x = 0; x < blockCount.x; ++x) {
			// If the block isn't free, just keep going
			if(blockOccupancy[lineStart + x]) continue;
			// We gotta skip the line altogether if there is not enough space from this offset
			const glm::ivec2 topLeft(x,y);
			const glm::ivec2 bottomRight = topLeft + blockToAllocate;
			if(bottomRight.x > blockCount.x || bottomRight.y > blockCount.y) break;
			// Okay, now we iterate through to see if there is a continuous free block
			bool freeBlock = true;
			for(int y2 = 0; y2 < blockToAllocate.y; ++y2) {
				const int lineStart2 = blockCount.x * (y + y2);
				for(int x2 = 0; x2 < blockToAllocate.x; ++x2) {
					freeBlock = freeBlock && !blockOccupancy[lineStart2 + x + x2];
					if(!freeBlock) break;
				}
				if(!freeBlock) break;
			}
			if(freeBlock) return glm::ivec2(x,y);
		}
	}
	return glm::ivec2(-1,-1);
}

TextureAtlas::TextureAtlas(std::unique_ptr<Texture>&& tex, const glm::ivec2& blockSize, const glm::ivec2& blockCount)
	: texture(std::move(tex)), blockSize(blockSize), blockCount(blockCount), blockOccupancy(blockCount.x * blockCount.y)
{
	const glm::ivec2 totalSize = blockSize * blockCount;
	if(texture->getWidth() != totalSize.x || texture->getHeight() != totalSize.y) {
		texture->resize(totalSize.x,totalSize.y);
	}
	std::fill(std::begin(blockOccupancy),std::end(blockOccupancy),false);
}

TextureAtlas::TextureAtlas(const TextureCreator& creatorFunction, const glm::ivec2& blockSize, const glm::ivec2& blockCount)
	: texture(creatorFunction(blockSize * blockCount)), blockSize(blockSize), blockCount(blockCount), blockOccupancy(blockCount.x * blockCount.y)
{
	std::fill(std::begin(blockOccupancy),std::end(blockOccupancy),false);
}

TextureAtlasSubsection* TextureAtlas::allocateBlocks(const Texture& textureToInsert)
{
	const glm::ivec2 textureSize(textureToInsert.getWidth(),textureToInsert.getHeight());
	const glm::ivec2 neededBlocks = calculateBlockCount(textureSize);
	glm::ivec2 topLeftBlockIndex= findFirstFreeBlock(neededBlocks);
	if(topLeftBlockIndex.x == -1) {
		expand(neededBlocks);
		topLeftBlockIndex = findFirstFreeBlock(neededBlocks);
		if(topLeftBlockIndex.x == -1) {
			return nullptr;
		}
	}
	const glm::ivec2 topLeftCoordinate = topLeftBlockIndex * blockSize;
	occupyBlocks(topLeftBlockIndex,neededBlocks);
	texture->blit(textureToInsert,topLeftCoordinate);
	return new TextureAtlasSubsection(this,topLeftBlockIndex,neededBlocks,topLeftBlockIndex * blockSize, textureSize.x, textureSize.y);
}


TextureAtlasSubsection::TextureAtlasSubsection(TextureAtlas* atlas, const glm::ivec2 topLeftBlockIndex, const glm::ivec2 occupiedBlocks, const glm::ivec2 topLeft, int width, int height)
	: atlas(atlas), topLeftBlockIndex(topLeftBlockIndex), occupiedBlocks(occupiedBlocks), topLeft(topLeft), width(width), height(height), widthF(width-1), heightF(height-1),
	  widthR(1.0f / static_cast<float>(width)), heightR(1.0f / static_cast<float>(height)), stride(0)
{

}

TextureAtlasSubsection::~TextureAtlasSubsection()
{
	atlas->freeBlocks(topLeftBlockIndex,occupiedBlocks);
}

int TextureAtlasSubsection::getWidth() const
{
	return width;
}

float TextureAtlasSubsection::getWidthF() const
{
	return widthF;
}

float TextureAtlasSubsection::getWidthR() const
{
	return widthR;
}

int TextureAtlasSubsection::getHeight() const
{
	return height;
}

float TextureAtlasSubsection::getHeightF() const
{
	return heightF;
}

int TextureAtlasSubsection::getStride() const
{
	return stride;
}

float TextureAtlasSubsection::getHeightR() const
{
	return heightR;
}

bool TextureAtlasSubsection::resize(int newWidth, int newHeight)
{
	(void)newWidth;
	(void)newHeight;
	return false;
}

void TextureAtlasSubsection::getPixel(const glm::ivec2& pos, glm::fvec4& colourKernel, Wrap wrap) const
{
	int x = 0;
	int y = 0;
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
		x = std::clamp(pos.x,0,width);
		y = std::clamp(pos.y,0,height);
	default: break;
	}
	atlas->texture->getPixel(glm::ivec2(x,y) + topLeft,colourKernel,CLAMP_TO_EDGE);
}

void TextureAtlasSubsection::setPixel(const glm::ivec2& pos, const glm::fvec4& colourKernel)
{
	atlas->texture->setPixel(pos + topLeft,colourKernel);
}

void TextureAtlasSubsection::setPixelDithered(const glm::ivec2& pos, const glm::fvec4& colourKernel)
{
	atlas->texture->setPixelDithered(pos + topLeft,colourKernel);
}

void* TextureAtlasSubsection::getRawPixels()
{
	return nullptr;
}

const void* TextureAtlasSubsection::getRawPixels() const
{
	return nullptr;
}

void TextureAtlasSubsection::clearToColour(const glm::fvec4& colourKernel)
{
	(void)colourKernel;
}

void TextureAtlasSubsection::clearToColour(const ColourProgrammer& program)
{
	(void)program;
}

void TextureAtlasSubsection::clearToColour(const ColourProgrammer2& program)
{
	(void)program;
}

void TextureAtlasSubsection::clearToColour(const ColourProgrammer3& program)
{
	(void)program;
}

void TextureAtlasSubsection::clearToColour(const ColourProgrammer4& program)
{
	(void)program;
}

void TextureAtlasSubsection::iterateOverPixels(const ColourIterator& program) const
{
	for(int y = 0; y < height; ++y) {
		for(int x = 0; x < width; ++x) {
			const glm::ivec2 pos(x,y);
			glm::fvec4 kernel;
			getPixel(pos,kernel,CLAMP_TO_BORDER);
			program(pos,kernel);
		}
	}
}

void TextureAtlasSubsection::iterateOverPixels(const ColourIterator2& program) const
{
	for(int y = 0; y < height; ++y) {
		for(int x = 0; x < width; ++x) {
			const glm::ivec2 pos(x,y);
			glm::fvec4 kernel;
			getPixel(pos,kernel,CLAMP_TO_BORDER);
			program(glm::fvec2(static_cast<float>(x)*widthR,static_cast<float>(y)*heightR),kernel);
		}
	}
}
