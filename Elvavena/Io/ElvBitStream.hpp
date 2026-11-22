#ifndef ELVBITSTREAM_HPP
#define ELVBITSTREAM_HPP
/**
 * @file ElvBitStream.hpp
 * @brief Bit-level packet writer and reader.
 *
 * `Elv::Io::DataStream` is a byte-oriented serializer: it writes native-width
 * primitives and container lengths. Network packets need finer control: bools
 * in one bit, enum flags in three bits, positions quantized to 16 bits, and
 * varints for identifiers. This header supplies that layer.
 *
 * Bits are written most-significant-bit first within each byte, which matches
 * the conventional Quake-family packet layout and keeps the wire format easy to
 * inspect. Multi-bit values are also emitted MSB-first.
 *
 * Both classes are allocation-free, bounds-checked and non-throwing. A decode
 * failure sets a status and leaves the caller in control; malformed packets
 * must not unwind through a network loop.
 */
#include <Elvavena/Util/half.hpp>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>

namespace Elv {
namespace Io {

/**
 * @enum BitStreamStatus
 * @brief Result of the most recent bit-stream operation.
 */
enum class BitStreamStatus : std::uint8_t {
	Ok,              ///< Every operation succeeded.
	BufferFull,      ///< A write would have exceeded the target span.
	BufferExhausted, ///< A read would have exceeded the source span.
	InvalidArgument, ///< A bit count, range or quantized parameter was invalid.
};

/**
 * @class BitWriter
 * @brief Writes packed fields into a caller-provided byte span.
 *
 * The writer never allocates and never grows the target. Once an operation
 * fails the status is latched and subsequent operations fail quickly.
 *
 * @note Not thread-safe.
 */
class BitWriter {
public:
	/**
	 * @brief Creates a writer over a caller-owned destination span.
	 * @param target Bytes that receive the packed output.
	 */
	explicit BitWriter(std::span<std::byte> target) noexcept : target(target) {}

	/** @brief Returns the latched status. */
	[[nodiscard]] BitStreamStatus status() const noexcept { return state; }

	/** @brief Returns whether no operation has failed. */
	[[nodiscard]] bool ok() const noexcept { return state == BitStreamStatus::Ok; }

	/** @brief Returns the number of bits written so far. */
	[[nodiscard]] std::size_t bitPosition() const noexcept { return bitPos; }

	/** @brief Returns the number of whole or partial bytes touched. */
	[[nodiscard]] std::size_t byteSize() const noexcept { return (bitPos + 7) / 8; }

	/** @brief Returns the total capacity in bits. */
	[[nodiscard]] std::size_t capacityBits() const noexcept { return target.size() * 8; }

	/** @brief Returns the bytes written so far, including a partial tail byte. */
	[[nodiscard]] std::span<std::byte> written() const noexcept { return target.first(byteSize()); }

	/** @brief Returns the remaining capacity in bits. */
	[[nodiscard]] std::size_t remainingBits() const noexcept {
		return capacityBits() - std::min(bitPos, capacityBits());
	}

	/**
	 * @brief Writes one bit.
	 * @param value Bit value.
	 * @return True on success.
	 */
	bool writeBit(bool value) { return writeBits(value ? 1u : 0u, 1); }

	/**
	 * @brief Writes the low @p count bits of @p value, MSB-first.
	 * @param value Source value; bits above @p count are ignored.
	 * @param count Number of bits to write, at most 64.
	 * @return True on success.
	 */
	bool writeBits(std::uint64_t value, unsigned count) {
		if (count > 64) return fail(BitStreamStatus::InvalidArgument);
		if (count == 0) return ok();
		if (!ok()) return false;
		if (count > remainingBits()) return fail(BitStreamStatus::BufferFull);

		for (unsigned index = 0; index < count; ++index) {
			const unsigned sourceBit = count - 1 - index;
			const bool bit = ((value >> sourceBit) & 1ull) != 0;
			const std::size_t absolute = bitPos + index;
			const std::size_t byteIndex = absolute / 8;
			const unsigned bitIndex = static_cast<unsigned>(absolute % 8);
			const std::uint8_t mask = static_cast<std::uint8_t>(1u << (7u - bitIndex));
			std::uint8_t current = std::to_integer<std::uint8_t>(target[byteIndex]);
			if (bit) current = static_cast<std::uint8_t>(current | mask);
			else current = static_cast<std::uint8_t>(current & static_cast<std::uint8_t>(~mask));
			target[byteIndex] = std::byte { current };
		}
		bitPos += count;
		return true;
	}

