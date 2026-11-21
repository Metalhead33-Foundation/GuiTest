#include "EuphSocket.hpp"
#include <algorithm>
#include <cerrno>
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
#endif

namespace Euph {
namespace Io {

namespace {

/**
 * @brief Copies a native sockaddr into the opaque networking endpoint.
 * @param address Native address.
 * @param length Native address length.
 * @return Endpoint value; empty when @p address is null or @p length is invalid.
 */
Elv::Net::Endpoint endpointFromSockaddr(const sockaddr* address, SocketLength length) {
	Elv::Net::Endpoint endpoint;
	if (address == nullptr || length <= 0) return endpoint;
	const std::size_t size = std::min<std::size_t>(
		static_cast<std::size_t>(length), Elv::Net::Endpoint::StorageSize);
	std::memcpy(endpoint.storage.data(), address, size);
	endpoint.size = static_cast<std::uint16_t>(size);
	endpoint.family = static_cast<std::uint16_t>(address->sa_family);
	return endpoint;
}

/** @brief Returns the platform's last socket error code. */
int lastSocketError() {
#ifdef _WIN32
	return WSAGetLastError();
#else
	return errno;
#endif
}

/**
 * @brief Maps a native socket error into a @ref Elv::Net::NetError.
 * @param error Native error code.
 * @return Descriptive networking error.
 */
Elv::Net::NetError netErrorFromSocketError(int error) {
#ifdef _WIN32
	switch (error) {
	case WSAEWOULDBLOCK:
		return Elv::Net::NetError::WouldBlock;
	case WSAEMSGSIZE:
		return Elv::Net::NetError::MessageTooLarge;
	case WSAENOTCONN:
	case WSAENOTSOCK:
		return Elv::Net::NetError::NotConnected;
	default:
		return Elv::Net::NetError::ProtocolError;
	}
#else
	switch (error) {
	case EAGAIN:
#if EWOULDBLOCK != EAGAIN
	case EWOULDBLOCK:
#endif
	case EINTR:
		return Elv::Net::NetError::WouldBlock;
	case EMSGSIZE:
		return Elv::Net::NetError::MessageTooLarge;
	case ENOTCONN:
	case EBADF:
		return Elv::Net::NetError::NotConnected;
	default:
		return Elv::Net::NetError::ProtocolError;
	}
#endif
}

} // namespace

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

Socket::~Socket()
{
	if (info != nullptr) {
		freeaddrinfo(info);
		info = nullptr;
	}
	if (sockfd_ != InvalidSocketHandle)
	{
#ifdef _WIN32
		closesocket(sockfd_);
#else
		close(sockfd_);
#endif
	}
}

Socket::Socket(Socket&& mov)
	: info(mov.info), sockfd_(mov.sockfd_), state(mov.state)
{
	mov.info = nullptr;
	mov.sockfd_ = InvalidSocketHandle;
	mov.state = SocketState::DISCONNECTED;
}

Socket& Socket::operator=(Socket&& mov)
{
	if (this == &mov) return *this;
	if (info != nullptr) {
		freeaddrinfo(info);
		info = nullptr;
	}
	if (sockfd_ != InvalidSocketHandle)
	{
#ifdef _WIN32
		closesocket(sockfd_);
#else
		close(sockfd_);
#endif
	}
	this->info = mov.info;
	this->sockfd_ = mov.sockfd_;
	this->state = mov.state;
	mov.info = nullptr;
	mov.sockfd_ = InvalidSocketHandle;
	mov.state = SocketState::DISCONNECTED;
	return *this;
}

void Socket::initialize(AddressFamily family, SocketType sockType, const char* host, const char* port, bool passive)
{
	addrinfo* result = nullptr;
	addrinfo hints;
	memset(&hints, 0, sizeof(addrinfo));
	hints.ai_family = int(family);
	hints.ai_flags = passive ? AI_PASSIVE : 0;
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
	this->sockfd_ = static_cast<SocketHandle>(socket(this->info->ai_family, this->info->ai_socktype, this->info->ai_protocol));
	if (this->sockfd_ == InvalidSocketHandle) {
#ifdef _WIN32
		throw WinsockSocketException(WSAGetLastError());
#else
		throw std::runtime_error("Invalid socket!");
#endif
	}
}

Socket::Socket(intptr_t rawSock) : info(nullptr), sockfd_(static_cast<SocketHandle>(rawSock)), state( (sockfd_ == InvalidSocketHandle) ? SocketState::DISCONNECTED : SocketState::CONNECTED)
{
	if (!init) init = std::shared_ptr<SocketSystemInitializer>(new SocketSystemInitializer(2, 2));
}

Socket::Socket(intptr_t rawSock, SocketState state) : info(nullptr), sockfd_(static_cast<SocketHandle>(rawSock)), state(state)
{
	if (!init) init = std::shared_ptr<SocketSystemInitializer>(new SocketSystemInitializer(2, 2));
}

Socket::Socket(AddressFamily family, SocketType sockType, const char* host, const char* port) : info(nullptr), sockfd_(InvalidSocketHandle), state(SocketState::DISCONNECTED)
{
	if (!init) init = std::shared_ptr<SocketSystemInitializer>(new SocketSystemInitializer(2, 2));
	initialize(family,sockType,host,port);
}

Socket::Socket(AddressFamily family, SocketType sockType, bool bindable)
	: info(nullptr), sockfd_(InvalidSocketHandle), state(SocketState::DISCONNECTED)
{
	if (!init) init = std::shared_ptr<SocketSystemInitializer>(new SocketSystemInitializer(2, 2));
	initialize(family, sockType, nullptr, "0", bindable);
}

std::optional<Elv::Net::Endpoint> Socket::resolveEndpoint(AddressFamily family,
	SocketType sockType, const char* host, const char* port, bool passive)
{
	addrinfo hints;
	std::memset(&hints, 0, sizeof(addrinfo));
	hints.ai_family = static_cast<int>(family);
	hints.ai_flags = passive ? AI_PASSIVE : 0;
	switch (sockType) {
	case SocketType::TCP: hints.ai_socktype = SOCK_STREAM; hints.ai_protocol = IPPROTO_TCP; break;
	case SocketType::UDP: hints.ai_socktype = SOCK_DGRAM; hints.ai_protocol = IPPROTO_UDP; break;
	}
	addrinfo* result = nullptr;
	const char* node = (host != nullptr && *host != '\0') ? host : nullptr;
	if (getaddrinfo(node, port, &hints, &result) != 0) return std::nullopt;
	Elv::Net::Endpoint endpoint = endpointFromSockaddr(result->ai_addr, result->ai_addrlen);
	freeaddrinfo(result);
	if (!endpoint.isValid()) return std::nullopt;
	return endpoint;
}

bool Socket::bindTo(const Elv::Net::Endpoint& endpoint)
{
	if (!isValid() || !endpoint.isValid()) return false;
	if (::bind(sockfd_, reinterpret_cast<const sockaddr*>(endpoint.storage.data()),
		static_cast<SocketLength>(endpoint.size)) != 0) {
		state = SocketState::DISCONNECTED;
		return false;
	}
	state = SocketState::BOUND;
	return true;
}

bool Socket::connectTo(const Elv::Net::Endpoint& endpoint)
{
	if (!isValid() || !endpoint.isValid()) return false;
	if (::connect(sockfd_, reinterpret_cast<const sockaddr*>(endpoint.storage.data()),
		static_cast<SocketLength>(endpoint.size)) != 0) {
		state = SocketState::DISCONNECTED;
		return false;
	}
	state = SocketState::CONNECTED;
	return true;
}

std::optional<Elv::Net::Endpoint> Socket::localEndpoint() const
{
	if (!isValid()) return std::nullopt;
	sockaddr_storage storage {};
	SocketLength length = sizeof(storage);
	if (getsockname(sockfd_, reinterpret_cast<sockaddr*>(&storage), &length) != 0) {
		return std::nullopt;
	}
	return endpointFromSockaddr(reinterpret_cast<const sockaddr*>(&storage), length);
}

std::optional<size_t> Socket::receiveDatagram(std::span<std::byte> buffer,
	Elv::Net::Endpoint& from, Elv::Net::NetError* status)
{
	if (status != nullptr) *status = Elv::Net::NetError::None;
	if (!isValid() || buffer.empty()) {
		if (status != nullptr) *status = Elv::Net::NetError::ProtocolError;
		return std::nullopt;
	}
	sockaddr_storage storage {};
	SocketLength length = sizeof(storage);
	const auto received = recvfrom(sockfd_, reinterpret_cast<char*>(buffer.data()),
		static_cast<int>(buffer.size()), 0,
		reinterpret_cast<sockaddr*>(&storage), &length);
	if (received < 0) {
		if (status != nullptr) *status = netErrorFromSocketError(lastSocketError());
		return std::nullopt;
	}
	from = endpointFromSockaddr(reinterpret_cast<const sockaddr*>(&storage), length);
	if (status != nullptr) *status = Elv::Net::NetError::None;
	return static_cast<size_t>(received);
}

Elv::Net::NetError Socket::sendDatagram(const Elv::Net::Endpoint& to,
	std::span<const std::byte> payload)
{
	if (!isValid() || !to.isValid()) return Elv::Net::NetError::InvalidEndpoint;
	const auto sent = sendto(sockfd_, reinterpret_cast<const char*>(payload.data()),
		static_cast<int>(payload.size()), 0,
		reinterpret_cast<const sockaddr*>(to.storage.data()),
		static_cast<SocketLength>(to.size));
	if (sent < 0) return netErrorFromSocketError(lastSocketError());
	if (static_cast<size_t>(sent) != payload.size()) return Elv::Net::NetError::ProtocolError;
	return Elv::Net::NetError::None;
}

size_t Socket::read(void* buffer, size_t size, size_t count)
{
	if (buffer == nullptr || size == 0 || count == 0) return 0;
	const auto bytesRead = recv(sockfd_, static_cast<char*>(buffer), size * count, 0);
	if (bytesRead <= 0) return 0;
	return static_cast<size_t>(bytesRead) / size;
}

size_t Socket::write(const void* buffer, size_t size, size_t count)
{
	if (buffer == nullptr || size == 0 || count == 0) return 0;
	const auto bytesWritten = send(sockfd_, static_cast<const char*>(buffer), size * count, 0);
	if (bytesWritten <= 0) return 0;
	return static_cast<size_t>(bytesWritten) / size;
}

int Socket::seek(long offset, Elv::Io::SeekOrigin whence)
{
	(void)offset;
	(void)whence;
	return -1;
}

long Socket::tell()
{
	return -1;
}

size_t Socket::size()
{
	return 0;
}

bool Socket::eof()
{
	return false;
}

Elv::Io::Mode Socket::getMode() const
{
	return Elv::Io::Mode::READ_WRITE;
}

bool Socket::flush()
{
	return true;
}

bool Socket::isValid() const
{
	return sockfd_ != InvalidSocketHandle;
}

void Socket::setBlocking(bool isBlocking)
{
#ifdef _WIN32
	u_long val = isBlocking ? 0u : 1u;
	ioctlsocket(sockfd_, FIONBIO, &val);
#else
	int val = isBlocking ? 0 : 1;
	ioctl(sockfd_, FIONBIO, &val);
#endif
}

SocketState Socket::getState() const
{
	return state;
}

bool Socket::connectTo()
{
	if (info == nullptr) return false;
	if (connect(sockfd_, info->ai_addr, info->ai_addrlen) != 0)
	{
		state = SocketState::DISCONNECTED;
		return false;
	}
	state = SocketState::CONNECTED;
	return true;
}

bool Socket::bindTo()
{
	if (info == nullptr) return false;
	if (bind(sockfd_, info->ai_addr, info->ai_addrlen) != 0)
	{
		state = SocketState::DISCONNECTED;
		return false;
	}
	state = SocketState::BOUND;
	return true;
}

int Socket::listenForConnections(int backlog)
{
	return listen(sockfd_,backlog);
}

intptr_t Socket::acceptConnection()
{
	return static_cast<intptr_t>(accept(sockfd_, nullptr, nullptr));
}

bool Socket::getDebug() const
{
	ReceiptClass retval;
	SocketLength len=sizeof(int);
	if(getsockopt(sockfd_,IPPROTO_TCP,SO_DEBUG,&retval,&len) == -1) return false;
	else return bool(retval.integer);
}
void Socket::setDebug(bool nval)
{
	int setto = nval;
	setsockopt(sockfd_,SOL_SOCKET,SO_DEBUG,&setto,sizeof(int));
}
bool Socket::getBroadcast() const
{
	ReceiptClass retval;
	SocketLength len=sizeof(int);
	if(getsockopt(sockfd_,SOL_SOCKET,SO_BROADCAST,&retval,&len) == -1) return false;
	else return bool(retval.integer);
}
void Socket::setBroadcast(bool nval)
{
	int setto = nval;
	setsockopt(sockfd_,SOL_SOCKET,SO_BROADCAST,&setto,sizeof(int));
}
bool Socket::getReuseAddr() const
{
	ReceiptClass retval;
	SocketLength len=sizeof(int);
	if(getsockopt(sockfd_,SOL_SOCKET,SO_REUSEADDR,&retval,&len) == -1) return false;
	else return bool(retval.integer);
}
void Socket::setReuseAddr(bool nval)
{
	int setto = nval;
	setsockopt(sockfd_,SOL_SOCKET,SO_REUSEADDR,&setto,sizeof(int));
}
bool Socket::getKeepAlive() const
{
	ReceiptClass retval;
	SocketLength len=sizeof(int);
	if(getsockopt(sockfd_,SOL_SOCKET,SO_KEEPALIVE,&retval,&len) == -1) return false;
	else return bool(retval.integer);
}
void Socket::setKeepAlive(bool nval)
{
	int setto = nval;
	setsockopt(sockfd_,SOL_SOCKET,SO_KEEPALIVE,&setto,sizeof(int));
}
int Socket::getOobinline() const
{
	ReceiptClass retval;
	SocketLength len=sizeof(int);
	if(getsockopt(sockfd_,SOL_SOCKET,SO_OOBINLINE,&retval,&len) == -1) return -1;
	else return retval.integer;
}
void Socket::setOobinline(int nval)
{
	setsockopt(sockfd_,SOL_SOCKET,SO_OOBINLINE,&nval,sizeof(int));
}
int Socket::getSndBuffSize() const
{
	ReceiptClass retval;
	SocketLength len=sizeof(int);
	if(getsockopt(sockfd_,SOL_SOCKET,SO_SNDBUF,&retval,&len) == -1) return -1;
	else return retval.integer;
}
void Socket::setSndBuffSize(int nval)
{
	setsockopt(sockfd_,SOL_SOCKET,SO_SNDBUF,&nval,sizeof(int));
}
int Socket::getRcvBuffSize() const
{
	ReceiptClass retval;
	SocketLength len=sizeof(int);
	if(getsockopt(sockfd_,SOL_SOCKET,SO_RCVBUF,&retval,&len) == -1) return -1;
	else return retval.integer;
}
void Socket::setRcvBuffSize(int nval)
{
	setsockopt(sockfd_,SOL_SOCKET,SO_RCVBUF,&nval,sizeof(int));
}
bool Socket::getDontRoute() const
{
	ReceiptClass retval;
	SocketLength len=sizeof(int);
	if(getsockopt(sockfd_,SOL_SOCKET,SO_DONTROUTE,&retval,&len) == -1) return false;
	else return bool(retval.integer);
}
void Socket::setDontRoute(bool nval)
{
	int setto = nval;
	setsockopt(sockfd_,SOL_SOCKET,SO_DONTROUTE,&setto,sizeof(int));
}
int Socket::getRclLoWat() const
{
	ReceiptClass retval;
	SocketLength len=sizeof(int);
	if(getsockopt(sockfd_,SOL_SOCKET,SO_RCVLOWAT,&retval,&len) == -1) return -1;
	else return retval.integer;
}
void Socket::setRclLoWat(int nval)
{
	setsockopt(sockfd_,SOL_SOCKET,SO_RCVLOWAT,&nval,sizeof(int));
}
timeval Socket::getRcvTimeout() const
{
	ReceiptClass retval;
	SocketLength len=sizeof(timeval);
	if(getsockopt(sockfd_,SOL_SOCKET,SO_RCVTIMEO,&retval,&len) == -1) return timeval();
	else return retval.tim;
}
void Socket::setRcvTimeout(timeval nval)
{
	setsockopt(sockfd_,SOL_SOCKET,SO_RCVTIMEO,&nval,sizeof(timeval));
}
int Socket::getSndLoWat() const
{
	ReceiptClass retval;
	SocketLength len=sizeof(int);
	if(getsockopt(sockfd_,SOL_SOCKET,SO_SNDLOWAT,&retval,&len) == -1) return -1;
	else return retval.integer;
}
void Socket::setSndLoWat(int nval)
{
	setsockopt(sockfd_,SOL_SOCKET,SO_SNDLOWAT,&nval,sizeof(int));
}
timeval Socket::getSndTimeout() const
{
	ReceiptClass retval;
	SocketLength len=sizeof(timeval);
	if(getsockopt(sockfd_,SOL_SOCKET,SO_SNDTIMEO,&retval,&len) == -1) return timeval();
	else return retval.tim;
}
void Socket::setSndTimeout(timeval nval)
{
	setsockopt(sockfd_,SOL_SOCKET,SO_SNDTIMEO,&nval,sizeof(timeval));
}

}
}
