#include "EuphPNG.hpp"
#include <png.h>
#include <algorithm>
#include <stdexcept>
#include <Euphemy/Media/Image/EuphPixelFormat.hpp>

void mh33_set_read_callbacks(png_structp read_ptr, Elv::Io::Device* dev);
void mh33_set_write_callbacks(png_structp write_ptr, Elv::Io::Device* dev);
void mh33_read_data(png_structp png_ptr, png_bytep data, png_size_t length);
void mh33_write_data(png_structp png_ptr, png_bytep data, png_size_t length);
void mh33_flush_data(png_structp png_ptr);
namespace Euph {
namespace Media {
namespace Image {
namespace PNG {

constexpr size_t type2bytes(uint8_t color_type, uint8_t bit_depth) {
	switch (color_type) {

	case PNG_COLOR_TYPE_RGB: return (bit_depth/8) * 3;
	case PNG_COLOR_TYPE_RGBA: return (bit_depth/8) * 4;
	case PNG_COLOR_TYPE_GRAY: return bit_depth/8;
	case PNG_COLOR_TYPE_PALETTE: return sizeof(uint8_t);
	default: return 0;
}
}

void decode(Elv::Io::Device& iodev, DecodeTarget& destination) {
	uint8_t color_type;
	uint8_t bit_depth;
	auto pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING,nullptr,nullptr,nullptr);
	if(!pngPtr) {
		throw std::runtime_error("Failed to allocate PNG read structure!");
	}
	auto infoPtr = png_create_info_struct(pngPtr);
	if(!infoPtr) {
		png_destroy_read_struct(&pngPtr,nullptr,nullptr);
		throw std::runtime_error("Failed to allocate PNG info structure!");
	}
	auto endInfoPtr = png_create_info_struct(pngPtr);
	if(!endInfoPtr) {
		png_destroy_read_struct(&pngPtr,&infoPtr,nullptr);
		throw std::runtime_error("Failed to allocate PNG info structure!");
	}
	png_set_read_fn(pngPtr,&iodev,mh33_read_data);
	if(iodev.tell()) png_set_sig_bytes(pngPtr, iodev.tell());
	png_read_info(pngPtr, infoPtr);

	destination.setIsAnimated(false);
	color_type = png_get_color_type(pngPtr,infoPtr);
	bit_depth = png_get_bit_depth(pngPtr,infoPtr);
	switch (color_type) {
	case PNG_COLOR_TYPE_RGB: switch (bit_depth) {
		case 8: destination.setFormat(Format::RGB8U); break;
		case 16: destination.setFormat(Format::RGB16U); break;
		default: destination.setFormat(Format::INVALID); break;
		} break;
	case PNG_COLOR_TYPE_RGBA: switch (bit_depth) {
		case 8: destination.setFormat(Format::RGBA8U); break;
		case 16: destination.setFormat(Format::RGBA16U); break;
		default: destination.setFormat(Format::INVALID); break;
		} break;
	case PNG_COLOR_TYPE_GRAY: switch (bit_depth) {
		case 1:
		case 2:
		case 4: png_set_expand_gray_1_2_4_to_8(pngPtr); [[fallthrough]];
		case 8: destination.setFormat(Format::R8U); break;
		case 16: destination.setFormat(Format::R16U); break;
		default: destination.setFormat(Format::INVALID); break;
		} break;
	case PNG_COLOR_TYPE_PALETTE: switch (bit_depth) {
		case 1:
		case 2:
		case 4: png_set_packing(pngPtr); [[fallthrough]];
		case 8: destination.setFormat(Format::INDEXED); break;
		default: destination.setFormat(Format::INVALID); break;
		} break;
	}
	if(destination.getFormat() == Format::INVALID) {
		png_destroy_read_struct(&pngPtr,&infoPtr,&endInfoPtr);
		throw std::runtime_error("Invalid image format!");
	}
	auto& frame = destination.addFrame(png_get_image_width(pngPtr,infoPtr),png_get_image_height(pngPtr,infoPtr));
	int number_of_passes;
	number_of_passes = png_set_interlace_handling(pngPtr);
	png_read_update_info(pngPtr, infoPtr);