	/**
	 * @brief Writes a boolean as one bit.
	 * @param value Boolean value.
	 * @return True on success.
	 */
	bool writeBool(bool value) { return writeBit(value); }

	/**
	 * @brief Writes a signed value using two's-complement truncation.
	 * @param value Signed value.
	 * @param count Number of bits to write, at most 64.
	 * @return True on success.
	 * @note The caller is responsible for choosing a width that can represent
	 *       the value. Prefer the zig-zag or varint helpers for unknown ranges.
	 */
	bool writeSigned(std::int64_t value, unsigned count) {
		return writeBits(static_cast<std::uint64_t>(value), count);
	}

	/**
	 * @brief Writes a signed value with zig-zag encoding.
	 * @param value Signed value.
	 * @param count Number of bits available, at most 64.
	 * @return True on success, false when the zig-zag value does not fit.
	 */
	bool writeSignedZigZag(std::int64_t value, unsigned count) {
		const std::uint64_t encoded = (static_cast<std::uint64_t>(value) << 1)
			^ static_cast<std::uint64_t>(value < 0 ? ~std::uint64_t { 0 } : 0);
		if (count < 64 && (encoded >> count) != 0)
			return fail(BitStreamStatus::InvalidArgument);
		return writeBits(encoded, count);
	}

	/**
	 * @brief Writes an unsigned LEB128 value.
	 * @param value Value to encode.
	 * @return True on success.
	 */
	bool writeVarUInt(std::uint64_t value) {
		do {
			std::uint8_t byte = static_cast<std::uint8_t>(value & 0x7Fu);
			value >>= 7;
			if (value != 0) byte = static_cast<std::uint8_t>(byte | 0x80u);
			if (!writeBits(byte, 8)) return false;
		} while (value != 0);
		return true;
	}

	/**
	 * @brief Writes a signed value as a zig-zag LEB128 value.
	 * @param value Value to encode.
	 * @return True on success.
	 */
	bool writeVarSInt(std::int64_t value) {
		const std::uint64_t encoded = (static_cast<std::uint64_t>(value) << 1)
			^ static_cast<std::uint64_t>(value < 0 ? ~std::uint64_t { 0 } : 0);
		return writeVarUInt(encoded);
	}

	/**
	 * @brief Writes an IEEE-754 binary32 value bit-exactly.
	 * @param value Floating-point value.
	 * @return True on success.
	 */
	bool writeFloat32(float value) {
		return writeBits(std::bit_cast<std::uint32_t>(value), 32);
	}

	/**
	 * @brief Writes an IEEE-754 binary64 value bit-exactly.
	 * @param value Floating-point value.
	 * @return True on success.
	 */
	bool writeFloat64(double value) {
		return writeBits(std::bit_cast<std::uint64_t>(value), 64);
	}

	/**
	 * @brief Writes an IEEE-754 binary16 value bit-exactly.
	 * @param value Half-float value.
	 * @return True on success.
	 */
	bool writeHalf(half_float::half value) {
		static_assert(sizeof(half_float::half) == sizeof(std::uint16_t));
		return writeBits(std::bit_cast<std::uint16_t>(value), 16);
	}

	/**
	 * @brief Quantizes a float into an unsigned fixed-width integer.
	 * @param value Value to encode; clamped to [@p minimum, @p maximum].
	 * @param minimum Lower bound of the representable range.
	 * @param maximum Upper bound of the representable range.
	 * @param bits Number of bits to write, 1 through 32.
	 * @return True on success.
	 * @note A degenerate range (`maximum <= minimum`) writes zero. NaN maps to
	 *       the lower bound. The inverse operation is
	 *       @ref BitReader::readQuantizedFloat.
	 */
	bool writeQuantizedFloat(float value, float minimum, float maximum, unsigned bits) {
		if (bits == 0 || bits > 32) return fail(BitStreamStatus::InvalidArgument);
		if (!(maximum > minimum)) return writeBits(0, bits);

		if (!std::isfinite(value)) value = minimum;
		const double clamped = std::clamp(static_cast<double>(value),
			static_cast<double>(minimum), static_cast<double>(maximum));
		const double normalized = (clamped - static_cast<double>(minimum))
			/ (static_cast<double>(maximum) - static_cast<double>(minimum));
		const std::uint64_t levels = (bits == 32)
			? 0xFFFFFFFFull
			: ((1ull << bits) - 1ull);
		std::uint64_t quantized = static_cast<std::uint64_t>(
			std::llround(normalized * static_cast<double>(levels)));
		if (quantized > levels) quantized = levels;
		return writeBits(quantized, bits);
	}

