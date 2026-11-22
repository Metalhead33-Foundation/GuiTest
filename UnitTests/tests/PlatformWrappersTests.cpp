/**
 * @file PlatformWrappersTests.cpp
 * @brief Tests for the contiguous streaming buffer and OS wrapper seams.
 *
 * The event tests use a pipe on Linux so they exercise the real epoll backend.
 * The io_uring case skips cleanly when the kernel or sandbox disables it, which
 * is the same runtime fallback the production factory expects.
 */
#include <catch2/catch_test_macros.hpp>
#include <Elvavena/Util/ElvContiguousRingBuffer.hpp>
#include <Elvavena/Util/ElvPollingEventLoop.hpp>
#include <Euphemy/Platform/EuphEventPollerFactory.hpp>
#include <Euphemy/Platform/EuphIocp.hpp>
#include <Euphemy/Platform/EuphIoUring.hpp>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <future>
#include <string>
#include <vector>

#if defined(__linux__)
#include <unistd.h>
#endif

namespace {

using namespace std::chrono_literals;
using Elv::Io::EventMask;
using Elv::Io::PollEvent;
using Elv::Util::ContiguousRingBuffer;

#if defined(__linux__)
struct EventLatch {
	std::promise<std::uint64_t> token;
	int descriptor = -1;
};

void onReadable(void* userData, const PollEvent& event) {
	auto* latch = static_cast<EventLatch*>(userData);
	char byte = 0;
	read(latch->descriptor, &byte, 1);
	try {
		latch->token.set_value(event.token);
	} catch (...) {
		// A later level-triggered notification may race the first delivery.
	}
}
#endif

} // namespace

TEST_CASE("Contiguous ring buffer exposes wrapped spans and bulk copies", "[platform][ringbuffer]")
{
	ContiguousRingBuffer<int> buffer(5);
	CHECK(buffer.capacity() == 5);
	CHECK(buffer.free() == 5);
	CHECK(buffer.writeSpan().size() == 5);

	const int first[] { 1, 2, 3, 4, 5 };
	CHECK(buffer.write(first, 5) == 5);
	CHECK(buffer.full());
	CHECK(buffer.readSpan().size() == 5);

	int output[5] {};
	CHECK(buffer.read(output, 3) == 3);
	CHECK(output[0] == 1);
	CHECK(output[2] == 3);
	CHECK(buffer.size() == 2);

	const int second[] { 6, 7, 8 };
	CHECK(buffer.write(second, 3) == 3); // wraps: [4,5,6,7,8]
	CHECK(buffer.size() == 5);
	CHECK(buffer[0] == 4);
	CHECK(buffer[4] == 8);

	CHECK(buffer.read(output, 5) == 5);
	CHECK(output[0] == 4);
	CHECK(output[2] == 6);
	CHECK(output[4] == 8);
	CHECK(buffer.empty());
}

TEST_CASE("Contiguous ring buffer supports push/pop and resize", "[platform][ringbuffer]")
{
	ContiguousRingBuffer<int> buffer(2);
	CHECK(buffer.push(1));
	CHECK(buffer.push(2));
	CHECK_FALSE(buffer.push(3));
	int value = 0;
	CHECK(buffer.pop(value));
	CHECK(value == 1);
	CHECK(buffer.push(3));
	CHECK(buffer[0] == 2);
	CHECK(buffer[1] == 3);

	buffer.resize(4);
	CHECK(buffer.capacity() == 4);
	CHECK(buffer.empty());
	CHECK(buffer.writeSpan().size() == 4);

	ContiguousRingBuffer<int> empty(0);
	CHECK_FALSE(empty.push(1));
	CHECK(empty.writeSpan().empty());
	CHECK(empty.readSpan().empty());
}

TEST_CASE("Event poller factory selects a usable backend", "[platform][poller]")
{
	auto poller = Euph::Platform::createEventPoller();
	REQUIRE(poller != nullptr);
	CHECK(poller->isValid());
	CHECK(poller->backendName() != nullptr);
	CHECK(std::string(poller->backendName()).size() > 0);
	CHECK(std::string(Euph::Platform::defaultEventPollerName()).size() > 0);
}

