#ifndef TEXTUREATLAS_HPP
#define TEXTUREATLAS_HPP
#include "Texture.hpp"

namespace SoftwareRenderer {
class TextureAtlas;

class TextureAtlasSubsection : public Texture {
public:
	friend class TextureAtlas;

private:
	TextureAtlas* atlas;
	glm::ivec2 topLeftBlockIndex;
	glm::ivec2 occupiedBlocks;
	glm::ivec2 topLeft;
	glm::ivec2 bottomRight;
	int width,height,stride;
	float widthF,heightF,widthR,heightR;
	TextureAtlasSubsection(TextureAtlas* atlas, const glm::ivec2 topLeftBlockIndex, const glm::ivec2 occupiedBlocks, const glm::ivec2 topLeft, int width, int height);
public:
	~TextureAtlasSubsection() override;
	int getWidth() const override;
	float getWidthF() const override;
	float getWidthR() const override;
	int getHeight() const override;
	float getHeightF() const override;
	int getStride() const override;
	float getHeightR() const override;
	bool resize(int newWidth, int newHeight) override;
	void getPixel(const glm::ivec2& pos, glm::fvec4& colourKernel, Wrap wrap) const override;
	void setPixel(const glm::ivec2& pos, const glm::fvec4& colourKernel) override;
	void setPixelDithered(const glm::ivec2& pos, const glm::fvec4& colourKernel) override;
	void* getRawPixels() override;
	const void* getRawPixels() const override;
	void clearToColour(const glm::fvec4& colourKernel) override;
	void clearToColour(const ColourProgrammer& program) override;
	void clearToColour(const ColourProgrammer2& program) override;
	void clearToColour(const ColourProgrammer3& program) override;
	void clearToColour(const ColourProgrammer4& program) override;
	void iterateOverPixels(const ColourIterator& program) const override;
	void iterateOverPixels(const ColourIterator2& program) const override;
};

class TextureAtlas
{
public:
	friend class TextureAtlasSubsection;
	typedef std::function<Texture*(const glm::ivec2&)> TextureCreator;
private:
	std::unique_ptr<Texture> texture;
	glm::ivec2 blockSize;
	glm::ivec2 blockCount;
	std::vector<bool> blockOccupancy;
	void expand(const glm::ivec2& byHowManyBlocks);
	void freeBlock(const glm::ivec2& blockIndex);
	void occupyBlock(const glm::ivec2& blockIndex);
	void freeBlocks(const glm::ivec2& offset, const glm::ivec2& size);
	void occupyBlocks(const glm::ivec2& blockIndex, const glm::ivec2& size);
	glm::ivec2 blockTopLeft(const glm::ivec2& offset) const;
	glm::ivec2 calculateBlockCount(const glm::ivec2& size) const;
	glm::ivec2 findFirstFreeBlock(const glm::ivec2& blockToAllocate);
public:
	TextureAtlas(std::unique_ptr<Texture>&& tex, const glm::ivec2& blockSize, const glm::ivec2& blockCount);
	TextureAtlas(const TextureCreator& creatorFunction, const glm::ivec2& blockSize, const glm::ivec2& blockCount);
	TextureAtlasSubsection* allocateBlocks(const Texture& textureToInsert);
	Texture* getTexture() const;
};
}

#endif // TEXTUREATLAS_HPP
