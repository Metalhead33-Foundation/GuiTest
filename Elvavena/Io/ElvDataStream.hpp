#ifndef ELVDATASTREAM_HPP
#define ELVDATASTREAM_HPP
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <Elvavena/Util/ElvContainerBasic.hpp>
#include <Elvavena/Util/ElvEndianness.hpp>
#include <optional>
namespace Elv {
namespace Io {

// ----------------------------------------------------------------------------
// Core Algorithms
// ----------------------------------------------------------------------------

template <typename UInt>
UInt readULEB128(Device& dev) {
	static_assert(std::is_unsigned_v<UInt>);
	UInt result = 0;
	unsigned shift = 0;
	std::uint8_t byte = 0;

	while (true) {
		if (dev.read(&byte, 1, 1) != 1)
			throw std::runtime_error("Unexpected EOF reading ULEB128");

		// Safety Guard: Don't shift if we exceed the type's width
		if (shift < sizeof(UInt) * 8) {
			result |= UInt(byte & 0x7F) << shift;
		} else if (byte & 0x7F) {
			// We have exceeded the type width, but the byte still has data.
			// This implies the number is too big for UInt.
			throw std::overflow_error("ULEB128 overflow");
		}

		if ((byte & 0x80) == 0) break;

		shift += 7;
	}
	return result;
}

template <typename SInt>
SInt readSLEB128(Device& dev) {
	static_assert(std::is_signed_v<SInt>);
	SInt result = 0;
	unsigned shift = 0;
	std::uint8_t byte = 0;
	const unsigned size = sizeof(SInt) * 8;

	while (true) {
		if (dev.read(&byte, 1, 1) != 1)
			throw std::runtime_error("Unexpected EOF reading SLEB128");

		if (shift < size) {
			result |= SInt(byte & 0x7F) << shift;
			shift += 7;
		} else if (byte & 0x7F) {
			// Only throw if data bits are lost; standard allows padding 0s (or 1s for negative)
			// But strictly speaking, if we shift out, we check overflow strictly here:
			throw std::overflow_error("SLEB128 overflow");
		}

		if ((byte & 0x80) == 0) break;
	}

	// Sign extension
	if (shift < size && (byte & 0x40)) {
		// -1 is all 1s. Shifting left creates a mask of 1s in the high bits.
		result |= SInt(-1) << shift;
	}

	return result;
}

// Write functions are generally safer, but your logic was good.
// Just ensuring 'value' shifts logically.
template <typename UInt>
void writeULEB128(Device& dev, UInt value) {
	static_assert(std::is_unsigned_v<UInt>);
	do {
		std::uint8_t byte = value & 0x7F;
		value >>= 7;
		if (value != 0) byte |= 0x80;
		dev.write(&byte, 1, 1);
	} while (value != 0);
}

template <typename SInt>
void writeSLEB128(Device& dev, SInt value) {
	static_assert(std::is_signed_v<SInt>);
	bool more = true;
	while (more) {
		std::uint8_t byte = value & 0x7F;
		// Arithmetic shift is guaranteed for signed types in C++20.
		// Before that, it is implementation defined (but usually arithmetic).
		value >>= 7;

		bool signBit = byte & 0x40; // The sign bit of the *byte*, not the original value

		if ((value == 0 && !signBit) || (value == -1 && signBit)) {
			more = false;
		} else {
			byte |= 0x80;
		}
		dev.write(&byte, 1, 1);
	}
}

// ----------------------------------------------------------------------------
// Wrappers (Hold References!)
// ----------------------------------------------------------------------------

// Wrapper for Unsigned LEB128
template <typename T>
struct ULEB128 {
	T& ref;
	explicit ULEB128(T& v) : ref(v) {}
};

// Wrapper for Signed LEB128
template <typename T>
struct SLEB128 {
	T& ref;
	explicit SLEB128(T& v) : ref(v) {}
};

// ----------------------------------------------------------------------------
// Helper Functions (For syntax: stream >> Leb(var))
// ----------------------------------------------------------------------------

template <typename T> auto Leb(T& val) {
	if constexpr (std::is_signed_v<T>) return SLEB128<T>(val);
	else return ULEB128<T>(val);
}

/**
 * @brief Template struct for handling data streaming with specified endianness.
 *
 * This struct provides functionality to read and write data to a device with a specified endianness.
 * It supports various data types and standard library containers, automatically handling endianness conversion where necessary.
 *
 * @tparam io_endianness The endianness to use for data streaming. Defaults to Util::Endian::Big.
 */
template <Util::Endian io_endianness = Util::Endian::Big>
struct DataStream {
	/// @brief Reference to the device used for reading and writing data.
	Device& device;

