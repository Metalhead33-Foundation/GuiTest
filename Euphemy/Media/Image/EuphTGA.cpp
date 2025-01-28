#include "EuphTGA.hpp"
#include <Elvavena/Io/ElvDataStream.hpp>
namespace Euph {
namespace Media {
namespace Image {
namespace TGA {

enum class Type {
	INVALID = 0x00,
	RGB = 0x01,
	GREYSCALE = 0x02,
	PALETTIZED = 0x03
};

struct TgaImageSpecification {
	uint16_t xOrigin;
	uint16_t yOrigin;
	uint16_t width;
	uint16_t height;
	uint8_t pixelDepth;
	uint8_t imageDescriptor;
};
template <Elv::Util::Endian endianness>
Elv::Io::DataStream<endianness>& operator<<(Elv::Io::DataStream<endianness>& left, const TgaImageSpecification& right) {
	return (left << right.xOrigin << right.yOrigin << right.width << right.height << right.pixelDepth << right.imageDescriptor);
}
template <Elv::Util::Endian endianness>
Elv::Io::DataStream<endianness>& operator>>(Elv::Io::DataStream<endianness>& left, TgaImageSpecification& right) {
	return (left >> right.xOrigin >> right.yOrigin >> right.width >> right.height >> right.pixelDepth >> right.imageDescriptor);
}
struct TgaColorMapSpecification {
	uint16_t firstEntryIndex;
	uint16_t colorMapLength;
	uint8_t colorMapEntrySize;
};
template <Elv::Util::Endian endianness>
Elv::Io::DataStream<endianness>& operator<<(Elv::Io::DataStream<endianness>& left, const TgaColorMapSpecification& right) {
	return (left << right.firstEntryIndex << right.colorMapLength << right.colorMapEntrySize );
}
template <Elv::Util::Endian endianness>
Elv::Io::DataStream<endianness>& operator>>(Elv::Io::DataStream<endianness>& left, TgaColorMapSpecification& right) {
	return (left >> right.firstEntryIndex >> right.colorMapLength >> right.colorMapEntrySize);
}

struct TgaExtensionInformation {
	uint16_t extensionSize;
	std::array<char,41> authorName;
	std::array<char,324> authorComment;
	std::array<uint16_t,6> timestamp;
	std::array<char,41> jobId;
	std::array<uint16_t,3> jobTime;
	std::array<char,41> softwareID;
	Version softwareVersion;
	uint32_t keyColour;
	std::array<uint16_t,2> pixelAspectRatio;
	std::array<uint16_t,2> gammaValue;
	uint32_t colourCorrectionOffset;
	uint32_t postageStampOffset;
	uint32_t scanlineOffset;
	uint8_t attributeType;
};

template <Elv::Util::Endian endianness>
Elv::Io::DataStream<endianness>& operator<<(Elv::Io::DataStream<endianness>& left, const TgaExtensionInformation& right) {
	left << right.extensionSize;
	left << right.authorName;
	left << right.authorComment;
	left << right.timestamp;
	left << right.jobId;
	left << right.jobTime;
	left << right.softwareID;
	left << right.softwareVersion.VersionNumberX100 << right.softwareVersion.VersionLetter;
	left << right.keyColour;
	left << right.pixelAspectRatio;
	left << right.gammaValue;
	left << right.colourCorrectionOffset;
	left << right.postageStampOffset;
	left << right.scanlineOffset;
	left << right.attributeType;
	return left;
}
template <Elv::Util::Endian endianness>
Elv::Io::DataStream<endianness>& operator>>(Elv::Io::DataStream<endianness>& left, TgaExtensionInformation& right) {
	left >> right.extensionSize;
	left >> right.authorName;
	left >> right.authorComment;
	left >> right.timestamp;
	left >> right.jobId;
	left >> right.jobTime;
	left >> right.softwareID;
	left >> right.softwareVersion.VersionNumberX100 >> right.softwareVersion.VersionLetter;
	left >> right.keyColour;
	left >> right.pixelAspectRatio;
	left >> right.gammaValue;
	left >> right.colourCorrectionOffset;
	left >> right.postageStampOffset;
	left >> right.scanlineOffset;
	left >> right.attributeType;
	return left;
}

struct TgaHeader {
public:
	// Read information - straight from the file
	uint8_t idLen;
	uint8_t colmapType;
	uint8_t imageType;
	TgaColorMapSpecification colorMapSpecification;
	TgaImageSpecification imageSpecification;
	std::array<char,255> idField;
	int32_t extensionOffset;
	int32_t developerAreaOffset;
	TgaExtensionInformation extensionInfo;
	// Deduced information - deduced from read info
	int version;
	Type format;
	std::pmr::vector<std::byte> colorMap;
	std::pmr::vector<std::byte> imageData;
	// Commands
	TgaHeader(std::pmr::memory_resource* memRes) : colorMap(memRes), imageData(memRes) {

	}
private:
	void decodeImage(size_t imageSize, Elv::Io::Device& input);
	void decodeCompressedImage(size_t imageSize, Elv::Io::Device& output);
	void flipVert();
	void flipHoriz();
public:
	template <Elv::Util::Endian endianness> friend Elv::Io::DataStream<endianness>& operator>>(Elv::Io::DataStream<endianness>& left, TgaHeader& right);
	template <Elv::Util::Endian endianness> friend Elv::Io::DataStream<endianness>& operator<<(Elv::Io::DataStream<endianness>& left, TgaHeader& right);
};

template <Elv::Util::Endian endianness> Elv::Io::DataStream<endianness>& operator>>(Elv::Io::DataStream<endianness>& left, TgaHeader& right) {
	Elv::Io::Device& input = left.device;
	auto currPos = input.tell();
	input.seek(-26,Elv::Io::SeekOrigin::END);
	left >> right.extensionOffset;
	left >> right.developerAreaOffset;
	char versionCheck[17] = "";
	input.read(versionCheck,16,1);
	if ( strcmp( versionCheck, "TRUEVISION-XFILE" ) == 0 )
		right.version = 2;
	else {
		right.version = 1;
		right.extensionOffset = -1;
		right.developerAreaOffset = -1;
	}
	if ( right.extensionOffset > 0 )
	{
		input.seek(right.extensionOffset, Elv::Io::SeekOrigin::SET );
		left >> right.extensionInfo;
	}

	input.seek(currPos,Elv::Io::SeekOrigin::SET);

	left >> right.idLen;
	left >> right.colmapType;
	left >> right.imageType;
	left >> right.colorMapSpecification;
	left >> right.imageSpecification;
	bool isCompressed = false;
	switch (right.imageType) {
	case 0:
		right.format = Type::INVALID;
		break;
	case 1:
		right.format = Type::PALETTIZED;
		break;
	case 2:
		right.format = Type::RGB;
		break;
	case 3:
		right.format = Type::GREYSCALE;
		break;
	case 9:
		right.format = Type::PALETTIZED;
		isCompressed = true;
		break;
	case 10:
		right.format = Type::RGB;
		isCompressed = true;
		break;
	case 11:
		right.format = Type::GREYSCALE;
		isCompressed = true;
		break;
	default:
		right.format = Type::INVALID;
		break;
	}
	//input.seek(right.idLen,Elv::Io::SeekOrigin::CUR); // Skip ID.
	if(right.idLen) input.read(right.idField.data(),1,right.idLen);
	if(right.colmapType) {
		size_t colMapSize = right.colorMapSpecification.colorMapLength * (right.colorMapSpecification.colorMapEntrySize / 8);
		right.colorMap.resize(colMapSize);
		input.read(right.colorMap.data(),sizeof(std::byte),colMapSize);
	}
	size_t imageSize = right.imageSpecification.width * right.imageSpecification.height * (right.imageSpecification.pixelDepth / 8);
	if(isCompressed) right.decodeCompressedImage(imageSize,input);
	else right.decodeImage(imageSize, input);
	const bool fliphoriz = (right.imageSpecification.imageDescriptor & 0x10) ? true : false;
	const bool flipvert = (right.imageSpecification.imageDescriptor & 0x20) ? false : true;
	if(flipvert) right.flipVert();
	if(fliphoriz) right.flipHoriz();
	return left;
}
template <Elv::Util::Endian endianness> Elv::Io::DataStream<endianness>& operator<<(Elv::Io::DataStream<endianness>& left, TgaHeader& right) {
	Elv::Io::Device& output = left.device;
	left << right.idLen;
	left << right.colmapType;
	left << right.imageType;
	left << right.colorMapSpecification;
	left << right.imageSpecification;
	// Write the ID field if there is one
	if (right.idLen) {
		output.write(right.idField.data(), sizeof(char), right.idLen);
	}
	if(right.colmapType)
	{
		output.write(right.colorMap.data(),sizeof(std::byte),right.colorMap.size());
	}
	output.write(right.imageData.data(),sizeof(std::byte), right.imageData.size());
	right.extensionOffset = output.tell();
	left << right.extensionInfo;
	right.developerAreaOffset = output.tell();
	output.write("MH33\0", sizeof(char), 5);
	left << right.extensionOffset;
	left << right.developerAreaOffset;
	output.seek(0,Elv::Io::SeekOrigin::END);
	output.write("TRUEVISION-XFILE.\0",1,18);
	return left;
}

void TgaHeader::decodeImage(size_t imageSize, Elv::Io::Device& input)
{
	imageData.resize(imageSize);
	input.read(imageData.data(),sizeof(std::byte),imageSize); // Will read past the end of file in case of RLE.
}

void TgaHeader::decodeCompressedImage(size_t imageSize, Elv::Io::Device &input)
{
	imageData.resize(imageSize);
	input.read(imageData.data(),sizeof(std::byte),imageSize); // Will read past the end of file in case of RLE.
	std::pmr::vector<std::byte> tempImageData = imageData;
	int indexAccum = 0;
	int bytesPerPixel = ( imageSpecification.pixelDepth / 8 );
	int bytesPerPixelRLE = bytesPerPixel + 1;
	for ( int i = 0; indexAccum < imageSize; ) {
		int runCount = ( 127 & reinterpret_cast<uint8_t&>(tempImageData[i]) ) + 1;
		// Compressed data
		if ( 128 & reinterpret_cast<uint8_t&>(tempImageData[i]) ) {
			for ( int j = 0; j < runCount; j++ ) {
				for ( int k = 1; k < bytesPerPixelRLE; k++ ) {
					imageData[indexAccum++] = tempImageData[i + k];
				}
			}
			i += bytesPerPixelRLE;
		}
		// Raw data?
		else if ( !( 128 & reinterpret_cast<uint8_t&>(tempImageData[i]) ) ) {
			i++;
			for ( int j = 0; j < runCount; j++ ) {
				for ( int k = 0; k < bytesPerPixel; k++ ) {
					imageData[indexAccum++] = tempImageData[i + k];
				}
				i += bytesPerPixel;
			}
		}
	}
}

void TgaHeader::flipVert()
{
	const uint32_t stride = imageSpecification.width * (imageSpecification.pixelDepth / 8);
	std::pmr::vector<std::byte> reverseImage(imageData.size());
	for(uint32_t i = 0; i < imageSpecification.height; ++i) {
		memcpy(&reverseImage[stride*(imageSpecification.height-(i+1))], &imageData[i*stride], stride);
	}
	imageData = std::move(reverseImage);
	/*for(uint32_t i = 0; i < imageSpecification.height; ++i) {
		Buffer tmpbuffA(stride);
		Buffer tmpbuffB(stride);
		std::byte* ptrA = &imageData[stride*i];
		std::byte* ptrB = &imageData[stride*(imageSpecification.height-(i+1))];
		memcpy(tmpbuffA.data(),ptrA,stride);
		memcpy(tmpbuffB.data(),ptrB,stride);
		memcpy(ptrA,tmpbuffB.data(),stride);
		memcpy(ptrB,tmpbuffA.data(),stride);
	}*/
}

void TgaHeader::flipHoriz()
{
	const uint32_t stride = imageSpecification.width * (imageSpecification.pixelDepth / 8);
	for(uint32_t i = 0; i < imageSpecification.height; ++i) {
		std::byte* ptrA = &imageData[stride*i];
		switch (imageSpecification.pixelDepth) {
		case 8:
		{
			std::span<std::byte> spn(ptrA,stride);
			std::reverse(std::begin(spn),std::end(spn));
			break;
		}
		case 15:
		case 16:
		{
			std::span<uint16_t> spn(reinterpret_cast<uint16_t*>(ptrA),stride/2);
			std::reverse(std::begin(spn),std::end(spn));
			break;
		}
		case 24:
		{
			std::span<std::array<uint8_t,3>> spn(reinterpret_cast<std::array<uint8_t,3>*>(ptrA),stride/3);
			std::reverse(std::begin(spn),std::end(spn));
			break;
		}
		case 32:
		{
			std::span<uint32_t> spn(reinterpret_cast<uint32_t*>(ptrA),stride/4);
			std::reverse(std::begin(spn),std::end(spn));
			break;
		}
		default:
			break;
		}
	}
}

void decode(Elv::Io::Device& iodev, DecodeTarget& destination) {
	TgaHeader head(destination.getMemResource());
	Elv::Io::DataStream<Elv::Util::Endian::Little> input(iodev);
	input >> head;
	switch (head.format) {
		case Type::INVALID: destination.setFormat(Format::INVALID); return;
		case Type::RGB: switch (head.imageSpecification.pixelDepth) {
			case 15: destination.setFormat(Format::RGB555); break;
			case 16: destination.setFormat(Format::RGB565); break;
			case 24: destination.setFormat(Format::BGR8U); break;
			case 32: destination.setFormat(Format::BGRA8U); break;
			default: destination.setFormat(Format::INVALID); return;
			} break;
		case Type::GREYSCALE: switch (head.imageSpecification.pixelDepth) {
			case 8: destination.setFormat(Format::R8U); break;
			case 16: destination.setFormat(Format::R16U); break;
			case 32: destination.setFormat(Format::R32U); break;
			default: destination.setFormat(Format::INVALID); return;
			} break;
		case Type::PALETTIZED: {
			destination.setFormat(Format::INDEXED);
			Format palFormat;
			switch (head.colorMapSpecification.colorMapEntrySize) {
			case 15: palFormat = Format::RGB555; break;
			case 16: palFormat = Format::RGB565; break;
			case 24: palFormat = Format::BGR8U; break;
			case 32: palFormat = Format::BGRA8U; break;
			default: palFormat = Format::INVALID; return;
			} break;
			if(palFormat != Format::INVALID) {
				auto& palette = destination.createPalette(palFormat, head.colorMapSpecification.colorMapLength, 0);
				palette.data = std::move(head.colorMap);
			}
		}
		default: destination.setFormat(Format::INVALID); return;
	}
	auto& frame = destination.addFrame(head.imageSpecification.width, head.imageSpecification.width);
	frame.data = std::move(head.imageData);
}

void encode(Elv::Io::Device& iodev, DecodeTarget& source, const std::optional<ExtensionInformation>& ext, bool stealSourceImage)
{
	TgaHeader head(source.getMemResource());
	Elv::Io::DataStream<Elv::Util::Endian::Little> output(iodev);
	head.idLen = 4;
	head.idField[0] = 'M';
	head.idField[1] = 'H';
	head.idField[2] = '3';
	head.idField[3] = '3';
	switch (source.getFormat()) {
		case Format::INDEXED: {
			head.imageSpecification.pixelDepth = 8;
			head.imageType = 1;
			head.imageSpecification.imageDescriptor = 0;
			break;
		}
		case Format::RGB555: {
			head.imageSpecification.pixelDepth = 15;
			head.imageType = 2;
			head.imageSpecification.imageDescriptor = 0;
			break;
		}
		case Format::RGB565: {
			head.imageSpecification.pixelDepth = 16;
			head.imageType = 2;
			head.imageSpecification.imageDescriptor = 0;
			break;
		}
		case Format::BGR8U: {
			head.imageSpecification.pixelDepth = 24;
			head.imageType = 2;
			head.imageSpecification.imageDescriptor = 0;
			break;
		}
		case Format::BGRA8U: {
			head.imageSpecification.pixelDepth = 32;
			head.imageType = 2;
			head.imageSpecification.imageDescriptor = 8;
			break;
		}
		case Format::R8U: {
			head.imageSpecification.pixelDepth = 8;
			head.imageType = 3;
			head.imageSpecification.imageDescriptor = 0;
			break;
		}
		case Format::R16U: {
			head.imageSpecification.pixelDepth = 16;
			head.imageType = 3;
			head.imageSpecification.imageDescriptor = 0;
			break;
		}
		case Format::R32U: {
			head.imageSpecification.pixelDepth = 32;
			head.imageType = 3;
			head.imageSpecification.imageDescriptor = 0;
			break;
		}
		default: throw std::runtime_error("Invalid image format for TGA export!");
	}
	head.imageSpecification.imageDescriptor |= 0x20;
	auto& optionalPalette = source.getPalette();
	if(optionalPalette.has_value()) {
		auto& palette = optionalPalette.value();
		head.colmapType = 1;
		head.colorMapSpecification.firstEntryIndex = 0;
		head.colorMapSpecification.colorMapEntrySize = pixelByteSize(palette.format) * 8;
		head.colorMapSpecification.colorMapLength = (palette.data.size() / static_cast<size_t>(head.colorMapSpecification.colorMapEntrySize));
		if(stealSourceImage) head.colorMap = std::move(palette.data);
		else head.colorMap = palette.data;
	} else head.colmapType = 0;
	auto& frame = source.getFrame(0);
	head.imageSpecification.width = frame.width;
	head.imageSpecification.height = frame.height;
	if(stealSourceImage) head.imageData = std::move(frame.data);
	else head.imageData = frame.data;
	head.extensionInfo.extensionSize = 495;
	memset(&head.extensionInfo,0,sizeof(TgaExtensionInformation));
	if(ext.has_value()) {
		auto& extval = ext.value();
		if(!extval.authorName.empty())
		{
			std::memcpy(head.extensionInfo.authorName.data(),extval.authorName.data(),
					std::min(head.extensionInfo.authorName.size(),extval.authorName.size()));
		}
		if(!extval.authorComment.empty())
		{
		std::memcpy(head.extensionInfo.authorComment.data(),extval.authorComment.data(),
					std::min(head.extensionInfo.authorComment.size(),extval.authorComment.size()));
		}
		if(!extval.jobId.empty())
		{
		std::memcpy(head.extensionInfo.jobId.data(),extval.jobId.data(),
					std::min(head.extensionInfo.jobId.size(),extval.jobId.size()));
		}
		if(!extval.softwareID.empty())
		{
		std::memcpy(head.extensionInfo.softwareID.data(),extval.softwareID.data(),
					std::min(head.extensionInfo.softwareID.size(),extval.softwareID.size()));
		}
		head.extensionInfo.softwareVersion = extval.softwareVersion;
		head.extensionInfo.keyColour = extval.keyColour;
		head.extensionInfo.pixelAspectRatio.fill(1);
		// Job time
		{
			const int seconds = extval.jobTime.count() % 60;
			const int minutesWhole = (extval.jobTime.count() - seconds) / 60;
			const int minutes = minutesWhole % 60;
			const int hours = (minutesWhole - minutes) / 60;
			head.extensionInfo.jobTime[0] = hours;
			head.extensionInfo.jobTime[1] = minutes;
			head.extensionInfo.jobTime[2] = seconds;
		}
		// Date time
		{
			tm datetime;
			gmtime_r(&extval.timestamp, &datetime);
			head.extensionInfo.timestamp[0] = datetime.tm_mon;
			head.extensionInfo.timestamp[1] = datetime.tm_mday;
			head.extensionInfo.timestamp[2] = datetime.tm_year;
			head.extensionInfo.timestamp[3] = datetime.tm_hour;
			head.extensionInfo.timestamp[4] = datetime.tm_min;
			head.extensionInfo.timestamp[5] = datetime.tm_sec;
		}
		head.extensionInfo.attributeType = (source.getFormat() == Format::BGRA8U) ? 4 : 0;
	}
	// Now we write.
	output << head;
}

}
}
}
}
