# Platform I/O wrappers and streaming buffers

This note records the OS-facing sidequest that sits beside the netcode milestones:
portable readiness pollers, completion wrappers, and the contiguous ring buffer
extracted from Haruka.

## Readiness vs. completion

Two different kernel models are intentionally kept separate:

| Model | Linux | Windows | Elvavena seam | Euphemy wrapper |
|---|---|---|---|---|
| Readiness: "which handle is ready?" | epoll / kqueue | WSAPoll | `Elv::Io::EventPoller` | `EpollPoller`, `KqueuePoller`, `WSAPollPoller`, `createEventPoller()` |
| Completion: "this operation finished, here is its result" | io_uring | IOCP | `Elv::Io::AsyncCompletion` | `IoUring`, `Iocp` |
| Control-thread wakeup | eventfd / pipe | loopback UDP pair | `Elv::Io::EventLoopWakeup` | `WakeupEvent`, `createWakeupEvent()` |

A completion API cannot be forced into a readiness interface without losing the
operation result, the submitted buffer ownership and the completion ordering. The
two seams therefore coexist.

### Readiness

`EventPoller` uses opaque `intptr_t` handles (file descriptor or SOCKET), an
`EventMask` (`Read`/`Write`/`Error`/`Hangup`), optional `PollFlags`
(edge-triggered/one-shot) and caller tokens. `wait()` takes an optional timeout;
an empty optional means infinite and zero means a non-blocking poll.

`EpollPoller` supports level/edge/one-shot registrations and maps epoll flags back
to the abstract mask. `KqueuePoller` maps the read/write filters to the same mask
and uses `EV_CLEAR`/`EV_ONESHOT`. `WSAPollPoller` supports sockets only; file and
pipe HANDLEs need IOCP. `createEventPoller()` selects epoll on Linux, kqueue on
Apple/BSD and WSAPoll on Windows, and returns `nullptr` when a backend is unavailable.

### Event loop variants

`Elv::Util::EventLoop` remains the pure command pump. `Elv::Util::PollingEventLoop`
is the second variant: it owns an `EventPoller` and an `EventLoopWakeup`, executes
commands sequentially, blocks in the poller, and dispatches ready handles to
function-pointer handlers. `add`/`modify`/`remove` from another thread marshal
through the command queue, keeping all poller access on the loop thread.
`createPollingEventLoop()` in Euphemy wires the native poller and wakeup together.

The wakeup is eventfd on Linux, a non-blocking pipe on other POSIX systems, and a
loopback UDP socket pair on Windows (WSAPoll cannot watch a Win32 event handle).

### Completion

`IoUring` wraps liburing with RAII queue ownership and helpers for NOP, read,
write, accept, connect, poll-add, timeout and registered fixed-buffer I/O.
`wait()` submits pending SQEs and fills a span of `Elv::Io::AsyncCompletion`
records. `Iocp` wraps `CreateIoCompletionPort`/`GetQueuedCompletionStatus` with
the same result record shape (`IocpCompletion` carries the overlapped pointer and
Win32 error). Windows accept/connect/send/receive are submitted as the caller's
overlapped operations on an associated socket, then delivered through `Iocp::wait`.

`liburing` is optional at compile time. `Euphemy.pro` links it only when
`pkg-config --exists liburing` succeeds and defines `MH_HAS_IO_URING`; otherwise
`EuphIoUring.cpp` compiles runtime-unavailable stubs. Even with liburing linked,
Android, ChromeOS, Docker seccomp profiles and some server kernels can reject
`io_uring_setup`. `IoUring::isValid()` and `IoUring::runtimeAvailable()` expose that,
and `createIoUringIfAvailable()` gives callers a one-call null fallback. Callers
should fall back to epoll/WSAPoll for readiness or
`Elv::Io::Async<Elv::Util::ThreadPool>` for blocking file work.

## Futex

No futex wrapper is needed. C++20 `std::atomic::wait`, `notify_one` and
`notify_all` cover the portable wait/notify primitive that used to require a
platform futex layer.

## Ring buffers: two contracts

`Elv::Util::RingBuffer<T>` stores `std::optional<T>` slots. It is a logical-order
history container: move-only and non-default-constructible elements are supported,
and network code uses it for clock samples, checksums and prediction history.

`Elv::Util::ContiguousRingBuffer<T>` stores one preallocated `std::vector<T>` and
exposes contiguous `readSpan()`/`writeSpan()` regions. It is the streaming
container extracted from `Hrk::RingBufferedStreamer`: decoders fill a contiguous
free span, consumers drain a contiguous filled span, and a wrapped buffer is
handled with at most two copies.

They are intentionally not unified. The slot buffer's flexibility (arbitrary
element lifetimes) conflicts with the contiguous buffer's bulk-span contract. The
Haruka streamer now uses `ContiguousRingBuffer<float>` and keeps only its decoder
and state-cursor logic.

## Not yet done

- A `select(2)` fallback is not implemented; epoll, kqueue and WSAPoll cover the
  current targets.
- WSAPoll, IOCP and kqueue are compiled but not exercised by Linux CI. The
  io_uring test skips cleanly when `io_uring_setup` is disabled.
- io_uring completion helpers cover accept/connect and registered fixed buffers;
  `sendmsg`/`recvmsg` receive-buffer rings and buffer-update tags are future
  additions.
- The polling loop processes command batches before waiting, but it does not yet
  expose per-handle timers or a cancellation token for in-flight poll waits.

## Tests

```sh
cd build/Desktop-Debug
LD_LIBRARY_PATH=Elvavena:Euphemy:Lotte:Haruka:KaldiVK ./UnitTests/UnitTests "[ringbuffer]"
LD_LIBRARY_PATH=Elvavena:Euphemy:Lotte:Haruka:KaldiVK ./UnitTests/UnitTests "[eventloop]"
LD_LIBRARY_PATH=Elvavena:Euphemy:Lotte:Haruka:KaldiVK ./UnitTests/UnitTests "[epoll]"
LD_LIBRARY_PATH=Elvavena:Euphemy:Lotte:Haruka:KaldiVK ./UnitTests/UnitTests "[kqueue]"
LD_LIBRARY_PATH=Elvavena:Euphemy:Lotte:Haruka:KaldiVK ./UnitTests/UnitTests "[uring]"
LD_LIBRARY_PATH=Elvavena:Euphemy:Lotte:Haruka:KaldiVK ./UnitTests/UnitTests "[iocp]"
```
