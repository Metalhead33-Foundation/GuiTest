#ifndef MHSOCKET_HPP
#define MHSOCKET_HPP
#include <MhLib/Io/MhIoDevice.hpp>
#include <memory>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#elif defined (__unix)
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#else
#error "Unsupported operating system type!"
#endif

namespace MH33 {
namespace Io {

enum class SocketState {
	DISCONNECTED,
	CONNECTED,
	BOUND
};
union ReceiptClass {
	timeval tim;
	int integer;
	uint64_t pad64;
};

struct SocketSystemInitializer;
enum class AddressFamily {
	IPV4 = AF_INET,
	IPV6 = AF_INET6,
	UNSPECIFIED = AF_UNSPEC
};
enum class SocketType {
	TCP,
	UDP
};

class MH_IO_API Socket : public Device
{
private:
	static std::shared_ptr<SocketSystemInitializer> init;
	addrinfo* info;
	intptr_t sock;
	SocketState state;
	void initialize(AddressFamily family, SocketType sockType, const char* host, const char* port);
	void initialize(AddressFamily family, SocketType sockType, const std::string& host, const std::string& port);
	Socket(const Socket& cpy) = delete;
	Socket& operator=(const Socket& cpy) = delete;
public:
	// Regular constructors and destructors
	Socket(intptr_t rawSock);
	Socket(intptr_t rawSock, SocketState state);
	Socket(AddressFamily family, SocketType sockType, const char* host, const char* port);
	Socket(AddressFamily family, SocketType sockType, const std::string& host, const std::string& port);
	Socket(AddressFamily family, SocketType sockType, const char* host, uint16_t port);
	Socket(AddressFamily family, SocketType sockType, const std::string& host, uint16_t port);
	~Socket();
	// Move-constructor and move assignment
	Socket(Socket&& mov);
	Socket& operator=(Socket&& mov);
	// Socket functions
	bool flush();
	bool seek(MH33::Io::SeekOrigin whence, intptr_t offset);
	intptr_t tell();
	intptr_t size();
	size_t write(const void* data, size_t dataSize);
	size_t read(void* destination, size_t dataSize);
	MH33::Io::Mode getMode() const;
	bool isValid() const;
	void setBlocking(bool isBlocking);
	SocketState getState() const;
	bool connectTo();
	bool bindTo();
	int listenForConnections(int backlog);
	intptr_t acceptConnection();

	bool getDebug() const;
	void setDebug(bool nval);
	bool getBroadcast() const;
	void setBroadcast(bool nval);
	bool getReuseAddr() const;
	void setReuseAddr(bool nval);
	bool getKeepAlive() const;
	void setKeepAlive(bool nval);
	int getOobinline() const;
	void setOobinline(int nval);
	int getSndBuffSize() const;
	void setSndBuffSize(int nval);
	int getRcvBuffSize() const;
	void setRcvBuffSize(int nval);
	bool getDontRoute() const;
	void setDontRoute(bool nval);
	int getRclLoWat() const;
	void setRclLoWat(int nval);
	timeval getRcvTimeout() const;
	void setRcvTimeout(timeval nval);
	int getSndLoWat() const;
	void setSndLoWat(int nval);
	timeval getSndTimeout() const;
	void setSndTimeout(timeval nval);
};

}
}

#endif // MHSOCKET_HPP
