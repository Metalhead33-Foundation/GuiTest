#include "EuphJPEG.hpp"
#include <turbojpeg.h>
#include <memory>
#include <algorithm>
namespace Euph {
namespace Media {
namespace Image {
namespace JPEG {

bool decode(const std::span<const std::byte>& inbuf, DecodeTarget& destination)
{
	if(inbuf.empty()) return false;
	auto handle = std::unique_ptr<void,decltype(&tjDestroy) >(tjInitDecompress(),tjDestroy);
	if(!handle) return false;
	int width,height,subsamp;
	uint8_t* sourceBuff = const_cast<uint8_t*>(reinterpret_cast<const unsigned char*>(inbuf.data()));
	tjDecompressHeader2(handle.get(), sourceBuff, inbuf.size(), &width, &height, &subsamp);
	destination.setIsAnimated(false);
	destination.setFormat(Format::RGB8U);
	auto& frame = destination.addFrame(width,height);
	tjDecompress2(handle.get(),sourceBuff,inbuf.size(),
				  reinterpret_cast<unsigned char*>(frame.data.data()),width,width*3,height,TJPF_RGB,TJFLAG_FASTDCT);
	return true;
}
bool decode(Elv::Io::Device& iodev, DecodeTarget& destination) {
	auto tmpBuff = iodev.readAllAsUHA();
	return decode(tmpBuff,destination);
}

bool encode(const Frame& frame, Format format, std::span<std::byte> buffer, SubsamplingMethod jpegSubsamp, float jpegQual, unsigned long& jpegSize)
{
	int pixelFormat = 0;
	switch (format) {
		case Format::RGB8U: pixelFormat = TJPF_RGB; break;
		case Format::BGR8U: pixelFormat = TJPF_BGR; break;
		case Format::RGBA8U: pixelFormat = TJPF_RGBA; break;
		case Format::BGRA8U: pixelFormat = TJPF_BGRA; break;
		case Format::ARGB8U: pixelFormat = TJPF_ARGB; break;
		case Format::R8U: pixelFormat = TJPF_GRAY; break;
		default: return false;
	}
	auto handle = std::unique_ptr<void,decltype(&tjDestroy) >(tjInitCompress(),tjDestroy);
	if(!handle) return false;
	const unsigned long requiredSize = tjBufSize(frame.width,frame.height,static_cast<int>(jpegSubsamp));
	if(buffer.size() < requiredSize) return false;
	unsigned char* dstptr = reinterpret_cast<unsigned char*>(buffer.data());
	jpegSize = static_cast<unsigned long>(buffer.size());
	tjCompress2(handle.get(),reinterpret_cast<const unsigned char*>(frame.data.data()),frame.width,frame.width*tjPixelSize[pixelFormat],frame.height,
				pixelFormat,&dstptr,&jpegSize,static_cast<int>(jpegSubsamp),int(((1.0f-std::clamp(jpegQual,0.0f,1.0f))*99.0f)+1.0f),TJFLAG_NOREALLOC | TJFLAG_FASTDCT);
	return true;
}

bool encode(const DecodeTarget& source, std::span<std::byte> buffer, SubsamplingMethod jpegSubsamp, float jpegQual, unsigned long& jpegSize)
{
	if(source.getFrames().empty()) return false;
	return encode(source.getFrame(0), source.getFormat(), buffer, jpegSubsamp, jpegQual, jpegSize);
}

bool encode(Elv::Io::Device& iodev, const Frame& frame, Format format, SubsamplingMethod jpegSubsamp, float jpegQual)
{
	std::vector<std::byte> tmpBuff(tjBufSize(frame.width,frame.height,static_cast<int>(jpegSubsamp)));
	unsigned long jpegSize = 0;
	bool toReturn = encode(frame, format, std::span<std::byte>(tmpBuff.data(), tmpBuff.size()), jpegSubsamp, jpegQual, jpegSize);
	if(toReturn) {
		iodev.write(tmpBuff.data(),sizeof(std::byte), jpegSize);
	}
	return toReturn;
}

bool encode(Elv::Io::Device& iodev, const DecodeTarget& source, SubsamplingMethod jpegSubsamp, float jpegQual)
{
	if(source.getFrames().empty()) return false;
	return encode(iodev, source.getFrame(0), source.getFormat(), jpegSubsamp, jpegQual);
}

}
}
}
}
