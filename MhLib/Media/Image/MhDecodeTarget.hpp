#ifndef MHDECODETARGET_HPP
#define MHDECODETARGET_HPP
#include <MhLib/Io/MhIoDevice.hpp>
#include <MhLib/IoSys/MhIoSystem.hpp>
#include <MhLib/Media/Image/MhStandardColourFormat.hpp>
#include <optional>
#include <MhLib/Media/Image/MhImageType.hpp>

namespace MH33 {
namespace Image {

struct Palette {
	std::vector<std::byte> palette;
	Format format;
	int transparentColorIndex;
};

struct Frame {
	std::vector<std::byte> imageData;
	unsigned width,height,stride;
	template <typename T> std::span<T> asDataSpan() {
		return std::span<T>( reinterpret_cast<T*>( imageData.data() ), width * height);
	}
	template <typename T> std::span<const T> asDataSpan() const {
		return std::span<const T>( reinterpret_cast<const T*>( imageData.data() ), width * height);
	}
};

struct DecodeTarget {
	std::vector<Frame> frames;
	std::optional<Palette> palette; // Used for INDEXED only.
	std::optional<float> delayTime; // Used for animated pictures only.
	bool isAnimated; // If false, then the frames are mipmaps. If true, then we except width and height to remain constant.
	Format format;
};


}
}
#endif // MHDECODETARGET_HPP
