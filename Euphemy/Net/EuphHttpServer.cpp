#include "EuphHttpServer.hpp"
#include <array>
#include <exception>
#include <utility>

namespace Euph {
namespace Net {

namespace {

/** @brief Maps a parser/transport error to a reasonable status code. */
int errorStatus(Elv::Net::HttpError error) {
	switch (error) {
	case Elv::Net::HttpError::HeaderTooLarge: return 431;
	case Elv::Net::HttpError::BodyTooLarge: return 413;
	case Elv::Net::HttpError::InvalidMethod: return 501;
	case Elv::Net::HttpError::UnsupportedTransfer: return 501;
	case Elv::Net::HttpError::Timeout: return 408;
	case Elv::Net::HttpError::SocketError: return 500;
	default: return 400;
	}
}

} // namespace

HttpServer::HttpServer(std::string host, std::string port, HttpRequestHandler& handler,
	HttpServerOptions options)
	: host_(std::move(host)), port_(std::move(port)), handler_(handler), options_(options) {}

HttpServer::~HttpServer() {
	stop();
}

bool HttpServer::start() {
	stop();
	try {
		const char* host = host_.empty() ? nullptr : host_.c_str();
		const auto endpoint = Io::Socket::resolveEndpoint(options_.family, options_.type,
			host, port_.c_str(), true);
		if (!endpoint.has_value()) {
			report(Elv::Net::HttpError::SocketError, "server endpoint resolution failed");
			return false;
		}
		Io::Socket listener(options_.family, options_.type, true);
		listener.setReuseAddr(true);
		HttpConnection::setTimeouts(listener, options_.requestTimeout);
		if (!listener.bindTo(*endpoint)) {
			report(Elv::Net::HttpError::SocketError, "server bind failed");
			return false;
		}
		if (options_.type == Io::SocketType::TCP
			&& listener.listenForConnections(options_.backlog) != 0) {
			report(Elv::Net::HttpError::SocketError, "server listen failed");
			return false;
		}
		socket_.emplace(std::move(listener));
		running_.store(true, std::memory_order_release);
		return true;
	} catch (const std::exception& exception) {
		report(Elv::Net::HttpError::SocketError, exception.what());
		return false;
	} catch (...) {
		report(Elv::Net::HttpError::SocketError, "server start failed");
		return false;
	}
}

void HttpServer::stop() {
	running_.store(false, std::memory_order_release);
	socket_.reset();
}

bool HttpServer::listening() const noexcept {
	return socket_.has_value();
}

std::optional<std::uint16_t> HttpServer::localPort() const {
	return socket_.has_value() ? socket_->localPort() : std::nullopt;
}

Elv::Net::HttpResponse HttpServer::makeErrorResponse(Elv::Net::HttpError error,
	std::string message) {
	Elv::Net::HttpResponse response;
	response.statusCode = errorStatus(error);
	response.reason = std::string(Elv::Net::httpReasonPhrase(response.statusCode));
	Elv::Net::setHeader(response.headers, "Content-Type", "text/plain; charset=utf-8");
	response.body = std::move(message);
	return response;
}

bool HttpServer::sendError(Io::Socket& socket, Elv::Net::HttpError error, std::string message) {
	const std::string wire = Elv::Net::serializeResponse(makeErrorResponse(error, std::move(message)));
	std::string ignored;
	const Elv::Net::HttpError writeError = HttpConnection::write(socket, wire, ignored);
	if (writeError != Elv::Net::HttpError::None) {
		report(writeError, ignored);
		return false;
	}
	return true;
}

void HttpServer::report(Elv::Net::HttpError error, std::string message) {
	onFailure.emit(Elv::Net::HttpFailure { error, std::move(message) });
}

bool HttpServer::serveTcp() {
	if (!socket_.has_value()) return false;
	const intptr_t raw = socket_->acceptConnection();
	if (raw == static_cast<intptr_t>(Io::InvalidSocketHandle)) {
		report(Elv::Net::HttpError::SocketError, "accept failed");
		return false;
	}
	Io::Socket connection(raw, Io::SocketState::CONNECTED);
	HttpConnection::setTimeouts(connection, options_.requestTimeout);
	Elv::Net::HttpRequest request;
	std::string message;
	const Elv::Net::HttpError readError = HttpConnection::readRequest(
		connection, options_.limits, request, message);
	if (readError != Elv::Net::HttpError::None) {
		report(readError, message);
		return sendError(connection, readError, std::move(message));
	}

	Elv::Net::HttpResponse response;
	bool handled = false;
	try {
		handled = handler_.handle(request, response);
	} catch (const std::exception& exception) {
		report(Elv::Net::HttpError::SocketError, exception.what());
		return sendError(connection, Elv::Net::HttpError::SocketError, exception.what());
	} catch (...) {
		report(Elv::Net::HttpError::SocketError, "handler failure");
		return sendError(connection, Elv::Net::HttpError::SocketError, "handler failure");
	}
	if (!handled) return sendError(connection, Elv::Net::HttpError::InvalidArgument,
		"handler rejected request");
	if (request.method == Elv::Net::HttpMethod::Head) {
		if (Elv::Net::findHeader(response.headers, "Content-Length") == nullptr) {
			Elv::Net::setHeader(response.headers, "Content-Length",
				std::to_string(response.body.size()));
		}
		response.body.clear();
	}
	const std::string wire = Elv::Net::serializeResponse(response);
	const Elv::Net::HttpError writeError = HttpConnection::write(connection, wire, message);
	if (writeError != Elv::Net::HttpError::None) {
		report(writeError, message);
		return false;
	}
	return true;
}

bool HttpServer::serveUdp() {
	if (!socket_.has_value()) return false;
	std::array<std::byte, 65535> buffer {};
	Elv::Net::Endpoint from;
	Elv::Net::NetError status = Elv::Net::NetError::None;
	const auto received = socket_->receiveDatagram(buffer, from, &status);
	if (!received.has_value()) {
		if (status == Elv::Net::NetError::WouldBlock) return true;
		report(Elv::Net::HttpError::Timeout, "UDP receive failed");
		return false;
	}
	Elv::Net::HttpRequest request;
	Elv::Net::HttpParseResult parsed;
	Elv::Net::HttpParser::parseRequest(
		std::string_view(reinterpret_cast<const char*>(buffer.data()), *received),
		request, parsed, options_.limits);
	Elv::Net::HttpResponse response;
	if (parsed.status != Elv::Net::HttpParseStatus::Ok) {
		const Elv::Net::HttpError error = parsed.status == Elv::Net::HttpParseStatus::Incomplete
			? Elv::Net::HttpError::BodyTooLarge : parsed.error;
		report(error, "UDP request parse failed");
		response = makeErrorResponse(error, std::string(Elv::Net::httpErrorName(error)));
	} else {
		bool handled = false;
		try {
			handled = handler_.handle(request, response);
		} catch (const std::exception& exception) {
			report(Elv::Net::HttpError::SocketError, exception.what());
			response = makeErrorResponse(Elv::Net::HttpError::SocketError, exception.what());
		} catch (...) {
			report(Elv::Net::HttpError::SocketError, "handler failure");
			response = makeErrorResponse(Elv::Net::HttpError::SocketError, "handler failure");
		}
		if (!handled) {
			response = makeErrorResponse(Elv::Net::HttpError::InvalidArgument,
				"handler rejected request");
		}
	}
	if (request.method == Elv::Net::HttpMethod::Head) {
		if (Elv::Net::findHeader(response.headers, "Content-Length") == nullptr) {
			Elv::Net::setHeader(response.headers, "Content-Length",
				std::to_string(response.body.size()));
		}
		response.body.clear();
	}
	const std::string wire = Elv::Net::serializeResponse(response);
	const auto payload = std::span<const std::byte>(
		reinterpret_cast<const std::byte*>(wire.data()), wire.size());
	const Elv::Net::NetError sendStatus = socket_->sendDatagram(from, payload);
	if (sendStatus != Elv::Net::NetError::None) {
		report(Elv::Net::HttpError::SocketError, "UDP response send failed");
		return false;
	}
	return true;
}

bool HttpServer::serveOne() {
	if (!running_.load(std::memory_order_acquire) || !socket_.has_value()) return false;
	return options_.type == Io::SocketType::UDP ? serveUdp() : serveTcp();
}

void HttpServer::run() {
	while (running_.load(std::memory_order_acquire)) {
		if (!serveOne()) break;
	}
}

} // namespace Net
} // namespace Euph

