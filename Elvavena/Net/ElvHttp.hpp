#ifndef ELVHTTP_HPP
#define ELVHTTP_HPP
/**
 * @file ElvHttp.hpp
 * @brief Minimal HTTP/1.1 request/response types, parser and serializer.
 *
 * This is intentionally a wire-format helper, not a web framework. It supports
 * the methods requested by the netcode/HTTP layer, case-insensitive header
 * lookup, `Content-Length` framing and fixed header/body limits. It does not
 * support chunked transfer encoding, compression, cookies, URL parsing, TLS or
 * automatic body serialization; callers add those above this layer.
 */
#include <Elvavena/Util/ElvUtilGlobals.hpp>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Elv {
namespace Net {

/** @brief Supported HTTP methods. */
enum class HttpMethod : std::uint8_t {
	Get,     ///< GET
	Post,    ///< POST
	Put,     ///< PUT
	Patch,   ///< PATCH
	Delete,  ///< DELETE
	Head,    ///< HEAD
	Options, ///< OPTIONS
	Unknown  ///< Unsupported method.
};

/** @brief Returns the canonical method name. */
[[nodiscard]] MH_UTIL_API std::string_view httpMethodName(HttpMethod method) noexcept;

/** @brief Parses a method token; unknown tokens become `HttpMethod::Unknown`. */
[[nodiscard]] MH_UTIL_API HttpMethod httpMethodFromString(std::string_view name) noexcept;

/** @brief Returns a default reason phrase for a status code. */
[[nodiscard]] MH_UTIL_API std::string_view httpReasonPhrase(int status) noexcept;

/** @brief One HTTP header. */
struct HttpHeader {
	std::string name;
	std::string value;
};

/** @brief Header list preserving wire order. */
using HttpHeaders = std::vector<HttpHeader>;

/** @brief Case-insensitive header lookup; returns null when absent. */
[[nodiscard]] MH_UTIL_API const HttpHeader* findHeader(const HttpHeaders& headers,
	std::string_view name) noexcept;

/** @brief Case-insensitive header value lookup; returns an empty view when absent. */
[[nodiscard]] MH_UTIL_API std::string_view headerValue(const HttpHeaders& headers,
	std::string_view name) noexcept;

/** @brief Adds or replaces a header by case-insensitive name. */
MH_UTIL_API void setHeader(HttpHeaders& headers, std::string_view name, std::string value);

/** @brief Removes a header by case-insensitive name. */
MH_UTIL_API void removeHeader(HttpHeaders& headers, std::string_view name);

/** @brief Parsed request. */
struct HttpRequest {
	HttpMethod method = HttpMethod::Get; ///< Request method.
	std::string target = "/";             ///< Origin-form target.
	std::string version = "HTTP/1.1";     ///< HTTP version.
	std::string host;                     ///< Optional Host header value.
	HttpHeaders headers;                  ///< Headers.
	std::string body;                     ///< Body bytes.
};

/** @brief Parsed response. */
struct HttpResponse {
	std::string version = "HTTP/1.1"; ///< HTTP version.
	int statusCode = 200;             ///< Status code.
	std::string reason;               ///< Reason phrase; serializer fills a default.
	HttpHeaders headers;              ///< Headers.
	std::string body;                 ///< Body bytes.
};

/** @brief Parser size limits. */
struct HttpLimits {
	std::size_t maxHeaderBytes = 64 * 1024; ///< Maximum header block size.
	std::size_t maxBodyBytes = 16 * 1024 * 1024; ///< Maximum Content-Length.
	std::size_t maxHeaderCount = 128; ///< Maximum header lines.
};

/** @brief Error categories. */
enum class HttpError : std::uint8_t {
	None,                    ///< Success.
	InvalidMethod,           ///< Unsupported method token.
	InvalidRequestLine,      ///< Malformed request line.
	InvalidResponseLine,     ///< Malformed status line.
	InvalidHeader,           ///< Malformed header line.
	UnsupportedVersion,      ///< Not HTTP/1.0 or HTTP/1.1.
	UnsupportedTransfer,     ///< Transfer-Encoding other than identity.
	HeaderTooLarge,          ///< Header block exceeds the limit.
	BodyTooLarge,            ///< Content-Length exceeds the limit.
	InvalidContentLength,    ///< Bad/duplicate/malformed Content-Length.
	ConnectionClosed,        ///< Peer closed before a complete message.
	Timeout,                 ///< Read/write timeout.
	SocketError,             ///< Transport failure.
	Cancelled,               ///< Caller cancelled the task.
	InvalidArgument          ///< Null/invalid input.
};

/** @brief Returns a stable error name. */
[[nodiscard]] MH_UTIL_API std::string_view httpErrorName(HttpError error) noexcept;

/** @brief Parser progress. */
enum class HttpParseStatus : std::uint8_t {
	Ok,         ///< A complete message is available.
	Incomplete, ///< More bytes are needed.
	Error       ///< The message is malformed/too large.
};

/** @brief Parse result with consumed byte count. */
struct HttpParseResult {
	HttpParseStatus status = HttpParseStatus::Incomplete;
	HttpError error = HttpError::None;
	std::size_t consumed = 0;
	std::size_t contentLength = 0;
};

/** @brief Failure payload emitted by client/server signals. */
struct HttpFailure {
	HttpError error = HttpError::None;
	std::string message;
};

/** @brief Serializes a request; adds Host/Content-Length when missing. */
[[nodiscard]] MH_UTIL_API std::string serializeRequest(const HttpRequest& request);

/** @brief Serializes a response; adds Content-Length/Connection when missing. */
[[nodiscard]] MH_UTIL_API std::string serializeResponse(const HttpResponse& response);

/** @brief Stateless HTTP/1.1 parser. */
class MH_UTIL_API HttpParser {
public:
	/**
	 * @brief Parses a request from a byte buffer.
	 * @param data Current receive buffer.
	 * @param request Receives the parsed request on `Ok`.
	 * @param result Parse status and consumed byte count.
	 * @param limits Size limits.
	 */
	static void parseRequest(std::string_view data, HttpRequest& request,
		HttpParseResult& result, const HttpLimits& limits = {});

	/**
	 * @brief Parses a response from a byte buffer.
	 * @param data Current receive buffer.
	 * @param response Receives the parsed response on `Ok`.
	 * @param result Parse status and consumed byte count.
	 * @param limits Size limits.
	 * @param expectBody False for HEAD responses; leaves the body empty.
	 */
	static void parseResponse(std::string_view data, HttpResponse& response,
		HttpParseResult& result, const HttpLimits& limits = {},
		bool expectBody = true);
};

} // namespace Net
} // namespace Elv
#endif // ELVHTTP_HPP