	/**
	 * @brief Constructor for DataStream.
	 *
	 * Initializes the DataStream with a reference to a device.
	 *
	 * @param ndevice Reference to the device to be used for data streaming.
	 */
	DataStream(Device& ndevice) : device(ndevice) {
	}

	/**
	 * @brief Operator to write a nullptr_t to the stream.
	 *
	 * This operator does nothing since nullptr_t has no data.
	 *
	 * @param ptr nullptr_t to be written (ignored).
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator<<(std::nullptr_t ptr) {
		(void)ptr;
		return *this;
	}

	/**
	 * @brief Operator to read a nullptr_t from the stream.
	 *
	 * This operator does nothing since nullptr_t has no data.
	 *
	 * @param ptr nullptr_t to be read (ignored).
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator>>(std::nullptr_t& ptr) {
		(void)ptr;
		return *this;
	}

	/**
	 * @brief Operator to write a std::nullopt_t to the stream.
	 *
	 * This operator does nothing since std::nullopt_t has no data.
	 *
	 * @param ptr std::nullopt_t to be written (ignored).
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator<<(std::nullopt_t ptr) {
		(void)ptr;
		return *this;
	}

	/**
	 * @brief Operator to read a std::nullopt_t from the stream.
	 *
	 * This operator does nothing since std::nullopt_t has no data.
	 *
	 * @param ptr std::nullopt_t to be read (ignored).
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator>>(std::nullopt_t& ptr) {
		(void)ptr;
		return *this;
	}

#ifdef __UINT8_TYPE__
	/**
	 * @brief Operator to write a uint8_t to the stream.
	 *
	 * Writes a single uint8_t value to the device.
	 *
	 * @param data uint8_t value to be written.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator<<(uint8_t data) {
		device.write(&data, sizeof(uint8_t), 1);
		return *this;
	}

	/**
	 * @brief Operator to read a uint8_t from the stream.
	 *
	 * Reads a single uint8_t value from the device.
	 *
	 * @param data uint8_t value to be read.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator>>(uint8_t& data) {
		device.read(&data, sizeof(uint8_t), 1);
		return *this;
	}
#endif

#ifdef __INT8_TYPE__
	/**
	 * @brief Operator to write an int8_t to the stream.
	 *
	 * Writes a single int8_t value to the device.
	 *
	 * @param data int8_t value to be written.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator<<(int8_t data) {
		device.write(&data, sizeof(int8_t), 1);
		return *this;
	}

	/**
	 * @brief Operator to read an int8_t from the stream.
	 *
	 * Reads a single int8_t value from the device.
	 *
	 * @param data int8_t value to be read.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator>>(int8_t& data) {
		device.read(&data, sizeof(int8_t), 1);
		return *this;
	}
#endif

	/**
	 * @brief Operator to write a bool to the stream.
	 *
	 * Writes a single bool value to the device.
	 *
	 * @param data bool value to be written.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator<<(bool data) {
		device.write(&data, sizeof(bool), 1);
		return *this;
	}

	/**
	 * @brief Operator to read a bool from the stream.
	 *
	 * Reads a single bool value from the device.
	 *
	 * @param data bool value to be read.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator>>(bool& data) {
		device.read(&data, sizeof(bool), 1);
		return *this;
	}

#ifdef __UINT16_TYPE__
	/**
	 * @brief Operator to write a uint16_t to the stream.
	 *
	 * Writes a single uint16_t value to the device, converting endianness if necessary.
	 *
	 * @param data uint16_t value to be written.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator<<(uint16_t data) {
		Util::convert_endian<Util::Endian::Native, io_endianness>(data);
		device.write(&data, sizeof(uint16_t), 1);
		return *this;
	}

	/**
	 * @brief Operator to read a uint16_t from the stream.
	 *
	 * Reads a single uint16_t value from the device, converting endianness if necessary.
	 *
	 * @param data uint16_t value to be read.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator>>(uint16_t& data) {
		device.read(&data, sizeof(uint16_t), 1);
		Util::convert_endian<io_endianness, Util::Endian::Native>(data);
		return *this;
	}
#endif

#ifdef __INT16_TYPE__
	/**
	 * @brief Operator to write an int16_t to the stream.
	 *
	 * Writes a single int16_t value to the device, converting endianness if necessary.
	 *
	 * @param data int16_t value to be written.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator<<(int16_t data) {
		Util::convert_endian<Util::Endian::Native, io_endianness>(data);
		device.write(&data, sizeof(int16_t), 1);
		return *this;
	}

	/**
	 * @brief Operator to read an int16_t from the stream.
	 *
	 * Reads a single int16_t value from the device, converting endianness if necessary.
	 *
	 * @param data int16_t value to be read.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator>>(int16_t& data) {
		device.read(&data, sizeof(int16_t), 1);
		Util::convert_endian<io_endianness, Util::Endian::Native>(data);
		return *this;
	}
#endif

	/**
	 * @brief Operator to write a half_float::half to the stream.
	 *
	 * Writes a single half_float::half value to the device, converting endianness if necessary.
	 *
	 * @param data half_float::half value to be written.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator<<(half_float::half data) {
		Util::convert_endian<Util::Endian::Native, io_endianness>(data);
		device.write(&data, sizeof(half_float::half), 1);
		return *this;
	}

	/**
	 * @brief Operator to read a half_float::half from the stream.
	 *
	 * Reads a single half_float::half value from the device, converting endianness if necessary.
	 *
	 * @param data half_float::half value to be read.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator>>(half_float::half& data) {
		device.read(&data, sizeof(half_float::half), 1);
		Util::convert_endian<io_endianness, Util::Endian::Native>(data);
		return *this;
	}

#ifdef __UINT24_TYPE__
	/**
	 * @brief Operator to write a uint24_t to the stream.
	 *
	 * Writes a single uint24_t value to the device, converting endianness if necessary.
	 *
	 * @param data uint24_t value to be written.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator<<(uint24_t data) {
		Util::convert_endian<Util::Endian::Native, io_endianness>(data);
		device.write(&data, sizeof(uint24_t), 1);
		return *this;
	}

	/**
	 * @brief Operator to read a uint24_t from the stream.
	 *
	 * Reads a single uint24_t value from the device, converting endianness if necessary.
	 *
	 * @param data uint24_t value to be read.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator>>(uint24_t& data) {
		device.read(&data, sizeof(uint24_t), 1);
		Util::convert_endian<io_endianness, Util::Endian::Native>(data);
		return *this;
	}
#endif

#ifdef __INT24_TYPE__
	/**
	 * @brief Operator to write an int24_t to the stream.
	 *
	 * Writes a single int24_t value to the device, converting endianness if necessary.
	 *
	 * @param data int24_t value to be written.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator<<(int24_t data) {
		Util::convert_endian<Util::Endian::Native, io_endianness>(data);
		device.write(&data, sizeof(int24_t), 1);
		return *this;
	}

	/**
	 * @brief Operator to read an int24_t from the stream.
	 *
	 * Reads a single int24_t value from the device, converting endianness if necessary.
	 *
	 * @param data int24_t value to be read.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator>>(int24_t& data) {
		device.read(&data, sizeof(int24_t), 1);
		Util::convert_endian<io_endianness, Util::Endian::Native>(data);
		return *this;
	}
#endif

#ifdef __UINT32_TYPE__
	/**
	 * @brief Operator to write a uint32_t to the stream.
	 *
	 * Writes a single uint32_t value to the device, converting endianness if necessary.
	 *
	 * @param data uint32_t value to be written.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator<<(uint32_t data) {
		Util::convert_endian<Util::Endian::Native, io_endianness>(data);
		device.write(&data, sizeof(uint32_t), 1);
		return *this;
	}

	/**
	 * @brief Operator to read a uint32_t from the stream.
	 *
	 * Reads a single uint32_t value from the device, converting endianness if necessary.
	 *
	 * @param data uint32_t value to be read.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator>>(uint32_t& data) {
		device.read(&data, sizeof(uint32_t), 1);
		Util::convert_endian<io_endianness, Util::Endian::Native>(data);
		return *this;
	}
#endif

#ifdef __INT32_TYPE__
	/**
	 * @brief Operator to write an int32_t to the stream.
	 *
	 * Writes a single int32_t value to the device, converting endianness if necessary.
	 *
	 * @param data int32_t value to be written.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator<<(int32_t data) {
		Util::convert_endian<Util::Endian::Native, io_endianness>(data);
		device.write(&data, sizeof(int32_t), 1);
		return *this;
	}

	/**
	 * @brief Operator to read an int32_t from the stream.
	 *
	 * Reads a single int32_t value from the device, converting endianness if necessary.
	 *
	 * @param data int32_t value to be read.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator>>(int32_t& data) {
		device.read(&data, sizeof(int32_t), 1);
		Util::convert_endian<io_endianness, Util::Endian::Native>(data);
		return *this;
	}
#endif

	/**
	 * @brief Operator to write a float to the stream.
	 *
	 * Writes a single float value to the device, converting endianness if necessary.
	 *
	 * @param data float value to be written.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator<<(float data) {
		Util::convert_endian<Util::Endian::Native, io_endianness>(data);
		device.write(&data, sizeof(float), 1);
		return *this;
	}

	/**
	 * @brief Operator to read a float from the stream.
	 *
	 * Reads a single float value from the device, converting endianness if necessary.
	 *
	 * @param data float value to be read.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator>>(float& data) {
		device.read(&data, sizeof(float), 1);
		Util::convert_endian<io_endianness, Util::Endian::Native>(data);
		return *this;
	}

#ifdef __UINT48_TYPE__
	/**
	 * @brief Operator to write a uint48_t to the stream.
	 *
	 * Writes a single uint48_t value to the device, converting endianness if necessary.
	 *
	 * @param data uint48_t value to be written.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator<<(uint48_t data) {
		Util::convert_endian<Util::Endian::Native, io_endianness>(data);
		device.write(&data, sizeof(uint48_t), 1);
		return *this;
	}

	/**
	 * @brief Operator to read a uint48_t from the stream.
	 *
	 * Reads a single uint48_t value from the device, converting endianness if necessary.
	 *
	 * @param data uint48_t value to be read.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator>>(uint48_t& data) {
		device.read(&data, sizeof(uint48_t), 1);
		Util::convert_endian<io_endianness, Util::Endian::Native>(data);
		return *this;
	}
#endif

#ifdef __INT48_TYPE__
	/**
	 * @brief Operator to write an int48_t to the stream.
	 *
	 * Writes a single int48_t value to the device, converting endianness if necessary.
	 *
	 * @param data int48_t value to be written.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator<<(int48_t data) {
		Util::convert_endian<Util::Endian::Native, io_endianness>(data);
		device.write(&data, sizeof(int48_t), 1);
		return *this;
	}

	/**
	 * @brief Operator to read an int48_t from the stream.
	 *
	 * Reads a single int48_t value from the device, converting endianness if necessary.
	 *
	 * @param data int48_t value to be read.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator>>(int48_t& data) {
		device.read(&data, sizeof(int48_t), 1);
		Util::convert_endian<io_endianness, Util::Endian::Native>(data);
		return *this;
	}
#endif

#ifdef __UINT64_TYPE__
	/**
	 * @brief Operator to write a uint64_t to the stream.
	 *
	 * Writes a single uint64_t value to the device, converting endianness if necessary.
	 *
	 * @param data uint64_t value to be written.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator<<(uint64_t data) {
		Util::convert_endian<Util::Endian::Native, io_endianness>(data);
		device.write(&data, sizeof(uint64_t), 1);
		return *this;
	}

	/**
	 * @brief Operator to read a uint64_t from the stream.
	 *
	 * Reads a single uint64_t value from the device, converting endianness if necessary.
	 *
	 * @param data uint64_t value to be read.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator>>(uint64_t& data) {
		device.read(&data, sizeof(uint64_t), 1);
		Util::convert_endian<io_endianness, Util::Endian::Native>(data);
		return *this;
	}
#endif

#ifdef __INT64_TYPE__
	/**
	 * @brief Operator to write an int64_t to the stream.
	 *
	 * Writes a single int64_t value to the device, converting endianness if necessary.
	 *
	 * @param data int64_t value to be written.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator<<(int64_t data) {
		Util::convert_endian<Util::Endian::Native, io_endianness>(data);
		device.write(&data, sizeof(int64_t), 1);
		return *this;
	}

	/**
	 * @brief Operator to read an int64_t from the stream.
	 *
	 * Reads a single int64_t value from the device, converting endianness if necessary.
	 *
	 * @param data int64_t value to be read.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator>>(int64_t& data) {
		device.read(&data, sizeof(int64_t), 1);
		Util::convert_endian<io_endianness, Util::Endian::Native>(data);
		return *this;
	}
#endif

	/**
	 * @brief Operator to write a double to the stream.
	 *
	 * Writes a single double value to the device, converting endianness if necessary.
	 *
	 * @param data double value to be written.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator<<(double data) {
		Util::convert_endian<Util::Endian::Native, io_endianness>(data);
		device.write(&data, sizeof(double), 1);
		return *this;
	}

	/**
	 * @brief Operator to read a double from the stream.
	 *
	 * Reads a single double value from the device, converting endianness if necessary.
	 *
	 * @param data double value to be read.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator>>(double& data) {
		device.read(&data, sizeof(double), 1);
		Util::convert_endian<io_endianness, Util::Endian::Native>(data);
		return *this;
	}

	/**
	 * @brief Operator to write a char to the stream.
	 *
	 * Writes a single char value to the device.
	 *
	 * @param data char value to be written.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator<<(char data) {
		device.write(&data, sizeof(char), 1);
		return *this;
	}

	/**
	 * @brief Operator to read a char from the stream.
	 *
	 * Reads a single char value from the device.
	 *
	 * @param data char value to be read.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator>>(char& data) {
		device.read(&data, sizeof(char), 1);
		return *this;
	}

	/**
	 * @brief Operator to write a char8_t to the stream.
	 *
	 * Writes a single char8_t value to the device.
	 *
	 * @param data char8_t value to be written.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator<<(char8_t data) {
		device.write(&data, sizeof(char8_t), 1);
		return *this;
	}

	/**
	 * @brief Operator to read a char8_t from the stream.
	 *
	 * Reads a single char8_t value from the device.
	 *
	 * @param data char8_t value to be read.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator>>(char8_t& data) {
		device.read(&data, sizeof(char8_t), 1);
		return *this;
	}

	/**
	 * @brief Operator to write a char16_t to the stream.
	 *
	 * Writes a single char16_t value to the device, converting endianness if necessary.
	 *
	 * @param data char16_t value to be written.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator<<(char16_t data) {
		Util::convert_endian<Util::Endian::Native, io_endianness>(data);
		device.write(&data, sizeof(char16_t), 1);
		return *this;
	}

	/**
	 * @brief Operator to read a char16_t from the stream.
	 *
	 * Reads a single char16_t value from the device, converting endianness if necessary.
	 *
	 * @param data char16_t value to be read.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator>>(char16_t& data) {
		device.read(&data, sizeof(char16_t), 1);
		Util::convert_endian<io_endianness, Util::Endian::Native>(data);
		return *this;
	}

	/**
	 * @brief Operator to write a char32_t to the stream.
	 *
	 * Writes a single char32_t value to the device, converting endianness if necessary.
	 *
	 * @param data char32_t value to be written.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator<<(char32_t data) {
		Util::convert_endian<Util::Endian::Native, io_endianness>(data);
		device.write(&data, sizeof(char32_t), 1);
		return *this;
	}

	/**
	 * @brief Operator to read a char32_t from the stream.
	 *
	 * Reads a single char32_t value from the device, converting endianness if necessary.
	 *
	 * @param data char32_t value to be read.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator>>(char32_t& data) {
		device.read(&data, sizeof(char32_t), 1);
		Util::convert_endian<io_endianness, Util::Endian::Native>(data);
		return *this;
	}

#ifdef __GNUC__
	/**
	 * @brief Operator to write an unsigned __int128 to the stream.
	 *
	 * Writes a single unsigned __int128 value to the device, converting endianness if necessary.
	 *
	 * @param data unsigned __int128 value to be written.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator<<(unsigned __int128 data) {
		Util::convert_endian<Util::Endian::Native, io_endianness>(data);
		device.write(&data, sizeof(unsigned __int128), 1);
		return *this;
	}

	/**
	 * @brief Operator to read an unsigned __int128 from the stream.
	 *
	 * Reads a single unsigned __int128 value from the device, converting endianness if necessary.
	 *
	 * @param data unsigned __int128 value to be read.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator>>(unsigned __int128& data) {
		device.read(&data, sizeof(unsigned __int128), 1);
		Util::convert_endian<io_endianness, Util::Endian::Native>(data);
		return *this;
	}

	/**
	 * @brief Operator to write a __int128 to the stream.
	 *
	 * Writes a single __int128 value to the device, converting endianness if necessary.
	 *
	 * @param data __int128 value to be written.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator<<(__int128 data) {
		Util::convert_endian<Util::Endian::Native, io_endianness>(data);
		device.write(&data, sizeof(__int128), 1);
		return *this;
	}

	/**
	 * @brief Operator to read a __int128 from the stream.
	 *
	 * Reads a single __int128 value from the device, converting endianness if necessary.
	 *
	 * @param data __int128 value to be read.
	 * @return Reference to the current DataStream instance.
	 */
	inline DataStream& operator>>(__int128& data) {
		device.read(&data, sizeof(__int128), 1);
		Util::convert_endian<io_endianness, Util::Endian::Native>(data);
		return *this;
	}
#endif

