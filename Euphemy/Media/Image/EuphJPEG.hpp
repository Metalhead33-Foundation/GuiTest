#ifndef EUPHJPEG_HPP
#define EUPHJPEG_HPP
/**
 * @file EuphJPEG.hpp
 * @brief Declares the EuphJPEG API in the Euphemy/Media/Image module.
 *
 * This header is part of the public declaration surface for Euphemy/Media/Image.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <Euphemy/Config/EuphLib.hpp>
#include <memory>
namespace Euph {
namespace Media {
namespace Image {
namespace JPEG {

/*
Common values are 0 (no subsampling), 1 (4:2:2), and 2 (4:2:0).
*/
/** @brief Documents the SubsamplingMethod type or declaration. */
enum class SubsamplingMethod : uint8_t {
	NO_SUBSAMPLING = 0,
	SUBSAMPLE_4_2_2 = 1,
	SUBSAMPLE_4_2_0 = 2
};

/**
 * @brief Decodes a JPEG image from a buffer.
 * @param inbuf A buffer containing undecoded JPEG data.
 * @param destination The DecodeTarget object where the decoded image will be stored.
 * @return True if decoding is successful, false otherwise.
 */
bool MH_EUPH_API decode(const std::span<const std::byte>& inbuf, DecodeTarget& destination);

/**
 * @brief Decodes a JPEG image from an IO device.
 * @param iodev The IO device to read the JPEG image from.
 * @param destination The DecodeTarget object where the decoded image will be stored.
 * @return True if decoding is successful, false otherwise.
 */
bool MH_EUPH_API decode(Elv::Io::Device& iodev, DecodeTarget& destination);

/**
 * @brief Encodes an image frame into a JPEG image.
 * @param frame The image frame to encode into a JPEG file.
 * @param format The format of the image. Only a limited number of formats are accepted for JPEG.
 * @param buffer Output buffer that must be preallocated. Use `tjBufSize` to determine a safe capacity.
 * @param jpegSubsamp The JPEG subsampling method.
 * @param jpegQual The JPEG quality. The smaller the value, the smaller the file will be, but the image will look worse. Valid range is typically between 0 and 100.
 * @param jpegSize A reference to write out the actual size of the JPEG data written to the buffer. This is not guaranteed to use the entire allocated buffer.
 * @return True if encoding is successful, false otherwise.
 */
bool MH_EUPH_API encode(const Frame& frame, Format format,
						std::span<std::byte> buffer,
						SubsamplingMethod jpegSubsamp, float jpegQual, unsigned long &jpegSize);

/**
 * @brief Encodes an image into a JPEG image.
 * @param source The image to encode into a JPEG file.
 * @param buffer Output buffer that must be preallocated. Use `tjBufSize` to determine a safe capacity.
 * @param jpegSubsamp The JPEG subsampling method.
 * @param jpegQual The JPEG quality. The smaller the value, the smaller the file will be, but the image will look worse. Valid range is typically between 0 and 100.
 * @param jpegSize A reference to write out the actual size of the JPEG data written to the buffer. This is not guaranteed to use the entire allocated buffer.
 * @return True if encoding is successful, false otherwise.
 */
bool MH_EUPH_API encode(const DecodeTarget& source,
						std::span<std::byte> buffer,
						SubsamplingMethod jpegSubsamp, float jpegQual, unsigned long &jpegSize);
/**
 * @brief Encodes an image frame into a JPEG image and writes it to an IO device.
 * @param iodev The IO device used to save the JPEG image.
 * @param frame The image frame to encode into a JPEG file.
 * @param format The format of the image. Only a limited number of formats are accepted for JPEG.
 * @param jpegSubsamp The JPEG subsampling method.
 * @param jpegQual The JPEG quality. The smaller the value, the smaller the file will be, but the image will look worse. Valid range is typically between 0 and 1.
 * @return True if encoding is successful, false otherwise.
 */
bool MH_EUPH_API encode(Elv::Io::Device& iodev, const Frame& frame, Format format, SubsamplingMethod jpegSubsamp, float jpegQual);

/**
 * @brief Encodes an image into a JPEG image and writes it to an IO device.
 * @param iodev The IO device used to save the JPEG image.
 * @param source The image to encode into a JPEG file.
 * @param jpegSubsamp The JPEG subsampling method.
 * @param jpegQual The JPEG quality. The smaller the value, the smaller the file will be, but the image will look worse. Valid range is typically between 0 and 1.
 * @return True if encoding is successful, false otherwise.
 */
bool MH_EUPH_API encode(Elv::Io::Device& iodev, const DecodeTarget& source, SubsamplingMethod jpegSubsamp, float jpegQual);

}
}
}
}
#endif // EUPHJPEG_HPP
