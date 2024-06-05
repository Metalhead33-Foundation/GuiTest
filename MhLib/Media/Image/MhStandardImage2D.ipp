template <MH33::Util::PixelConcept PixelType, Format fmt> class StandardImage2D;
template <MH33::Util::PixelConcept PixelType, Format fmt> class ReferenceImage2D;

template <MH33::Util::PixelConcept PixelType, Format fmt> class StandardImage2D : public Image2D {
private:
	std::vector<PixelType> pixels;
	int width,height,stride;
	float widthF,heightF,widthR,heightR;
public:
	friend class ReferenceImage2D<PixelType,fmt>;
	virtual Format getFormat() const override { return fmt; }
	StandardImage2D(const StandardImage2D& cpy);
	StandardImage2D(StandardImage2D&& mov);
	StandardImage2D& operator=(const StandardImage2D& cpy);
	StandardImage2D& operator=(StandardImage2D&& mov);
	StandardImage2D(int width, int height);
	StandardImage2D(const PixelType* pixelsToCopy, int width, int height);
	StandardImage2D(const std::span<PixelType> pixelsToCopy, int width, int height);
	StandardImage2D(std::vector<PixelType>&& mov, int width, int height);
	StandardImage2D(const ReferenceImage2D<PixelType,fmt>& cpy);
	StandardImage2D& operator=(const ReferenceImage2D<PixelType,fmt>& cpy);
	bool resize(int newWidth, int newHeight) override;
	unsigned getWidth() const override { return width; }
	float getWidthF() const override { return widthF; }
	float getWidthR() const override { return widthR; }
	unsigned getHeight() const override { return height; }
	float getHeightF() const override { return heightF; }
	float getHeightR() const override { return heightR; }
	unsigned getStride() const override { return stride; }
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
	void clearToColour(const glm::fvec4& colourKernel, const glm::ivec2 offset, const glm::ivec2& dimensions) override;
	void clearToColour(const ColourProgrammer& program, const glm::ivec2 offset, const glm::ivec2& dimensions) override;
	void clearToColour(const ColourProgrammer2& program, const glm::ivec2 offset, const glm::ivec2& dimensions) override;
	void clearToColour(const ColourProgrammer3& program, const glm::ivec2 offset, const glm::ivec2& dimensions) override;
	void clearToColour(const ColourProgrammer4& program, const glm::ivec2 offset, const glm::ivec2& dimensions) override;
	void iterateOverPixels(const ColourIterator& program) const override;
	void iterateOverPixels(const ColourIterator2& program) const override;
	void iterateOverPixels(const ColourIterator& program, const glm::ivec2 offset, const glm::ivec2& dimensions) const override;
	void iterateOverPixels(const ColourIterator2& program, const glm::ivec2 offset, const glm::ivec2& dimensions) const override;
	void blit(const std::span<const std::byte>& data, Format format, const glm::ivec2 offset, const glm::ivec2& dimensions) override
	{
		if(format != fmt) return;
		int linesToCopy = std::min(dimensions.y,height - offset.y);
		int pixelsPerLines = std::min(dimensions.x,width - offset.x);
		if(linesToCopy <= 0 || pixelsPerLines <= 0) return;
		const PixelType * const inPixels = reinterpret_cast<const PixelType*>(data.data());
		for(int y = 0; y < linesToCopy; ++y) {
			PixelType * const destLinePtr = &(pixels[(offset.y + y) * width + offset.x]);
			const PixelType * const inPixelLine = &(inPixels[y * dimensions.x]);
			memcpy(destLinePtr,inPixelLine,pixelsPerLines * sizeof(PixelType));
		}
	}
};
typedef StandardImage2D<MH33::Util::PixelGreyscale_U8, Format::R8U> ImgGreyscale_U8;
typedef StandardImage2D<MH33::Util::PixelGreyscale_U16, Format::R16U> ImgGreyscale_U16;
typedef StandardImage2D<MH33::Util::PixelGreyscale_U32, Format::R32U> ImgGreyscale_U32;
typedef StandardImage2D<MH33::Util::PixelRG_U8, Format::RG8U> ImgRG_U8;
typedef StandardImage2D<MH33::Util::PixelRG_U16, Format::RG16U> ImgRG_U16;
typedef StandardImage2D<MH33::Util::PixelRG_U32, Format::RG32U> ImgRG_U32;
typedef StandardImage2D<MH33::Util::PixelRGB_U8, Format::RGB8U> ImgRGB_U8;
typedef StandardImage2D<MH33::Util::PixelRGB_U16, Format::RGB16U> ImgRGB_U16;
typedef StandardImage2D<MH33::Util::PixelRGB_U32, Format::RGB32U> ImgRGB_U32;
typedef StandardImage2D<MH33::Util::PixelBGR_U8, Format::BGR8U> ImgBGR_U8;
typedef StandardImage2D<MH33::Util::PixelBGR_U16, Format::BGR16U> ImgBGR_U16;
typedef StandardImage2D<MH33::Util::PixelBGR_U32, Format::BGR32U> ImgBGR_U32;
typedef StandardImage2D<MH33::Util::PixelRGBA_U8, Format::RGBA8U> ImgRGBA_U8;
typedef StandardImage2D<MH33::Util::PixelRGBA_U16, Format::RGBA16U> ImgRGBA_U16;
typedef StandardImage2D<MH33::Util::PixelRGBA_U32, Format::RGBA32U> ImgRGBA_U32;
typedef StandardImage2D<MH33::Util::PixelBGRA_U8, Format::BGRA8U> ImgBGRA_U8;
typedef StandardImage2D<MH33::Util::PixelBGRA_U16, Format::BGRA16U> ImgBGRA_U16;
typedef StandardImage2D<MH33::Util::PixelBGRA_U32, Format::BGRA32U> ImgBGRA_U32;
typedef StandardImage2D<MH33::Util::PixelARGB_U8, Format::ARGB8U> ImgARGB_U8;
typedef StandardImage2D<MH33::Util::PixelARGB_U16, Format::ARGB16U> ImgARGB_U16;
typedef StandardImage2D<MH33::Util::PixelARGB_U32, Format::ARGB32U> ImgARGB_U32;
typedef StandardImage2D<MH33::Util::PixelABGR_U8, Format::INVALID> ImgABGR_U8;
typedef StandardImage2D<MH33::Util::PixelABGR_U16, Format::INVALID> ImgABGR_U16;
typedef StandardImage2D<MH33::Util::PixelABGR_U32, Format::INVALID> ImgABGR_U32;
typedef StandardImage2D<MH33::Util::PixelGreyscale_S8, Format::R8S> ImgGreyscale_S8;
typedef StandardImage2D<MH33::Util::PixelGreyscale_S16, Format::R16S> ImgGreyscale_S16;
typedef StandardImage2D<MH33::Util::PixelGreyscale_S32, Format::R32S> ImgGreyscale_S32;
typedef StandardImage2D<MH33::Util::PixelRG_S8, Format::RG8S> ImgRG_S8;
typedef StandardImage2D<MH33::Util::PixelRG_S16, Format::RG16S> ImgRG_S16;
typedef StandardImage2D<MH33::Util::PixelRG_S32, Format::RG32S> ImgRG_S32;
typedef StandardImage2D<MH33::Util::PixelRGB_S8, Format::RGB8S> ImgRGB_S8;
typedef StandardImage2D<MH33::Util::PixelRGB_S16, Format::RGB16S> ImgRGB_S16;
typedef StandardImage2D<MH33::Util::PixelRGB_S32, Format::RGB32S> ImgRGB_S32;
typedef StandardImage2D<MH33::Util::PixelBGR_S8, Format::BGR8S> ImgBGR_S8;
typedef StandardImage2D<MH33::Util::PixelBGR_S16, Format::BGR16S> ImgBGR_S16;
typedef StandardImage2D<MH33::Util::PixelBGR_S32, Format::BGR32S> ImgBGR_S32;
typedef StandardImage2D<MH33::Util::PixelRGBA_S8, Format::RGBA8S> ImgRGBA_S8;
typedef StandardImage2D<MH33::Util::PixelRGBA_S16, Format::RGBA16S> ImgRGBA_S16;
typedef StandardImage2D<MH33::Util::PixelRGBA_S32, Format::RGBA32S> ImgRGBA_S32;
typedef StandardImage2D<MH33::Util::PixelBGRA_S8, Format::BGRA8S> ImgBGRA_S8;
typedef StandardImage2D<MH33::Util::PixelBGRA_S16, Format::BGRA16S> ImgBGRA_S16;
typedef StandardImage2D<MH33::Util::PixelBGRA_S32, Format::BGRA32S> ImgBGRA_S32;
typedef StandardImage2D<MH33::Util::PixelARGB_S8, Format::ARGB8S> ImgARGB_S8;
typedef StandardImage2D<MH33::Util::PixelARGB_S16, Format::ARGB16S> ImgARGB_S16;
typedef StandardImage2D<MH33::Util::PixelARGB_S32, Format::ARGB32S> ImgARGB_S32;
typedef StandardImage2D<MH33::Util::PixelABGR_S8, Format::INVALID> ImgABGR_S8;
typedef StandardImage2D<MH33::Util::PixelABGR_S16, Format::INVALID> ImgABGR_S16;
typedef StandardImage2D<MH33::Util::PixelABGR_S32, Format::INVALID> ImgABGR_S32;
typedef StandardImage2D<MH33::Util::PixelRGB332, Format::INVALID> ImgRGB332;
typedef StandardImage2D<MH33::Util::PixelRGB444, Format::RGB444> ImgRGB444;
typedef StandardImage2D<MH33::Util::PixelRGB555, Format::RGB555> ImgRGB555;
typedef StandardImage2D<MH33::Util::PixelRGB565, Format::RGB565> ImgRGB565;
typedef StandardImage2D<MH33::Util::PixelBGR555, Format::INVALID> ImgBGR555;
typedef StandardImage2D<MH33::Util::PixelBGR565, Format::INVALID> ImgBGR565;
typedef StandardImage2D<MH33::Util::PixelARGB4444, Format::INVALID> ImgARGB4444;
typedef StandardImage2D<MH33::Util::PixelBGRA4444, Format::INVALID> ImgBGRA4444;
typedef StandardImage2D<MH33::Util::PixelRGBA4444, Format::RGBA444> ImgRGBA4444;
typedef StandardImage2D<MH33::Util::PixelABGR4444, Format::INVALID> ImgABGR4444;
typedef StandardImage2D<MH33::Util::PixelARGB1555, Format::INVALID> ImgARGB1555;
typedef StandardImage2D<MH33::Util::PixelRGBA5551, Format::INVALID> ImgRGBA5551;
typedef StandardImage2D<MH33::Util::PixelABGR1555, Format::INVALID> ImgABGR1555;
typedef StandardImage2D<MH33::Util::PixelBGRA5551, Format::INVALID> ImgBGRA5551;
typedef StandardImage2D<MH33::Util::PixelRGBX8888, Format::INVALID> ImgRGBX8888;
typedef StandardImage2D<MH33::Util::PixelBGRX8888, Format::INVALID> ImgBGRX8888;
typedef StandardImage2D<MH33::Util::PixelRGB888, Format::RGB8U> ImgRGB888;
typedef StandardImage2D<MH33::Util::PixelBGR888, Format::BGR8U> ImgBGR888;
typedef StandardImage2D<MH33::Util::PixelARGB8888, Format::ARGB8U> ImgARGB8888;
typedef StandardImage2D<MH33::Util::PixelRGBA8888, Format::RGBA8U> ImgRGBA8888;
typedef StandardImage2D<MH33::Util::PixelABGR8888, Format::INVALID> ImgABGR8888;
typedef StandardImage2D<MH33::Util::PixelBGRA8888, Format::BGRA8U> ImgBGRA8888;

