#ifndef ELVNETRPC_HPP
#define ELVNETRPC_HPP
/**
 * @file ElvNetRpc.hpp
 * @brief Typed, type-erased remote procedure dispatch.
 *
 * Procedures are identified by a stable `RpcId` and bound once at startup with
 * their argument types. The wire payload is a varint id followed by the
 * arguments encoded by `RpcCodec<T>`; the registry decodes them and invokes the
 * bound callable. The outbound `RpcEncoder` and inbound `RpcDispatcher` are
 * independent of any transport: the game decides whether a message travels on a
 * reliable, unreliable, multicast or owner-targeted channel.
 *
 * Routing is deliberately policy, not protocol: `RpcTarget` names the intended
 * audience, while the game supplies the peer directory and calls `Session::send`.
 */
#include <Elvavena/Io/ElvBitStream.hpp>
#include <Elvavena/Net/ElvNetTypes.hpp>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <span>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Elv {
namespace Net {

/// @brief Stable procedure identifier; never derived from a type name or hash.
using RpcId = std::uint32_t;

/**
 * @enum RpcTarget
 * @brief Intended audience of an outgoing RPC.
 */
enum class RpcTarget : std::uint8_t {
	Server,    ///< The authoritative server.
	Multicast, ///< A caller-selected set of peers.
	Owner,     ///< The peer that owns a referenced entity.
	Others,    ///< Every peer except the sender.
};

/**
 * @enum RpcStatus
 * @brief Result of an inbound dispatch.
 */
enum class RpcStatus : std::uint8_t {
	Ok,                ///< Handler ran successfully.
	UnknownProcedure,  ///< No handler is bound to the id.
	Malformed,         ///< Argument decoding failed or the payload was truncated.
	TooLarge,          ///< A bounded codec rejected an oversized value.
	Unauthorized,      ///< The policy rejected the call.
	BufferTooSmall,    ///< The outbound encoder could not fit the message.
	HandlerError,      ///< The handler threw.
};

/**
 * @struct RpcContext
 * @brief Metadata supplied to an inbound handler.
 */
struct RpcContext {
	PeerId sender = InvalidPeer; ///< Peer that sent the call.
	Tick tick = 0;               ///< Simulation tick associated with the call.
	bool server = false;         ///< Whether this process is the server.
	void* user = nullptr;        ///< Game-owned context pointer.
};

/**
 * @class RpcPolicy
 * @brief Optional authorization seam for inbound calls.
 */
class RpcPolicy {
public:
	/** @brief Virtual destructor. */
	virtual ~RpcPolicy() = default;

	/**
	 * @brief Returns whether a call may be executed.
	 * @param context Sender metadata.
	 * @param id Procedure id.
	 * @return True to allow; the default allows everything.
	 */
	[[nodiscard]] virtual bool allow(const RpcContext& context, RpcId id) const {
		(void)context;
		(void)id;
		return true;
	}
};

/**
 * @struct RpcCodec
 * @brief Wire codec for one argument type.
 *
 * Built-in support covers bool, integral and enum types, float, double,
 * `std::string` and `std::vector<std::byte>`. Games can specialize this struct
 * for their own value types.
 *
 * @tparam T Argument type.
 */
template <typename T, typename Enable = void>
struct RpcCodec {
	/**
	 * @brief Encodes a value.
	 * @param writer Destination writer.
	 * @param value Value to encode.
	 * @return True on success.
	 */
	static bool encode(Io::BitWriter& writer, const T& value) {
		if constexpr (std::is_same_v<T, bool>) {
			return writer.writeBool(value);
		} else if constexpr (std::is_enum_v<T>) {
			using Underlying = std::underlying_type_t<T>;
			return RpcCodec<Underlying>::encode(writer, static_cast<Underlying>(value));
		} else if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) {
			return writer.writeVarSInt(static_cast<std::int64_t>(value));
		} else if constexpr (std::is_integral_v<T>) {
			return writer.writeVarUInt(static_cast<std::uint64_t>(value));
		} else if constexpr (std::is_same_v<T, float>) {
			return writer.writeFloat32(value);
		} else if constexpr (std::is_same_v<T, double>) {
			return writer.writeFloat64(value);
		} else {
			static_assert(!std::is_same_v<T, T>, "No RpcCodec specialization for this type");
			return false;
		}
	}

