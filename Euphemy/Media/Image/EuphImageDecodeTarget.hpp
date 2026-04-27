#ifndef EUPHIMAGEDECODETARGET_H
#define EUPHIMAGEDECODETARGET_H
/**
 * @file EuphImageDecodeTarget.hpp
 * @brief Declares the EuphImageDecodeTarget API in the Euphemy/Media/Image module.
 *
 * This header is part of the public declaration surface for Euphemy/Media/Image.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Euphemy/Media/Image/EuphImageType.hpp>
#include <vector>
#include <optional>
#include <span>
#include <memory_resource>
#include <Euphemy/Config/EuphLib.hpp>

namespace Euph {
namespace Media {
namespace Image {

/**
 * @struct Palette
 * @brief Represents a color palette with optional transparency.
 */
struct MH_EUPH_API Palette {
	/**
	 * @brief The raw palette data.
	 */
	std::pmr::vector<std::byte> data;

	/**
	 * @brief The format of the palette.
	 */
	Format format;

	/**
	 * @brief The index of the transparent color, if any.
	 */
	int transparentColorIndex;

	/**
	 * @brief Constructs a Palette with a custom memory resource.
	 * @param memResource The memory resource to use for allocations.
	 */
	Palette(std::pmr::memory_resource* memResource);

	/**
	 * @brief Interprets the palette data as a span of a specific type.
	 * @tparam T The type to interpret the data as.
	 * @return A span of the interpreted data.
	 */
	template <typename T>
	inline std::span<T> asDataSpan() {
		return std::span<T>(reinterpret_cast<T*>(data.data()), data.size() / sizeof(T));
	}

	/**
	 * @brief Interprets the palette data as a read-only span of a specific type.
	 * @tparam T The type to interpret the data as.
	 * @return A read-only span of the interpreted data.
	 */
	template <typename T>
	inline std::span<const T> asDataSpan() const {
		return std::span<const T>(reinterpret_cast<const T*>(data.data()), data.size() / sizeof(T));
	}
};

/**
 * @struct Frame
 * @brief Represents a single frame of image data.
 */
struct MH_EUPH_API Frame {
	/**
	 * @brief The raw frame data.
	 */
	std::pmr::vector<std::byte> data;

	/**
	 * @brief The width of the frame.
	 */
	unsigned width{};

	/**
	 * @brief The height of the frame.
	 */
	unsigned height{};

	/**
	 * @brief The stride (row width in bytes) of the frame.
	 */
	unsigned stride{};

	/**
	 * @brief Constructs a Frame with a custom memory resource.
	 * @param memResource The memory resource to use for allocations.
	 */
	Frame(std::pmr::memory_resource* memResource);

	/**
	 * @brief Interprets the frame data as a span of a specific type.
	 * @tparam T The type to interpret the data as.
	 * @return A span of the interpreted data.
	 */
	template <typename T>
	inline std::span<T> asDataSpan() {
		return std::span<T>(reinterpret_cast<T*>(data.data()), width * height);
	}

	/**
	 * @brief Interprets the frame data as a read-only span of a specific type.
	 * @tparam T The type to interpret the data as.
	 * @return A read-only span of the interpreted data.
	 */
	template <typename T>
	inline std::span<const T> asDataSpan() const {
		return std::span<const T>(reinterpret_cast<const T*>(data.data()), width * height);
	}

	/**
	 * @brief Allocates memory for the frame based on the format and dimensions.
	 * @param format The format of the frame.
	 * @param width The width of the frame.
	 * @param height The height of the frame.
	 */
	void allocate(Format format, unsigned width, unsigned height);
};

/**
 * @class DecodeTarget
 * @brief Represents a target for decoding image or animation data.
 */
class MH_EUPH_API DecodeTarget {
	std::pmr::vector<Frame> frames; ///< The collection of frames.
	std::optional<Palette> palette; ///< The optional palette associated with the target.
	bool isAnimated{};			  ///< Whether the target represents an animation.
	Format format{};				///< The format of the data.
	std::optional<float> delayTime; ///< The optional delay time for animations.
	std::pmr::memory_resource* memResource; ///< The memory resource for allocations.

public:
	/**
	 * @brief Constructs a DecodeTarget with an optional custom memory resource.
	 * @param memResource The memory resource to use for allocations. Defaults to the global default resource.
	 */
	DecodeTarget(std::pmr::memory_resource* memResource = std::pmr::get_default_resource());

	/**
	 * @brief Adds a new frame to the target.
	 * @param width The width of the frame.
	 * @param height The height of the frame.
	 * @return A reference to the newly added frame.
	 */
	Frame& addFrame(unsigned width, unsigned height);

	/**
	 * @brief Creates a new palette for the target.
	 * @param format The format of the palette.
	 * @param indices The number of indices in the palette.
	 * @param transparentIndex The index of the transparent color.
	 * @return A reference to the newly created palette.
	 */
	Palette& createPalette(Format format, unsigned indices, int transparentIndex);

	/**
	 * @brief Retrieves a frame by index.
	 * @param index The index of the frame to retrieve.
	 * @return A reference to the frame.
	 */
	Frame& getFrame(size_t index);

	/**
	 * @brief Retrieves a frame by index (const version).
	 * @param index The index of the frame to retrieve.
	 * @return A const reference to the frame.
	 */
	const Frame& getFrame(size_t index) const;

	/**
	 * @brief Retrieves the optional palette associated with the target.
	 * @return A reference to the optional palette.
	 */
	std::optional<Palette>& getPalette();

	/**
	 * @brief Retrieves the optional palette associated with the target (const version).
	 * @return A const reference to the optional palette.
	 */
	const std::optional<Palette>& getPalette() const;

	/**
	 * @brief Checks if the target represents an animation.
	 * @return True if the target is animated, false otherwise.
	 */
	bool getIsAnimated() const;

	/**
	 * @brief Sets whether the target represents an animation.
	 * @param newIsAnimated The new animation status.
	 */
	void setIsAnimated(bool newIsAnimated);

	/**
	 * @brief Retrieves the format of the target.
	 * @return The format of the target.
	 */
	Format getFormat() const;

	/**
	 * @brief Sets the format of the target.
	 * @param newFormat The new format of the target.
	 */
	void setFormat(Format newFormat);

	/**
	 * @brief Retrieves the optional delay time for animations.
	 * @return The optional delay time.
	 */
	std::optional<float> getDelayTime() const;

	/**
	 * @brief Sets the optional delay time for animations.
	 * @param newDelayTime The new delay time.
	 */
	void setDelayTime(std::optional<float> newDelayTime);

	/**
	 * @brief Retrieves the collection of frames (const version).
	 * @return A const reference to the collection of frames.
	 */
	const std::pmr::vector<Frame>& getFrames() const;

	/**
	 * @brief Retrieves the collection of frames.
	 * @return A reference to the collection of frames.
	 */
	std::pmr::vector<Frame>& getFrames();

	/**
	 * @brief Retrieves the memory resource used for allocations.
	 * @return The memory resource.
	 */
	std::pmr::memory_resource* getMemResource() const;
};


}
}
}
#endif // EUPHIMAGEDECODETARGET_H
