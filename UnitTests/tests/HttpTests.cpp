/**
 * @file HttpTests.cpp
 * @brief Minimal HTTP parser, client and server tests.
 */
#include <catch2/catch_test_macros.hpp>
#include <Elvavena/Net/ElvHttp.hpp>
#include <Elvavena/Util/ElvEventLoop.hpp>
#include <Euphemy/Net/EuphHttpClient.hpp>
#include <Euphemy/Net/EuphHttpServer.hpp>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>

namespace {

using namespace Elv;
using namespace Elv::Net;

class EchoHandler final : public Euph::Net::HttpRequestHandler {
public:
	bool handle(const HttpRequest& request, HttpResponse& response) override {
		response.statusCode = 200;
		response.reason = "OK";
		setHeader(response.headers, "Content-Type", "text/plain");
		response.body = request.method == HttpMethod::Head ? "head-body" : request.body;
		if (response.body.empty()) response.body = "ok";
		return true;
	}
};

std::string portString(std::uint16_t port) {
	return std::to_string(port);
}

} // namespace

TEST_CASE("HTTP request and response parser round-trips", "[http]")
{
	HttpRequest request;
	request.method = HttpMethod::Post;
	request.target = "/items?limit=2";
	request.host = "example.test:8080";
	request.body = "hello";
	setHeader(request.headers, "Content-Type", "text/plain");
	const std::string wire = serializeRequest(request);
	CHECK(wire.find("POST /items?limit=2 HTTP/1.1\r\n") == 0);
	CHECK(wire.find("Host: example.test:8080\r\n") != std::string::npos);
	CHECK(wire.find("Content-Length: 5\r\n") != std::string::npos);

	HttpRequest parsed;
	HttpParseResult result;
	HttpParser::parseRequest(wire, parsed, result);
	REQUIRE(result.status == HttpParseStatus::Ok);
	CHECK(result.consumed == wire.size());
	CHECK(parsed.method == HttpMethod::Post);
	CHECK(parsed.target == "/items?limit=2");
	CHECK(parsed.host == "example.test:8080");
	CHECK(parsed.body == "hello");
	CHECK(headerValue(parsed.headers, "content-type") == "text/plain");

	HttpResponse response;
	response.statusCode = 201;
	response.reason = "Created";
	response.body = "made";
	const std::string responseWire = serializeResponse(response);
	HttpResponse parsedResponse;
	HttpParser::parseResponse(responseWire, parsedResponse, result);
	REQUIRE(result.status == HttpParseStatus::Ok);
	CHECK(parsedResponse.statusCode == 201);
	CHECK(parsedResponse.reason == "Created");
	CHECK(parsedResponse.body == "made");
}

TEST_CASE("HTTP parser reports incomplete, oversized and malformed messages", "[http]")
{
	const std::string partial = "POST / HTTP/1.1\r\nContent-Length: 5\r\n\r\nhe";
	HttpRequest request;
	HttpParseResult result;
	HttpParser::parseRequest(partial, request, result);
	CHECK(result.status == HttpParseStatus::Incomplete);
	CHECK(result.contentLength == 5);

	HttpParser::parseRequest("GET / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n",
		request, result);
	CHECK(result.status == HttpParseStatus::Error);
	CHECK(result.error == HttpError::UnsupportedTransfer);

	HttpParser::parseRequest("BOGUS / HTTP/1.1\r\n\r\n", request, result);
	CHECK(result.status == HttpParseStatus::Error);
	CHECK(result.error == HttpError::InvalidMethod);

	HttpLimits limits;
	limits.maxHeaderBytes = 8;
	HttpParser::parseRequest("GET / HTTP/1.1\r\nHost: example\r\n\r\n", request, result, limits);
	CHECK(result.status == HttpParseStatus::Error);
	CHECK(result.error == HttpError::HeaderTooLarge);
}

TEST_CASE("HTTP parser omits HEAD response bodies", "[http]")
{
	const std::string wire = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello";
	HttpResponse response;
	HttpParseResult result;
	HttpParser::parseResponse(wire, response, result, {}, false);
	REQUIRE(result.status == HttpParseStatus::Ok);
	CHECK(response.body.empty());
	CHECK(result.consumed == wire.size() - 5);
	CHECK(headerValue(response.headers, "Content-Length") == "5");
}