	/**
	 * @brief Operator to write a std::basic_string_view to the stream.
	 *
	 * Writes the size and contents of a std::basic_string_view to the device.
	 *
	 * @tparam CharT Character type of the string.
	 * @tparam Traits Traits class of the string.
	 * @param data std::basic_string_view to be written.
	 * @return Reference to the current DataStream instance.
	 */
		template< class CharT, class Traits = std::char_traits<CharT>>
		inline DataStream& operator<<(const std::basic_string_view<CharT, Traits>& data) {
		*this << static_cast<uint32_t>(data.size());
		if constexpr(sizeof(CharT) == sizeof(std::byte)) {
			device.write(data.data(), 1, data.size());
			return *this;
		} else {
			return writeElements<CharT>(data.begin(), data.end(), false);
		}
	}

	/**
	 * @brief Operator to write a std::basic_string to the stream.
	 *
	 * Writes the size and contents of a std::basic_string to the device.
	 *
	 * @tparam CharT Character type of the string.
	 * @tparam Traits Traits class of the string.
	 * @tparam Allocator Allocator type of the string.
	 * @param data std::basic_string to be written.
	 * @return Reference to the current DataStream instance.
	 */
	template< class CharT, class Traits = std::char_traits<CharT>, class Allocator = std::allocator<CharT>>
	inline DataStream& operator<<(const std::basic_string<CharT, Traits, Allocator>& data) {
		*this << static_cast<uint32_t>(data.size());
		if constexpr(sizeof(CharT) == sizeof(std::byte)) {
			device.write(data.data(), 1, data.size());
			return *this;
		} else {
			return writeElements<CharT>(data.begin(), data.end(), false);
		}
	}