template <MH33::Util::PixelConcept PixelType, Format fmt> class ReferenceImage2D : public Image2D {
private:
	std::span<PixelType> pixels;
	int width,height,stride;
	float widthF,heightF,widthR,heightR;
public:
	friend class StandardImage2D<PixelType,fmt>;
	virtual Format getFormat() const override { return fmt; }
	ReferenceImage2D(const ReferenceImage2D& cpy);
	ReferenceImage2D& operator=(const ReferenceImage2D& cpy);
	ReferenceImage2D(PixelType* pixelsPointing, int width, int height);
	ReferenceImage2D(const std::span<PixelType> pixelsToCopy, int width, int height);
	ReferenceImage2D(std::span<PixelType>&& mov, int width, int height);
	ReferenceImage2D(const StandardImage2D<PixelType,fmt>& cpy);
	ReferenceImage2D& operator=(const StandardImage2D<PixelType,fmt>& cpy);
	bool resize(int newWidth, int newHeight) override;
	unsigned getWidth() const override { return width; }
	float getWidthF() const override { return widthF; }
	float getWidthR() const override { return widthR; }
	unsigned getHeight() const override { return height; }
	float getHeightF() const override { return heightF; }
	float getHeightR() const override { return heightR; }
	unsigned getStride() const override { return stride; }
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
	void clearToColour(const glm::fvec4& colourKernel, const glm::ivec2 offset, const glm::ivec2& dimensions) override;
	void clearToColour(const ColourProgrammer& program, const glm::ivec2 offset, const glm::ivec2& dimensions) override;
	void clearToColour(const ColourProgrammer2& program, const glm::ivec2 offset, const glm::ivec2& dimensions) override;
	void clearToColour(const ColourProgrammer3& program, const glm::ivec2 offset, const glm::ivec2& dimensions) override;
	void clearToColour(const ColourProgrammer4& program, const glm::ivec2 offset, const glm::ivec2& dimensions) override;
	void iterateOverPixels(const ColourIterator& program) const override;
	void iterateOverPixels(const ColourIterator2& program) const override;
	void iterateOverPixels(const ColourIterator& program, const glm::ivec2 offset, const glm::ivec2& dimensions) const override;
	void iterateOverPixels(const ColourIterator2& program, const glm::ivec2 offset, const glm::ivec2& dimensions) const override;
	void blit(const std::span<const std::byte>& data, Format format, const glm::ivec2 offset, const glm::ivec2& dimensions) override
	{
		if(format != fmt) return;
		int linesToCopy = std::min(dimensions.y,height - offset.y);
		int pixelsPerLines = std::min(dimensions.x,width - offset.x);
		if(linesToCopy <= 0 || pixelsPerLines <= 0) return;
		const PixelType * const inPixels = reinterpret_cast<const PixelType*>(data.data());
		for(int y = 0; y < linesToCopy; ++y) {
			PixelType * const destLinePtr = &(pixels[(offset.y + y) * width + offset.x]);
			const PixelType * const inPixelLine = &(inPixels[y * dimensions.x]);
			memcpy(destLinePtr,inPixelLine,pixelsPerLines * sizeof(PixelType));
		}
	}
};
typedef ReferenceImage2D<MH33::Util::PixelGreyscale_U8, Format::R8U> RefImgGreyscale_U8;
typedef ReferenceImage2D<MH33::Util::PixelGreyscale_U16, Format::R16U> RefImgGreyscale_U16;
typedef ReferenceImage2D<MH33::Util::PixelGreyscale_U32, Format::R32U> RefImgGreyscale_U32;
typedef ReferenceImage2D<MH33::Util::PixelRG_U8, Format::RG8U> RefImgRG_U8;
typedef ReferenceImage2D<MH33::Util::PixelRG_U16, Format::RG16U> RefImgRG_U16;
typedef ReferenceImage2D<MH33::Util::PixelRG_U32, Format::RG32U> RefImgRG_U32;
typedef ReferenceImage2D<MH33::Util::PixelRGB_U8, Format::RGB8U> RefImgRGB_U8;
typedef ReferenceImage2D<MH33::Util::PixelRGB_U16, Format::RGB16U> RefImgRGB_U16;
typedef ReferenceImage2D<MH33::Util::PixelRGB_U32, Format::RGB32U> RefImgRGB_U32;
typedef ReferenceImage2D<MH33::Util::PixelBGR_U8, Format::BGR8U> RefImgBGR_U8;
typedef ReferenceImage2D<MH33::Util::PixelBGR_U16, Format::BGR16U> RefImgBGR_U16;
typedef ReferenceImage2D<MH33::Util::PixelBGR_U32, Format::BGR32U> RefImgBGR_U32;
typedef ReferenceImage2D<MH33::Util::PixelRGBA_U8, Format::RGBA8U> RefImgRGBA_U8;
typedef ReferenceImage2D<MH33::Util::PixelRGBA_U16, Format::RGBA16U> RefImgRGBA_U16;
typedef ReferenceImage2D<MH33::Util::PixelRGBA_U32, Format::RGBA32U> RefImgRGBA_U32;
typedef ReferenceImage2D<MH33::Util::PixelBGRA_U8, Format::BGRA8U> RefImgBGRA_U8;
typedef ReferenceImage2D<MH33::Util::PixelBGRA_U16, Format::BGRA16U> RefImgBGRA_U16;
typedef ReferenceImage2D<MH33::Util::PixelBGRA_U32, Format::BGRA32U> RefImgBGRA_U32;
typedef ReferenceImage2D<MH33::Util::PixelARGB_U8, Format::ARGB8U> RefImgARGB_U8;
typedef ReferenceImage2D<MH33::Util::PixelARGB_U16, Format::ARGB16U> RefImgARGB_U16;
typedef ReferenceImage2D<MH33::Util::PixelARGB_U32, Format::ARGB32U> RefImgARGB_U32;
typedef ReferenceImage2D<MH33::Util::PixelABGR_U8, Format::INVALID> RefImgABGR_U8;
typedef ReferenceImage2D<MH33::Util::PixelABGR_U16, Format::INVALID> RefImgABGR_U16;
typedef ReferenceImage2D<MH33::Util::PixelABGR_U32, Format::INVALID> RefImgABGR_U32;
typedef ReferenceImage2D<MH33::Util::PixelGreyscale_S8, Format::R8S> RefImgGreyscale_S8;
typedef ReferenceImage2D<MH33::Util::PixelGreyscale_S16, Format::R16S> RefImgGreyscale_S16;
typedef ReferenceImage2D<MH33::Util::PixelGreyscale_S32, Format::R32S> RefImgGreyscale_S32;
typedef ReferenceImage2D<MH33::Util::PixelRG_S8, Format::RG8S> RefImgRG_S8;
typedef ReferenceImage2D<MH33::Util::PixelRG_S16, Format::RG16S> RefImgRG_S16;
typedef ReferenceImage2D<MH33::Util::PixelRG_S32, Format::RG32S> RefImgRG_S32;
typedef ReferenceImage2D<MH33::Util::PixelRGB_S8, Format::RGB8S> RefImgRGB_S8;
typedef ReferenceImage2D<MH33::Util::PixelRGB_S16, Format::RGB16S> RefImgRGB_S16;
typedef ReferenceImage2D<MH33::Util::PixelRGB_S32, Format::RGB32S> RefImgRGB_S32;
typedef ReferenceImage2D<MH33::Util::PixelBGR_S8, Format::BGR8S> RefImgBGR_S8;
typedef ReferenceImage2D<MH33::Util::PixelBGR_S16, Format::BGR16S> RefImgBGR_S16;
typedef ReferenceImage2D<MH33::Util::PixelBGR_S32, Format::BGR32S> RefImgBGR_S32;
typedef ReferenceImage2D<MH33::Util::PixelRGBA_S8, Format::RGBA8S> RefImgRGBA_S8;
typedef ReferenceImage2D<MH33::Util::PixelRGBA_S16, Format::RGBA16S> RefImgRGBA_S16;
typedef ReferenceImage2D<MH33::Util::PixelRGBA_S32, Format::RGBA32S> RefImgRGBA_S32;
typedef ReferenceImage2D<MH33::Util::PixelBGRA_S8, Format::BGRA8S> RefImgBGRA_S8;
typedef ReferenceImage2D<MH33::Util::PixelBGRA_S16, Format::BGRA16S> RefImgBGRA_S16;
typedef ReferenceImage2D<MH33::Util::PixelBGRA_S32, Format::BGRA32S> RefImgBGRA_S32;
typedef ReferenceImage2D<MH33::Util::PixelARGB_S8, Format::ARGB8S> RefImgARGB_S8;
typedef ReferenceImage2D<MH33::Util::PixelARGB_S16, Format::ARGB16S> RefImgARGB_S16;
typedef ReferenceImage2D<MH33::Util::PixelARGB_S32, Format::ARGB32S> RefImgARGB_S32;
typedef ReferenceImage2D<MH33::Util::PixelABGR_S8, Format::INVALID> RefImgABGR_S8;
typedef ReferenceImage2D<MH33::Util::PixelABGR_S16, Format::INVALID> RefImgABGR_S16;
typedef ReferenceImage2D<MH33::Util::PixelABGR_S32, Format::INVALID> RefImgABGR_S32;
typedef ReferenceImage2D<MH33::Util::PixelRGB332, Format::INVALID> RefImgRGB332;
typedef ReferenceImage2D<MH33::Util::PixelRGB444, Format::RGB444> RefImgRGB444;
typedef ReferenceImage2D<MH33::Util::PixelRGB555, Format::RGB555> RefImgRGB555;
typedef ReferenceImage2D<MH33::Util::PixelRGB565, Format::RGB565> RefImgRGB565;
typedef ReferenceImage2D<MH33::Util::PixelBGR555, Format::INVALID> RefImgBGR555;
typedef ReferenceImage2D<MH33::Util::PixelBGR565, Format::INVALID> RefImgBGR565;
typedef ReferenceImage2D<MH33::Util::PixelARGB4444, Format::INVALID> RefImgARGB4444;
typedef ReferenceImage2D<MH33::Util::PixelBGRA4444, Format::INVALID> RefImgBGRA4444;
typedef ReferenceImage2D<MH33::Util::PixelRGBA4444, Format::RGBA444> RefImgRGBA4444;
typedef ReferenceImage2D<MH33::Util::PixelABGR4444, Format::INVALID> RefImgABGR4444;
typedef ReferenceImage2D<MH33::Util::PixelARGB1555, Format::INVALID> RefImgARGB1555;
typedef ReferenceImage2D<MH33::Util::PixelRGBA5551, Format::INVALID> RefImgRGBA5551;
typedef ReferenceImage2D<MH33::Util::PixelABGR1555, Format::INVALID> RefImgABGR1555;
typedef ReferenceImage2D<MH33::Util::PixelBGRA5551, Format::INVALID> RefImgBGRA5551;
typedef ReferenceImage2D<MH33::Util::PixelRGBX8888, Format::INVALID> RefImgRGBX8888;
typedef ReferenceImage2D<MH33::Util::PixelBGRX8888, Format::INVALID> RefImgBGRX8888;
typedef ReferenceImage2D<MH33::Util::PixelRGB888, Format::RGB8U> RefImgRGB888;
typedef ReferenceImage2D<MH33::Util::PixelBGR888, Format::BGR8U> RefImgBGR888;
typedef ReferenceImage2D<MH33::Util::PixelARGB8888, Format::ARGB8U> RefImgARGB8888;
typedef ReferenceImage2D<MH33::Util::PixelRGBA8888, Format::RGBA8U> RefImgRGBA8888;
typedef ReferenceImage2D<MH33::Util::PixelABGR8888, Format::INVALID> RefImgABGR8888;
typedef ReferenceImage2D<MH33::Util::PixelBGRA8888, Format::BGRA8U> RefImgBGRA8888;

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline StandardImage2D<PixelType,fmt>::StandardImage2D(const StandardImage2D& cpy)
	: pixels(cpy.pixels), width(cpy.width), height(cpy.height), stride(cpy.stride),
	  widthF(cpy.widthF), heightF(cpy.heightF), widthR(cpy.widthR), heightR(cpy.heightR) {

}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline StandardImage2D<PixelType,fmt>::StandardImage2D(StandardImage2D&& mov)
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

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline StandardImage2D<PixelType,fmt>& StandardImage2D<PixelType,fmt>::operator=(const StandardImage2D& cpy) {
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

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline StandardImage2D<PixelType,fmt>& StandardImage2D<PixelType,fmt>::operator=(StandardImage2D&& mov) {
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

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline StandardImage2D<PixelType,fmt>::StandardImage2D(int width, int height)
	: pixels(width*height), width(width), height(height), stride(width*sizeof(PixelType)),
	  widthF(width-1), heightF(height-1), widthR(1.0f / static_cast<float>(width)), heightR(1.0f / static_cast<float>(height))
{
	std::memset(pixels.data(),0,sizeof(PixelType)*pixels.size());
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline StandardImage2D<PixelType,fmt>::StandardImage2D(const PixelType* pixelsToCopy, int width, int height)
	: pixels(width*height), width(width), height(height), stride(width*sizeof(PixelType)),
	  widthF(width-1), heightF(height-1), widthR(1.0f / static_cast<float>(width)), heightR(1.0f / static_cast<float>(height))
{
	std::memcpy(pixels.data(),pixelsToCopy,sizeof(PixelType)*pixels.size());
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline StandardImage2D<PixelType,fmt>::StandardImage2D(const std::span<PixelType> pixelsToCopy, int width, int height)
	: pixels(width*height), width(width), height(height), stride(width*sizeof(PixelType)),
	  widthF(width-1), heightF(height-1), widthR(1.0f / static_cast<float>(width)), heightR(1.0f / static_cast<float>(height))
{
	std::memcpy(pixels.data(),pixelsToCopy.data(),sizeof(PixelType)*pixels.size());
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline StandardImage2D<PixelType,fmt>::StandardImage2D(std::vector<PixelType>&& mov, int width, int height)
	: pixels(std::move(mov)), width(width), height(height), stride(width*sizeof(PixelType)),
	  widthF(width-1), heightF(height-1), widthR(1.0f / static_cast<float>(width)), heightR(1.0f / static_cast<float>(height))
{

}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline bool StandardImage2D<PixelType,fmt>::resize(int newWidth, int newHeight) {
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

/*template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void StandardImage2D<PixelType,fmt>::blit(const PixelType* cpy, const glm::ivec2 offset, const glm::ivec2& dimensions) {
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

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void StandardImage2D<PixelType,fmt>::blit(const Image2D& cpy, const glm::ivec2 offset, const glm::ivec2& dimensions) {
	const StandardImage2D* standardImage2D = dynamic_cast<const StandardImage2D*>(&cpy);
	if(standardImage2D) blit(standardImage2D->pixels.data(),offset,dimensions);
	else {
		const ReferenceImage2D<PixelType,fmt>* referenceImage2D = dynamic_cast<const ReferenceImage2D<PixelType,fmt>*>(&cpy);
		if(referenceImage2D) blit(referenceImage2D->pixels.data(),offset,dimensions);
		else Image2D::blit(cpy,offset,dimensions);
	}
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void StandardImage2D<PixelType,fmt>::blit(const Image2D& cpy, const glm::ivec2 offset) {
	const StandardImage2D* standardImage2D = dynamic_cast<const StandardImage2D*>(&cpy);
	if(standardImage2D) blit(standardImage2D->pixels.data(),offset,glm::ivec2(standardImage2D->width,standardImage2D->height));
	else {
		const ReferenceImage2D<PixelType,fmt>* referenceImage2D = dynamic_cast<const ReferenceImage2D<PixelType,fmt>*>(&cpy);
		if(referenceImage2D) blit(referenceImage2D->pixels.data(),offset,glm::ivec2(referenceImage2D->width,referenceImage2D->height));
		else Image2D::blit(cpy,offset);
	}
}*/

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void StandardImage2D<PixelType,fmt>::getPixel(const glm::ivec2& pos, glm::fvec4& colourKernel, Wrap wrap) const {
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

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void StandardImage2D<PixelType,fmt>::setPixel(const glm::ivec2& pos, const glm::fvec4& colourKernel) {
	pixels[ (pos.y * width) + pos.x ].fromKernel(colourKernel);
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void StandardImage2D<PixelType,fmt>::setPixelDithered(const glm::ivec2& pos, const glm::fvec4& colourKernel) {
	pixels[ (pos.y * width) + pos.x ].fromKernelDithered(colourKernel,pos);
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void* StandardImage2D<PixelType,fmt>::getRawPixels() {
	return pixels.data();
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline const void* StandardImage2D<PixelType,fmt>::getRawPixels() const {
	return pixels.data();
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void StandardImage2D<PixelType,fmt>::clearToColour(const glm::fvec4& colourKernel) {
	PixelType p;
	p.fromKernel(colourKernel);
	for(int y = 0; y < height; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			scanline[x] = p;
		}
	}
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void StandardImage2D<PixelType,fmt>::clearToColour(const ColourProgrammer& program) {
	for(int y = 0; y < height; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			scanline[x].fromKernelDithered(program(pos),pos);
		}
	}
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void StandardImage2D<PixelType,fmt>::clearToColour(const ColourProgrammer2& program) {
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

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void StandardImage2D<PixelType,fmt>::clearToColour(const ColourProgrammer3& program)
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

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void StandardImage2D<PixelType,fmt>::clearToColour(const ColourProgrammer4& program)
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

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void StandardImage2D<PixelType,fmt>::iterateOverPixels(const ColourIterator& program) const
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

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void StandardImage2D<PixelType,fmt>::iterateOverPixels(const ColourIterator2& program) const
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

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline ReferenceImage2D<PixelType,fmt>::ReferenceImage2D(const ReferenceImage2D& cpy)
	: pixels(cpy.pixels), width(cpy.width), height(cpy.height), stride(cpy.stride),
	  widthF(cpy.widthF), heightF(cpy.heightF), widthR(cpy.widthR), heightR(cpy.heightR)  {

}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline ReferenceImage2D<PixelType,fmt>& ReferenceImage2D<PixelType,fmt>::operator=(const ReferenceImage2D& cpy) {
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

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline ReferenceImage2D<PixelType,fmt>::ReferenceImage2D(PixelType* pixelsPointing, int width, int height)
	: pixels(pixelsPointing,width*height), width(width), height(height), stride(width*sizeof(PixelType)),
	  widthF(width-1), heightF(height-1), widthR(1.0f / static_cast<float>(width)), heightR(1.0f / static_cast<float>(height))
{

}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline ReferenceImage2D<PixelType,fmt>::ReferenceImage2D(const std::span<PixelType> pixelsToCopy, int width, int height)
	: pixels(pixelsToCopy), width(width), height(height), stride(width*sizeof(PixelType)),
	  widthF(width-1), heightF(height-1), widthR(1.0f / static_cast<float>(width)), heightR(1.0f / static_cast<float>(height))
{

}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline ReferenceImage2D<PixelType,fmt>::ReferenceImage2D(std::span<PixelType>&& mov, int width, int height)
	: pixels(std::move(mov)), width(width), height(height), stride(width*sizeof(PixelType)),
	  widthF(width-1), heightF(height-1), widthR(1.0f / static_cast<float>(width)), heightR(1.0f / static_cast<float>(height))
{

}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline bool ReferenceImage2D<PixelType,fmt>::resize(int newWidth, int newHeight) {
	(void)newWidth;
	(void)newHeight;
	return false;
}

/*template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void ReferenceImage2D<PixelType,fmt>::blit(const PixelType* cpy, const glm::ivec2 offset, const glm::ivec2& dimensions) {
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

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void ReferenceImage2D<PixelType,fmt>::blit(const Image2D& cpy, const glm::ivec2 offset, const glm::ivec2& dimensions) {
	const ReferenceImage2D* refTxt = dynamic_cast<const ReferenceImage2D*>(&cpy);
	if(refTxt) blit(refTxt->pixels.data(),offset,dimensions);
	else {
		const StandardImage2D<PixelType,fmt>* standardImage2D = dynamic_cast<const StandardImage2D<PixelType,fmt>*>(&cpy);
		if(standardImage2D) blit(standardImage2D->pixels.data(),offset,dimensions);
		else Image2D::blit(cpy,offset,dimensions);
	}
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void ReferenceImage2D<PixelType,fmt>::blit(const Image2D& cpy, const glm::ivec2 offset) {
	const ReferenceImage2D* refTxt = dynamic_cast<const ReferenceImage2D*>(&cpy);
	if(refTxt) blit(refTxt->pixels.data(),offset,glm::ivec2(refTxt->width,refTxt->height));
	else {
		const StandardImage2D<PixelType,fmt>* standardImage2D = dynamic_cast<const StandardImage2D<PixelType,fmt>*>(&cpy);
		if(standardImage2D) blit(standardImage2D->pixels.data(),offset,glm::ivec2(standardImage2D->width,standardImage2D->height));
		else Image2D::blit(cpy,offset);
	}
}*/

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void ReferenceImage2D<PixelType,fmt>::getPixel(const glm::ivec2& pos, glm::fvec4& colourKernel, Wrap wrap) const {
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

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void ReferenceImage2D<PixelType,fmt>::setPixel(const glm::ivec2& pos, const glm::fvec4& colourKernel) {
	pixels[ (pos.y * width) + pos.x ].fromKernel(colourKernel);
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void ReferenceImage2D<PixelType,fmt>::setPixelDithered(const glm::ivec2& pos, const glm::fvec4& colourKernel) {
	pixels[ (pos.y * width) + pos.x ].fromKernelDithered(colourKernel,pos);
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void* ReferenceImage2D<PixelType,fmt>::getRawPixels() {
	return pixels.data();
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline const void* ReferenceImage2D<PixelType,fmt>::getRawPixels() const {
	return pixels.data();
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void ReferenceImage2D<PixelType,fmt>::clearToColour(const glm::fvec4& colourKernel) {
	PixelType p;
	p.fromKernel(colourKernel);
	for(int y = 0; y < height; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			scanline[x] = p;
		}
	}
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void ReferenceImage2D<PixelType,fmt>::clearToColour(const ColourProgrammer& program) {
	for(int y = 0; y < height; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			scanline[x].fromKernelDithered(program(pos),pos);
		}
	}
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void ReferenceImage2D<PixelType,fmt>::clearToColour(const ColourProgrammer2& program) {
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

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void ReferenceImage2D<PixelType,fmt>::clearToColour(const ColourProgrammer3& program)
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

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void ReferenceImage2D<PixelType,fmt>::clearToColour(const ColourProgrammer4& program)
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

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void ReferenceImage2D<PixelType,fmt>::iterateOverPixels(const ColourIterator& program) const
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

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void ReferenceImage2D<PixelType,fmt>::iterateOverPixels(const ColourIterator2& program) const
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

template <MH33::Util::PixelConcept PixelType, Format fmt> StandardImage2D<PixelType,fmt>::StandardImage2D(const ReferenceImage2D<PixelType,fmt>& cpy) {
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
template <MH33::Util::PixelConcept PixelType, Format fmt> StandardImage2D<PixelType,fmt>& StandardImage2D<PixelType,fmt>::operator=(const ReferenceImage2D<PixelType,fmt>& cpy) {
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
template <MH33::Util::PixelConcept PixelType, Format fmt> ReferenceImage2D<PixelType,fmt>::ReferenceImage2D(const StandardImage2D<PixelType,fmt>& cpy) {
	this->pixels = cpy.pixels;
	this->width = cpy.width;
	this->height = cpy.height;
	this->stride = cpy.stride;
	this->widthF = cpy.widthF;
	this->heightF = cpy.heightF;
	this->widthR = cpy.widthR;
	this->heightR = cpy.heightR;
}
template <MH33::Util::PixelConcept PixelType, Format fmt> ReferenceImage2D<PixelType,fmt>& ReferenceImage2D<PixelType,fmt>::operator=(const StandardImage2D<PixelType,fmt>& cpy) {
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

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void StandardImage2D<PixelType,fmt>::clearToColour(const glm::fvec4& colourKernel, const glm::ivec2 offset, const glm::ivec2& dimensions) {
	PixelType p;
	p.fromKernel(colourKernel);
	const int minX = std::min(offset.x,width);
	const int maxX = std::min(offset.x + dimensions.x, width);
	const int minY = std::min(offset.y,height);
	const int maxY = std::min(offset.y + dimensions.y, height);
	for(int y = minY; y < maxY; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = minX; x < maxX; ++x) {
			scanline[x] = p;
		}
	}
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void StandardImage2D<PixelType,fmt>::clearToColour(const ColourProgrammer& program, const glm::ivec2 offset, const glm::ivec2& dimensions) {
	const int minX = std::min(offset.x,width);
	const int maxX = std::min(offset.x + dimensions.x, width);
	const int minY = std::min(offset.y,height);
	const int maxY = std::min(offset.y + dimensions.y, height);
	for(int y = minY; y < maxY; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = minX; x < maxX; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			scanline[x].fromKernelDithered(program(pos),pos);
		}
	}
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void StandardImage2D<PixelType,fmt>::clearToColour(const ColourProgrammer2& program, const glm::ivec2 offset, const glm::ivec2& dimensions) {
	const int minX = std::min(offset.x,width);
	const int maxX = std::min(offset.x + dimensions.x, width);
	const int minY = std::min(offset.y,height);
	const int maxY = std::min(offset.y + dimensions.y, height);
	for(int y = minY; y < maxY; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = minX; x < maxX; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			glm::fvec4 kernel;
			scanline[x].toKernel(kernel);
			scanline[x].fromKernelDithered(program(pos,kernel),pos);
		}
	}
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void StandardImage2D<PixelType,fmt>::clearToColour(const ColourProgrammer3& program, const glm::ivec2 offset, const glm::ivec2& dimensions)
{
	const int minX = std::min(offset.x,width);
	const int maxX = std::min(offset.x + dimensions.x, width);
	const int minY = std::min(offset.y,height);
	const int maxY = std::min(offset.y + dimensions.y, height);
	const glm::fvec2 reciprocal = glm::fvec2( 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height) );
	for(int y = minY; y < maxY; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = minX; x < maxX; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			glm::fvec2 fpos = glm::fvec2(static_cast<float>(x) * reciprocal.x, static_cast<float>(y) * reciprocal.y);
			scanline[x].fromKernelDithered(program(fpos),pos);
		}
	}
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void StandardImage2D<PixelType,fmt>::clearToColour(const ColourProgrammer4& program, const glm::ivec2 offset, const glm::ivec2& dimensions)
{
	const int minX = std::min(offset.x,width);
	const int maxX = std::min(offset.x + dimensions.x, width);
	const int minY = std::min(offset.y,height);
	const int maxY = std::min(offset.y + dimensions.y, height);
	const glm::fvec2 reciprocal = glm::fvec2( 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height) );
	for(int y = minY; y < maxY; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = minX; x < maxX; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			glm::fvec2 fpos = glm::fvec2(static_cast<float>(x) * reciprocal.x, static_cast<float>(y) * reciprocal.y);
			glm::fvec4 kernel;
			scanline[x].toKernel(kernel);
			scanline[x].fromKernelDithered(program(fpos,kernel),pos);
		}
	}
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void ReferenceImage2D<PixelType,fmt>::clearToColour(const glm::fvec4& colourKernel, const glm::ivec2 offset, const glm::ivec2& dimensions) {
	PixelType p;
	p.fromKernel(colourKernel);
	const int minX = std::min(offset.x,width);
	const int maxX = std::min(offset.x + dimensions.x, width);
	const int minY = std::min(offset.y,height);
	const int maxY = std::min(offset.y + dimensions.y, height);
	for(int y = minY; y < maxY; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = minX; x < maxX; ++x) {
			scanline[x] = p;
		}
	}
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void ReferenceImage2D<PixelType,fmt>::clearToColour(const ColourProgrammer& program, const glm::ivec2 offset, const glm::ivec2& dimensions) {
	const int minX = std::min(offset.x,width);
	const int maxX = std::min(offset.x + dimensions.x, width);
	const int minY = std::min(offset.y,height);
	const int maxY = std::min(offset.y + dimensions.y, height);
	for(int y = minY; y < maxY; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = minX; x < maxX; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			scanline[x].fromKernelDithered(program(pos),pos);
		}
	}
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void ReferenceImage2D<PixelType,fmt>::clearToColour(const ColourProgrammer2& program, const glm::ivec2 offset, const glm::ivec2& dimensions) {
	const int minX = std::min(offset.x,width);
	const int maxX = std::min(offset.x + dimensions.x, width);
	const int minY = std::min(offset.y,height);
	const int maxY = std::min(offset.y + dimensions.y, height);
	for(int y = minY; y < maxY; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = minX; x < maxX; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			glm::fvec4 kernel;
			scanline[x].toKernel(kernel);
			scanline[x].fromKernelDithered(program(pos,kernel),pos);
		}
	}
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void ReferenceImage2D<PixelType,fmt>::clearToColour(const ColourProgrammer3& program, const glm::ivec2 offset, const glm::ivec2& dimensions)
{
	const int minX = std::min(offset.x,width);
	const int maxX = std::min(offset.x + dimensions.x, width);
	const int minY = std::min(offset.y,height);
	const int maxY = std::min(offset.y + dimensions.y, height);
	const glm::fvec2 reciprocal = glm::fvec2( 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height) );
	for(int y = minY; y < maxY; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = minX; x < maxX; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			glm::fvec2 fpos = glm::fvec2(static_cast<float>(x) * reciprocal.x, static_cast<float>(y) * reciprocal.y);
			scanline[x].fromKernelDithered(program(fpos),pos);
		}
	}
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void ReferenceImage2D<PixelType,fmt>::clearToColour(const ColourProgrammer4& program, const glm::ivec2 offset, const glm::ivec2& dimensions)
{
	const int minX = std::min(offset.x,width);
	const int maxX = std::min(offset.x + dimensions.x, width);
	const int minY = std::min(offset.y,height);
	const int maxY = std::min(offset.y + dimensions.y, height);
	const glm::fvec2 reciprocal = glm::fvec2( 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height) );
	for(int y = minY; y < maxY; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = minX; x < maxX; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			glm::fvec2 fpos = glm::fvec2(static_cast<float>(x) * reciprocal.x, static_cast<float>(y) * reciprocal.y);
			glm::fvec4 kernel;
			scanline[x].toKernel(kernel);
			scanline[x].fromKernelDithered(program(fpos,kernel),pos);
		}
	}
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void StandardImage2D<PixelType,fmt>::iterateOverPixels(const ColourIterator& program, const glm::ivec2 offset, const glm::ivec2& dimensions) const
{
	const int minX = std::min(offset.x,width);
	const int maxX = std::min(offset.x + dimensions.x, width);
	const int minY = std::min(offset.y,height);
	const int maxY = std::min(offset.y + dimensions.y, height);
	for(int y = minY; y < maxY; ++y) {
		const PixelType* const scanline = &pixels[y*width];
		for(int x = minX; x < maxX; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			glm::fvec4 kernel;
			scanline[x].toKernel(kernel);
			program(pos,kernel);
		}
	}
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void StandardImage2D<PixelType,fmt>::iterateOverPixels(const ColourIterator2& program, const glm::ivec2 offset, const glm::ivec2& dimensions) const
{
	const int minX = std::min(offset.x,width);
	const int maxX = std::min(offset.x + dimensions.x, width);
	const int minY = std::min(offset.y,height);
	const int maxY = std::min(offset.y + dimensions.y, height);
	const glm::fvec2 reciprocal = glm::fvec2( 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height) );
	for(int y = minY; y < maxY; ++y) {
		const PixelType* const scanline = &pixels[y*width];
		for(int x = minX; x < maxX; ++x) {
			glm::fvec2 fpos = glm::fvec2(static_cast<float>(x) * reciprocal.x, static_cast<float>(y) * reciprocal.y);
			glm::fvec4 kernel;
			scanline[x].toKernel(kernel);
			program(fpos,kernel);
		}
	}
}
template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void ReferenceImage2D<PixelType,fmt>::iterateOverPixels(const ColourIterator& program, const glm::ivec2 offset, const glm::ivec2& dimensions) const
{
	const int minX = std::min(offset.x,width);
	const int maxX = std::min(offset.x + dimensions.x, width);
	const int minY = std::min(offset.y,height);
	const int maxY = std::min(offset.y + dimensions.y, height);
	for(int y = minY; y < maxY; ++y) {
		const PixelType* const scanline = &pixels[y*width];
		for(int x = minX; x < maxX; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			glm::fvec4 kernel;
			scanline[x].toKernel(kernel);
			program(pos,kernel);
		}
	}
}

template <MH33::Util::PixelConcept PixelType, Format fmt>
inline void ReferenceImage2D<PixelType,fmt>::iterateOverPixels(const ColourIterator2& program, const glm::ivec2 offset, const glm::ivec2& dimensions) const
{
	const int minX = std::min(offset.x,width);
	const int maxX = std::min(offset.x + dimensions.x, width);
	const int minY = std::min(offset.y,height);
	const int maxY = std::min(offset.y + dimensions.y, height);
	const glm::fvec2 reciprocal = glm::fvec2( 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height) );
	for(int y = minY; y < maxY; ++y) {
		const PixelType* const scanline = &pixels[y*width];
		for(int x = minX; x < maxX; ++x) {
			glm::fvec2 fpos = glm::fvec2(static_cast<float>(x) * reciprocal.x, static_cast<float>(y) * reciprocal.y);
			glm::fvec4 kernel;
			scanline[x].toKernel(kernel);
			program(fpos,kernel);
		}
	}
}