	/**
	 * @brief Decodes a value.
	 * @param reader Source reader.
	 * @param value Receives the decoded value.
	 * @return True on success.
	 */
	static bool decode(Io::BitReader& reader, T& value) {
		if constexpr (std::is_same_v<T, bool>) {
			return reader.readBool(value);
		} else if constexpr (std::is_enum_v<T>) {
			using Underlying = std::underlying_type_t<T>;
			Underlying raw {};
			if (!RpcCodec<Underlying>::decode(reader, raw)) return false;
			value = static_cast<T>(raw);
			return true;
		} else if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) {
			std::int64_t raw = 0;
			if (!reader.readVarSInt(raw)) return false;
			if (raw < static_cast<std::int64_t>(std::numeric_limits<T>::min())
				|| raw > static_cast<std::int64_t>(std::numeric_limits<T>::max())) {
				return false;
			}
			value = static_cast<T>(raw);
			return true;
		} else if constexpr (std::is_integral_v<T>) {
			std::uint64_t raw = 0;
			if (!reader.readVarUInt(raw)) return false;
			if (raw > static_cast<std::uint64_t>(std::numeric_limits<T>::max())) return false;
			value = static_cast<T>(raw);
			return true;
		} else if constexpr (std::is_same_v<T, float>) {
			return reader.readFloat32(value);
		} else if constexpr (std::is_same_v<T, double>) {
			return reader.readFloat64(value);
		} else {
			static_assert(!std::is_same_v<T, T>, "No RpcCodec specialization for this type");
			return false;
		}
	}
};

/** @brief Codec specialization for UTF-8/ASCII strings. */
template <>
struct RpcCodec<std::string> {
	/// @brief Maximum accepted string length in bytes.
	static constexpr std::uint64_t MaxLength = 1024 * 1024;

	/** @brief Encodes a length-prefixed string. */
	static bool encode(Io::BitWriter& writer, const std::string& value) {
		if (value.size() > MaxLength) return false;
		return writer.writeVarUInt(value.size())
			&& writer.writeBytes(value.data(), value.size());
	}

	/** @brief Decodes a length-prefixed string with a bounded allocation. */
	static bool decode(Io::BitReader& reader, std::string& value) {
		std::uint64_t length = 0;
		if (!reader.readVarUInt(length) || length > MaxLength) return false;
		value.resize(static_cast<std::size_t>(length));
		if (length == 0) return true;
		return reader.readBytes(value.data(), static_cast<std::size_t>(length));
	}
};

/** @brief Codec specialization for binary blobs. */
template <>
struct RpcCodec<std::vector<std::byte>> {
	/// @brief Maximum accepted blob length in bytes.
	static constexpr std::uint64_t MaxLength = 1024 * 1024;

	/** @brief Encodes a length-prefixed byte vector. */
	static bool encode(Io::BitWriter& writer, const std::vector<std::byte>& value) {
		if (value.size() > MaxLength) return false;
		return writer.writeVarUInt(value.size())
			&& writer.writeBytes(value.data(), value.size());
	}

	/** @brief Decodes a length-prefixed byte vector with a bounded allocation. */
	static bool decode(Io::BitReader& reader, std::vector<std::byte>& value) {
		std::uint64_t length = 0;
		if (!reader.readVarUInt(length) || length > MaxLength) return false;
		value.resize(static_cast<std::size_t>(length));
		if (length == 0) return true;
		return reader.readBytes(value.data(), static_cast<std::size_t>(length));
	}
};

/**
 * @class RpcEncoder
 * @brief Builds an RPC payload from a procedure id and typed arguments.
 */
class RpcEncoder {
public:
	/**
	 * @brief Encodes an RPC message into a new byte vector.
	 * @tparam Id Stable procedure id.
	 * @tparam Args Argument types.
	 * @param capacity Maximum payload size; the result is resized down.
	 * @param args Arguments to encode.
	 * @return Encoded payload, or an empty vector on failure.
	 * @note An empty result is distinguishable because a successful payload
	 *       always contains at least the encoded id.
	 */
	template <RpcId Id, typename... Args>
	static std::vector<std::byte> encode(std::size_t capacity, const Args&... args) {
		std::vector<std::byte> buffer(capacity);
		Io::BitWriter writer(buffer);
		if (!writer.writeVarUInt(Id)) return {};
		if (!(RpcCodec<std::decay_t<Args>>::encode(writer, args) && ...)) return {};
		buffer.resize(writer.byteSize());
		return buffer;
	}

	/**
	 * @brief Encodes an RPC message into a caller-provided buffer.
	 * @tparam Id Stable procedure id.
	 * @tparam Args Argument types.
	 * @param target Destination span.
	 * @param args Arguments to encode.
	 * @return True when the whole message fits and encodes.
	 */
	template <RpcId Id, typename... Args>
	static bool encodeInto(std::span<std::byte> target, const Args&... args) {
		Io::BitWriter writer(target);
		return writer.writeVarUInt(Id)
			&& (RpcCodec<std::decay_t<Args>>::encode(writer, args) && ...)
			&& writer.ok();
	}
};


/**
 * @class RpcRegistry
 * @brief Maps stable procedure ids to typed callables.
 *
 * Binding is a startup operation and may use `std::function` internally. The
 * dispatch path decodes arguments directly and invokes the callable without an
 * additional per-argument virtual call.
 */
