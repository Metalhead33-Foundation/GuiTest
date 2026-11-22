#ifndef EUPHHTTPSERVER_HPP
#define EUPHHTTPSERVER_HPP
/**
 * @file EuphHttpServer.hpp
 * @brief Minimal HTTP server scaffold over `Euph::Io::Socket`.
 *
 * The server owns only the boring part: bind/listen (or bind for UDP), read one
 * request, call a pure-virtual handler, serialize the handler's response, write
 * it and close. Routing, authentication, body interpretation, generated JSON and
 * keep-alive are deliberately left to subclasses/application code.
 */
#include <Euphemy/Config/EuphLib.hpp>
#include <Euphemy/Io/EuphSocket.hpp>
#include <Euphemy/Net/EuphHttpConnection.hpp>
#include <Elvavena/Net/ElvHttp.hpp>
#include <Elvavena/Util/ElvSignal.hpp>
#include <atomic>
#include <chrono>
#include <optional>
#include <string>

namespace Euph {
namespace Net {

/** @brief Server configuration. */
struct HttpServerOptions {
	Io::AddressFamily family = Io::AddressFamily::UNSPECIFIED; ///< Address family.
	Io::SocketType type = Io::SocketType::TCP;                 ///< TCP or UDP.
	int backlog = 16;                                          ///< TCP listen backlog.
	std::chrono::milliseconds requestTimeout { 5000 };          ///< I/O timeout.
	Elv::Net::HttpLimits limits {};                             ///< Parser limits.
};

/**
 * @class HttpRequestHandler
 * @brief Pure-virtual request policy supplied by the application.
 */
class MH_EUPH_API HttpRequestHandler {
public:
	virtual ~HttpRequestHandler() = default;

	/**
	 * @brief Handles a complete request.
	 * @param request Parsed request, including headers/body.
	 * @param response Response to serialize; the handler fills status/headers/body.
	 * @return False to let the server send a generic 500 response.
	 */
	virtual bool handle(const Elv::Net::HttpRequest& request,
		Elv::Net::HttpResponse& response) = 0;
};

/**
 * @class HttpServer
 * @brief Single-request-per-connection HTTP server scaffold.
 */
class MH_EUPH_API HttpServer {
public:
	/**
	 * @brief Creates a server.
	 * @param host Bind host, or empty for wildcard.
	 * @param port Bind port/service.
	 * @param handler Request policy; must outlive the server.
	 * @param options Transport/parser settings.
	 */
	HttpServer(std::string host, std::string port, HttpRequestHandler& handler,
		HttpServerOptions options = {});

	/** @brief Stops the server and releases the listening socket. */
	~HttpServer();

	HttpServer(const HttpServer&) = delete;
	HttpServer& operator=(const HttpServer&) = delete;

	/** @brief Binds/listens. @return True on success. */
	bool start();

	/** @brief Closes the listening socket. */
	void stop();

	/** @brief Returns whether the server owns a bound socket. */
	[[nodiscard]] bool listening() const noexcept;

	/** @brief Returns the bound port, useful when starting with port `0`. */
	[[nodiscard]] std::optional<std::uint16_t> localPort() const;

	/**
	 * @brief Serves one TCP connection or one UDP datagram.
	 * @return False when the transport failed or run should stop.
	 */
	bool serveOne();

	/** @brief Calls `serveOne()` until it fails or `stop()` is called. */
	void run();

	/** @brief Transport/parser failures; emitted on the calling thread. */
	Elv::Util::Signal<Elv::Net::HttpFailure> onFailure;

private:
	[[nodiscard]] bool serveTcp();
	[[nodiscard]] bool serveUdp();
	[[nodiscard]] bool sendError(Io::Socket& socket, Elv::Net::HttpError error,
		std::string message);
	[[nodiscard]] static Elv::Net::HttpResponse makeErrorResponse(Elv::Net::HttpError error,
		std::string message);
	void report(Elv::Net::HttpError error, std::string message);

	std::string host_;
	std::string port_;
	HttpRequestHandler& handler_;
	HttpServerOptions options_;
	std::optional<Io::Socket> socket_;
	std::atomic<bool> running_ { false };
};

} // namespace Net
} // namespace Euph
#endif // EUPHHTTPSERVER_HPP
