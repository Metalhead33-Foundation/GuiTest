#include "ElvHttp.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <limits>
#include <utility>

namespace Elv {
namespace Net {

namespace {

/** @brief Case-insensitive ASCII comparison. */
bool iequals(std::string_view lhs, std::string_view rhs) noexcept {
	if (lhs.size() != rhs.size()) return false;
	for (std::size_t index = 0; index < lhs.size(); ++index) {
		const auto left = static_cast<unsigned char>(lhs[index]);
		const auto right = static_cast<unsigned char>(rhs[index]);
		if (std::tolower(left) != std::tolower(right)) return false;
	}
	return true;
}

/** @brief Trims HTTP optional whitespace. */
std::string_view trim(std::string_view value) noexcept {
	while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
		value.remove_prefix(1);
	}
	while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
		value.remove_suffix(1);
	}
	return value;
}

/** @brief Parses a non-negative decimal Content-Length. */
bool parseLength(std::string_view value, std::size_t& length) noexcept {
	value = trim(value);
	if (value.empty()) return false;
	std::uint64_t parsed = 0;
	const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
	if (result.ec != std::errc {} || result.ptr != value.data() + value.size()) return false;
	if (parsed > std::numeric_limits<std::size_t>::max()) return false;
	length = static_cast<std::size_t>(parsed);
	return true;
}

/** @brief Parsed head fields shared by request and response parsing. */
struct HeadInfo {
	std::string_view firstLine;
	std::size_t contentLength = 0;
	bool hasContentLength = false;
};

/** @brief Splits and validates the header block. */
bool parseHeaderLines(std::string_view block, const HttpLimits& limits, HeadInfo& info,
	HttpError& error) {
	std::size_t position = 0;
	std::size_t headerCount = 0;
	bool first = true;
	while (position < block.size()) {
		const std::size_t lineEnd = block.find("\r\n", position);
		const std::string_view line = lineEnd == std::string_view::npos
			? block.substr(position) : block.substr(position, lineEnd - position);
		position = lineEnd == std::string_view::npos ? block.size() : lineEnd + 2;
		if (line.empty()) {
			error = HttpError::InvalidHeader;
			return false;
		}
		if (first) {
			info.firstLine = line;
			first = false;
			continue;
		}
		++headerCount;
		if (headerCount > limits.maxHeaderCount) {
			error = HttpError::HeaderTooLarge;
			return false;
		}
		const std::size_t colon = line.find(':');
		if (colon == std::string_view::npos || colon == 0) {
			error = HttpError::InvalidHeader;
			return false;
		}
		const std::string_view name = line.substr(0, colon);
		const std::string_view value = trim(line.substr(colon + 1));
		if (name.find_first_of(" \t") != std::string_view::npos) {
			error = HttpError::InvalidHeader;
			return false;
		}
		if (iequals(name, "Transfer-Encoding") && !value.empty() && !iequals(value, "identity")) {
			error = HttpError::UnsupportedTransfer;
			return false;
		}
		if (iequals(name, "Content-Length")) {
			std::size_t length = 0;
			if (!parseLength(value, length)) {
				error = HttpError::InvalidContentLength;
				return false;
			}
			if (info.hasContentLength && info.contentLength != length) {
				error = HttpError::InvalidContentLength;
				return false;
			}
			info.contentLength = length;
			info.hasContentLength = true;
		}
	}
	if (first) {
		error = HttpError::InvalidHeader;
		return false;
	}
	return true;
}

/** @brief Finds the header terminator and prepares a common parse result. */
bool findHead(std::string_view data, const HttpLimits& limits, std::size_t& headerEnd,
	HttpParseResult& result) {
	headerEnd = data.find("\r\n\r\n");
	if (headerEnd == std::string_view::npos) {
		if (data.size() > limits.maxHeaderBytes) {
			result.status = HttpParseStatus::Error;
			result.error = HttpError::HeaderTooLarge;
			return false;
		}
		result.status = HttpParseStatus::Incomplete;
		return false;
	}
	if (headerEnd > limits.maxHeaderBytes) {
		result.status = HttpParseStatus::Error;
		result.error = HttpError::HeaderTooLarge;
		return false;
	}
	return true;
}

/** @brief Parses an HTTP version token. */
bool validVersion(std::string_view version) noexcept {
	return version == "HTTP/1.0" || version == "HTTP/1.1";
}

} // namespace

