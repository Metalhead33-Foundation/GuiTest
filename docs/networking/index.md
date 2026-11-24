# Networking and platform I/O {#docs_networking_index}

*The netcode foundation, the minimal HTTP layer, and the OS-facing I/O seams*

Status: index (revision 1 — 2026-11-24). Platform-independent protocol contracts live
in Elvavena, the OS calls live in Euphemy; these notes record that split and the state
of each milestone. The module pages are
[`Elvavena`](../modules/index.md) and [`Euphemy`](../modules/index.md).

| Note | What it covers |
|---|---|
| \subpage doc_netcode | The transport-agnostic session: packet/fragment layout, reliable channels, snapshots and deltas, interpolation/jitter, prediction/rollback, lockstep, the reference replication pipeline and the remaining milestones. |
| \subpage doc_http | The deliberately minimal HTTP/1.1 layer over `Euph::Io::Socket`: parser/serializer split, client/server, limits, and what is intentionally absent. |
| \subpage doc_platform | The OS-facing sidequest: readiness (`EventPoller`) vs completion (`IoUring`/`Iocp`), the wakeup seam, the polling event loop, and the two ring-buffer contracts. |

## References

- [`../index.md`](../index.md) — the documentation tree this folder is a child of.
- [`../../Elvavena/Net`](../../Elvavena/Net) — the protocol headers the netcode note
  describes.
- [`../../Elvavena/Net/ElvHttp.hpp`](../../Elvavena/Net/ElvHttp.hpp) — the wire types.
- [`../../Euphemy/Platform`](../../Euphemy/Platform) — the concrete pollers, wakeups and
  completion wrappers.
- [`../../UnitTests/README.md`](../../UnitTests/README.md) — how to run the `[net]`,
  `[http]` and `[ringbuffer]` tags.
