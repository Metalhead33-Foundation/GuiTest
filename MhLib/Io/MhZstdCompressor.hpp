#ifndef MHZSTDCOMPRESSOR_HPP
#define MHZSTDCOMPRESSOR_HPP
#include <MhLib/Io/MhProxyWriteStream.hpp>

namespace MH33 {
namespace Io {
class MH_IO_API ZstdCompressor : public ProxyWriteStream
{
private:
	void* handle;
	// No copy construction or assignment
	ZstdCompressor(const ZstdCompressor& cpy) = delete;
	ZstdCompressor& operator=(const ZstdCompressor& cpy) = delete;
protected:
	void fillBuffers(const void* input, size_t inSize, void* outBuff, size_t& outBuffCursor, size_t outBuffMaxSize);
public:
	// Actual constructor and destructor
	ZstdCompressor();
	ZstdCompressor(Device* input);
	~ZstdCompressor();
	void setCompressionLevel(float value);
	void setChecksum(bool value);
};
}
}
#endif // MHZSTDCOMPRESSOR_HPP