	std::vector<png_bytep> rowPtrs(frame.height);
	for (int y=0; y<frame.height; y++) {
		rowPtrs[y] = reinterpret_cast<png_bytep>(&(frame.data[y*frame.stride]));
	}
	png_read_image(pngPtr, rowPtrs.data());
	if(color_type == PNG_COLOR_TYPE_PALETTE) {
		png_colorp png_palette = nullptr;
		int palette_entries = 0;
		png_get_PLTE(pngPtr,infoPtr, &png_palette, &palette_entries);
		auto& palette = destination.createPalette(Format::RGB8U, palette_entries, 0);
		PixelRGB_U8* entries = reinterpret_cast<PixelRGB_U8*>(palette.data.data());
		for(int i = 0; i < palette_entries; ++i) {
			entries[i].r = png_palette[i].red;
			entries[i].g = png_palette[i].green;
			entries[i].b = png_palette[i].blue;
		}
	}
	png_destroy_read_struct(&pngPtr,&infoPtr,&endInfoPtr);
}

void encode(Elv::Io::Device& iodev, Format format, const Frame& frame, float compressionLevel)
{
	uint8_t color_type = 0;
	uint8_t bit_depth = 0;
	switch (format) {
		case Format::R8U:
			color_type = PNG_FORMAT_GRAY;
			bit_depth = 8;
			break;
		case Format::R16U:
			color_type = PNG_FORMAT_GRAY;
			bit_depth = 16;
			break;
		case Format::RGB8U:
			color_type = PNG_FORMAT_RGB;
			bit_depth = 8;
			break;
		case Format::RG16U:
			color_type = PNG_FORMAT_RGB;
			bit_depth = 16;
			break;
		case Format::RGBA8U:
			color_type = PNG_FORMAT_RGBA;
			bit_depth = 8;
			break;
		case Format::RGBA16U:
			color_type = PNG_FORMAT_RGBA;
			bit_depth = 16;
			break;
		case Format::ARGB8U:
			color_type = PNG_FORMAT_ARGB;
			bit_depth = 8;
			break;
		case Format::ARGB16U:
			color_type = PNG_FORMAT_ARGB;
			bit_depth = 16;
			break;
		default: break;
	}
	auto pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING,nullptr,nullptr,nullptr);
	if(!pngPtr) {
		throw std::runtime_error("Failed to allocate PNG write structure!");
	}
	auto infoPtr = png_create_info_struct(pngPtr);
	if(!infoPtr) {
		png_destroy_write_struct(&pngPtr,nullptr);
		throw std::runtime_error("Failed to allocate PNG info structure!");
	}
	png_set_write_fn(pngPtr,&iodev,mh33_write_data,mh33_flush_data);
	png_set_IHDR(pngPtr,infoPtr,frame.width,frame.width,bit_depth,color_type,PNG_INTERLACE_NONE,
				 PNG_COMPRESSION_TYPE_DEFAULT,PNG_FILTER_TYPE_DEFAULT);
	png_set_compression_level(pngPtr,int(std::clamp(compressionLevel,0.0f,1.0f)*9.0f));
	std::vector<png_bytep> rowPtrs(frame.height);
	for (int y=0; y<frame.height; y++) {
		rowPtrs[y] = const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(&frame.data[type2bytes(color_type,bit_depth)*frame.width*y]));
	}
	png_write_info(pngPtr,infoPtr);
	png_write_rows(pngPtr,rowPtrs.data(),rowPtrs.size());
	png_write_end(pngPtr,infoPtr);
	png_destroy_write_struct(&pngPtr,&infoPtr);
}

void encode(Elv::Io::Device& iodev, const DecodeTarget& source, float compressionLevel)
{
	if(source.getFrames().empty()) {
		throw std::runtime_error("Empty image!");
	}
	encode(iodev,source.getFormat(),source.getFrame(0),compressionLevel);
}

}
}
}
}
void mh33_set_read_callbacks(png_structp read_ptr, Elv::Io::Device* dev) {
	png_set_read_fn(read_ptr, dev, mh33_read_data);
}
void mh33_set_write_callbacks(png_structp write_ptr, Elv::Io::Device* dev) {
	png_set_write_fn(write_ptr, dev, mh33_write_data, mh33_flush_data);
}
void mh33_read_data(png_structp png_ptr, png_bytep data, png_size_t length) {
	reinterpret_cast<Elv::Io::Device*>(png_get_io_ptr(png_ptr))->read(data,sizeof(uint8_t),length);
}
void mh33_write_data(png_structp png_ptr, png_bytep data, png_size_t length) {
	reinterpret_cast<Elv::Io::Device*>(png_get_io_ptr(png_ptr))->write(data,sizeof(uint8_t),length);
}
void mh33_flush_data(png_structp png_ptr) {
	reinterpret_cast<Elv::Io::Device*>(png_get_io_ptr(png_ptr))->flush();
}
