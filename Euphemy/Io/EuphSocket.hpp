#ifndef SOCKET_HPP
#define SOCKET_HPP
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <cstdint>
#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#elif defined (__unix)
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#else
#error "Unsupported operating system type!"
#endif
namespace Euph {
namespace Io {

/**
 * @brief Enumeration defining the possible states of a socket.
 */
enum class SocketState : uint8_t {
	/**
	 * @brief The socket is disconnected.
	 *
	 * No active connection exists.
	 */
	DISCONNECTED,

	/**
	 * @brief The socket is connected to a remote endpoint.
	 *
	 * A successful connection has been established.
	 */
	CONNECTED,

	/**
	 * @brief The socket is bound to a local address and port.
	 *
	 * The socket is ready to listen for incoming connections.
	 */
	BOUND
};

/**
 * @brief Union for storing receipt-related data in various formats.
 *
 * This union allows for efficient storage of different types of receipt data.
 */
union ReceiptClass {
	/**
	 * @brief Time value representation (e.g., for timeouts).
	 */
	timeval tim;

	/**
	 * @brief Integer representation (e.g., for error codes).
	 */
	int integer;

	/**
	 * @brief 64-bit padding for alignment purposes.
	 */
	uint64_t pad64;
};

/**
 * @brief Forward declaration of the SocketSystemInitializer class.
 *
 * This class is responsible for initializing the socket system.
 */
struct SocketSystemInitializer;

/**
 * @brief Enumeration defining the address families supported by the socket.
 */
enum class AddressFamily : uint8_t {
	/**
	 * @brief Internet Protocol version 4 (IPv4).
	 *
	 * Address family for IPv4 connections.
	 */
	IPV4 = AF_INET,

	/**
	 * @brief Internet Protocol version 6 (IPv6).
	 *
	 * Address family for IPv6 connections.
	 */
	IPV6 = AF_INET6,

	/**
	 * @brief Unspecified address family.
	 *
	 * Can be used to automatically determine the address family.
	 */
	UNSPECIFIED = AF_UNSPEC
};

/**
 * @brief Enumeration defining the socket types supported.
 */
enum class SocketType : uint8_t {
	/**
	 * @brief Transmission Control Protocol (TCP).
	 *
	 * Connection-oriented, reliable socket type.
	 */
	TCP,

	/**
	 * @brief User Datagram Protocol (UDP).
	 *
	 * Connectionless, best-effort socket type.
	 */
	UDP
};

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(Socket)
/**
 * @brief Socket class, inheriting from Elv::Io::Device.
 *
 * Provides an interface for working with network sockets.
 */
class MH_EUPH_API Socket : public Elv::Io::Device {
private:
	/**
	 * @brief Shared pointer to the socket system initializer.
	 *
	 * Ensures the socket system is initialized only once.
	 */
	static std::shared_ptr<SocketSystemInitializer> init;

	/**
	 * @brief Pointer to address information.
	 *
	 * Stores the results of a successful address resolution.
	 */
	addrinfo* info;

	/**
	 * @brief Socket file descriptor.
	 *
	 * The underlying file descriptor for the socket.
	 */
	int sockfd_;

	/**
	 * @brief Current state of the socket.
	 *
	 * Reflects the socket's connection status.
	 */
	SocketState state;

	/**
	 * @brief Private copy constructor to prevent copying.
	 *
	 * Sockets cannot be copied; use move semantics instead.
	 */
	Socket(const Socket& cpy) = delete;

	/**
	 * @brief Private copy assignment operator to prevent copying.
	 *
	 * Sockets cannot be copied; use move semantics instead.
	 */
	Socket& operator=(const Socket& cpy) = delete;

	/**
	 * @brief Initializes the socket with the specified parameters.
	 *
	 * @param family	Address family (e.g., IPv4, IPv6).
	 * @param sockType  Socket type (e.g., TCP, UDP).
	 * @param host	  Hostname or IP address to connect to.
	 * @param port	  Port number to connect to.
	 */
	void initialize(AddressFamily family, SocketType sockType, const char* host, const char* port);

public:
	/**
	 * @brief Constructs a Socket object from a raw socket file descriptor.
	 *
	 * @param rawSock   The raw socket file descriptor.
	 */
	Socket(intptr_t rawSock);

	/**
	 * @brief Constructs a Socket object from a raw socket file descriptor and initial state.
	 *
	 * @param rawSock   The raw socket file descriptor.
	 * @param state	 The initial socket state.
	 */
	Socket(intptr_t rawSock, SocketState state);