TEST_CASE("HTTP client and TCP server exchange a POST body", "[http][socket]")
{
	EchoHandler handler;
	Euph::Net::HttpServer server("127.0.0.1", "0", handler);
	REQUIRE(server.start());
	const auto port = server.localPort();
	REQUIRE(port.has_value());
	std::atomic<bool> served { false };
	std::thread serverThread([&] { served.store(server.serveOne()); });
	Euph::Net::HttpClient client("127.0.0.1", portString(*port));
	HttpRequest request;
	request.method = HttpMethod::Post;
	request.target = "/echo";
	request.body = "hello over tcp";
	setHeader(request.headers, "Content-Type", "text/plain");
	const Euph::Net::HttpResult result = client.request(request);
	serverThread.join();
	CHECK(served.load());
	REQUIRE(result.ok);
	CHECK(result.response.statusCode == 200);
	CHECK(result.response.body == "hello over tcp");
	CHECK(headerValue(result.response.headers, "Content-Type") == "text/plain");
	server.stop();
}

TEST_CASE("HTTP HEAD response has headers but no body", "[http][socket]")
{
	EchoHandler handler;
	Euph::Net::HttpServer server("127.0.0.1", "0", handler);
	REQUIRE(server.start());
	const auto port = server.localPort();
	REQUIRE(port.has_value());
	std::thread serverThread([&] { (void)server.serveOne(); });
	Euph::Net::HttpClient client("127.0.0.1", portString(*port));
	HttpRequest request;
	request.method = HttpMethod::Head;
	request.target = "/head";
	const Euph::Net::HttpResult result = client.request(request);
	serverThread.join();
	REQUIRE(result.ok);
	CHECK(result.response.statusCode == 200);
	CHECK(result.response.body.empty());
	CHECK(headerValue(result.response.headers, "Content-Length") == "9");
	server.stop();
}

TEST_CASE("HTTP client can run through an EventLoop", "[http][socket]")
{
	EchoHandler handler;
	Euph::Net::HttpServer server("127.0.0.1", "0", handler);
	REQUIRE(server.start());
	const auto port = server.localPort();
	REQUIRE(port.has_value());
	std::thread serverThread([&] { (void)server.serveOne(); });
	Euph::Net::HttpClient client("127.0.0.1", portString(*port));
	Elv::Util::EventLoop loop;
	HttpRequest request;
	request.method = HttpMethod::Get;
	request.target = "/async";
	auto future = client.requestAsync(loop, request);
	const Euph::Net::HttpResult result = future.get();
	serverThread.join();
	REQUIRE(result.ok);
	CHECK(result.response.body == "ok");
	server.stop();
}

TEST_CASE("HTTP client and UDP server exchange a datagram request", "[http][socket]")
{
	EchoHandler handler;
	Euph::Net::HttpServerOptions serverOptions;
	serverOptions.type = Euph::Io::SocketType::UDP;
	Euph::Net::HttpServer server("127.0.0.1", "0", handler, serverOptions);
	REQUIRE(server.start());
	const auto port = server.localPort();
	REQUIRE(port.has_value());
	std::thread serverThread([&] { (void)server.serveOne(); });
	Euph::Net::HttpClientOptions clientOptions;
	clientOptions.type = Euph::Io::SocketType::UDP;
	Euph::Net::HttpClient client("127.0.0.1", portString(*port), clientOptions);
	HttpRequest request;
	request.method = HttpMethod::Put;
	request.target = "/udp";
	request.body = "udp-body";
	const Euph::Net::HttpResult result = client.request(request);
	serverThread.join();
	REQUIRE(result.ok);
	CHECK(result.response.body == "udp-body");
	server.stop();
}

TEST_CASE("HTTP client failure signal is emitted for invalid methods", "[http]")
{
	Euph::Net::HttpClient client("127.0.0.1", "1");
	std::atomic<int> failures { 0 };
	auto connection = client.onFailure.connect([&](HttpFailure failure) {
		if (failure.error == HttpError::InvalidMethod) failures.fetch_add(1);
	});
	HttpRequest request;
	request.method = HttpMethod::Unknown;
	const Euph::Net::HttpResult result = client.request(request);
	CHECK_FALSE(result.ok);
	CHECK(result.error == HttpError::InvalidMethod);
	CHECK(failures.load() == 1);
	connection.disconnect();
}

