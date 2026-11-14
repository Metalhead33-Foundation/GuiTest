#ifndef ELVSIGNAL_HPP
#define ELVSIGNAL_HPP
/**
 * @file ElvSignal.hpp
 * @brief Lightweight signal/slot implementation for Elvavena.
 *
 * The `NewBeginning` branch used the third-party `sigslot.h` header
 * (<https://github.com/palacaze/sigslot>) to give widgets
 * `signal_onStateChanged` / `signal_onPositionChanged` members and to let
 * connections be scoped to a subscriber's lifetime. This is Elvavena's own
 * replacement, providing the same two ideas without a 2,000-line header or
 * macro-generated class names:
 *
 * - @ref Elv::Util::Signal is a multi-subscriber callable object; `connect()`
 *   returns an @ref Elv::Util::Connection that can cancel the subscription.
 * - @ref Elv::Util::ScopedConnection disconnects on destruction, which is what
 *   keeps widget listeners safe when the listener dies first.
 *
 * @section elv_signal_semantics Semantics
 *
 * - Slots are invoked in connection order.
 * - A slot may connect or disconnect during emission. The implementation walks a
 *   snapshot of shared slot records, so mutating the subscription list while
 *   emitting is safe and a cancelled record simply becomes a no-op.
 * - A slot that connects *during* emission is not called for the emission in
 *   progress; it is called from the next emission onwards. This matches sigslot.
 * - The owner of the @ref Elv::Util::Signal must outlive its connections. A
 *   @ref Elv::Util::Connection only holds a weak reference, so a stale
 *   connection becomes inert instead of dangling.
 * - This class is intentionally single-threaded: GUI event delivery happens on
 *   the owning thread. Wrap it externally if it must cross threads.
 *
 * @note Elvavena also has `Observable`/`Observer` (`ElvObserv.hpp`), an abstract
 *       interface with `shared_ptr` subscriptions. That pattern suits long-lived
 *       producer/consumer relationships; `Signal` is for event fan-out inside a
 *       widget tree. Both are kept.
 */

#include <Elvavena/Util/ElvUtilGlobals.hpp>
#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace Elv {
namespace Util {

/**
 * @brief Handle to one subscription of a @ref Elv::Util::Signal.
 *
 * Copyable and cheap. Disconnecting is idempotent, and a default-constructed
 * connection is inert.
 */
template <typename... Args>
class Connection
{
public:
	/** @brief Slot record shared between the signal and its connections. */
	struct Slot {
		/** @brief Callable invoked by the signal. */
		std::function<void(Args...)> callback;
		/** @brief Cleared when the subscription is cancelled. */
		bool connected = true;
	};
private:
	std::weak_ptr<Slot> slot;
public:
	/** @brief Creates an inert connection. */
	Connection() = default;
	/**
	 * @brief Wraps a slot record.
	 * @param slotRecord Record owned by the signal.
	 */
	explicit Connection(std::weak_ptr<Slot> slotRecord) : slot(std::move(slotRecord)) {}

	/** @brief Returns true when the subscription is still live. */
	bool isConnected() const
	{
		const std::shared_ptr<Slot> record = slot.lock();
		return record && record->connected;
	}

	/** @brief Returns true when this handle refers to a record that still exists. */
	bool isValid() const { return !slot.expired(); }

	/** @brief Cancels the subscription; safe to call repeatedly. */
	void disconnect()
	{
		if (const std::shared_ptr<Slot> record = slot.lock()) record->connected = false;
	}

	/** @brief Returns the number of arguments the connection carries. */
	static constexpr std::size_t getArity() noexcept { return sizeof...(Args); }
};

/**
 * @brief RAII wrapper that disconnects a @ref Elv::Util::Connection on destruction.
 *
 * Use this for listeners owned by another object: the signal keeps only a weak
 * reference, so when the listener is destroyed the subscription is cancelled
 * instead of delivering into freed memory.
 */
template <typename... Args>
class ScopedConnection
{
private:
	Connection<Args...> connection;
public:
	/** @brief Creates an inert scoped connection. */
	ScopedConnection() = default;
	/**
	 * @brief Adopts a connection.
	 * @param value Connection to disconnect on destruction.
	 */
	explicit ScopedConnection(Connection<Args...> value) : connection(std::move(value)) {}
	/** @brief Disconnects the held connection. */
	~ScopedConnection() { connection.disconnect(); }

