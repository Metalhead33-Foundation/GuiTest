#ifndef EUPHTGA_HPP
#define EUPHTGA_HPP
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <Euphemy/Config/EuphLib.hpp>
#include <ctime>
#include <chrono>

namespace Euph {
namespace Media {
namespace Image {
namespace TGA {

struct Version {
	uint16_t VersionNUmberX100;
	char VersionLetter;
};

struct ExtensionInformation {
	std::string_view authorName;
	std::string_view authorComment;
	time_t timestamp;
	std::string_view jobId;
	std::chrono::seconds jobTime;
	std::string_view softwareID;
	Version softwareVersion;
	uint32_t keyColour;
	double gammaValue;
};

void MH_EUPH_API decode(Elv::Io::Device& iodev, DecodeTarget& destination);
void MH_EUPH_API encode(Elv::Io::Device& iodev, DecodeTarget& source, const std::optional<ExtensionInformation>& ext = {}, bool stealSourceImage=false);

}
}
}
}
#endif // EUPHTGA_HPP
