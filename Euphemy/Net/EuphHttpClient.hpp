#ifndef EUPHHTTPCLIENT_HPP
#define EUPHHTTPCLIENT_HPP
/**
 * @file EuphHttpClient.hpp
 * @brief Minimal blocking HTTP client over TCP or connected UDP.
 *
 * The client opens one socket per request, writes a serialized request, reads
 * one response and closes. TCP uses `Content-Length` framing; UDP treats one
 * datagram as one request/response. Failures are returned as `HttpResult` and
 * emitted through `onFailure`.
 */
#include <Euphemy/Config/EuphLib.hpp>
#include <Euphemy/Io/EuphSocket.hpp>
#include <Euphemy/Net/EuphHttpConnection.hpp>
#include <Elvavena/Net/ElvHttp.hpp>
#include <Elvavena/Util/ElvAsyncExecutor.hpp>
#include <Elvavena/Util/ElvSignal.hpp>
#include <chrono>
#include <string>
#include <string_view>
#include <utility>

namespace Euph {
namespace Net {

/** @brief Client configuration. */
struct HttpClientOptions {
	Io::AddressFamily family = Io::AddressFamily::UNSPECIFIED; ///< Address family.
	Io::SocketType type = Io::SocketType::TCP;                 ///< TCP or UDP.
	std::chrono::milliseconds timeout { 5000 };                 ///< I/O timeout.
	Elv::Net::HttpLimits limits {};                             ///< Parser limits.
};

/** @brief Result of one request. */
struct HttpResult {
	bool ok = false;                       ///< Transport/parse success.
	Elv::Net::HttpError error = Elv::Net::HttpError::None; ///< Failure category.
	std::string message;                   ///< Diagnostic.
	Elv::Net::HttpResponse response;       ///< Parsed response when `ok`.
};

/**
 * @class HttpClient
 * @brief One-request-per-connection HTTP client.
 */
class MH_EUPH_API HttpClient {
public:
	/**
	 * @brief Creates a client bound to a host/service.
	 * @param host Hostname or textual address.
	 * @param port Service/port.
	 * @param options Transport and parser settings.
	 */
	HttpClient(std::string host, std::string port, HttpClientOptions options = {});

	HttpClient(const HttpClient&) = delete;
	HttpClient& operator=(const HttpClient&) = delete;

	/**
	 * @brief Performs a blocking request.
	 * @param request Request; `host` is filled when empty.
	 * @return Parsed response or failure.
	 */
	[[nodiscard]] HttpResult request(const Elv::Net::HttpRequest& request);

	/**
	 * @brief Submits a request to an executor.
	 *
	 * The client must outlive the task. The failure signal is emitted on the
	 * executor thread; connect/marshal accordingly.
	 * @tparam Executor Async executor (EventLoop/ThreadPool/PollingEventLoop).
	 * @param executor Executor receiving the task.
	 * @param request Request value.
	 * @return Future result.
	 */
	template <Elv::Util::AsyncExecutor Executor>
	[[nodiscard]] auto requestAsync(Executor& executor, Elv::Net::HttpRequest request) {
		return executor.enqueueAsync([this, request = std::move(request)]() mutable {
			return this->request(request);
		});
	}

	/** @brief Failure notification. Emitted for transport/parse failures. */
	Elv::Util::Signal<Elv::Net::HttpFailure> onFailure;

	/** @brief Returns the configured host. */
	[[nodiscard]] const std::string& host() const noexcept { return host_; }
	/** @brief Returns the configured port. */
	[[nodiscard]] const std::string& port() const noexcept { return port_; }

private:
	[[nodiscard]] HttpResult fail(Elv::Net::HttpError error, std::string message);

	std::string host_;
	std::string port_;
	HttpClientOptions options_;
};

} // namespace Net
} // namespace Euph
#endif // EUPHHTTPCLIENT_HPP