	ScopedConnection(const ScopedConnection&) = delete;
	ScopedConnection& operator=(const ScopedConnection&) = delete;

	/** @brief Takes over another scoped connection. */
	ScopedConnection(ScopedConnection&& other) noexcept : connection(std::move(other.connection))
	{
		other.connection = Connection<Args...>();
	}
	/** @brief Disconnects the current connection and takes over another. */
	ScopedConnection& operator=(ScopedConnection&& other) noexcept
	{
		if (this != &other) {
			connection.disconnect();
			connection = std::move(other.connection);
			other.connection = Connection<Args...>();
		}
		return *this;
	}

	/** @brief Returns the held connection. */
	const Connection<Args...>& getConnection() const { return connection; }
	/** @brief Cancels the subscription early. */
	void disconnect() { connection.disconnect(); }
};

/**
 * @brief Multi-subscriber callable with explicit connection handles.
 * @tparam Args Argument types delivered to every slot.
 */
template <typename... Args>
class Signal
{
public:
	/** @brief Slot record type. */
	using Slot = typename Connection<Args...>::Slot;
	/** @brief Callable accepted by @ref connect(). */
	using Callback = std::function<void(Args...)>;
private:
	std::vector<std::shared_ptr<Slot>> slots;
public:
	/** @brief Creates an empty signal. */
	Signal() = default;

	Signal(const Signal&) = delete;
	Signal& operator=(const Signal&) = delete;

	/**
	 * @brief Subscribes a callable.
	 * @param callback Function to invoke on emission; an empty callable is ignored.
	 * @return A connection handle for the new subscription.
	 */
	Connection<Args...> connect(Callback callback)
	{
		if (!callback) return Connection<Args...>();
		auto record = std::make_shared<Slot>();
		record->callback = std::move(callback);
		slots.push_back(record);
		return Connection<Args...>(record);
	}

	/**
	 * @brief Subscribes a member function.
	 * @tparam Object Receiver type.
	 * @param object Receiver; must outlive the subscription.
	 * @param method Member function called with the signal arguments.
	 * @return A connection handle for the new subscription.
	 */
	template <typename Object>
	Connection<Args...> connect(Object* object, void (Object::*method)(Args...))
	{
		if (!object) return Connection<Args...>();
		return connect([object, method](Args... args) { (object->*method)(std::forward<Args>(args)...); });
	}

	/** @brief Drops every subscription. */
	void disconnectAll()
	{
		for (const auto& record : slots) record->connected = false;
		slots.clear();
	}

	/** @brief Returns the number of live subscriptions. */
	std::size_t getConnectionCount() const
	{
		std::size_t count = 0;
		for (const auto& record : slots) {
			if (record->connected) ++count;
		}
		return count;
	}

	/** @brief Returns true when nobody is listening. */
	bool empty() const { return getConnectionCount() == 0; }

	/**
	 * @brief Invokes every live slot in connection order.
	 * @param args Arguments forwarded to each slot.
	 */
	void emit(Args... args) const
	{
		if (slots.empty()) return;
		// Snapshot the records: a slot may connect or disconnect while we iterate,
		// and `slots` may reallocate when it appends.
		std::vector<std::shared_ptr<Slot>> snapshot(slots);
		for (const auto& record : snapshot) {
			if (record->connected) record->callback(args...);
		}
	}

	/** @brief Invokes every live slot, same as @ref emit(). */
	void operator()(Args... args) const { emit(std::forward<Args>(args)...); }

	/** @brief Removes cancelled records from the subscription list. */
	void prune()
	{
		slots.erase(std::remove_if(slots.begin(), slots.end(), [](const std::shared_ptr<Slot>& record) {
			return !record->connected;
		}), slots.end());
	}
};

}
}

#endif // ELVSIGNAL_HPP