std::string_view httpMethodName(HttpMethod method) noexcept {
	switch (method) {
	case HttpMethod::Get: return "GET";
	case HttpMethod::Post: return "POST";
	case HttpMethod::Put: return "PUT";
	case HttpMethod::Patch: return "PATCH";
	case HttpMethod::Delete: return "DELETE";
	case HttpMethod::Head: return "HEAD";
	case HttpMethod::Options: return "OPTIONS";
	default: return "";
	}
}

HttpMethod httpMethodFromString(std::string_view name) noexcept {
	if (iequals(name, "GET")) return HttpMethod::Get;
	if (iequals(name, "POST")) return HttpMethod::Post;
	if (iequals(name, "PUT")) return HttpMethod::Put;
	if (iequals(name, "PATCH")) return HttpMethod::Patch;
	if (iequals(name, "DELETE")) return HttpMethod::Delete;
	if (iequals(name, "HEAD")) return HttpMethod::Head;
	if (iequals(name, "OPTIONS")) return HttpMethod::Options;
	return HttpMethod::Unknown;
}

std::string_view httpReasonPhrase(int status) noexcept {
	switch (status) {
	case 200: return "OK";
	case 201: return "Created";
	case 202: return "Accepted";
	case 204: return "No Content";
	case 301: return "Moved Permanently";
	case 302: return "Found";
	case 304: return "Not Modified";
	case 400: return "Bad Request";
	case 401: return "Unauthorized";
	case 403: return "Forbidden";
	case 404: return "Not Found";
	case 405: return "Method Not Allowed";
	case 408: return "Request Timeout";
	case 409: return "Conflict";
	case 413: return "Payload Too Large";
	case 415: return "Unsupported Media Type";
	case 418: return "I'm a teapot";
	case 429: return "Too Many Requests";
	case 431: return "Request Header Fields Too Large";
	case 500: return "Internal Server Error";
	case 501: return "Not Implemented";
	case 503: return "Service Unavailable";
	default: return "Unknown";
	}
}

const HttpHeader* findHeader(const HttpHeaders& headers, std::string_view name) noexcept {
	for (const HttpHeader& header : headers) {
		if (iequals(header.name, name)) return &header;
	}
	return nullptr;
}

std::string_view headerValue(const HttpHeaders& headers, std::string_view name) noexcept {
	const HttpHeader* header = findHeader(headers, name);
	return header == nullptr ? std::string_view {} : std::string_view(header->value);
}

void setHeader(HttpHeaders& headers, std::string_view name, std::string value) {
	for (HttpHeader& header : headers) {
		if (iequals(header.name, name)) {
			header.name.assign(name);
			header.value = std::move(value);
			return;
		}
	}
	headers.push_back(HttpHeader { std::string(name), std::move(value) });
}

void removeHeader(HttpHeaders& headers, std::string_view name) {
	headers.erase(std::remove_if(headers.begin(), headers.end(),
		[name](const HttpHeader& header) { return iequals(header.name, name); }), headers.end());
}

std::string_view httpErrorName(HttpError error) noexcept {
	switch (error) {
	case HttpError::None: return "None";
	case HttpError::InvalidMethod: return "InvalidMethod";
	case HttpError::InvalidRequestLine: return "InvalidRequestLine";
	case HttpError::InvalidResponseLine: return "InvalidResponseLine";
	case HttpError::InvalidHeader: return "InvalidHeader";
	case HttpError::UnsupportedVersion: return "UnsupportedVersion";
	case HttpError::UnsupportedTransfer: return "UnsupportedTransfer";
	case HttpError::HeaderTooLarge: return "HeaderTooLarge";
	case HttpError::BodyTooLarge: return "BodyTooLarge";
	case HttpError::InvalidContentLength: return "InvalidContentLength";
	case HttpError::ConnectionClosed: return "ConnectionClosed";
	case HttpError::Timeout: return "Timeout";
	case HttpError::SocketError: return "SocketError";
	case HttpError::Cancelled: return "Cancelled";
	case HttpError::InvalidArgument: return "InvalidArgument";
	default: return "Unknown";
	}
}


