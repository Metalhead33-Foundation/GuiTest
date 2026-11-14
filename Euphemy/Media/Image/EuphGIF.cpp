#include "EuphGIF.hpp"
#include <gif_lib.h>
#include <memory>
#include <cstring>
#include <cassert>
#include <Euphemy/Media/Image/EuphPixelFormat.hpp>
namespace Euph {
namespace Media {
namespace Image {
namespace GIF {

static int LATEST_ERROR;
static void CLOSE_GIF(GifFileType* ptr) { DGifCloseFile(ptr,&LATEST_ERROR); }
static int GIF_INPUT(GifFileType* container, GifByteType* buffer, int size)
{
	assert(container);
	assert(container->UserData);
	if(!container) return -1;
	Elv::Io::Device* filehandle = reinterpret_cast<Elv::Io::Device*>(container->UserData);
	if(!filehandle) return -1;
	return int(filehandle->read(buffer,sizeof(std::byte),size));
}
typedef std::unique_ptr<GifFileType, decltype (&CLOSE_GIF)> HandleType;

void decode(Elv::Io::Device& iodev, DecodeTarget& destination) {
	HandleType handle(DGifOpen(&iodev,&GIF_INPUT,&LATEST_ERROR),CLOSE_GIF);
	DGifSlurp(handle.get());
	int tmpFramerate = 0;
	destination.setFormat(Format::INDEXED);
	destination.setIsAnimated(true);
	ColorMapObject *gifcolormap = handle->SColorMap;
	if(!gifcolormap) return;
	GifColorType *gifpalette = gifcolormap->Colors;
	if(!gifpalette) return;
	auto& palette = destination.createPalette(Format::RGB8U, gifcolormap->ColorCount, -1);
	for(int i = 0; i < handle->ExtensionBlockCount; ++i)
	{
		GraphicsControlBlock block;
		if (DGifExtensionToGCB(handle->ExtensionBlocks[i].ByteCount,handle->ExtensionBlocks[i].Bytes,
							   &block)) {
			if(block.TransparentColor > palette.transparentColorIndex) palette.transparentColorIndex = block.TransparentColor;
			if(block.DelayTime > tmpFramerate) tmpFramerate = block.DelayTime;
		}
	}
	destination.setDelayTime(static_cast<float>(tmpFramerate) * 0.01f);
	PixelRGB_U8* ptr = reinterpret_cast<PixelRGB_U8*>(palette.data.data());
	for(int i = 0; i < gifcolormap->ColorCount; ++i) {
		ptr[i].r = gifcolormap->Colors[i].Red;
		ptr[i].g = gifcolormap->Colors[i].Green;
		ptr[i].b = gifcolormap->Colors[i].Blue;
	}
	for(int i = 0; i < handle->ImageCount; ++i) {
		auto& frame = destination.addFrame(handle->SWidth, handle->SHeight);
		const int fullsize = handle->SWidth * handle->SHeight;
		memcpy(frame.data.data(),handle->SavedImages[i].RasterBits,fullsize);
	}
}

}
}
}
}