	/**
	 * @brief Operator to read a std::basic_string from the stream.
	 *
	 * Reads the size and contents of a std::basic_string from the device.
	 *
	 * @tparam CharT Character type of the string.
	 * @tparam Traits Traits class of the string.
	 * @tparam Allocator Allocator type of the string.
	 * @param data std::basic_string to be read.
	 * @return Reference to the current DataStream instance.
	 */
	template< class CharT, class Traits = std::char_traits<CharT>, class Allocator = std::allocator<CharT>>
	inline DataStream& operator>>(std::basic_string<CharT, Traits, Allocator>& data) {
		uint32_t size;
		*this >> size;
		data.resize(size);
		if constexpr(sizeof(CharT) == sizeof(std::byte)) {
			device.read(data.data(), 1, data.size());
			return *this;
		} else {
			return readElementsInto<CharT>(data.begin(), data.end());
		}
	}

	/**
	 * @brief Operator to write a std::optional to the stream.
	 *
	 * Writes the presence and value (if any) of a std::optional to the device.
	 *
	 * @tparam T Type of the value contained in the std::optional.
	 * @param opt std::optional to be written.
	 * @return Reference to the current DataStream instance.
	 */
	template <typename T>
	inline DataStream& operator<<(const std::optional<T>& opt) {
		bool hasValue = opt.has_value();
		*this << hasValue;
		if(hasValue) {
			*this << opt.value();
		}
		return *this;
	}

