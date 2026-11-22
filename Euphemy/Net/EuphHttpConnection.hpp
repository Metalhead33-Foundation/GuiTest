#ifndef EUPHHTTPCONNECTION_HPP
#define EUPHHTTPCONNECTION_HPP
/**
 * @file EuphHttpConnection.hpp
 * @brief Small blocking HTTP read/write helpers over `Euph::Io::Socket`.
 *
 * These helpers deliberately assume a blocking socket with SO_RCVTIMEO/
 * SO_SNDTIMEO configured. They stop at the first complete HTTP message; no
 * connection reuse, chunking, pipelining or TLS is attempted.
 */
#include <Euphemy/Config/EuphLib.hpp>
#include <Euphemy/Io/EuphSocket.hpp>
#include <Elvavena/Net/ElvHttp.hpp>
#include <chrono>
#include <string>
#include <string_view>

namespace Euph {
namespace Net {

/** @brief Shared blocking HTTP connection helpers. */
class MH_EUPH_API HttpConnection {
public:
	/**
	 * @brief Applies receive/send timeouts to a socket.
	 * @param socket Socket to configure.
	 * @param timeout Timeout in milliseconds.
	 */
	static void setTimeouts(Io::Socket& socket, std::chrono::milliseconds timeout);

	/**
	 * @brief Reads one request from a TCP socket.
	 * @param socket Blocking connected socket.
	 * @param limits Parser limits.
	 * @param request Receives the request.
	 * @param message Receives a diagnostic on failure.
	 * @return Error code.
	 */
	static Elv::Net::HttpError readRequest(Io::Socket& socket,
		const Elv::Net::HttpLimits& limits, Elv::Net::HttpRequest& request,
		std::string& message);

	/**
	 * @brief Reads one response from a TCP socket.
	 * @param socket Blocking connected socket.
	 * @param limits Parser limits.
	 * @param expectBody False for HEAD.
	 * @param response Receives the response.
	 * @param message Receives a diagnostic on failure.
	 * @return Error code.
	 */
	static Elv::Net::HttpError readResponse(Io::Socket& socket,
		const Elv::Net::HttpLimits& limits, bool expectBody, Elv::Net::HttpResponse& response,
		std::string& message);

	/**
	 * @brief Writes every byte in a message.
	 * @param socket Connected socket.
	 * @param data Serialized message.
	 * @param message Receives a diagnostic on failure.
	 * @return Error code.
	 */
	static Elv::Net::HttpError write(Io::Socket& socket, std::string_view data,
		std::string& message);
};

} // namespace Net
} // namespace Euph
#endif // EUPHHTTPCONNECTION_HPP