#if defined(__linux__)
TEST_CASE("Epoll poller reports pipe readiness and honours modify/remove", "[platform][epoll]")
{
	auto poller = Euph::Platform::createEventPoller(Euph::Platform::PollerBackend::Epoll);
	REQUIRE(poller != nullptr);
	REQUIRE(poller->isValid());

	int descriptors[2] { -1, -1 };
	REQUIRE(pipe(descriptors) == 0);
	const auto readHandle = static_cast<Elv::Io::NativeHandle>(descriptors[0]);
	REQUIRE(poller->add(readHandle, EventMask::Read, 11));

	std::array<PollEvent, 4> events {};
	CHECK(poller->wait(events, 0ms) == 0);

	const std::byte byte { 0x5A };
	REQUIRE(write(descriptors[1], &byte, 1) == 1);
	int ready = poller->wait(events, 100ms);
	REQUIRE(ready >= 1);
	CHECK(events[0].handle == readHandle);
	CHECK(events[0].token == 11);
	CHECK(Elv::Io::hasEvent(events[0].events, EventMask::Read));

	REQUIRE(poller->modify(readHandle, EventMask::Read, 22));
	CHECK(poller->wait(events, 0ms) >= 1);
	CHECK(events[0].token == 22);

	REQUIRE(poller->remove(readHandle));
	CHECK_FALSE(poller->remove(readHandle));

	close(descriptors[0]);
	close(descriptors[1]);
}
#endif

TEST_CASE("Polling event loop executes commands and dispatches readiness", "[platform][eventloop]")
{
	auto loop = Euph::Platform::createPollingEventLoop();
	REQUIRE(loop != nullptr);
	CHECK(loop->isValid());
	CHECK(loop->running());
	CHECK(loop->enqueueSync([] { return 42; }) == 42);
	auto asyncResult = loop->enqueueAsync([] { return 7; });
	CHECK(asyncResult.get() == 7);

#if defined(__linux__)
	int descriptors[2] { -1, -1 };
	REQUIRE(pipe(descriptors) == 0);
	EventLatch latch;
	latch.descriptor = descriptors[0];
	const auto readHandle = static_cast<Elv::Io::NativeHandle>(descriptors[0]);
	REQUIRE(loop->add(readHandle, EventMask::Read, 77, { onReadable, &latch }));
	const std::byte byte { 0x33 };
	REQUIRE(write(descriptors[1], &byte, 1) == 1);
	auto tokenFuture = latch.token.get_future();
	REQUIRE(tokenFuture.wait_for(2s) == std::future_status::ready);
	CHECK(tokenFuture.get() == 77);
	CHECK(loop->remove(readHandle));
	close(descriptors[0]);
	close(descriptors[1]);
#endif
	loop->stop();
	CHECK_FALSE(loop->running());
}

TEST_CASE("kqueue backend is available only on BSD-like platforms", "[platform][kqueue]")
{
	auto poller = Euph::Platform::createEventPoller(Euph::Platform::PollerBackend::Kqueue);
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) \
	|| defined(__OpenBSD__) || defined(__DragonFly__)
	REQUIRE(poller != nullptr);
	CHECK(poller->isValid());
#else
	CHECK(poller == nullptr);
#endif
}

TEST_CASE("io_uring wrapper submits a NOP when available", "[platform][uring]")
{
	auto ring = Euph::Platform::createIoUringIfAvailable(16);
	if (!ring) {
		SUCCEED("io_uring is disabled on this machine");
		return;
	}
	REQUIRE(ring->isValid());
	REQUIRE(ring->queueDepth() >= 16);
	REQUIRE(ring->submitNop(1234));

	std::array<Elv::Io::AsyncCompletion, 1> completions {};
	const int completed = ring->wait(completions, 1s);
	REQUIRE(completed == 1);
	CHECK(completions[0].userData == 1234);
	CHECK(completions[0].result == 0);
}

TEST_CASE("IOCP is a safe unavailable stub outside Windows", "[platform][iocp]")
{
	Euph::Platform::Iocp iocp;
#if defined(_WIN32)
	CHECK(iocp.isValid());
#else
	CHECK_FALSE(iocp.isValid());
	CHECK(iocp.lastError() != 0);
#endif
	CHECK(iocp.backendName() != nullptr);
}
