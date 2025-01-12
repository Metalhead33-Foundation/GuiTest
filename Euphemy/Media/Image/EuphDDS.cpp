#include "EuphDDS.hpp"
#include <Elvavena/Io/ElvDataStream.hpp>
namespace Euph {
namespace Media {
namespace Image {
namespace DDS {

static const uint32_t FOURCC_DXT1 = 0x31545844;
static const uint32_t FOURCC_DXT3 = 0x33545844;
static const uint32_t FOURCC_DXT5 = 0x35545844;
static const uint32_t FOURCC_DX10 = 0x30315844;
enum Type {
	INVALID = 0,
	DXT1,
	DXT3,
	DXT5,
	DX10
};
struct Mipmap {
	uint32_t width, height;
	std::pmr::vector<std::byte> bytes;
	Mipmap(uint32_t width, uint32_t height, std::pmr::memory_resource* memRes)
		: width(width), height(height), bytes(memRes) {

	};
	Mipmap(const Mipmap& cpy) : width(cpy.width), height(cpy.height), bytes(cpy.bytes) {

	}
	Mipmap(Mipmap&& mov) : width(mov.width), height(mov.height), bytes(std::move(mov.bytes)) {

	}
	Mipmap& operator=(const Mipmap& cpy) {
		width = cpy.width;
		height = cpy.height;
		bytes = cpy.bytes;
		return *this;
	}
	Mipmap operator=(Mipmap&& mov) {
		width = mov.width;
		height = mov.height;
		bytes = std::move(mov.bytes);
		return *this;
	}
};
struct Header
{
	// We read this from the file
	uint32_t dwSize;
	uint32_t dwFlags;
	uint32_t dwHeight;
	uint32_t dwWidth;
	uint32_t dwPitchOrLinearSize;
	uint32_t dwDepth;
	uint32_t dwMipMapCount;
	std::array<uint32_t,11> dwReserved1;
	struct {
		uint32_t dwSize;
		uint32_t dwFlags;
		uint32_t dwFourCC;
		uint32_t dwRGBBitCount;
		uint32_t dwRBitMask;
		uint32_t dwGBitMask;
		uint32_t dwBBitMask;
		uint32_t dwABitMask;
	} ddspf;
	uint32_t dwCaps;
	uint32_t dwCaps2;
	uint32_t dwCaps3;
	uint32_t dwCaps4;
	uint32_t dwReserved2;
	// This part is optional
	struct {
		uint32_t dxgiFormat;
		uint32_t resourceDimension;
		uint32_t miscFlag;
		uint32_t arraySize;
		uint32_t miscFlags2;
	} headerDXT10;
	// We infer this from the file
	Type type;
	std::vector<Mipmap> mipmaps;
	void load(Elv::Io::Device& input, std::pmr::memory_resource* memRes);
};

void Header::load(Elv::Io::Device& input, std::pmr::memory_resource* memRes)
{
	input.seek(4,Elv::Io::SeekOrigin::SET);
	Elv::Io::DataStream<Elv::Util::Endian::Little> ddsInput(input); // Implying we already read the magic word at the beginning.
	ddsInput >> dwSize;
	if(dwSize != 124) {
		this->type = Type::INVALID;
		throw std::runtime_error("Invalid image format!");
	}
	ddsInput >> dwFlags;
	ddsInput >> dwHeight;
	ddsInput >> dwWidth;
	ddsInput >> dwPitchOrLinearSize;
	ddsInput >> dwDepth;
	ddsInput >> dwMipMapCount;
	ddsInput >> dwReserved1;
	ddsInput >> ddspf.dwSize;
	ddsInput >> ddspf.dwFlags;
	ddsInput >> ddspf.dwFourCC;
	ddsInput >> ddspf.dwRGBBitCount;
	ddsInput >> ddspf.dwRBitMask;
	ddsInput >> ddspf.dwGBitMask;
	ddsInput >> ddspf.dwBBitMask;
	ddsInput >> ddspf.dwABitMask;
	ddsInput >> dwCaps;
	ddsInput >> dwCaps2;
	ddsInput >> dwCaps3;
	ddsInput >> dwCaps4;
	ddsInput >> dwReserved2;
	switch(ddspf.dwFourCC)
	{
	case FOURCC_DXT1:
		this->type = Type::DXT1;
		break;
	case FOURCC_DXT3:
		this->type = Type::DXT3;
		break;
	case FOURCC_DXT5:
		this->type = Type::DXT5;
		break;
	case FOURCC_DX10:
		this->type = Type::DX10;
		ddsInput >> headerDXT10.dxgiFormat;
		ddsInput >> headerDXT10.resourceDimension;
		ddsInput >> headerDXT10.miscFlag;
		ddsInput >> headerDXT10.arraySize;
		ddsInput >> headerDXT10.miscFlags2;
		break;
	default:
		this->type = Type::INVALID;
		throw std::runtime_error("Invalid image format!");
	}
	uint32_t width = dwWidth;
	uint32_t height = dwHeight;
	const uint32_t buffsize = dwMipMapCount > 1 ? dwPitchOrLinearSize * 2 : dwPitchOrLinearSize;
	std::pmr::vector<std::byte> buffer(memRes);
	buffer.resize(buffsize);
	input.read(buffer.data(),1,buffsize);
	const uint32_t blockSize = (this->type == Type::DXT1) ? 8 : 16;
	uint32_t offset = 0;
	for(uint32_t i = 0; i < dwMipMapCount; ++i) {
		const unsigned int size = ((width+3)/4)*((height+3)/4)*blockSize;
		mipmaps.emplace_back(width,height,memRes);
		auto& x = mipmaps.back();
		x.bytes.resize(size);
		memcpy(x.bytes.data(),buffer.data() + offset,size);
		offset += size;
		width  /= 2;
		height /= 2;
	}
}

void decode(Elv::Io::Device& iodev, DecodeTarget& destination)
{
	Header head;
	head.load(iodev, destination.getMemResource());
	switch (head.type) {
	case Type::DXT1:
		destination.setFormat(Format::B_DXT1);
		break;
	case Type::DXT3:
		destination.setFormat(Format::B_DXT3);
		break;
	case Type::DXT5:
		destination.setFormat(Format::B_DXT5);
		break;
	default:
		destination.setFormat(Format::INVALID);
		throw std::runtime_error("Invalid image format!");
	}
	destination.setIsAnimated(false);
	for(auto& it : head.mipmaps) {
		auto& frame = destination.addFrame(it.width,it.height);
		frame.data = std::move(it.bytes);
	}
}

}
}
}
}