std::string serializeRequest(const HttpRequest& request) {
	const std::string_view method = httpMethodName(request.method);
	if (method.empty()) return {};
	std::string output;
	output.reserve(128 + request.body.size());
	output.append(method).append(" ").append(request.target).append(" ")
		.append(validVersion(request.version) ? request.version : "HTTP/1.1").append("\r\n");
	const bool hasHost = findHeader(request.headers, "Host") != nullptr;
	if (!hasHost && !request.host.empty()) {
		output.append("Host: ").append(request.host).append("\r\n");
	}
	const bool hasLength = findHeader(request.headers, "Content-Length") != nullptr;
	const bool bodyMethods = request.method == HttpMethod::Post || request.method == HttpMethod::Put
		|| request.method == HttpMethod::Patch;
	if (!hasLength && (!request.body.empty() || bodyMethods)) {
		output.append("Content-Length: ").append(std::to_string(request.body.size())).append("\r\n");
	}
	for (const HttpHeader& header : request.headers) {
		output.append(header.name).append(": ").append(header.value).append("\r\n");
	}
	output.append("\r\n").append(request.body);
	return output;
}

std::string serializeResponse(const HttpResponse& response) {
	std::string output;
	output.reserve(128 + response.body.size());
	const int status = response.statusCode >= 100 && response.statusCode <= 999
		? response.statusCode : 500;
	const std::string_view phrase = response.reason.empty()
		? httpReasonPhrase(status) : std::string_view(response.reason);
	output.append(validVersion(response.version) ? response.version : "HTTP/1.1")
		.append(" ").append(std::to_string(status)).append(" ").append(phrase).append("\r\n");
	if (findHeader(response.headers, "Content-Length") == nullptr) {
		output.append("Content-Length: ").append(std::to_string(response.body.size())).append("\r\n");
	}
	if (findHeader(response.headers, "Connection") == nullptr) {
		output.append("Connection: close\r\n");
	}
	for (const HttpHeader& header : response.headers) {
		output.append(header.name).append(": ").append(header.value).append("\r\n");
	}
	output.append("\r\n").append(response.body);
	return output;
}


void HttpParser::parseRequest(std::string_view data, HttpRequest& request,
	HttpParseResult& result, const HttpLimits& limits) {
	request = HttpRequest {};
	result = HttpParseResult {};
	std::size_t headerEnd = 0;
	if (!findHead(data, limits, headerEnd, result)) return;
	const std::string_view block = data.substr(0, headerEnd);
	HeadInfo info;
	HttpError error = HttpError::None;
	if (!parseHeaderLines(block, limits, info, error)) {
		result.status = HttpParseStatus::Error;
		result.error = error;
		return;
	}
	const std::size_t firstSpace = info.firstLine.find(' ');
	const std::size_t secondSpace = firstSpace == std::string_view::npos
		? std::string_view::npos : info.firstLine.find(' ', firstSpace + 1);
	if (firstSpace == std::string_view::npos || secondSpace == std::string_view::npos) {
		result.status = HttpParseStatus::Error;
		result.error = HttpError::InvalidRequestLine;
		return;
	}
	request.method = httpMethodFromString(info.firstLine.substr(0, firstSpace));
	if (request.method == HttpMethod::Unknown) {
		result.status = HttpParseStatus::Error;
		result.error = HttpError::InvalidMethod;
		return;
	}
	request.target = std::string(info.firstLine.substr(firstSpace + 1,
		secondSpace - firstSpace - 1));
	request.version = std::string(info.firstLine.substr(secondSpace + 1));
	if (request.target.empty()) {
		result.status = HttpParseStatus::Error;
		result.error = HttpError::InvalidRequestLine;
		return;
	}
	if (!validVersion(request.version)) {
		result.status = HttpParseStatus::Error;
		result.error = HttpError::UnsupportedVersion;
		return;
	}
	std::size_t position = block.find("\r\n");
	if (position != std::string_view::npos) position += 2;
	while (position < block.size()) {
		const std::size_t lineEnd = block.find("\r\n", position);
		const std::string_view line = lineEnd == std::string_view::npos
			? block.substr(position) : block.substr(position, lineEnd - position);
		position = lineEnd == std::string_view::npos ? block.size() : lineEnd + 2;
		const std::size_t colon = line.find(':');
		request.headers.push_back(HttpHeader {
			std::string(line.substr(0, colon)),
			std::string(trim(line.substr(colon + 1)))
		});
	}
	if (const HttpHeader* host = findHeader(request.headers, "Host")) request.host = host->value;
	if (info.contentLength > limits.maxBodyBytes) {
		result.status = HttpParseStatus::Error;
		result.error = HttpError::BodyTooLarge;
		return;
	}
	const std::size_t bodyStart = headerEnd + 4;
	const std::size_t total = bodyStart + info.contentLength;
	result.contentLength = info.contentLength;
	if (data.size() < total) {
		result.status = HttpParseStatus::Incomplete;
		return;
	}
	request.body.assign(data.data() + bodyStart, info.contentLength);
	result.status = HttpParseStatus::Ok;
	result.consumed = total;
}


