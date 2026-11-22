#include "EuphHttpConnection.hpp"
#include <algorithm>
#include <cstring>

namespace Euph {
namespace Net {

namespace {

/** @brief Converts milliseconds into a platform `timeval`. */
timeval toTimeval(std::chrono::milliseconds timeout) {
	timeval value {};
	if (timeout.count() > 0) {
		value.tv_sec = static_cast<decltype(value.tv_sec)>(timeout.count() / 1000);
		value.tv_usec = static_cast<decltype(value.tv_usec)>((timeout.count() % 1000) * 1000);
	}
	return value;
}

/** @brief Sets a diagnostic from an error code. */
void setMessage(std::string& message, Elv::Net::HttpError error) {
	message.assign(Elv::Net::httpErrorName(error));
}

} // namespace

void HttpConnection::setTimeouts(Io::Socket& socket, std::chrono::milliseconds timeout) {
	const timeval value = toTimeval(timeout);
	socket.setRcvTimeout(value);
	socket.setSndTimeout(value);
}

Elv::Net::HttpError HttpConnection::readRequest(Io::Socket& socket,
	const Elv::Net::HttpLimits& limits, Elv::Net::HttpRequest& request, std::string& message) {
	std::string buffer;
	char chunk[4096];
	for (;;) {
		Elv::Net::HttpParseResult result;
		Elv::Net::HttpParser::parseRequest(buffer, request, result, limits);
		if (result.status == Elv::Net::HttpParseStatus::Ok) return Elv::Net::HttpError::None;
		if (result.status == Elv::Net::HttpParseStatus::Error) {
			setMessage(message, result.error);
			return result.error;
		}
		const std::size_t received = socket.read(chunk, 1, sizeof(chunk));
		if (received == 0) {
			setMessage(message, Elv::Net::HttpError::Timeout);
			return Elv::Net::HttpError::Timeout;
		}
		buffer.append(chunk, received);
		if (buffer.size() > limits.maxHeaderBytes + limits.maxBodyBytes) {
			setMessage(message, Elv::Net::HttpError::BodyTooLarge);
			return Elv::Net::HttpError::BodyTooLarge;
		}
	}
}

Elv::Net::HttpError HttpConnection::readResponse(Io::Socket& socket,
	const Elv::Net::HttpLimits& limits, bool expectBody, Elv::Net::HttpResponse& response,
	std::string& message) {
	std::string buffer;
	char chunk[4096];
	for (;;) {
		Elv::Net::HttpParseResult result;
		Elv::Net::HttpParser::parseResponse(buffer, response, result, limits, expectBody);
		if (result.status == Elv::Net::HttpParseStatus::Ok) return Elv::Net::HttpError::None;
		if (result.status == Elv::Net::HttpParseStatus::Error) {
			setMessage(message, result.error);
			return result.error;
		}
		const std::size_t received = socket.read(chunk, 1, sizeof(chunk));
		if (received == 0) {
			setMessage(message, Elv::Net::HttpError::Timeout);
			return Elv::Net::HttpError::Timeout;
		}
		buffer.append(chunk, received);
		if (buffer.size() > limits.maxHeaderBytes + limits.maxBodyBytes) {
			setMessage(message, Elv::Net::HttpError::BodyTooLarge);
			return Elv::Net::HttpError::BodyTooLarge;
		}
	}
}

Elv::Net::HttpError HttpConnection::write(Io::Socket& socket, std::string_view data,
	std::string& message) {
	std::size_t offset = 0;
	while (offset < data.size()) {
		const std::size_t written = socket.write(data.data() + offset, 1, data.size() - offset);
		if (written == 0) {
			setMessage(message, Elv::Net::HttpError::SocketError);
			return Elv::Net::HttpError::SocketError;
		}
		offset += written;
	}
	return Elv::Net::HttpError::None;
}

} // namespace Net
} // namespace Euph