	/**
	 * @brief Operator to read a std::optional from the stream.
	 *
	 * Reads the presence and value (if any) of a std::optional from the device.
	 *
	 * @tparam T Type of the value contained in the std::optional.
	 * @param opt std::optional to be read.
	 * @return Reference to the current DataStream instance.
	 */
	template <typename T>
	inline DataStream& operator>>(std::optional<T>& opt) {
		bool hasValue;
		*this >> hasValue;
		if(hasValue) {
			T value;
			*this >> value;
			opt.emplace(std::move(value));
		} else {
			opt = std::nullopt;
		}
		return *this;
	}

	/**
	 * @brief Operator to write a std::pair to the stream.
	 *
	 * Writes both elements of a std::pair to the device.
	 *
	 * @tparam T1 Type of the first element in the pair.
	 * @tparam T2 Type of the second element in the pair.
	 * @param pair std::pair to be written.
	 * @return Reference to the current DataStream instance.
	 */
	template <typename T1, typename T2>
	inline DataStream& operator<<(const std::pair<T1, T2>& pair) {
		return *this << pair.first << pair.second;
	}

	/**
	 * @brief Operator to read a std::pair from the stream.
	 *
	 * Reads both elements of a std::pair from the device.
	 *
	 * @tparam T1 Type of the first element in the pair.
	 * @tparam T2 Type of the second element in the pair.
	 * @param pair std::pair to be read.
	 * @return Reference to the current DataStream instance.
	 */
	template <typename T1, typename T2>
	inline DataStream& operator>>(std::pair<T1, T2>& pair) {
		return *this >> pair.first >> pair.second;
	}