	/**
	 * @brief Writes raw bytes at the current bit position.
	 * @param bytes Bytes to copy.
	 * @return True on success.
	 * @note When the writer is byte-aligned this is a single `memcpy`;
	 *       otherwise each byte is written as eight bits.
	 */
	bool writeBytes(std::span<const std::byte> bytes) {
		if (bytes.empty()) return ok();
		if (!ok()) return false;
		if (bitPos % 8 == 0) {
			if (bytes.size() > remainingBits() / 8) return fail(BitStreamStatus::BufferFull);
			std::memcpy(target.data() + byteSize(), bytes.data(), bytes.size());
			bitPos += bytes.size() * 8;
			return true;
		}
		for (const std::byte value : bytes) {
			if (!writeBits(std::to_integer<std::uint8_t>(value), 8)) return false;
		}
		return true;
	}

	/**
	 * @brief Writes raw bytes from a pointer and size.
	 * @param data Pointer to the bytes; may be null when @p size is zero.
	 * @param size Number of bytes.
	 * @return True on success.
	 */
	bool writeBytes(const void* data, std::size_t size) {
		if (size == 0) return ok();
		if (data == nullptr) return fail(BitStreamStatus::InvalidArgument);
		return writeBytes(std::span<const std::byte>(
			static_cast<const std::byte*>(data), size));
	}

	/**
	 * @brief Pads with zero bits until the next byte boundary.
	 * @return True on success.
	 */
	bool alignToByte() {
		const unsigned remainder = static_cast<unsigned>(bitPos % 8);
		if (remainder == 0) return ok();
		return writeBits(0, 8 - remainder);
	}

	/** @brief Returns the writer to its initial state without clearing the buffer. */
	void reset() noexcept {
		bitPos = 0;
		state = BitStreamStatus::Ok;
	}

private:
	/**
	 * @brief Latches the first failure.
	 * @param next Failure status.
	 * @return Always false, so callers can `return fail(...)`.
	 */
	bool fail(BitStreamStatus next) noexcept {
		if (state == BitStreamStatus::Ok) state = next;
		return false;
	}

	std::span<std::byte> target;         ///< Caller-owned destination.
	std::size_t bitPos = 0;              ///< Number of bits written.
	BitStreamStatus state = BitStreamStatus::Ok; ///< Latched result.
};


/**
 * @class BitReader
 * @brief Reads packed fields from a caller-provided byte span.
 *
 * The reader mirrors @ref BitWriter. It never allocates, never throws and
 * latches the first failure. Callers must check the return value or
 * @ref status before using output arguments.
 *
 * @note Not thread-safe.
 */
class BitReader {
public:
	/**
	 * @brief Creates a reader over a caller-owned source span.
	 * @param source Bytes to decode.
	 */
	explicit BitReader(std::span<const std::byte> source) noexcept : source(source) {}

	/** @brief Returns the latched status. */
	[[nodiscard]] BitStreamStatus status() const noexcept { return state; }

	/** @brief Returns whether no operation has failed. */
	[[nodiscard]] bool ok() const noexcept { return state == BitStreamStatus::Ok; }

	/** @brief Returns the number of bits consumed so far. */
	[[nodiscard]] std::size_t bitPosition() const noexcept { return bitPos; }

	/** @brief Returns the number of whole or partial bytes consumed. */
	[[nodiscard]] std::size_t byteSize() const noexcept { return (bitPos + 7) / 8; }

	/** @brief Returns the total source size in bits. */
	[[nodiscard]] std::size_t capacityBits() const noexcept { return source.size() * 8; }

	/** @brief Returns the number of unread bits. */
	[[nodiscard]] std::size_t remainingBits() const noexcept {
		return capacityBits() - std::min(bitPos, capacityBits());
	}

	/**
	 * @brief Reads one bit.
	 * @param value Receives the bit value.
	 * @return True on success.
	 */
	bool readBit(bool& value) {
		std::uint64_t raw = 0;
		if (!readBits(raw, 1)) return false;
		value = raw != 0;
		return true;
	}

