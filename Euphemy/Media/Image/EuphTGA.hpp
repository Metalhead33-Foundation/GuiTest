#ifndef EUPHTGA_HPP
#define EUPHTGA_HPP
/**
 * @file EuphTGA.hpp
 * @brief Declares the EuphTGA API in the Euphemy/Media/Image module.
 *
 * This header is part of the public declaration surface for Euphemy/Media/Image.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <Euphemy/Config/EuphLib.hpp>
#include <ctime>
#include <chrono>

namespace Euph {
namespace Media {
namespace Image {
namespace TGA {

/**
 * @struct Version
 * @brief Represents the version of a software or data format.
 */
struct Version {
	uint16_t VersionNumberX100; /**< Version number multiplied by 100. */
	char VersionLetter;		 /**< Version letter (e.g., 'a', 'b', 'c'). */
};

/**
 * @struct ExtensionInformation
 * @brief Holds additional information that can be encoded or decoded with the main data.
 */
struct ExtensionInformation {
	std::string_view authorName;	/**< Name of the author of the data or software. */
	std::string_view authorComment; /**< Comment from the author. */
	time_t timestamp;			 /**< Timestamp indicating when the data was created or modified. */
	std::string_view jobId;		 /**< Unique identifier for the job or session. */
	std::chrono::seconds jobTime;   /**< Duration of the job or processing time. */
	std::string_view softwareID;	/**< Identifier for the software used. */
	Version softwareVersion;		/**< Version of the software. */
	uint32_t keyColour;			 /**< Key color used in the data (e.g., for transparency). */
	double gammaValue;			  /**< Gamma correction value applied to the data. */
};

/**
 * @brief Decodes TGA Image data from a device into a destination target.
 *
 * @param iodev	  The input/output device from which to read the data.
 * @param destination The target where the decoded data will be stored.
 */
void MH_EUPH_API decode(Elv::Io::Device& iodev, DecodeTarget& destination);

/**
 * @brief Encodes TGA Image data from a source target into a device, optionally including extension information.
 *
 * @param iodev	  The input/output device into which the data will be written.
 * @param source	 The source from which the data will be read.
 * @param ext		Optional extension information to include with the encoded data.
 * @param stealSourceImage If true, the source image will be left in an invalid state after being saved. This can be more performant than making a full copy of the source image.
 */
void MH_EUPH_API encode(Elv::Io::Device& iodev, DecodeTarget& source, const std::optional<ExtensionInformation>& ext = {}, bool stealSourceImage = false);

}
}
}
}
#endif // EUPHTGA_HPP