void HttpParser::parseResponse(std::string_view data, HttpResponse& response,
	HttpParseResult& result, const HttpLimits& limits, bool expectBody) {
	response = HttpResponse {};
	result = HttpParseResult {};
	std::size_t headerEnd = 0;
	if (!findHead(data, limits, headerEnd, result)) return;
	const std::string_view block = data.substr(0, headerEnd);
	HeadInfo info;
	HttpError error = HttpError::None;
	if (!parseHeaderLines(block, limits, info, error)) {
		result.status = HttpParseStatus::Error;
		result.error = error;
		return;
	}
	const std::size_t firstSpace = info.firstLine.find(' ');
	const std::size_t secondSpace = firstSpace == std::string_view::npos
		? std::string_view::npos : info.firstLine.find(' ', firstSpace + 1);
	if (firstSpace == std::string_view::npos) {
		result.status = HttpParseStatus::Error;
		result.error = HttpError::InvalidResponseLine;
		return;
	}
	response.version = std::string(info.firstLine.substr(0, firstSpace));
	if (!validVersion(response.version)) {
		result.status = HttpParseStatus::Error;
		result.error = HttpError::UnsupportedVersion;
		return;
	}
	const std::string_view statusStart = secondSpace == std::string_view::npos
		? info.firstLine.substr(firstSpace + 1)
		: info.firstLine.substr(firstSpace + 1, secondSpace - firstSpace - 1);
	unsigned status = 0;
	const auto statusResult = std::from_chars(statusStart.data(),
		statusStart.data() + statusStart.size(), status);
	if (statusResult.ec != std::errc {} || statusResult.ptr != statusStart.data() + statusStart.size()
		|| status < 100 || status > 999) {
		result.status = HttpParseStatus::Error;
		result.error = HttpError::InvalidResponseLine;
		return;
	}
	response.statusCode = static_cast<int>(status);
	response.reason = secondSpace == std::string_view::npos
		? std::string(httpReasonPhrase(response.statusCode))
		: std::string(trim(info.firstLine.substr(secondSpace + 1)));
	std::size_t position = block.find("\r\n");
	if (position != std::string_view::npos) position += 2;
	while (position < block.size()) {
		const std::size_t lineEnd = block.find("\r\n", position);
		const std::string_view line = lineEnd == std::string_view::npos
			? block.substr(position) : block.substr(position, lineEnd - position);
		position = lineEnd == std::string_view::npos ? block.size() : lineEnd + 2;
		const std::size_t colon = line.find(':');
		response.headers.push_back(HttpHeader {
			std::string(line.substr(0, colon)),
			std::string(trim(line.substr(colon + 1)))
		});
	}
	if (info.contentLength > limits.maxBodyBytes) {
		result.status = HttpParseStatus::Error;
		result.error = HttpError::BodyTooLarge;
		return;
	}
	const std::size_t bodyStart = headerEnd + 4;
	const std::size_t bodyLength = expectBody ? info.contentLength : 0;
	const std::size_t total = bodyStart + bodyLength;
	result.contentLength = bodyLength;
	if (data.size() < total) {
		result.status = HttpParseStatus::Incomplete;
		return;
	}
	if (bodyLength != 0) response.body.assign(data.data() + bodyStart, bodyLength);
	result.status = HttpParseStatus::Ok;
	result.consumed = total;
}

} // namespace Net
} // namespace Elv

