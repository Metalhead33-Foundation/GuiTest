#ifndef MHSslDecryptor_HPP
#define MHSslDecryptor_HPP
#include <MhLib/Io/MhProxyReadStream.hpp>

namespace MH33 {
namespace Io {
class MH_IO_API SslDecryptor : public ProxyReadStream
{
private:
	// No copy construction or assignment
	SslDecryptor(const SslDecryptor& cpy) = delete;
	SslDecryptor& operator=(const SslDecryptor& cpy) = delete;
protected:
	void fillBuffers(const void* inBuff, size_t& inBuffCursor, size_t inBuffSize, void* outBuff, size_t& outBuffEnd, size_t outBuffSize);
public:
	// Actual constructor and destructor
	SslDecryptor();
	SslDecryptor(Device* input);
	~SslDecryptor();
};
}
}

#endif // MHSslDecryptor_HPP
