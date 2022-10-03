#ifndef MHSslEncryptor_HPP
#define MHSslEncryptor_HPP
#include <MhLib/Io/MhProxyWriteStream.hpp>

namespace MH33 {
namespace Io {
class MH_IO_API SslEncryptor : public ProxyWriteStream
{
private:
	// No copy construction or assignment
	SslEncryptor(const SslEncryptor& cpy) = delete;
	SslEncryptor& operator=(const SslEncryptor& cpy) = delete;
protected:
	void fillBuffers(const void* input, size_t inSize, void* outBuff, size_t& outBuffCursor, size_t outBuffMaxSize);
public:
	// Actual constructor and destructor
	SslEncryptor();
	SslEncryptor(Device* input);
	~SslEncryptor();
};
}
}
#endif // MHSslEncryptor_HPP