	/**
	 * @brief Writes a range of elements to the stream.
	 *
	 * Writes the size and contents of a range of elements to the device.
	 *
	 * @tparam Element Type of the elements.
	 * @tparam Iterator Iterator type.
	 * @param first Beginning iterator of the range.
	 * @param last End iterator of the range.
	 * @param writeSize Whether to write the size of the range.
	 * @return Reference to the current DataStream instance.
	 */
	template <typename Element, typename Iterator>
	DataStream& writeElements(Iterator first, Iterator last, bool writeSize = true) {
		if(writeSize) {
			uint32_t elements = std::distance(first, last);
			*this << elements;
		}
		std::for_each(first, last, [this](const Element& iter) { *this << iter; });
		return *this;
	}

	/**
	 * @brief Reads a range of elements from the stream.
	 *
	 * Reads the contents of a range of elements from the device.
	 *
	 * @tparam Element Type of the elements.
	 * @tparam Iterator Iterator type.
	 * @param first Beginning iterator of the range.
	 * @param last End iterator of the range.
	 * @return Reference to the current DataStream instance.
	 */
	template <typename Element, typename Iterator>
	DataStream& readElementsInto(Iterator first, Iterator last) {
		std::for_each(first, last, [this](Element& iter) { *this >> iter; });
		return *this;
	}

