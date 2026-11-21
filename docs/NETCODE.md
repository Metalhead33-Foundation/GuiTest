# Netcode foundation

This note records the layer boundaries and the current state of the networking
foundation. The headers are the API reference; this page explains where the
pieces live and what is deliberately not implemented.

**Status: M0 (wire primitives), M1 (reliable session + UDP transport), M2 (clock sync +
RPC) and the replication core (NetId, component schema, entity registry, interest hook)
implemented and tested.** Later milestones are listed at the end.

## Layer split

The networking layer follows the same dependency rule as the device layer:
platform-independent contracts and protocol logic live in Elvavena, concrete OS
sockets live in Euphemy.

| Layer | Module | Contents |
|---|---|---|
| Wire primitives | `Elvavena/Io/ElvBitStream.hpp`, `Elvavena/Util/ElvChecksum.hpp`, `Elvavena/Util/ElvRingBuffer.hpp` | Bit packing, quantized floats, varints, stable CRC-32C/FNV-1a, fixed-capacity history |
| Packet protocol | `Elvavena/Net/ElvNetProtocol.hpp`, `ElvNetSequence.hpp`, `ElvNetFragment.hpp` | Header/handshake/fragment layouts, wrap-safe ack window, bounded reassembly |
| Transport seam | `Elvavena/Net/ElvDatagramTransport.hpp` | Non-blocking datagram send/receive over opaque endpoints |
| Session | `Elvavena/Net/ElvNetSession.hpp/.cpp`, `ElvNetCongestion.hpp` | Handshake, per-channel reliability, acking, fragmentation, heartbeat/timeout, pacing seam |
| Clock sync | `Elvavena/Net/ElvNetClockSync.hpp` | Lowest-RTT offset estimate, EWMA smoothing, tick/time conversion |
| RPC | `Elvavena/Net/ElvNetRpc.hpp` | Typed argument codecs, registry, encoder, dispatcher, policy, target enum |
| Replication core | `Elvavena/Net/ElvNetId.hpp`, `ElvNetReplicationSchema.*`, `ElvNetReplicationRegistry.hpp`, `ElvNetInterest.hpp` | Generation-checked ids, validated component/entity-kind descriptors, non-owning entity/component slots, batch interest hook |
| Test wire | `Elvavena/Net/ElvLoopbackTransport.hpp` | Deterministic latency/jitter/loss/duplication; seeded RNG |
| OS transport | `Euphemy/Net/EuphUdpTransport.hpp` | UDP socket adapter over `Euph::Io::Socket` |
| Socket hardening | `Euphemy/Io/EuphSocket.hpp/.cpp` | Endpoint conversion, datagram methods, non-blocking mode, `addrinfo` ownership, state fixes |

The dependency direction is one-way: `Euphemy` links `Elvavena`, never the reverse.
A game that only needs the protocol can link Elvavena and supply its own transport.

## Session contract

`Elv::Net::Session` is poll-driven and owns no thread and no tick loop:

```cpp
Elv::Net::TimePoint now = /* game or test clock */;
session.poll(now);   // drain incoming datagrams
session.flush(now);  // retransmit, heartbeat, expire peers
session.send(peer, payload, options, now);
```

- **Channels.** Each channel has a configured `ChannelReliability`:
  `Unreliable`, `UnreliableSequenced`, `ReliableUnordered`, `ReliableOrdered`.
  Reliability is a property of the channel, not of an individual send.
- **Acking.** Reliable datagrams carry a 16-bit per-channel sequence and a
  64-entry ack window. `SendOptions::userTag` is reported back through
  `SessionEvents::onPacketAcknowledged`/`onPacketLost`.
- **Fragmentation.** Payloads larger than the MTU are split into indexed
  fragments and reassembled with a timeout. Reliable channels acknowledge each
  fragment; unreliable fragmented messages are best-effort.
- **Handshake.** The client sends a versioned request with a nonce and optional
  schema hash; the server assigns a peer id and a connection token. The client
  retries the request while connecting, and the server re-sends the accept for a
  repeated request.
