#ifndef EUPHIMAGEDECODETARGET_H
#define EUPHIMAGEDECODETARGET_H
#include <Euphemy/Media/Image/EuphImageType.hpp>
#include <vector>
#include <optional>
#include <span>
#include <memory_resource>

namespace Euph {
namespace Media {
namespace Image {

struct Palette {
	std::pmr::vector<std::byte> data;
	Format format;
	int transparentColorIndex;

	Palette(std::pmr::memory_resource* memResource);

	template <typename T>
	std::span<T> asDataSpan() {
		return std::span<T>(reinterpret_cast<T*>(data.data()), data.size() / sizeof(T));
	}

	template <typename T>
	std::span<const T> asDataSpan() const {
		return std::span<const T>(reinterpret_cast<const T*>(data.data()), data.size() / sizeof(T));
	}
};
struct Frame {
	std::pmr::vector<std::byte> data;
	unsigned width{}, height{}, stride{};

	Frame(std::pmr::memory_resource* memResource);

	template <typename T>
	std::span<T> asDataSpan() {
		return std::span<T>(reinterpret_cast<T*>(data.data()), width * height);
	}

	template <typename T>
	std::span<const T> asDataSpan() const {
		return std::span<const T>(reinterpret_cast<const T*>(data.data()), width * height);
	}

	void allocate(Format format, unsigned width, unsigned height);
};
class DecodeTarget {
	std::pmr::vector<Frame> frames;
	std::optional<Palette> palette;
	bool isAnimated{};
	Format format{};
	std::optional<float> delayTime{};
	std::pmr::memory_resource* memResource;

public:
	DecodeTarget(std::pmr::memory_resource* memResource = std::pmr::get_default_resource());

	Frame& addFrame(unsigned width, unsigned height);

	Palette& createPalette(Format format, unsigned indices, unsigned transparentIndex);

	Frame& getFrame(size_t index);
	const Frame& getFrame(size_t index) const;
	std::optional<Palette>& getPalette();
	const std::optional<Palette>& getPalette() const;
	bool getIsAnimated() const;
	void setIsAnimated(bool newIsAnimated);
	Format getFormat() const;
	void setFormat(Format newFormat);
	std::optional<float> getDelayTime() const;
	void setDelayTime(std::optional<float> newDelayTime);
	const std::pmr::vector<Frame>& getFrames() const;
	std::pmr::vector<Frame>& getFrames();
	std::pmr::memory_resource* getMemResource() const;
};

}
}
}
#endif // EUPHIMAGEDECODETARGET_H