	/**
	 * @brief Operator to write a container to the stream.
	 *
	 * Writes the size and contents of a container to the device.
	 *
	 * @tparam Container Type of the container.
	 * @param data Container to be written.
	 * @return Reference to the current DataStream instance.
	 */
	template <typename Container>
		requires Util::VectorLike<Container>
	inline DataStream& operator<<(const Container& data) {
		*this << static_cast<uint32_t>(data.size());
		if constexpr(sizeof(typename Container::value_type) == sizeof(std::byte)) {
			device.write(data.data(), 1, data.size());
			return *this;
		} else {
			return writeElements<typename Container::value_type>(data.begin(), data.end(), false);
		}
	}

	/**
	 * @brief Operator to read a container from the stream.
	 *
	 * Reads the size and contents of a container from the device.
	 *
	 * @tparam Container Type of the container.
	 * @param data Container to be read.
	 * @return Reference to the current DataStream instance.
	 */
	template <typename Container>
		requires Util::VectorLike<Container>
	inline DataStream& operator>>(Container& data) {
		uint32_t size;
		*this >> size;
		data.resize(size);
		if constexpr(sizeof(typename Container::value_type) == sizeof(std::byte)) {
			device.read(data.data(), 1, data.size());
			return *this;
		} else {
			return readElementsInto<typename Container::value_type>(data.begin(), data.end());
		}
	}

	/**
	 * @brief Operator to write a map-like container to the stream.
	 *
	 * Writes the size and contents of a map-like container to the device.
	 *
	 * @tparam Container Type of the container.
	 * @param data Container to be written.
	 * @return Reference to the current DataStream instance.
	 */
	template <typename Container>
		requires Util::MapLike<Container>
	inline DataStream& operator<<(const Container& data) {
		*this << static_cast<uint32_t>(data.size());
		return writeElements<typename Container::value_type>(data.begin(), data.end(), false);
	}

	/**
	 * @brief Operator to read a map-like container from the stream.
	 *
	 * Reads the size and contents of a map-like container from the device.
	 *
	 * @tparam Container Type of the container.
	 * @param data Container to be read.
	 * @return Reference to the current DataStream instance.
	 */
	template <typename Container>
		requires Util::MapLike<Container>
	inline DataStream& operator>>(Container& data) {
		uint32_t size;
		*this >> size;
		for(uint32_t i = 0; i < size; ++i) {
			typename Container::key_type tmpKey;
			typename Container::mapped_type tmpVal;
			*this >> tmpKey >> tmpVal;
			data.emplace(std::move(tmpKey), std::move(tmpVal));
		}
		return *this;
	}

