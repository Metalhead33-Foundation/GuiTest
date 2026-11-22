# Minimal HTTP over the GuiTest sockets

This page documents the deliberately small HTTP layer added on top of
`Euph::Io::Socket`. It is a wire-format and connection scaffold, not a web
framework.

## Split

| Layer | Module | Contents |
|---|---|---|
| HTTP types/parser | `Elvavena/Net/ElvHttp.hpp/.cpp` | Methods, headers, request/response, `Content-Length` framing, limits, serializer, incomplete/error results |
| Shared socket I/O | `Euphemy/Net/EuphHttpConnection.hpp/.cpp` | Blocking read/write loops, timeout setup, parser-progress handling |
| Client | `Euphemy/Net/EuphHttpClient.hpp/.cpp` | One socket per request, TCP or connected UDP, sync result, executor-friendly async, failure signal |
| Server | `Euphemy/Net/EuphHttpServer.hpp/.cpp` | Bind/listen, one request per connection/datagram, pure-virtual handler, serialize/close |

`Client` and `Server` stay in Euphemy because they own sockets. Parsing and
signals stay in Elvavena so a game can use the wire types without linking the OS
layer.

## Client

```cpp
Euph::Net::HttpClient client("example.test", "8080");
Elv::Net::HttpRequest request;
request.method = Elv::Net::HttpMethod::Post;
request.target = "/items";
request.body = R"({"name":"sword"})";

Elv::Util::ScopedConnection<Elv::Net::HttpFailure> failure(
    client.onFailure.connect([](Elv::Net::HttpFailure f) {
        std::cerr << Elv::Net::httpErrorName(f.error) << ": " << f.message << '\n';
    }));

Euph::Net::HttpResult result = client.request(request);
if (result.ok) {
    // result.response.statusCode / headers / body
}
```

`request()` is blocking. `requestAsync(executor, request)` returns the executor's
future (`EventLoop`, `PollingEventLoop`, `ThreadPool`); the client must outlive the
task and the failure signal is emitted on the executor thread. `timeout` applies to
socket send/receive; TCP connect itself uses the OS connect timeout. A read timeout
produces `HttpError::Timeout` rather than an indefinitely pending task.

Supported methods: `GET`, `POST`, `PUT`, `PATCH`, `DELETE`, `HEAD`, `OPTIONS`.
The client sends `Host` and `Content-Length` when missing.

## Server

```cpp
class Echo final : public Euph::Net::HttpRequestHandler {
public:
    bool handle(const Elv::Net::HttpRequest& request,
                Elv::Net::HttpResponse& response) override {
        response.statusCode = 200;
        response.body = request.body.empty() ? "ok" : request.body;
        Elv::Net::setHeader(response.headers, "Content-Type", "text/plain");
        return true;
    }
};

Echo echo;
Euph::Net::HttpServer server("127.0.0.1", "8080", echo);
if (!server.start()) return 1;
server.run(); // or call serveOne() from your own loop
```

The server owns only bind/listen, request parsing, response serialization and
close. `HttpRequestHandler::handle()` receives the complete headers/body and
decides status, headers and payload. Returning `false` sends a generic error.

`serveOne()` handles one TCP connection or one UDP datagram. `localPort()` is
useful when starting on port `0`. Both client and server expose
`Signal<HttpFailure>`; connect on the owning thread.

## Intentionally not implemented

- TLS, authentication, cookies, redirects, proxies.
- Chunked transfer encoding, compression, multipart parsing.
- Keep-alive, pipelining or connection pooling.
- Routing, middleware, URL decoding or JSON serialization.
- HTTP/2/HTTP/3; UDP mode is one datagram per request/response, not QUIC.
- Automatic body-to-object deserialization. Subclasses/application code fill the gap.

These are extension points, not hidden promises. The parser rejects chunked
transfer encoding rather than pretending to support it.

## Limits and errors

`HttpLimits` caps header bytes, body bytes and header count. Parser status is
explicit:

- `Ok`: complete message; `consumed` says how many bytes belong to it;
- `Incomplete`: read more;
- `Error`: malformed/too large, with an `HttpError`.

`HEAD` responses parse headers but leave the body empty. Missing or duplicate
conflicting `Content-Length` values are rejected.

## Tests

```sh
cd build/Desktop-Debug
LD_LIBRARY_PATH=Elvavena:Euphemy:Lotte:Haruka:KaldiVK ./UnitTests/UnitTests "[http]"
LD_LIBRARY_PATH=Elvavena:Euphemy:Lotte:Haruka:KaldiVK ./UnitTests/UnitTests "[http][socket]"
```

`[http]` covers serialization, incomplete/malformed/oversize parsing and HEAD
suppression. `[http][socket]` covers TCP POST, TCP HEAD, `EventLoop` async,
UDP request/response and failure-signal emission.
