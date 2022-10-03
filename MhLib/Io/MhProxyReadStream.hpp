#ifndef MHPROXYREADSTREAM_HPP
#define MHPROXYREADSTREAM_HPP
#include <MhLib/Io/MhIoDevice.hpp>
#include <vector>

namespace MH33 {
namespace Io {

class MH_IO_API ProxyReadStream : public Device
{
private:
	ProxyReadStream(const ProxyReadStream& cpy) = delete; // No copy constructor
	ProxyReadStream& operator=(const ProxyReadStream& cpy) = delete; // No copy assignment operator
	Device* inFront;
	std::vector<std::byte> inBuff;
	size_t inBuffCursor;
	size_t inBuffSize;
	std::vector<std::byte> outBuff;
	size_t outBuffCursor;
	size_t outBuffEnd;
protected:
	virtual void fillBuffers(const void* inBuff, size_t& inBuffCursor, size_t inBuffSize, void* outBuff, size_t& outBuffEnd, size_t outBuffSize) = 0;
public:
	// Destructor
	virtual ~ProxyReadStream() = default;
	// Default constructor
	ProxyReadStream(size_t inBuffSize, size_t outBuffSize);
	ProxyReadStream(Device* inFront, size_t inBuffSize, size_t outBuffSize);
	// Actual imnplementation
	bool flush();
	bool seek(SeekOrigin whence, intptr_t offset);
	intptr_t tell();
	intptr_t size();
	size_t write(const void* data, size_t dataSize);
	size_t read(void* destination, size_t dataSize);
	Mode getMode() const;
	bool isValid() const;
	Device *getInFront() const;
	void setInFront(Device *value);
};

}
}
#endif // MHPROXYREADSTREAM_HPP
