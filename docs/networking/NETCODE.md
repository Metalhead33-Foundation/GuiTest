# Netcode foundation {#doc_netcode}

This note records the layer boundaries and the current state of the networking
foundation. The headers are the API reference; this page explains where the
pieces live and what is deliberately not implemented.

**Status: M0 through M4 and the M5 reference pipeline implemented and tested.** The
remaining M5 gap is broader WinSock/IPv6 socket-path coverage; the protocol,
snapshot/delta, interpolation/jitter, prediction and lockstep helpers are in place.

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
| Snapshot/delta | `Elvavena/Net/ElvNetSnapshot.hpp`, `ElvNetFieldDelta.hpp` | Spawn/update/despawn framing, full or field-delta component payloads, acknowledged-baseline delta, ack-tag promotion/rejection, filtered per-interest writes, schema fixed-field delta codec |
| Interpolation/jitter | `Elvavena/Net/ElvNetInterpolation.hpp` | `InterpolationOps`, arithmetic and schema-field interpolation, generic tick `JitterBuffer`, snapshot payload jitter buffer |
| Prediction | `Elvavena/Net/ElvNetPrediction.hpp` | Ordered input queues, delay buffer, checksum history, `PredictionSession` mismatch detection, `replayInputs` |
| Lockstep | `Elvavena/Net/ElvNetLockstep.hpp` | All-peers-ready input collector, per-peer checksum exchange, first-divergence lookup |
| Replication pipeline | `Elvavena/Net/ElvNetReplicationPipeline.hpp` | Per-peer snapshot writers driven by `InterestManager`, ack/loss routing, client receive/jitter buffer |
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
  the existing `Elv::Io::CompositeOps` precedent. `ComponentOps` drives snapshot
  payloads; `InterpolationOps` drives jitter-buffer sampling and schema field
  interpolation. Snapshot deltas compare encoded component payloads by default;
  components may install an optional `FieldDeltaOps` table (`ElvNetFieldDelta.hpp`
  provides a schema fixed-field implementation) to send a changed-field mask plus
  raw changed bytes instead.

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

## Milestones

| Milestone | Surface | State |
|---|---|---|
| M3b | `ElvNetSnapshot.hpp`: snapshot writer/reader, spawn/despawn framing, per-component delta against an acknowledged baseline, ack-tag integration | Implemented |
| M4 | `ElvNetInterpolation.hpp`, `ElvNetPrediction.hpp`, `ElvNetLockstep.hpp`: interpolation/jitter, input queues/delay, rollback mismatch detection, lockstep input/checksum exchange | Implemented |
| M5 | Reference replication pipeline (`ElvNetReplicationPipeline.hpp`), Sandbox loopback demo, broader WinSock/IPv6 socket coverage | Pipeline/demo implemented; WinSock/IPv6 CI remains |

The snapshot writer owns the acknowledged baseline and keeps candidate baselines
pending by the `SendOptions::userTag` of the in-flight snapshot. `acknowledge(tag)`
promotes the candidate; `reject(tag)` drops it. `ReplicationServer` keeps one
writer per peer and feeds it the ids returned by `InterestManager`, so visibility
changes naturally become spawn/despawn records. `ReplicationClient` parses
payloads and keeps recent encoded frames in a jitter buffer; applying records to
the game ECS remains game-owned.

Interpolation is opt-in: use `makeInterpolationOps<T>()` for arithmetic types or
`schemaInterpolationOps()` with explicit per-field rules. Prediction and lockstep
provide the bookkeeping only; the game supplies the simulation/replay callback.

## Tests

The networking tests run without the OS socket path:

```sh
cd build/Desktop-Debug
LD_LIBRARY_PATH=Elvavena:Euphemy:Lotte:Haruka:KaldiVK ./UnitTests/UnitTests "[net]"
```

The localhost UDP smoke test is tagged separately:

```sh
LD_LIBRARY_PATH=Elvavena:Euphemy:Lotte:Haruka:KaldiVK ./UnitTests/UnitTests "[net][socket]"
```

`[net]` covers bit streams, sequence/ack windows, checksums, ring buffers,
loopback delivery/loss/duplication, handshake retry, reliable delivery over a
35% loss link, fragmentation, heartbeat timeout, disconnect, channel
validation, clock estimation/tick conversion, typed RPC dispatch, NetId reuse,
component schema validation, the object-schema codec, entity registry/component
masks, interest filtering, snapshot spawn/delta/removal/despawn/ack baselines,
interpolation/jitter buffers, prediction input/checksum rollback, lockstep
input/checksum exchange and the per-peer replication pipeline. The current suite
has 132 cases and 1393 assertions; `[net][socket]` adds the localhost UDP path and
`[http][socket]` adds the HTTP TCP/UDP path.

## References

- [`../index.md`](../index.md) — the networking folder this note belongs to.
- [`../../Elvavena/Net`](../../Elvavena/Net) — the protocol headers this note
  describes.
- [`../../Euphemy/Net/EuphUdpTransport.hpp`](../../Euphemy/Net/EuphUdpTransport.hpp)
  — the concrete transport the session layer is driven by.
- [`../../UnitTests/tests/NetSessionTests.cpp`](../../UnitTests/tests/NetSessionTests.cpp)
  and its siblings — the executable contract for every milestone row above.
- \ref mod_elvavena and \ref mod_euphemy — the module pages that own the two halves.

