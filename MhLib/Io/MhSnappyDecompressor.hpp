#ifndef MHSnappyDECOMPRESSOR_HPP
#define MHSnappyDECOMPRESSOR_HPP
#include <MhLib/Io/MhProxyReadStream.hpp>

namespace MH33 {
namespace Io {
class MH_IO_API SnappyDecompressor : public ProxyReadStream
{
private:
	// No copy construction or assignment
	SnappyDecompressor(const SnappyDecompressor& cpy) = delete;
	SnappyDecompressor& operator=(const SnappyDecompressor& cpy) = delete;
protected:
	void fillBuffers(const void* inBuff, size_t& inBuffCursor, size_t inBuffSize, void* outBuff, size_t& outBuffEnd, size_t outBuffSize);
public:
	// Actual constructor and destructor
	SnappyDecompressor();
	SnappyDecompressor(Device* input);
	~SnappyDecompressor();
};
}
}

#endif // MHSnappyDECOMPRESSOR_HPP