class RpcRegistry {
public:
	/**
	 * @brief Binds a typed handler.
	 * @tparam Id Stable procedure id.
	 * @tparam Args Argument types in wire order.
	 * @tparam Fn Callable type.
	 * @param callable Handler invoked as `callable(const RpcContext&, Args...)`.
	 * @return True when the id was not already bound.
	 */
	template <RpcId Id, typename... Args, typename Fn>
	bool bind(Fn&& callable) {
		static_assert(std::is_invocable_v<Fn&, const RpcContext&, Args...>,
			"RPC handler signature must be (const RpcContext&, Args...)");
		auto handler = [function = std::forward<Fn>(callable)](
			const RpcContext& context, Io::BitReader& reader) mutable -> bool {
			std::tuple<std::decay_t<Args>...> arguments {};
			if (!decodeArguments(reader, arguments, std::index_sequence_for<Args...> {})) {
				return false;
			}
			std::apply([&](auto&... values) {
				std::invoke(function, context, values...);
			}, arguments);
			return true;
		};
		return handlers.emplace(static_cast<RpcId>(Id), std::move(handler)).second;
	}

	/**
	 * @brief Dispatches one procedure call.
	 * @param id Procedure id.
	 * @param reader Reader positioned at the first argument.
	 * @param context Sender metadata.
	 * @param policy Optional authorization policy.
	 * @return Dispatch status.
	 */
	RpcStatus invoke(RpcId id, Io::BitReader& reader, const RpcContext& context,
		const RpcPolicy* policy = nullptr) const {
		if (policy != nullptr && !policy->allow(context, id)) {
			return RpcStatus::Unauthorized;
		}
		const auto iterator = handlers.find(id);
		if (iterator == handlers.end()) return RpcStatus::UnknownProcedure;
		bool decoded = false;
		try {
			decoded = iterator->second(context, reader);
		} catch (...) {
			return RpcStatus::HandlerError;
		}
		return decoded ? RpcStatus::Ok : RpcStatus::Malformed;
	}

	/**
	 * @brief Returns whether a procedure id is bound.
	 * @param id Procedure id.
	 */
	[[nodiscard]] bool contains(RpcId id) const { return handlers.find(id) != handlers.end(); }

	/** @brief Returns the number of bound procedures. */
	[[nodiscard]] std::size_t size() const noexcept { return handlers.size(); }

	/** @brief Removes every binding. */
	void clear() noexcept { handlers.clear(); }

private:
	using Handler = std::function<bool(const RpcContext&, Io::BitReader&)>;

	/**
	 * @brief Decodes a tuple of arguments in order.
	 * @tparam Tuple Decayed argument tuple.
	 * @tparam Indices Argument indices.
	 * @param reader Source reader.
	 * @param arguments Receives the values.
	 * @return True when every argument decoded.
	 */
	template <typename Tuple, std::size_t... Indices>
	static bool decodeArguments(Io::BitReader& reader, Tuple& arguments,
		std::index_sequence<Indices...>) {
		return (RpcCodec<std::tuple_element_t<Indices, Tuple>>::decode(
			reader, std::get<Indices>(arguments)) && ...);
	}

	std::unordered_map<RpcId, Handler> handlers; ///< Bound procedures.
};

/**
 * @class RpcDispatcher
 * @brief Decodes an inbound payload and dispatches its procedure.
 */
class RpcDispatcher {
public:
	/**
	 * @brief Creates a dispatcher.
	 * @param registry Registry that owns the bindings.
	 * @param policy Optional authorization policy.
	 */
	explicit RpcDispatcher(RpcRegistry& registry, const RpcPolicy* policy = nullptr)
		: registry(registry), policy(policy) {}

	/**
	 * @brief Decodes the procedure id and invokes the handler.
	 * @param payload Complete RPC payload.
	 * @param context Sender metadata.
	 * @return Dispatch status.
	 */
	RpcStatus dispatch(std::span<const std::byte> payload, const RpcContext& context) {
		Io::BitReader reader(payload);
		std::uint64_t rawId = 0;
		if (!reader.readVarUInt(rawId)
			|| rawId > std::numeric_limits<RpcId>::max()) {
			return RpcStatus::Malformed;
		}
		return registry.invoke(static_cast<RpcId>(rawId), reader, context, policy);
	}

	/** @brief Replaces the authorization policy. */
	void setPolicy(const RpcPolicy* next) noexcept { policy = next; }

private:
	RpcRegistry& registry;     ///< Bound procedure table.
	const RpcPolicy* policy;   ///< Optional authorization policy.
};

} // namespace Net
} // namespace Elv

#endif // ELVNETRPC_HPP