- **Heartbeats.** Idle peers exchange Ping/Pong; a peer that stays silent for
  `peerTimeout` is removed with `NetError::Timeout`.
- **Errors.** Protocol methods return `NetError`; malformed packets never throw
  through the network loop. The transport is single-threaded and must outlive
  the session.

The wire header is documented in `ElvNetProtocol.hpp`. It is 21 bytes, followed
by type-specific fields: 10 more bytes for a fragment. Multi-bit fields are
written MSB-first by `ElvBitStream`.

## Hooks and extension points

There are two deliberate mechanisms:

- **Abstract interfaces** for services selected at runtime:
  `DatagramTransport`, `CongestionController`, `InterestManager` and `RpcPolicy`.
- **Function-pointer ops tables** for per-type/per-schema hot paths, following
  the existing `Elv::Io::CompositeOps` precedent. `ComponentOps` is implemented;
  `FieldDeltaOps` and `InterpolationOps<T>` are the planned tables for snapshot
  deltas and interpolation. They avoid virtual dispatch per entity or field and
  keep the replication registry data-oriented.

Notification callbacks (`SessionEvents`) have no-op defaults, so a game only
overrides what it needs. Setup-time callbacks may use `std::function`; per-packet
and per-entity paths should not.

## What is intentionally not here

- No encryption or authentication. The handshake token prevents casual spoofing
  and cross-address injection; it is not a security mechanism. Put DTLS or a
  game-level authenticated handshake above the transport.
- No NAT traversal, matchmaking or relay infrastructure. Relay is a game
  topology built from `Session::send`/`sendTo`.
- No internal network thread and no tick loop. Wrap the session in
  `Elv::Util::EventLoop` if a dedicated network thread is wanted.
- No game-state ownership. The future replication registry stores non-owning
  component pointers; the game owns its ECS storage.
- No RTT-driven congestion control yet. `FixedRateCongestion` is a simple token
  bucket; the interface is the place for a better controller.
- No strict ordering *across* fragmented messages. Fragments are reassembled
  and delivered as a complete payload; ordering relative to other fragmented
  messages is a later refinement.

## Remaining milestones

| Milestone | Surface |
|---|---|
| M3b | `ElvNetSnapshot.hpp`: snapshot writer/reader, spawn/despawn framing, per-component delta against an acknowledged baseline, ack-tag integration |
| M4 | `ElvNetInterpolation.hpp` (`InterpolationOps<T>`, schema field interpolation, snapshot/jitter buffers), `ElvNetPrediction.hpp` (input queues, delay buffer, rollback replay, mismatch/checksum history), `ElvNetLockstep.hpp` (input collector, checksum exchange) |
| M5 | Reference replication pipeline, Sandbox loopback demo, broader WinSock/IPv6 socket coverage |

The replication core deliberately stops at the metadata layer. It gives the
snapshot writer a stable id, a validated component descriptor, a component mask
and a non-owning pointer; it does not yet define a snapshot wire format or a
delta baseline store. That is M3b, and it is the next dependency for the
interpolation/prediction helpers.

## Tests

The networking tests run without the OS socket path:

```sh
cd build/Desktop-Debug
LD_LIBRARY_PATH=Elvavena:Euphemy:Lotte:Haruka ./UnitTests/UnitTests "[net]"
```

The localhost UDP smoke test is tagged separately:

```sh
LD_LIBRARY_PATH=Elvavena:Euphemy:Lotte:Haruka ./UnitTests/UnitTests "[net][socket]"
```

`[net]` covers bit streams, sequence/ack windows, checksums, ring buffers,
loopback delivery/loss/duplication, handshake retry, reliable delivery over a
35% loss link, fragmentation, heartbeat timeout, disconnect, channel
validation, clock estimation/tick conversion, typed RPC dispatch, NetId reuse,
component schema validation, the object-schema codec, entity registry/component
masks and interest filtering.

