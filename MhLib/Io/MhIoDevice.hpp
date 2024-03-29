#ifndef MHIODEVICE_HPP
#define MHIODEVICE_HPP
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <sstream>
#include <MhLib/Util/MhBuffer.hpp>
#include <MhLib/Io/MhIo.hpp>

namespace MH33 {
namespace Io {

enum class SeekOrigin : uint8_t {
	SET, // Beginning of file
	CUR, // Current position
	END // End of file
};

enum class Mode : uint8_t {
	READ = 1,
	WRITE = 2,
	APPEND = 4,
	READ_WRITE = (READ | WRITE),
	READ_APPEND = (READ | APPEND)
};

DEFINE_CLASS(Device)
class Device {
public:
	virtual ~Device() = default;
	virtual bool flush() = 0;
	virtual bool seek(SeekOrigin whence, intptr_t offset) = 0;
	virtual intptr_t tell() = 0;
	virtual intptr_t size() = 0;
	virtual size_t write(const void* data, size_t dataSize) = 0;
	virtual size_t read(void* destination, size_t dataSize) = 0;
	virtual Mode getMode() const = 0;
	virtual bool isValid() const = 0;
	// A convenience function
	void readAll(Util::Buffer& dst) {
		auto sz = size()-tell();
		dst.resize(sz);
		read(dst.data(),sz);
	}
	Util::Buffer readAll(void) {
		Util::Buffer tmp;
		readAll(tmp);
		return tmp;
	}
	void readAllAsString(std::string& dst) {
		auto sz = size()-tell();
		dst.resize(sz,0);
		read(&dst[0],sz);
		dst.shrink_to_fit();
	}
	std::string readAllAsString(void) {
		std::string tmp;
		readAllAsString(tmp);
		return tmp;
	}
	void readLine(std::stringstream& stream) {
		char tmp = 0;
		while(read(&tmp,sizeof(char)) && tmp != '\n') {
			stream << tmp;
		}
	}
	std::string readLine(void) {
		std::stringstream stream;
		readLine(stream);
		return stream.str();
	}
	size_t write(const Util::Buffer& src) {
		return write(src.data(),src.size());
	}
};
typedef std::function<Device*(Mode)> DeviceCreator;

}
}
#endif // MHIODEVICE_HPP
