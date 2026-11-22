#include "ElvNetReplicationSchema.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <utility>
#include <vector>

namespace Elv {
namespace Net {
namespace {

/**
 * @class VectorDevice
 * @brief Internal growable/read-only device for schema payloads.
 *
 * This is a private implementation detail of the object component codec. The
 * public tree deliberately exposes no second generic buffer device; Euphemy's
 * concrete devices remain the application-facing ones.
 */
class VectorDevice final : public Io::Device {
public:
	VectorDevice() = default;
	explicit VectorDevice(std::vector<std::byte> data) : buffer(std::move(data)), writable(false) {}

	size_t read(void* destination, size_t size, size_t count) override {
		if (destination == nullptr || size == 0 || count == 0) return 0;
		if (count > std::numeric_limits<size_t>::max() / size) return 0;
		const size_t requested = size * count;
		const size_t available = buffer.size() - cursor;
		const size_t copied = std::min(requested, available);
		if (copied != 0) std::memcpy(destination, buffer.data() + cursor, copied);
		cursor += copied;
		return copied / size;
	}

	size_t write(const void* source, size_t size, size_t count) override {
		if (!writable || source == nullptr || size == 0 || count == 0) return 0;
		if (count > std::numeric_limits<size_t>::max() / size) return 0;
		const size_t requested = size * count;
		if (buffer.size() < cursor + requested) buffer.resize(cursor + requested);
		std::memcpy(buffer.data() + cursor, source, requested);
		cursor += requested;
		return count;
	}

	int seek(long offset, Io::SeekOrigin whence) override {
		const std::int64_t total = static_cast<std::int64_t>(buffer.size());
		std::int64_t base = 0;
		switch (whence) {
		case Io::SeekOrigin::SET: base = 0; break;
		case Io::SeekOrigin::CUR: base = static_cast<std::int64_t>(cursor); break;
		case Io::SeekOrigin::END: base = total; break;
		}
		const std::int64_t requested = base + static_cast<std::int64_t>(offset);
		if (requested < 0 || requested > total) return -1;
		cursor = static_cast<size_t>(requested);
		return 0;
	}

	long tell() override { return static_cast<long>(cursor); }
	size_t size() override { return buffer.size(); }
	bool eof() override { return cursor >= buffer.size(); }
	Io::Mode getMode() const override { return writable ? Io::Mode::READ_WRITE : Io::Mode::READ; }
	bool flush() override { return true; }
	bool isValid() const override { return true; }

	/** @brief Returns whether the read cursor consumed the entire payload. */
	[[nodiscard]] bool consumed() const noexcept { return cursor == buffer.size(); }

	/** @brief Moves the buffer out of the device. */
	std::vector<std::byte> release() { return std::move(buffer); }

private:
	std::vector<std::byte> buffer; ///< Owned bytes.
	size_t cursor = 0;             ///< Read/write position.
	bool writable = true;          ///< Whether writes are allowed.
};

/**
 * @brief Serializes an object using the existing byte schema.
 * @param schema Object layout.
 * @param data Object instance.
 * @return Serialized bytes.
 */
std::vector<std::byte> serializeObject(const Io::ObjectDefinition& schema, const void* data) {
	VectorDevice device;
	schema.serialize(device, NetEndian, data);
	return device.release();
}

/**
 * @brief Encodes an object as a length-prefixed byte payload.
 * @param writer Destination bit writer.
 * @param data Object instance.
 * @param context `const Io::ObjectDefinition*`.
 * @return True on success.
 */
bool encodeObject(Io::BitWriter& writer, const void* data, const void* context) {
	if (data == nullptr || context == nullptr) return false;
	const auto* schema = static_cast<const Io::ObjectDefinition*>(context);
	const std::vector<std::byte> bytes = serializeObject(*schema, data);
	if (bytes.size() > 1024 * 1024) return false;
	return writer.writeVarUInt(bytes.size()) && writer.writeBytes(bytes);
}

/**
 * @brief Decodes a length-prefixed object payload.
 * @param reader Source bit reader.
 * @param data Object instance to populate.
 * @param context `const Io::ObjectDefinition*`.
 * @return True when the whole payload decoded.
 */
bool decodeObject(Io::BitReader& reader, void* data, const void* context) {
	if (data == nullptr || context == nullptr) return false;
	const auto* schema = static_cast<const Io::ObjectDefinition*>(context);
	std::uint64_t length = 0;
	if (!reader.readVarUInt(length) || length > 1024 * 1024) return false;
	std::vector<std::byte> bytes(static_cast<std::size_t>(length));
	if (length != 0 && !reader.readBytes(bytes.data(), bytes.size())) return false;
	VectorDevice device(std::move(bytes));
	schema->deserialize(device, NetEndian, data);
	return device.consumed();
}

/**
 * @brief Compares two objects by their serialized representation.
 * @param lhs First object.
 * @param rhs Second object.
 * @param context `const Io::ObjectDefinition*`.
 * @return True when both serialize to the same bytes.
 */
bool equalObject(const void* lhs, const void* rhs, const void* context) {
	if (lhs == nullptr || rhs == nullptr || context == nullptr) return false;
	const auto* schema = static_cast<const Io::ObjectDefinition*>(context);
	return serializeObject(*schema, lhs) == serializeObject(*schema, rhs);
}

} // namespace

const ComponentOps* objectComponentOps() noexcept {
	static const ComponentOps ops { encodeObject, decodeObject, equalObject };
	return &ops;
}

} // namespace Net
} // namespace Elv
