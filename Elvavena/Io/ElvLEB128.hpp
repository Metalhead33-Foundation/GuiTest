#ifndef ELVLEB128_HPP
#define ELVLEB128_HPP
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <Elvavena/Io/ElvDataStream.hpp>

namespace Elv {
namespace Io {

template <typename UInt = std::uint64_t>
UInt readULEB128(Device& dev) {
	static_assert(std::is_unsigned_v<UInt>);

	UInt result = 0;
	unsigned shift = 0;
	std::uint8_t byte = 0;

	do {
		if (dev.read(&byte, 1, 1) != 1)
			throw std::runtime_error("Unexpected EOF while reading ULEB128");

		result |= UInt(byte & 0x7F) << shift;
		shift += 7;

		if (shift >= sizeof(UInt) * 8 && (byte & 0x80))
			throw std::overflow_error("ULEB128 too large for target type");
	} while (byte & 0x80);

	return result;
}


template <typename UInt = std::uint64_t>
void writeULEB128(Device& dev, UInt value) {
	static_assert(std::is_unsigned_v<UInt>);

	do {
		std::uint8_t byte = value & 0x7F;
		value >>= 7;

		if (value != 0)
			byte |= 0x80;

		dev.write(&byte, 1, 1);
	} while (value != 0);
}


template <typename SInt = std::int64_t>
SInt readSLEB128(Device& dev) {
	static_assert(std::is_signed_v<SInt>);

	SInt result = 0;
	unsigned shift = 0;
	std::uint8_t byte = 0;

	do {
		if (dev.read(&byte, 1, 1) != 1)
			throw std::runtime_error("Unexpected EOF while reading SLEB128");

		result |= SInt(byte & 0x7F) << shift;
		shift += 7;
	} while (byte & 0x80);

	// Sign extension if the sign bit of the final byte is set
	if ((shift < sizeof(SInt) * 8) && (byte & 0x40)) {
		result |= SInt(-1) << shift;
	}

	return result;
}

template <typename SInt = std::int64_t>
void writeSLEB128(Device& dev, SInt value) {
	static_assert(std::is_signed_v<SInt>);

	bool more = true;

	while (more) {
		std::uint8_t byte = value & 0x7F;
		bool signBit = byte & 0x40;

		value >>= 7;

		if ((value == 0 && !signBit) ||
			(value == -1 && signBit)) {
			more = false;
		} else {
			byte |= 0x80;
		}

		dev.write(&byte, 1, 1);
	}
}

template <typename UInt = std::uint64_t>
struct ValULEB128 {
	UInt val;
	UInt get() const { return val; }
	operator UInt() const { return val; }
	static ValULEB128 make(UInt v) { return ValULEB128{v}; }
};

template <typename SInt = std::int64_t>
struct ValSLEB128 {
	SInt val;
	SInt get() const { return val; }
	operator SInt() const { return val; }
	static ValSLEB128 make(SInt v) { return ValSLEB128{v}; }
};

template <Util::Endian io_endianness = Util::Endian::Little, typename UInt = std::uint64_t>
DataStream<io_endianness>& operator>>(DataStream<io_endianness>& left, ValULEB128<UInt>& right) {
	right.val = readULEB128<UInt>(left.device);
	return left;
}
template <Util::Endian io_endianness = Util::Endian::Little, typename SInt = std::int64_t>
DataStream<io_endianness>& operator>>(DataStream<io_endianness>& left, ValSLEB128<SInt>& right) {
	right.val = readSLEB128<SInt>(left.device);
	return left;
}
template <Util::Endian io_endianness = Util::Endian::Little, typename UInt = std::uint64_t>
DataStream<io_endianness>& operator<<(DataStream<io_endianness>& left, ValULEB128<UInt> right) {
	writeULEB128<UInt>(left.device, right.val);
	return left;
}
template <Util::Endian io_endianness = Util::Endian::Little, typename SInt = std::int64_t>
DataStream<io_endianness>& operator<<(DataStream<io_endianness>& left, ValSLEB128<SInt> right) {
	writeSLEB128<SInt>(left.device, right.val);
	return left;
}

}
}
#endif // ELVLEB128_HPP
