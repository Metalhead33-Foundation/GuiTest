#include "EuphJPEG.hpp"
#include <turbojpeg.h>
#include <memory>
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

bool encode(const Frame& frame, Format format, const std::function<std::span<std::byte> (unsigned long)>& allocator, SubsamplingMethod jpegSubsamp, float jpegQual, unsigned long& jpegSize)
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
	auto buffer = allocator(tjBufSize(frame.width,frame.height,jpegSubsamp));
	if(buffer.empty()) return false;
	unsigned char* dstptr = reinterpret_cast<unsigned char*>(buffer.data());
	tjCompress2(handle.get(),reinterpret_cast<const unsigned char*>(frame.data.data()),frame.width,frame.width*tjPixelSize[pixelFormat],frame.height,
				pixelFormat,&dstptr,&jpegSize,jpegSubsamp,int(((1.0f-std::clamp(jpegQual,0.0f,1.0f))*99.0f)+1.0f),TJFLAG_NOREALLOC | TJFLAG_FASTDCT);
	return true;
}

bool encode(const DecodeTarget& source, const std::function<std::span<std::byte>(unsigned long)>& allocator, SubsamplingMethod jpegSubsamp, float jpegQual, unsigned long& jpegSize)
{
	if(source.getFrames().empty()) return false;
	return encode(source.getFrame(0), source.getFormat(), allocator, jpegSubsamp, jpegQual, jpegSize);
}

bool encode(Elv::Io::Device& iodev, const Frame& frame, Format format, SubsamplingMethod jpegSubsamp, float jpegQual)
{
	std::vector<std::byte> tmpBuff;
	unsigned long jpegSize;
	bool toReturn = encode(frame, format, [&tmpBuff](unsigned long sz) {
		tmpBuff.resize(sz);
		return std::span<std::byte>(tmpBuff.data(), tmpBuff.size());
	}, jpegSubsamp, jpegQual, jpegSize);
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
