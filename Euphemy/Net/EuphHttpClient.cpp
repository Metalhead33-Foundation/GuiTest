#include "EuphHttpClient.hpp"
#include <array>
#include <exception>

namespace Euph {
namespace Net {

HttpClient::HttpClient(std::string host, std::string port, HttpClientOptions options)
	: host_(std::move(host)), port_(std::move(port)), options_(options) {}

HttpResult HttpClient::fail(Elv::Net::HttpError error, std::string message) {
	HttpResult result;
	result.error = error;
	result.message = message;
	onFailure.emit(Elv::Net::HttpFailure { error, result.message });
	return result;
}

HttpResult HttpClient::request(const Elv::Net::HttpRequest& request) {
	Elv::Net::HttpRequest outgoing = request;
	if (outgoing.host.empty()) {
		outgoing.host = host_;
		if (outgoing.host.find(':') != std::string::npos
			&& (outgoing.host.empty() || outgoing.host.front() != '[')) {
			outgoing.host = "[" + outgoing.host + "]";
		}
		if (port_ != "80" && port_ != "443") outgoing.host.append(":").append(port_);
	}
	const std::string wire = Elv::Net::serializeRequest(outgoing);
	if (wire.empty()) return fail(Elv::Net::HttpError::InvalidMethod, "unsupported HTTP method");

	try {
		Io::Socket socket(options_.family, options_.type, host_.c_str(), port_.c_str());
		socket.setBlocking(true);
		HttpConnection::setTimeouts(socket, options_.timeout);
		if (!socket.connectTo()) return fail(Elv::Net::HttpError::SocketError, "connect failed");
		std::string message;
		const Elv::Net::HttpError writeError = HttpConnection::write(socket, wire, message);
		if (writeError != Elv::Net::HttpError::None) return fail(writeError, std::move(message));

		Elv::Net::HttpResponse response;
		const bool expectBody = outgoing.method != Elv::Net::HttpMethod::Head;
		if (options_.type == Io::SocketType::UDP) {
			std::array<std::byte, 65535> buffer {};
			const std::size_t received = socket.read(buffer.data(), 1, buffer.size());
			if (received == 0) return fail(Elv::Net::HttpError::Timeout, "UDP response timeout");
			Elv::Net::HttpParseResult parsed;
			Elv::Net::HttpParser::parseResponse(
				std::string_view(reinterpret_cast<const char*>(buffer.data()), received),
				response, parsed, options_.limits, expectBody);
			if (parsed.status != Elv::Net::HttpParseStatus::Ok) {
				return fail(parsed.status == Elv::Net::HttpParseStatus::Incomplete
					? Elv::Net::HttpError::BodyTooLarge : parsed.error,
					"UDP response parse failed");
			}
		} else {
			const Elv::Net::HttpError readError = HttpConnection::readResponse(
				socket, options_.limits, expectBody, response, message);
			if (readError != Elv::Net::HttpError::None) return fail(readError, std::move(message));
		}
		HttpResult result;
		result.ok = true;
		result.response = std::move(response);
		return result;
	} catch (const std::exception& exception) {
		return fail(Elv::Net::HttpError::SocketError, exception.what());
	} catch (...) {
		return fail(Elv::Net::HttpError::SocketError, "socket failure");
	}
}

} // namespace Net
} // namespace Euph
