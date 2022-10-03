#ifndef MHSnappyCOMPRESSOR_HPP
#define MHSnappyCOMPRESSOR_HPP
#include <MhLib/Io/MhProxyWriteStream.hpp>

namespace MH33 {
namespace Io {
class MH_IO_API SnappyCompressor : public ProxyWriteStream
{
private:
	// No copy construction or assignment
	SnappyCompressor(const SnappyCompressor& cpy) = delete;
	SnappyCompressor& operator=(const SnappyCompressor& cpy) = delete;
protected:
	void fillBuffers(const void* input, size_t inSize, void* outBuff, size_t& outBuffCursor, size_t outBuffMaxSize);
public:
	// Actual constructor and destructor
	SnappyCompressor();
	SnappyCompressor(Device* input);
	~SnappyCompressor();
};
}
}
#endif // MHSnappyCOMPRESSOR_HPP