	/**
	 * @brief Operator to write a sequential container to the stream.
	 *
	 * Writes the size and contents of a sequential container to the device.
	 *
	 * @tparam Container Type of the container.
	 * @param data Container to be written.
	 * @return Reference to the current DataStream instance.
	 */
	template <typename Container>
		requires Util::SequentialContainer<Container>
	inline DataStream& operator<<(const Container& data) {
		*this << static_cast<uint32_t>(data.size());
		return writeElements<Container::value_type>(data.begin(), data.end(), false);
	}

	/**
	 * @brief Operator to read a sequential container from the stream.
	 *
	 * Reads the size and contents of a sequential container from the device.
	 *
	 * @tparam Container Type of the container.
	 * @param data Container to be read.
	 * @return Reference to the current DataStream instance.
	 */
	template <typename Container>
		requires Util::SequentialContainer<Container>
	inline DataStream& operator>>(Container& data) {
		uint32_t size;
		*this >> size;
		for(uint32_t i = 0; i < size; ++i) {
			typename Container::value_type tmpData;
			*this >> tmpData;
			data.insert(std::move(tmpData));
		}
		return *this;
	}

	/**
	 * @brief Operator to write a std::span to the stream.
	 *
	 * Writes the contents of a std::span to the device.
	 *
	 * @tparam T Type of the elements in the span.
	 * @param data std::span to be written.
	 * @return Reference to the current DataStream instance.
	 */
	template <typename T>
	inline DataStream& operator<<(const std::span<const T>& data) {
		if constexpr(sizeof(T) == sizeof(std::byte)) {
			device.write(data.data(), 1, data.size());
		} else {
			return writeElements<T>(data.begin(), data.end(), false);
		}
		return *this;
	}

	/**
	 * @brief Operator to read a std::span from the stream.
	 *
	 * Reads the contents of a std::span from the device.
	 *
	 * @tparam T Type of the elements in the span.
	 * @param data std::span to be read.
	 * @return Reference to the current DataStream instance.
	 */
	template <typename T>
	inline DataStream& operator>>(std::span<T>& data) {
		if constexpr(sizeof(T) == sizeof(std::byte)) {
			device.read(data.data(), 1, data.size());
		} else {
			readElementsInto<T>(data.begin(), data.end());
		}
		return *this;
	}

	/**
	 * @brief Operator to write a std::array to the stream.
	 *
	 * Writes the contents of a std::array to the device.
	 *
	 * @tparam T Type of the elements in the array.
	 * @tparam N Size of the array.
	 * @param data std::array to be written.
	 * @return Reference to the current DataStream instance.
	 */
	template <typename T, size_t N>
	inline DataStream& operator<<(const std::array<T, N>& data) {
		if constexpr(sizeof(T) == sizeof(std::byte)) {
			device.write(data.data(), 1, data.size());
		} else {
			return writeElements<T>(data.begin(), data.end(), false);
		}
		return *this;
	}

	/**
	 * @brief Operator to read a std::array from the stream.
	 *
	 * Reads the contents of a std::array from the device.
	 *
	 * @tparam T Type of the elements in the array.
	 * @tparam N Size of the array.
	 * @param data std::array to be read.
	 * @return Reference to the current DataStream instance.
	 */
	template <typename T, size_t N> inline DataStream& operator>>(std::array<T,N>& data) {
		if constexpr(sizeof(T) == sizeof(std::byte)) {
			device.read(data.data (), 1, data.size () );
			return *this;
		} else readElementsInto<T>(data.begin(), data.end());
		return *this;
	}


	// ----------------------------------------------------------------------------
	// Stream Operators
	// ----------------------------------------------------------------------------

	// Read ULEB
	template <typename T> DataStream& operator>>(ULEB128<T> wrapper) {
		wrapper.ref = readULEB128<T>(device);
		return *this;
	}

	// Write ULEB
	template <typename T> DataStream& operator<<(ULEB128<T> wrapper) {
		writeULEB128<T>(device, wrapper.ref);
		return *this;
	}

	// Read SLEB
	template <typename T> DataStream& operator>>(SLEB128<T> wrapper) {
		wrapper.ref = readSLEB128<T>(device);
		return *this;
	}

	// Write SLEB
	template <typename T> DataStream& operator<<(SLEB128<T> wrapper) {
		writeSLEB128<T>(device, wrapper.ref);
		return *this;
	}

	template <typename T> inline T read() {
		 T data;
		 *this >> data;
		 return data;
	};

	template <typename T> inline T readLEB128() {
		 T data;
		 *this >> Leb(data);
		 return data;
	};
};


}
}
#endif // ELVDATASTREAM_HPP
