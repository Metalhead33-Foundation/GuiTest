#include <MhLib/Io/MhSocket.hpp>
#include <exception>
#include <string>
#include <sstream>
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
class WinsockStartupException : public std::exception {
private:
	int code;
	std::string message;
public:
	WinsockStartupException(int code) : code(code) {
		std::stringstream stream;
		stream << "WSAStartup failed.\n";
		switch (code) {
		case WSASYSNOTREADY: stream << "The underlying network subsystem is not ready for network communication.\n"; break;
		case WSAVERNOTSUPPORTED: stream << "The version of Windows Sockets support requested is not provided by this particular Windows Sockets implementation.\n"; break;
		case WSAEINPROGRESS: stream << "A blocking Windows Sockets 1.1 operation is in progress.\n"; break;
		case WSAEPROCLIM: stream << "A limit on the number of tasks supported by the Windows Sockets implementation has been reached.\n"; break;
		case WSAEFAULT: stream << "The lpWSAData parameter is not a valid pointer..\n"; break;
		default: stream << "Error code: " << code << "\n"; break;
		}
		this->message = stream.str();
	}
	const char* what() const noexcept {
		return this->message.c_str();
	}
};
class WinsockCleanupException : public std::exception {
private:
	int code;
	std::string message;
public:
	WinsockCleanupException(int code) : code(code) {
		std::stringstream stream;
		stream << "WSACleanup failed.\n";
		switch (code) {
		case WSANOTINITIALISED: stream << "A successful WSAStartup call must occur before using this function.\n"; break;
		case WSAENETDOWN: stream << "The network subsystem has failed.\n"; break;
		case WSAEINPROGRESS: stream << "A blocking Windows Sockets 1.1 operation is in progress.\n"; break;
		default: stream << "Error code: " << code << "\n"; break;
		}
		this->message = stream.str();
	}
	const char* what() const noexcept {
		return this->message.c_str();
	}
};
class WinsockAddressException : public std::exception {
private:
	int code;
	std::string message;
public:
	WinsockAddressException(int code) : code(code) {
		std::stringstream stream;
		stream << "WinSock Getaddrinfo failed.\n";
		switch (code) {
		case WSATRY_AGAIN: stream << "A temporary failure in name resolution occurred.\n"; break;
		case WSAEINVAL: stream << "An invalid value was provided for the ai_flags member of the pHints parameter.\n"; break;
		case WSANO_RECOVERY: stream << "A nonrecoverable failure in name resolution occurred.\n"; break;
		case WSAEAFNOSUPPORT: stream << "The ai_family member of the pHints parameter is not supported.\n"; break;
		case WSA_NOT_ENOUGH_MEMORY: stream << "A memory allocation failure occurred.\n"; break;
		case WSAHOST_NOT_FOUND: stream << "The name does not resolve for the supplied parameters or the pNodeName and pServiceName parameters were not provided.\n"; break;
		case WSATYPE_NOT_FOUND: stream << "The pServiceName parameter is not supported for the specified ai_socktype member of the pHints parameter.\n"; break;
		case WSAESOCKTNOSUPPORT: stream <<  "The ai_socktype member of the pHints parameter is not supported.\n"; break;
		default: stream << "Error code: " << code << "\n"; break;
		}
		this->message = stream.str();
	}
	const char* what() const noexcept {
		return this->message.c_str();
	}
};
class WinsockSocketException : public std::exception {
private:
	int code;
	std::string message;
public:
	WinsockSocketException(int code) : code(code) {
		std::stringstream stream;
		stream << "WinSock socket failed.\n";
		switch (code) {
		case WSANOTINITIALISED: stream << "A successful WSAStartup call must occur before using this function.\n"; break;
		case WSAENETDOWN: stream << "The network subsystem or the associated service provider has failed.\n"; break;
		case WSAEAFNOSUPPORT: stream << "The specified address family is not supported. For example, an application tried to create a socket for the AF_IRDA address family but an infrared adapter and device driver is not installed on the local computer.\n"; break;
		case WSAEINPROGRESS: stream << "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.\n"; break;
		case WSAEMFILE: stream << "No more socket descriptors are available.\n"; break;
		case WSAEINVAL: stream << " An invalid argument was supplied. This error is returned if the af parameter is set to AF_UNSPEC and the type and protocol parameter are unspecified.\n"; break;
		case WSAEINVALIDPROVIDER: stream << " The service provider returned a version other than 2.2.\n"; break;
		case WSAEINVALIDPROCTABLE: stream << " The service provider returned an invalid or incomplete procedure table to the WSPStartup.\n"; break;
		case WSAENOBUFS: stream << " No buffer space is available. The socket cannot be created.\n"; break;
		case WSAEPROTONOSUPPORT: stream << " The specified protocol is not supported.\n"; break;
		case WSAEPROTOTYPE: stream << " The specified protocol is the wrong type for this socket.\n"; break;
		case WSAEPROVIDERFAILEDINIT: stream << " The service provider failed to initialize. This error is returned if a layered service provider (LSP) or namespace provider was improperly installed or the provider fails to operate correctly.\n"; break;
		case WSAESOCKTNOSUPPORT: stream << " The specified socket type is not supported in this address family.\n"; break;
		default: stream << "Error code: " << code << "\n"; break;
		}
		this->message = stream.str();
	}
	const char* what() const noexcept {
		return this->message.c_str();
	}
};
#pragma comment(lib, "Ws2_32.lib")
#else
#define INVALID_SOCKET -1
#endif

