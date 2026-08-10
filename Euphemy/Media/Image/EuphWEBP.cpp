#include "EuphWEBP.hpp"
#include <algorithm>
#include <webp/decode.h>
#include <webp/encode.h>
#include <webp/demux.h>
#include <cmath>
#include <limits>
#include <cstring>
#include <memory>
#include <stdexcept>
namespace Euph {
namespace Media {
namespace Image {
namespace WEBP {

bool decode(const std::span<const std::byte>& srcBuffer, DecodeTarget& destination)
{
	if(srcBuffer.empty()) return false;
	WebPBitstreamFeatures features;
	if(!WebPGetFeatures(reinterpret_cast<const uint8_t*>(srcBuffer.data()),srcBuffer.size(),&features)) return false;
	destination.setFormat(features.has_alpha ? Format::RGBA8U : Format::RGB8U);
	auto& frame = destination.addFrame(features.width,features.height);
	switch (destination.getFormat()) {
	case Format::RGB8U:
		return WebPDecodeRGBInto(reinterpret_cast<const uint8_t*>(srcBuffer.data()),srcBuffer.size(),
								 reinterpret_cast<uint8_t*>(frame.data.data()),frame.data.size(),frame.stride) != nullptr;
	case Format::RGBA8U:
		return WebPDecodeRGBAInto(reinterpret_cast<const uint8_t*>(srcBuffer.data()),srcBuffer.size(),
								  reinterpret_cast<uint8_t*>(frame.data.data()),frame.data.size(),frame.stride) != nullptr;
	default:
		return false;
	}
}
bool decode(Elv::Io::Device& iodev, DecodeTarget& destination) {
	auto tmpBuff = iodev.readAllAsUHA();
	return decode(tmpBuff,destination);
}

size_t encode(const Frame& frame, Format format, float compressionLevel, uint8_t** ptr)
{
	const uint8_t* pixdat = reinterpret_cast<const uint8_t*>(frame.data.data());
	if(compressionLevel <= std::numeric_limits<float>::epsilon()) { // Lossless
		switch (format) {
		case Format::BGR8U:
			return WebPEncodeLosslessBGR(pixdat,frame.width,frame.height,frame.stride,ptr);
		case Format::BGRA8U:
			return WebPEncodeLosslessBGRA(pixdat,frame.width,frame.height,frame.stride,ptr);
		case Format::RGB8U:
			return WebPEncodeLosslessRGB(pixdat,frame.width,frame.height,frame.stride,ptr);
		case Format::RGBA8U:
			return WebPEncodeLosslessRGBA(pixdat,frame.width,frame.height,frame.stride,ptr);
		default:
			return 0;
		}
	} else {
		const float quality_factor = (1.0f-std::clamp(compressionLevel,0.0f,1.0f))*100.f;
		switch (format) {
		case Format::BGR8U:
			return WebPEncodeBGR(pixdat,frame.width,frame.height,frame.stride,quality_factor,ptr);
		case Format::BGRA8U:
			return WebPEncodeBGRA(pixdat,frame.width,frame.height,frame.stride,quality_factor,ptr);
		case Format::RGB8U:
			return WebPEncodeRGB(pixdat,frame.width,frame.height,frame.stride,quality_factor,ptr);
		case Format::RGBA8U:
			return WebPEncodeRGBA(pixdat,frame.width,frame.height,frame.stride,quality_factor,ptr);
		default:
			return 0;
		}
	}
}

bool encode(Elv::Io::Device& iodev, const Frame& frame, Format format, float compressionLevel)
{
	uint8_t* out;
	auto encoded = encode(frame,format,compressionLevel,&out);
	if(!encoded) return false;
	iodev.write(out,sizeof(uint8_t),encoded);
	WebPFree(out);
	return true;
}

bool encode(Elv::Io::Device& iodev, const DecodeTarget& source, float compressionLevel)
{
	if(source.getFrames().empty()) return false;
	return encode(iodev,source.getFrame(0),source.getFormat(),compressionLevel);
}

bool demux(const std::span<const std::byte>& srcBuffer, DecodeTarget& destination)
{
	if(srcBuffer.empty()) return false;
	switch (destination.getFormat()) {
	case Format::BGR8U:
		break;
	case Format::BGRA8U:
		break;
	case Format::RGB8U:
		break;
	case Format::RGBA8U:
		break;
	case Format::ARGB8U:
		break;
	default:
		destination.setFormat(Format::RGB8U);
		break;
	}
	typedef std::unique_ptr<WebPDemuxer,decltype (&WebPDemuxDelete)> Configuration;
	WebPData riff;
	riff.bytes = reinterpret_cast<const uint8_t*>(srcBuffer.data());
	riff.size = srcBuffer.size();
	Configuration config = Configuration(WebPDemux(&riff),WebPDemuxDelete);
	if(!config) return false;
	uint32_t width = WebPDemuxGetI(config.get(), WEBP_FF_CANVAS_WIDTH);
	uint32_t height = WebPDemuxGetI(config.get(), WEBP_FF_CANVAS_HEIGHT);
	//uint32_t flags = WebPDemuxGetI(config.get(), WEBP_FF_FORMAT_FLAGS);
	//uint32_t frameCount = WebPDemuxGetI(config.get(), WEBP_FF_FRAME_COUNT);
	WebPIterator iter;
	if (WebPDemuxGetFrame(config.get(), 1, &iter)) {
		do {
			auto& frame = destination.addFrame(width,height);
			switch (destination.getFormat()) {
			/*case Format::BGR8U:
				data = WebPDecodeBGR(iter.fragment.bytes,iter.fragment.size,nullptr,nullptr);
				break;*/
			case Format::BGR8U:
				WebPDecodeBGRInto(iter.fragment.bytes,iter.fragment.size,
								reinterpret_cast<uint8_t*>(frame.data.data()),frame.data.size(),frame.stride);
				break;
			case Format::BGRA8U:
				WebPDecodeBGRAInto(iter.fragment.bytes,iter.fragment.size,
								   reinterpret_cast<uint8_t*>(frame.data.data()),frame.data.size(),frame.stride);
				break;
			case Format::RGB8U:
				WebPDecodeRGBInto(iter.fragment.bytes,iter.fragment.size,
								  reinterpret_cast<uint8_t*>(frame.data.data()),frame.data.size(),frame.stride);
				break;
			case Format::RGBA8U:
				WebPDecodeRGBAInto(iter.fragment.bytes,iter.fragment.size,
								   reinterpret_cast<uint8_t*>(frame.data.data()),frame.data.size(),frame.stride);
				break;
			case Format::ARGB8U:
				WebPDecodeARGBInto(iter.fragment.bytes,iter.fragment.size,
								   reinterpret_cast<uint8_t*>(frame.data.data()),frame.data.size(),frame.stride);
				break;
			default:
				return false;
				break;
			}
		} while (WebPDemuxNextFrame(&iter));
		WebPDemuxReleaseIterator(&iter);
	}
	if(destination.getFrames().size() > 1) destination.setIsAnimated(true);
	else destination.setIsAnimated(false);
	return true;
}

bool demux(Elv::Io::Device& iodev, DecodeTarget& destination)
{
	auto tmpBuff = iodev.readAllAsUHA();
	return demux(tmpBuff,destination);
}


}
}
}
}