	/**
	 * @brief Reads @p count bits MSB-first into the low bits of @p value.
	 * @param value Receives the decoded value.
	 * @param count Number of bits to read, at most 64.
	 * @return True on success.
	 */
	bool readBits(std::uint64_t& value, unsigned count) {
		if (count > 64) return fail(BitStreamStatus::InvalidArgument);
		if (count == 0) {
			value = 0;
			return ok();
		}
		if (!ok()) return false;
		if (count > remainingBits()) return fail(BitStreamStatus::BufferExhausted);

		std::uint64_t result = 0;
		for (unsigned index = 0; index < count; ++index) {
			const std::size_t absolute = bitPos + index;
			const std::size_t byteIndex = absolute / 8;
			const unsigned bitIndex = static_cast<unsigned>(absolute % 8);
			const std::uint8_t current = std::to_integer<std::uint8_t>(source[byteIndex]);
			const std::uint64_t bit = (current >> (7u - bitIndex)) & 1u;
			result = (result << 1) | bit;
		}
		value = result;
		bitPos += count;
		return true;
	}

	/**
	 * @brief Reads a boolean stored in one bit.
	 * @param value Receives the boolean.
	 * @return True on success.
	 */
	bool readBool(bool& value) { return readBit(value); }

	/**
	 * @brief Reads a two's-complement signed value.
	 * @param value Receives the signed value.
	 * @param count Number of bits to read, at most 64.
	 * @return True on success.
	 */
	bool readSigned(std::int64_t& value, unsigned count) {
		std::uint64_t raw = 0;
		if (!readBits(raw, count)) return false;
		if (count == 0) {
			value = 0;
			return true;
		}
		if (count < 64) {
			const std::uint64_t signBit = 1ull << (count - 1);
			if ((raw & signBit) != 0) raw |= ~((1ull << count) - 1ull);
		}
		value = static_cast<std::int64_t>(raw);
		return true;
	}

	/**
	 * @brief Reads a zig-zag encoded signed value.
	 * @param value Receives the signed value.
	 * @param count Number of bits to read, at most 64.
	 * @return True on success.
	 */
	bool readSignedZigZag(std::int64_t& value, unsigned count) {
		std::uint64_t encoded = 0;
		if (!readBits(encoded, count)) return false;
		value = (encoded & 1ull) != 0
			? static_cast<std::int64_t>(~(encoded >> 1))
			: static_cast<std::int64_t>(encoded >> 1);
		return true;
	}


	/**
	 * @brief Reads an unsigned LEB128 value.
	 * @param value Receives the decoded value.
	 * @return True on success, false on truncation or malformed input.
	 */
	bool readVarUInt(std::uint64_t& value) {
		std::uint64_t result = 0;
		unsigned shift = 0;
		for (unsigned index = 0; index < 10; ++index) {
			std::uint64_t byte = 0;
			if (!readBits(byte, 8)) return false;
			if (shift >= 64) return fail(BitStreamStatus::InvalidArgument);
			if (shift == 63 && (byte & 0xFEull) != 0) {
				return fail(BitStreamStatus::InvalidArgument);
			}
			result |= (byte & 0x7Full) << shift;
			if ((byte & 0x80ull) == 0) {
				value = result;
				return true;
			}
			shift += 7;
		}
		return fail(BitStreamStatus::InvalidArgument);
	}

	/**
	 * @brief Reads a zig-zag LEB128 signed value.
	 * @param value Receives the decoded value.
	 * @return True on success.
	 */
	bool readVarSInt(std::int64_t& value) {
		std::uint64_t encoded = 0;
		if (!readVarUInt(encoded)) return false;
		value = (encoded & 1ull) != 0
			? static_cast<std::int64_t>(~(encoded >> 1))
			: static_cast<std::int64_t>(encoded >> 1);
		return true;
	}

	/**
	 * @brief Reads an IEEE-754 binary32 value bit-exactly.
	 * @param value Receives the value.
	 * @return True on success.
	 */
	bool readFloat32(float& value) {
		std::uint64_t raw = 0;
		if (!readBits(raw, 32)) return false;
		value = std::bit_cast<float>(static_cast<std::uint32_t>(raw));
		return true;
	}