namespace MH33 {
namespace Io {

struct SocketSystemInitializer {
	int iResult;
#ifdef _WIN32
	WSADATA wsaData;
#endif
	SocketSystemInitializer(int major, int minor) {
#ifdef _WIN32
		iResult = WSAStartup(MAKEWORD(major, minor), &wsaData);
		if (iResult != 0) {
			throw WinsockStartupException(WSAGetLastError());
		}
#else
		(void)major;
		(void)minor;
		iResult = 1;
#endif
	}
	~SocketSystemInitializer() {
#ifdef _WIN32
		iResult = WSACleanup();
		if (iResult != 0) {
			throw WinsockCleanupException(WSAGetLastError());
		}
#else
		iResult = 0;
#endif
	}
};
std::shared_ptr<SocketSystemInitializer> Socket::init = nullptr;

SocketState Socket::getState() const
{
	return state;
}

bool Socket::connectTo()
{
	if(connect(sock,info->ai_addr,sizeof(sockaddr)) == -1)
	{
		state = SocketState::DISCONNECTED;
		return false;
	}
	else {
		state = SocketState::CONNECTED;
		return true;
	}
}

bool Socket::bindTo()
{
	if(bind(sock,info->ai_addr,sizeof(sockaddr)) == -1)
	{
		state = SocketState::DISCONNECTED;
		return false;
	}
	else {
		state = SocketState::CONNECTED;
		return true;
	}
}

int Socket::listenForConnections(int backlog)
{
	return listen(sock,backlog);
}

intptr_t Socket::acceptConnection()
{
	return accept(sock,nullptr,nullptr);
}

void Socket::initialize(AddressFamily family, SocketType sockType, const char *host, const char *port)
{
	addrinfo* result = nullptr;
	addrinfo hints;
	memset(&hints, 0, sizeof(addrinfo));
	hints.ai_family = int(family);
	switch (sockType) {
	case SocketType::TCP: hints.ai_socktype = SOCK_STREAM; hints.ai_protocol = IPPROTO_TCP; break;
	case SocketType::UDP: hints.ai_socktype = SOCK_DGRAM; hints.ai_protocol = IPPROTO_UDP; break;
	}
	int iResult = getaddrinfo(host, port, &hints, &result);
	if (iResult != 0) {
#ifdef _WIN32
		throw WinsockAddressException(WSAGetLastError());
#else
		throw std::runtime_error("Invalid address!");
#endif
	}
	this->info = result;
	this->sock = socket(this->info->ai_family, this->info->ai_socktype, this->info->ai_protocol);
	if (this->sock == INVALID_SOCKET) {
#ifdef _WIN32
		throw WinsockSocketException(WSAGetLastError());
#else
		throw std::runtime_error("Invalid socket!");
#endif
	}
}

void Socket::initialize(AddressFamily family, SocketType sockType, const std::string &host, const std::string &port)
{
	initialize(family,sockType,host.c_str(),port.c_str());
}

Socket::Socket(intptr_t rawSock) : info(nullptr), sock(rawSock), state( (rawSock == INVALID_SOCKET) ? SocketState::DISCONNECTED : SocketState::CONNECTED)
{
	if (!init) init = std::shared_ptr<SocketSystemInitializer>(new SocketSystemInitializer(2, 2));
}

Socket::Socket(intptr_t rawSock, SocketState state) : info(nullptr), sock(rawSock), state(state)
{
	if (!init) init = std::shared_ptr<SocketSystemInitializer>(new SocketSystemInitializer(2, 2));
}

Socket::Socket(AddressFamily family, SocketType sockType, const char *host, const char *port) : info(nullptr), sock(INVALID_SOCKET), state(SocketState::DISCONNECTED)
{
	if (!init) init = std::shared_ptr<SocketSystemInitializer>(new SocketSystemInitializer(2, 2));
	initialize(family,sockType,host,port);
}

Socket::Socket(AddressFamily family, SocketType sockType, const std::string &host, const std::string &port) : info(nullptr), sock(INVALID_SOCKET), state(SocketState::DISCONNECTED)
{
	if (!init) init = std::shared_ptr<SocketSystemInitializer>(new SocketSystemInitializer(2, 2));
	initialize(family,sockType,host,port);
}

Socket::Socket(AddressFamily family, SocketType sockType, const char *host, uint16_t port) : info(nullptr), sock(INVALID_SOCKET), state(SocketState::DISCONNECTED)
{
	if (!init) init = std::shared_ptr<SocketSystemInitializer>(new SocketSystemInitializer(2, 2));
	std::stringstream stream;
	stream << port;
	std::string portstr = stream.str();
	initialize(family,sockType,host,portstr.c_str());

}

Socket::Socket(AddressFamily family, SocketType sockType, const std::string &host, uint16_t port) : info(nullptr), sock(INVALID_SOCKET), state(SocketState::DISCONNECTED)
{
	if (!init) init = std::shared_ptr<SocketSystemInitializer>(new SocketSystemInitializer(2, 2));
	std::stringstream stream;
	stream << port;
	initialize(family,sockType,host,stream.str());
}

Socket::~Socket()
{
#ifdef _WIN32
	if (sock != INVALID_SOCKET) closesocket(sock);
#else
	if (sock != INVALID_SOCKET) close(sock);
#endif
	if (info) freeaddrinfo(info);
}

Socket::Socket(Socket &&mov)
{
	this->info = mov.info;
	mov.info = nullptr;
	this->sock = mov.sock;
	mov.sock = INVALID_SOCKET;
	this->state = mov.state;
	mov.state = SocketState::DISCONNECTED;
}

Socket &Socket::operator=(Socket &&mov)
{
	// First we clean up.
#ifdef _WIN32
	if (sock != INVALID_SOCKET) closesocket(sock);
#else
	if (sock != INVALID_SOCKET) close(sock);
#endif
	if (info) freeaddrinfo(info);
	// Now move stuff
	this->info = mov.info;
	mov.info = nullptr;
	this->sock = mov.sock;
	mov.sock = INVALID_SOCKET;
	this->state = mov.state;
	mov.state = SocketState::DISCONNECTED;
	return *this;
}

bool Socket::flush()
{
	return false;
}

bool Socket::seek(SeekOrigin whence, intptr_t offset)
{
	(void)whence;
	(void)offset;
	return false;
}

intptr_t Socket::tell()
{
	return -1; // Sequential.
}

intptr_t Socket::size()
{
	return -1; // Sequential
}

size_t Socket::write(const void *data, size_t dataSize)
{
	return send(sock,data,dataSize,0);
}

size_t Socket::read(void *destination, size_t dataSize)
{
	return recv(sock,destination,dataSize,0);
}

Mode Socket::getMode() const
{
	return Mode::READ_WRITE;
}

bool Socket::isValid() const
{
	return sock != INVALID_SOCKET;
}

void Socket::setBlocking(bool isBlocking)
{
	u_long val = !isBlocking;
#ifdef _WIN32
	ioctlsocket(sock, FIONBIO, &val);
#else
	ioctl(sock, FIONBIO, &val);
#endif
}

bool Socket::getDebug() const
{
	ReceiptClass retval;
	socklen_t len=sizeof(int);
	if(getsockopt(sock,IPPROTO_TCP,SO_DEBUG,&retval,&len) == -1) return false;
	else return bool(retval.integer);
}
void Socket::setDebug(bool nval)
{
	int setto = nval;
	setsockopt(sock,SOL_SOCKET,SO_DEBUG,&setto,sizeof(int));
}
bool Socket::getBroadcast() const
{
	ReceiptClass retval;
	socklen_t len=sizeof(int);
	if(getsockopt(sock,SOL_SOCKET,SO_BROADCAST,&retval,&len) == -1) return false;
	else return bool(retval.integer);
}
void Socket::setBroadcast(bool nval)
{
	int setto = nval;
	setsockopt(sock,SOL_SOCKET,SO_BROADCAST,&setto,sizeof(int));
}
bool Socket::getReuseAddr() const
{
	ReceiptClass retval;
	socklen_t len=sizeof(int);
	if(getsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&retval,&len) == -1) return false;
	else return bool(retval.integer);
}
void Socket::setReuseAddr(bool nval)
{
	int setto = nval;
	setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&setto,sizeof(int));
}
bool Socket::getKeepAlive() const
{
	ReceiptClass retval;
	socklen_t len=sizeof(int);
	if(getsockopt(sock,SOL_SOCKET,SO_KEEPALIVE,&retval,&len) == -1) return false;
	else return bool(retval.integer);
}
void Socket::setKeepAlive(bool nval)
{
	int setto = nval;
	setsockopt(sock,SOL_SOCKET,SO_KEEPALIVE,&setto,sizeof(int));
}
int Socket::getOobinline() const
{
	ReceiptClass retval;
	socklen_t len=sizeof(int);
	if(getsockopt(sock,SOL_SOCKET,SO_OOBINLINE,&retval,&len) == -1) return -1;
	else return retval.integer;
}
void Socket::setOobinline(int nval)
{
	setsockopt(sock,SOL_SOCKET,SO_OOBINLINE,&nval,sizeof(int));
}
int Socket::getSndBuffSize() const
{
	ReceiptClass retval;
	socklen_t len=sizeof(int);
	if(getsockopt(sock,SOL_SOCKET,SO_SNDBUF,&retval,&len) == -1) return -1;
	else return retval.integer;
}
void Socket::setSndBuffSize(int nval)
{
	setsockopt(sock,SOL_SOCKET,SO_SNDBUF,&nval,sizeof(int));
}
int Socket::getRcvBuffSize() const
{
	ReceiptClass retval;
	socklen_t len=sizeof(int);
	if(getsockopt(sock,SOL_SOCKET,SO_RCVBUF,&retval,&len) == -1) return -1;
	else return retval.integer;
}
void Socket::setRcvBuffSize(int nval)
{
	setsockopt(sock,SOL_SOCKET,SO_RCVBUF,&nval,sizeof(int));
}
bool Socket::getDontRoute() const
{
	ReceiptClass retval;
	socklen_t len=sizeof(int);
	if(getsockopt(sock,SOL_SOCKET,SO_DONTROUTE,&retval,&len) == -1) return false;
	else return bool(retval.integer);
}
void Socket::setDontRoute(bool nval)
{
	int setto = nval;
	setsockopt(sock,SOL_SOCKET,SO_DONTROUTE,&setto,sizeof(int));
}
int Socket::getRclLoWat() const
{
	ReceiptClass retval;
	socklen_t len=sizeof(int);
	if(getsockopt(sock,SOL_SOCKET,SO_RCVLOWAT,&retval,&len) == -1) return -1;
	else return retval.integer;
}
void Socket::setRclLoWat(int nval)
{
	setsockopt(sock,SOL_SOCKET,SO_RCVLOWAT,&nval,sizeof(int));
}
timeval Socket::getRcvTimeout() const
{
	ReceiptClass retval;
	socklen_t len=sizeof(timeval);
	if(getsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,&retval,&len) == -1) return timeval();
	else return retval.tim;
}
void Socket::setRcvTimeout(timeval nval)
{
	setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,&nval,sizeof(timeval));
}
int Socket::getSndLoWat() const
{
	ReceiptClass retval;
	socklen_t len=sizeof(int);
	if(getsockopt(sock,SOL_SOCKET,SO_SNDLOWAT,&retval,&len) == -1) return -1;
	else return retval.integer;
}
void Socket::setSndLoWat(int nval)
{
	setsockopt(sock,SOL_SOCKET,SO_SNDLOWAT,&nval,sizeof(int));
}
timeval Socket::getSndTimeout() const
{
	ReceiptClass retval;
	socklen_t len=sizeof(timeval);
	if(getsockopt(sock,SOL_SOCKET,SO_SNDTIMEO,&retval,&len) == -1) return timeval();
	else return retval.tim;
}
void Socket::setSndTimeout(timeval nval)
{
	setsockopt(sock,SOL_SOCKET,SO_SNDTIMEO,&nval,sizeof(timeval));
}

}
}
