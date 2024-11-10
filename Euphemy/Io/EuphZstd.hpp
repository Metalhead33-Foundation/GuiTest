#ifndef EUPHZSTD_H
#define EUPHZSTD_H
#include <Elvavena/Io/ElvIoDeviceProxy.hpp>
namespace Euph {
namespace Io {

class ZstdCompressor : public Elv::Io::BufferedWriteDeviceProxy
{
private:
	void* handle;
	bool finalized;
protected:
	size_t processInBufferToOutBuffer() override;
	size_t writePrefix(Device& device) override;
	size_t writePostfix(Device& device) override;
public:
	ZstdCompressor(Device* underlyingDevice);
	~ZstdCompressor();
	void setCompressionLevel(float value);
	void setChecksum(bool value);
	void onFinish() override;
};

class ZstdDecompressor : public Elv::Io::BufferedReadDeviceProxy
{
private:
	void* handle;
protected:
	size_t processInBufferToOutBuffer(size_t bytesRead) override;
public:
	ZstdDecompressor(Device* underlyingDevice);
	~ZstdDecompressor();
};

}
}
#endif // EUPHZSTD_H