	/**
	 * @brief Reads an IEEE-754 binary64 value bit-exactly.
	 * @param value Receives the value.
	 * @return True on success.
	 */
	bool readFloat64(double& value) {
		std::uint64_t raw = 0;
		if (!readBits(raw, 64)) return false;
		value = std::bit_cast<double>(raw);
		return true;
	}

	/**
	 * @brief Reads an IEEE-754 binary16 value bit-exactly.
	 * @param value Receives the value.
	 * @return True on success.
	 */
	bool readHalf(half_float::half& value) {
		std::uint64_t raw = 0;
		if (!readBits(raw, 16)) return false;
		value = std::bit_cast<half_float::half>(static_cast<std::uint16_t>(raw));
		return true;
	}

	/**
	 * @brief Decodes a value written by @ref BitWriter::writeQuantizedFloat.
	 * @param value Receives the decoded float.
	 * @param minimum Lower bound of the representable range.
	 * @param maximum Upper bound of the representable range.
	 * @param bits Number of bits to read, 1 through 32.
	 * @return True on success.
	 */
	bool readQuantizedFloat(float& value, float minimum, float maximum, unsigned bits) {
		if (bits == 0 || bits > 32) return fail(BitStreamStatus::InvalidArgument);
		std::uint64_t quantized = 0;
		if (!readBits(quantized, bits)) return false;
		if (!(maximum > minimum)) {
			value = minimum;
			return true;
		}
		const std::uint64_t levels = (bits == 32)
			? 0xFFFFFFFFull
			: ((1ull << bits) - 1ull);
		const double normalized = static_cast<double>(quantized)
			/ static_cast<double>(levels);
		value = static_cast<float>(static_cast<double>(minimum)
			+ normalized * (static_cast<double>(maximum) - static_cast<double>(minimum)));
		return true;
	}

	/**
	 * @brief Reads raw bytes from the current bit position.
	 * @param bytes Destination span; its full size is filled.
	 * @return True on success.
	 */
	bool readBytes(std::span<std::byte> bytes) {
		if (bytes.empty()) return ok();
		if (!ok()) return false;
		if (remainingBits() < bytes.size() * 8) {
			return fail(BitStreamStatus::BufferExhausted);
		}
		if (bitPos % 8 == 0) {
			std::memcpy(bytes.data(), source.data() + byteSize(), bytes.size());
			bitPos += bytes.size() * 8;
			return true;
		}
		for (std::byte& value : bytes) {
			std::uint64_t raw = 0;
			if (!readBits(raw, 8)) return false;
			value = std::byte { static_cast<std::uint8_t>(raw) };
		}
		return true;
	}

	/**
	 * @brief Reads raw bytes into a pointer and size.
	 * @param data Destination pointer; may be null when @p size is zero.
	 * @param size Number of bytes.
	 * @return True on success.
	 */
	bool readBytes(void* data, std::size_t size) {
		if (size == 0) return ok();
		if (data == nullptr) return fail(BitStreamStatus::InvalidArgument);
		return readBytes(std::span<std::byte>(static_cast<std::byte*>(data), size));
	}

	/**
	 * @brief Discards @p count bits.
	 * @param count Number of bits to skip.
	 * @return True on success.
	 */
	bool skipBits(std::size_t count) {
		if (!ok()) return false;
		if (count > remainingBits()) return fail(BitStreamStatus::BufferExhausted);
		bitPos += count;
		return true;
	}

	/**
	 * @brief Skips padding until the next byte boundary.
	 * @return True on success.
	 */
	bool alignToByte() {
		const unsigned remainder = static_cast<unsigned>(bitPos % 8);
		if (remainder == 0) return ok();
		return skipBits(8 - remainder);
	}

	/** @brief Returns the reader to its initial state. */
	void reset() noexcept {
		bitPos = 0;
		state = BitStreamStatus::Ok;
	}

private:
	/**
	 * @brief Latches the first failure.
	 * @param next Failure status.
	 * @return Always false, so callers can `return fail(...)`.
	 */
	bool fail(BitStreamStatus next) noexcept {
		if (state == BitStreamStatus::Ok) state = next;
		return false;
	}

	std::span<const std::byte> source; ///< Caller-owned source.
	std::size_t bitPos = 0;            ///< Number of bits consumed.
	BitStreamStatus state = BitStreamStatus::Ok; ///< Latched result.
};

} // namespace Io
} // namespace Elv

#endif // ELVBITSTREAM_HPP

