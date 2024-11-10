#ifndef ELVIODEVICEPROXY_HPP
#define ELVIODEVICEPROXY_HPP
#include <Elvavena/Io/ElvIoDevice.hpp>
namespace Elv {
namespace Io {

class BufferedDeviceProxy : public Device
{
protected:
	Device* underlyingDevice;
	std::vector<std::byte> inBuffer, outBuffer;
	intptr_t bufferCursor;
public:
	BufferedDeviceProxy(Device* underlyingDevice, size_t inBufferSize, size_t outBufferSize);
	int seek(long offset, SeekOrigin whence) override;
	long tell() override;
	size_t size() override;
	bool eof() override;
	bool isValid() const override;
	Device* getUnderlyingDevice() const;
	void setUnderlyingDevice(Device* newUnderlyingDevice);
};

class BufferedWriteDeviceProxy : public BufferedDeviceProxy
{
protected:
	bool flushBufferToUnderlyingDevice();
	virtual size_t processInBufferToOutBuffer() = 0;
	virtual size_t writePrefix(Device& device) = 0;
	virtual size_t writePostfix(Device& device) = 0;
public:
	BufferedWriteDeviceProxy(Device* underlyingDevice, size_t inBufferSize, size_t outBufferSize);
	size_t read(void* buffer, size_t size, size_t count) override;
	size_t write(const void* buffer, size_t size, size_t count) override; // Does not guarantee a write to the underlying device - call flush() if you want to ensure that your data is written!
	Mode getMode() const override;
	bool flush() override;
	virtual void onFinish() = 0; // Make sure to call it from the destructor of all derived classes!
};

class BufferedReadDeviceProxy : public BufferedDeviceProxy
{
protected:
	intptr_t outBufferFence;
	bool fillBuffer();
	virtual size_t processInBufferToOutBuffer(size_t bytesRead) = 0;
	//! TODO: Functions, similar to the ones in the BufferedWriteDeviceProxy?
public:
	BufferedReadDeviceProxy(Device* underlyingDevice, size_t inBufferSize, size_t outBufferSize);
	size_t read(void* buffer, size_t size, size_t count) override;
	size_t write(const void* buffer, size_t size, size_t count) override;
	Mode getMode() const override;
	bool flush() override;
};

}
}
#endif // ELVIODEVICEPROXY_HPP
