#ifndef MHZSTDDECOMPRESSOR_HPP
#define MHZSTDDECOMPRESSOR_HPP
#include <MhLib/Io/MhProxyReadStream.hpp>

namespace MH33 {
namespace Io {
class MH_IO_API ZstdDecompressor : public ProxyReadStream
{
private:
	void* handle;
	// No copy construction or assignment
	ZstdDecompressor(const ZstdDecompressor& cpy) = delete;
	ZstdDecompressor& operator=(const ZstdDecompressor& cpy) = delete;
protected:
	void fillBuffers(const void* inBuff, size_t& inBuffCursor, size_t inBuffSize, void* outBuff, size_t& outBuffEnd, size_t outBuffSize);
public:
	// Actual constructor and destructor
	ZstdDecompressor();
	ZstdDecompressor(Device* input);
	~ZstdDecompressor();
};
}
}

#endif // MHZSTDDECOMPRESSOR_HPP
