#ifndef EUPHIMAGEDECODETARGET_HPP
#define EUPHIMAGEDECODETARGET_HPP
#include <Euphemy/Media/EuphImageType.hpp>
#include <Euphemy/Memory/EuphMemory.hpp>
#include <vector>
#include <optional>
#include <span>
#include <functional>
namespace Euph {
namespace Media {

struct PaletteRef {
	std::span<std::byte> palette;
	ImageFormat format;
	int transparentColorIndex;
	template <typename T> inline std::span<T> asDataSpan() {
		return std::span<T>( reinterpret_cast<T*>( palette.data() ), palette.size_bytes() / sizeof(T) );
	}
	template <typename T> inline std::span<const T> asDataSpan() const {
		return std::span<const T>( reinterpret_cast<const T*>( palette.data() ), palette.size_bytes() / sizeof(T) );
	}
};
typedef std::function<std::span<std::byte>(ImageFormat format, unsigned indices, unsigned transparentIndex)> PaletteAllocationFunction;
template <class Allocator = std::allocator<std::byte>>
requires Elv::Util::Allocator<Allocator, std::byte>
struct Palette {
	std::vector<std::byte, Allocator> palette;
	ImageFormat format;
	int transparentColorIndex;
	template <typename T> inline std::span<T> asDataSpan() {
		return std::span<T>( reinterpret_cast<T*>( palette.data() ), palette.size_bytes() / sizeof(T) );
	}
	template <typename T> inline std::span<const T> asDataSpan() const {
		return std::span<const T>( reinterpret_cast<const T*>( palette.data() ), palette.size_bytes() / sizeof(T) );
	}
	inline PaletteRef toRef() const {
		return PaletteRef{ palette, format, transparentColorIndex };
	}
	inline operator PaletteRef() const {
		return toRef();
	}
	inline PaletteAllocationFunction getAllocationFunction() {
		return [this](ImageFormat format, unsigned indices, unsigned transparentIndex) {
			this->format = format;
			this->transparentColorIndex = transparentIndex;
			this->palette.resize(pixelByteSize(format) * indices);
			return this->palette;
		};
	}
};


struct FrameRef {
	std::span<std::byte> imageData;
	unsigned width,height,stride;
	template <typename T> inline std::span<T> asDataSpan() {
		return std::span<T>( reinterpret_cast<T*>( imageData.data() ), width * height);
	}
	template <typename T> inline std::span<const T> asDataSpan() const {
		return std::span<const T>( reinterpret_cast<const T*>( imageData.data() ), width * height);
	}
};
typedef std::function<std::span<std::byte>(ImageFormat format, unsigned width, unsigned height)> ImageAllocationFunction;
template <class Allocator = std::allocator<std::byte>>
requires Elv::Util::Allocator<Allocator, std::byte>
struct Frame {
	std::vector<std::byte,Allocator> imageData;
	unsigned width,height,stride;
	template <typename T> inline std::span<T> asDataSpan() {
		return std::span<T>( reinterpret_cast<T*>( imageData.data() ), width * height);
	}
	template <typename T> inline std::span<const T> asDataSpan() const {
		return std::span<const T>( reinterpret_cast<const T*>( imageData.data() ), width * height);
	}
	inline FrameRef toRef() const {
		return FrameRef{ imageData, width, height, stride };
	}
	inline operator FrameRef() const {
		return toRef();
	}
	inline ImageAllocationFunction getAllocationFunction() {
		return [this](ImageFormat format, unsigned width, unsigned height) {
			this->width = width;
			this->height = height;
			this->stride = pixelByteSize(format) * width;
			this->imageData.resize(stride * height);
			return this->imageData;
		};
	}
};
typedef std::function<std::span<std::byte>(unsigned index, ImageFormat format, unsigned width, unsigned height)> FrameAllocationFunction;
typedef std::function<size_t(unsigned frameCount, ImageFormat format, bool isAnimated, std::optional<float> delayTime)> FrameCountInitializerFunction;
template <class Allocator = std::allocator<std::byte>>
requires Elv::Util::Allocator<Allocator, std::byte>
struct DecodeTarget {
	std::vector<Frame<Allocator>,Allocator> frames;
	std::optional<Palette<Allocator>> palette; // Used for INDEXED only.
	std::optional<float> delayTime; // Used for animated pictures only.
	bool isAnimated; // If false, then the frames are mipmaps. If true, then we except width and height to remain constant.
	ImageFormat format;
	inline FrameCountInitializerFunction getFrameInitializerFunction() {
		return [this](unsigned frameCount, ImageFormat format, bool isAnimated, std::optional<float> delayTime) {
			this->format = format;
			this->delayTime = delayTime;
			this->isAnimated = isAnimated;
			this->frames.resize(frameCount);
			return this->frames.size();
		};
	}
	inline FrameAllocationFunction getFrameAllocationFunction() {
		return [this](unsigned index, ImageFormat format, unsigned width, unsigned height) {
			Frame<Allocator>& frame = frames[index];
			return frame.getAllocationFunction()(format,width,height);
			/*frame.width = width;
			frame.height = height;
			frame.stride = pixelByteSize(format) * width;
			frame.imageData.resize(frame.stride * frame.height);
			return frame.imageData;*/
		};
	}
	inline PaletteAllocationFunction getPaletteAllocationFunction() {
		return [this](ImageFormat format, unsigned indices, unsigned transparentIndex)
		{
			Palette<Allocator> pal;
			auto toReturn = pal.getAllocationFunction()(format, indices, transparentIndex);
			palette.emplace(std::move(pal));
			return toReturn;
		};
	}
};

}
}
#endif // EUPHIMAGEDECODETARGET_HPP