	/**
	 * @brief Constructs a Socket object with the specified parameters.
	 *
	 * @param family	Address family (e.g., IPv4, IPv6).
	 * @param sockType  Socket type (e.g., TCP, UDP).
	 * @param host	  Hostname or IP address to connect to.
	 * @param port	  Port number to connect to.
	 */
	Socket(AddressFamily family, SocketType sockType, const char* host, const char* port);

	/**
	 * @brief Destructor for the Socket class.
	 */
	~Socket();

	/**
	 * @brief Move constructor for the Socket class.
	 *
	 * Transfers ownership of the socket.
	 *
	 * @param mov   The Socket object to move from.
	 */
	Socket(Socket&& mov);

	/**
	 * @brief Move assignment operator for the Socket class.
	 *
	 * Transfers ownership of the socket.
	 *
	 * @param mov   The Socket object to move from.
	 * @return	  Reference to the assigned Socket object.
	 */
	Socket& operator=(Socket&& mov);

	// Elv::Io::Device overrides

	/**
	 * @brief Reads data from the socket into a buffer.
	 *
	 * @param buffer   Pointer to the buffer to fill with data from the socket.
	 * @param size	 Size of each element to read.
	 * @param count	Number of elements to read.
	 * @return		 Number of elements successfully read.
	 */
	size_t read(void* buffer, size_t size, size_t count) override;

	/**
	 * @brief Writes data from a buffer to the socket.
	 *
	 * @param buffer   Pointer to the buffer containing data to write to the socket.
	 * @param size	 Size of each element to write.
	 * @param count	Number of elements to write.
	 * @return		 Number of elements successfully written.
	 */
	size_t write(const void* buffer, size_t size, size_t count) override;

	/**
	 * @brief Seeks to a position in the socket (not applicable for sockets).
	 *
	 * Sockets are sequential streams, so seeking is not supported.
	 *
	 * @param offset   Number of bytes to offset from the origin (ignored).
	 * @param whence   Origin from which to seek (ignored).
	 * @return		 Always returns a non-zero error code.
	 */
	int seek(long offset, Elv::Io::SeekOrigin whence) override;

	/**
	 * @brief Retrieves the current position in the socket (not applicable for sockets).
	 *
	 * Sockets are sequential streams, so the concept of a position is not applicable.
	 *
	 * @return		 Always returns an invalid value.
	 */
	long tell() override;

	/**
	 * @brief Retrieves the size of the socket (not applicable for sockets).
	 *
	 * Sockets are sequential streams, so the concept of a size is not applicable.
	 *
	 * @return		 Always returns an invalid value.
	 */
	size_t size() override;

	/**
	 * @brief Checks if the end of the socket has been reached (not applicable for sockets).
	 *
	 * Sockets are sequential streams, so the concept of an end is not applicable.
	 *
	 * @return		 Always returns false.
	 */
	bool eof() override;

	/**
	 * @brief Retrieves the current socket access mode.
	 *
	 * Returns the mode in which the socket was opened (e.g., read, write, or a combination).
	 *
	 * @return		 The current socket access mode.
	 */
	Elv::Io::Mode getMode() const override;

	/**
	 * @brief Flushes the socket buffer to ensure written data is committed.
	 *
	 * Forces any buffered write operations to be written to the underlying socket, ensuring data integrity.
	 *
	 * @return		 True if the flush operation was successful, false otherwise.
	 */
	bool flush() override;

	/**
	 * @brief Checks if the socket is in a valid state.
	 *
	 * Verifies the internal state of the socket, indicating whether it can be used for operations.
	 *
	 * @return		 True if the socket is valid, false otherwise.
	 */
	bool isValid() const override;

	// Socket-specific methods

	/**
	 * @brief Sets the socket's blocking behavior.
	 *
	 * @param isBlocking   True to enable blocking, false for non-blocking.
	 */
	void setBlocking(bool isBlocking);

	/**
	 * @brief Retrieves the current socket state.
	 *
	 * @return		 The current socket state (e.g., DISCONNECTED, CONNECTED, BOUND).
	 */
	SocketState getState() const;

	/**
	 * @brief Establishes a connection to the remote endpoint.
	 *
	 * @return		 True if the connection was successful, false otherwise.
	 */
	bool connectTo();

	/**
	 * @brief Binds the socket to a local address and port.
	 *
	 * Prepares the socket to listen for incoming connections.
	 *
	 * @return		 True if the binding was successful, false otherwise.
	 */
	bool bindTo();

	/**
	 * @brief Listens for incoming connections on the bound socket.
	 *
	 * Allows the socket to accept incoming connection requests.
	 *
	 * @param backlog   Maximum number of pending connections to queue.
	 * @return		 True if listening was successful, false otherwise.
	 */
	int listenForConnections(int backlog);

