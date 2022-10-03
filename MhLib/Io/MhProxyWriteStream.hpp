#ifndef MHPROXYWRITESTREAM_HPP
#define MHPROXYWRITESTREAM_HPP
#include <MhLib/Io/MhIoDevice.hpp>
#include <vector>

namespace MH33 {
namespace Io {

class MH_IO_API ProxyWriteStream : public Device
{
private:
	ProxyWriteStream(const ProxyWriteStream& cpy) = delete; // No copy constructor
	ProxyWriteStream& operator=(const ProxyWriteStream& cpy) = delete; // No copy assignment operator
	Device* outFront;
	std::vector<std::byte> inBuff;
	size_t inBuffCursor;
	std::vector<std::byte> outBuff;
	size_t outBuffCursor;
protected:
	virtual void fillBuffers(const void* input, size_t inSize, void* outBuff, size_t& outBuffCursor, size_t outBuffMaxSize) = 0;
public:
	// Destructor
	virtual ~ProxyWriteStream() = default;
	// Default constructor
	ProxyWriteStream(size_t inBuffSize, size_t outBuffSize);
	ProxyWriteStream(Device* inFront, size_t inBuffSize, size_t outBuffSize);
	// Actual imnplementation
	bool flush();
	bool seek(SeekOrigin whence, intptr_t offset);
	intptr_t tell();
	intptr_t size();
	size_t write(const void* data, size_t dataSize);
	size_t read(void* destination, size_t dataSize);
	Mode getMode() const;
	bool isValid() const;
	Device *getOutFront() const;
	void setOutFront(Device *value);
};

}
}
#endif // MHPROXYWRITESTREAM_HPP