	/**
	 * @brief Accepts an incoming connection on the listening socket.
	 *
	 * Establishes a new connection with a remote endpoint.
	 *
	 * @return		 The file descriptor of the newly accepted socket, or an error value.
	 */
	intptr_t acceptConnection();

	// Socket option getters and setters

	/**
	 * @brief Retrieves the current debug mode setting.
	 *
	 * @return		 True if debug mode is enabled, false otherwise.
	 */
	bool getDebug() const;

	/**
	 * @brief Enables or disables debug mode for the socket.
	 *
	 * @param nval	 True to enable debug mode, false to disable.
	 */
	void setDebug(bool nval);

	/**
	 * @brief Retrieves the current broadcast setting.
	 *
	 * @return		 True if broadcast is enabled, false otherwise.
	 */
	bool getBroadcast() const;

	/**
	 * @brief Enables or disables broadcast mode for the socket.
	 *
	 * @param nval	 True to enable broadcast, false to disable.
	 */
	void setBroadcast(bool nval);

	/**
	 * @brief Retrieves the current reuse address setting.
	 *
	 * @return		 True if address reuse is enabled, false otherwise.
	 */
	bool getReuseAddr() const;

	/**
	 * @brief Enables or disables address reuse for the socket.
	 *
	 * @param nval	 True to enable address reuse, false to disable.
	 */
	void setReuseAddr(bool nval);

	/**
	 * @brief Retrieves the current keep-alive setting.
	 *
	 * @return		 True if keep-alive is enabled, false otherwise.
	 */
	bool getKeepAlive() const;

	/**
	 * @brief Enables or disables keep-alive for the socket.
	 *
	 * @param nval	 True to enable keep-alive, false to disable.
	 */
	void setKeepAlive(bool nval);

	/**
	 * @brief Retrieves the current out-of-band (OOB) inline setting.
	 *
	 * @return		 The current OOB inline value.
	 */
	int getOobinline() const;

	/**
	 * @brief Sets the out-of-band (OOB) inline behavior for the socket.
	 *
	 * @param nval	 The new OOB inline value.
	 */
	void setOobinline(int nval);

	/**
	 * @brief Retrieves the current send buffer size.
	 *
	 * @return		 The current send buffer size.
	 */
	int getSndBuffSize() const;

	/**
	 * @brief Sets the send buffer size for the socket.
	 *
	 * @param nval	 The new send buffer size.
	 */
	void setSndBuffSize(int nval);

	/**
	 * @brief Retrieves the current receive buffer size.
	 *
	 * @return		 The current receive buffer size.
	 */
	int getRcvBuffSize() const;

	/**
	 * @brief Sets the receive buffer size for the socket.
	 *
	 * @param nval	 The new receive buffer size.
	 */
	void setRcvBuffSize(int nval);

	/**
	 * @brief Retrieves the current don't route setting.
	 *
	 * @return		 True if don't route is enabled, false otherwise.
	 */
	bool getDontRoute() const;

	/**
	 * @brief Enables or disables don't route for the socket.
	 *
	 * @param nval	 True to enable don't route, false to disable.
	 */
	void setDontRoute(bool nval);

	/**
	 * @brief Retrieves the current receive low water mark.
	 *
	 * @return		 The current receive low water mark value.
	 */
	int getRclLoWat() const;

	/**
	 * @brief Sets the receive low water mark for the socket.
	 *
	 * @param nval	 The new receive low water mark value.
	 */
	void setRclLoWat(int nval);

	/**
	 * @brief Retrieves the current receive timeout.
	 *
	 * @return		 The current receive timeout value.
	 */
	timeval getRcvTimeout() const;

	/**
	 * @brief Sets the receive timeout for the socket.
	 *
	 * @param nval	 The new receive timeout value.
	 */
	void setRcvTimeout(timeval nval);

	/**
	 * @brief Retrieves the current send low water mark.
	 *
	 * @return		 The current send low water mark value.
	 */
	int getSndLoWat() const;

	/**
	 * @brief Sets the send low water mark for the socket.
	 *
	 * @param nval	 The new send low water mark value.
	 */
	void setSndLoWat(int nval);

	/**
	 * @brief Retrieves the current send timeout.
	 *
	 * @return		 The current send timeout value.
	 */
	timeval getSndTimeout() const;

	/**
	 * @brief Sets the send timeout for the socket.
	 *
	 * @param nval	 The new send timeout value.
	 */
	void setSndTimeout(timeval nval);
};

}
}
#endif // SOCKET_HPP
