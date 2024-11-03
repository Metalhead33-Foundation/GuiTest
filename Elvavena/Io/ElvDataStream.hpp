#ifndef ELVDATASTREAM_HPP
#define ELVDATASTREAM_HPP
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <Elvavena/Util/ElvContainerBasic.hpp>
#include <Elvavena/Util/ElvEndianness.hpp>
#include <optional>
namespace Elv {
namespace Io {

template <Util::Endian io_endianness = Util::Endian::Big> struct DataStream {
	Device& device;
	DataStream(Device& ndevice) : device(ndevice) {

	}
	// Primitives
	inline DataStream& operator<<(std::nullopt_t ptr) {
		(void)ptr;
		return *this;
	}
	inline DataStream& operator>>(std::nullopt_t& ptr) {
		(void)ptr;
		return *this;
	}
#ifdef __UINT8_TYPE__
	inline DataStream& operator<<(uint8_t data) {
		device.write(&data,sizeof(uint8_t),1);
		return *this;
	}
	inline DataStream& operator>>(uint8_t& data) {
		device.read(&data,sizeof(uint8_t),1);
		return *this;
	}
#endif
#ifdef __INT8_TYPE__
	inline DataStream& operator<<(int8_t data) {
		device.write(&data,sizeof(int8_t),1);
		return *this;
	}
	inline DataStream& operator>>(int8_t& data) {
		device.read(&data,sizeof(int8_t),1);
		return *this;
	}
#endif
	inline DataStream& operator<<(bool data) {
		device.write(&data,sizeof(bool),1);
		return *this;
	}
	inline DataStream& operator>>(bool& data) {
		device.read(&data,sizeof(bool),1);
		return *this;
	}
#ifdef __UINT16_TYPE__
	inline DataStream& operator<<(uint16_t data) {
		Util::convert_endian<Util::Endian::Native,io_endianness>(data);
		device.write(&data,sizeof(uint16_t),1);
		return *this;
	}
	inline DataStream& operator>>(uint16_t& data) {
		device.read(&data,sizeof(uint16_t),1);
		Util::convert_endian<io_endianness,Util::Endian::Native>(data);
		return *this;
	}
#endif
#ifdef __INT16_TYPE__
	inline DataStream& operator<<(int16_t data) {
		Util::convert_endian<Util::Endian::Native,io_endianness>(data);
		device.write(&data,sizeof(int16_t),1);
		return *this;
	}
	inline DataStream& operator>>(int16_t& data) {
		device.read(&data,sizeof(int16_t),1);
		Util::convert_endian<io_endianness,Util::Endian::Native>(data);
		return *this;
	}
#endif
	inline DataStream& operator<<(half_float::half data) {
		Util::convert_endian<Util::Endian::Native,io_endianness>(data);
		device.write(&data,sizeof(half_float::half),1);
		return *this;
	}
	inline DataStream& operator>>(half_float::half& data) {
		device.read(&data,sizeof(half_float::half),1);
		Util::convert_endian<io_endianness,Util::Endian::Native>(data);
		return *this;
	}
#ifdef __UINT24_TYPE__
	inline DataStream& operator<<(uint24_t data) {
		Util::convert_endian<Util::Endian::Native,io_endianness>(data);
		device.write(&data,sizeof(uint24_t),1);
		return *this;
	}
	inline DataStream& operator>>(uint24_t& data) {
		device.read(&data,sizeof(uint24_t),1);
		Util::convert_endian<io_endianness,Util::Endian::Native>(data);
		return *this;
	}
#endif
#ifdef __INT24_TYPE__
	inline DataStream& operator<<(int24_t data) {
		Util::convert_endian<Util::Endian::Native,io_endianness>(data);
		device.write(&data,sizeof(int24_t),1);
		return *this;
	}
	inline DataStream& operator>>(int24_t& data) {
		device.read(&data,sizeof(int24_t),1);
		Util::convert_endian<io_endianness,Util::Endian::Native>(data);
		return *this;
	}
#endif
#ifdef __UINT32_TYPE__
	inline DataStream& operator<<(uint32_t data) {
		Util::convert_endian<Util::Endian::Native,io_endianness>(data);
		device.write(&data,sizeof(uint32_t),1);
		return *this;
	}
	inline DataStream& operator>>(uint32_t& data) {
		device.read(&data,sizeof(uint32_t),1);
		Util::convert_endian<io_endianness,Util::Endian::Native>(data);
		return *this;
	}
#endif
#ifdef __INT32_TYPE__
	inline DataStream& operator<<(int32_t data) {
		Util::convert_endian<Util::Endian::Native,io_endianness>(data);
		device.write(&data,sizeof(int32_t),1);
		return *this;
	}
	inline DataStream& operator>>(int32_t& data) {
		device.read(&data,sizeof(int32_t),1);
		Util::convert_endian<io_endianness,Util::Endian::Native>(data);
		return *this;
	}
#endif
	inline DataStream& operator<<(float data) {
		Util::convert_endian<Util::Endian::Native,io_endianness>(data);
		device.write(&data,sizeof(float),1);
		return *this;
	}
	inline DataStream& operator>>(float& data) {
		device.read(&data,sizeof(float),1);
		Util::convert_endian<io_endianness,Util::Endian::Native>(data);
		return *this;
	}
#ifdef __UINT48_TYPE__
	inline DataStream& operator<<(uint48_t data) {
		Util::convert_endian<Util::Endian::Native,io_endianness>(data);
		device.write(&data,sizeof(uint48_t),1);
		return *this;
	}
	inline DataStream& operator>>(uint48_t& data) {
		device.read(&data,sizeof(uint48_t),1);
		Util::convert_endian<io_endianness,Util::Endian::Native>(data);
		return *this;
	}
#endif
#ifdef __INT48_TYPE__
	inline DataStream& operator<<(int48_t data) {
		Util::convert_endian<Util::Endian::Native,io_endianness>(data);
		device.write(&data,sizeof(int48_t),1);
		return *this;
	}
	inline DataStream& operator>>(int48_t& data) {
		device.read(&data,sizeof(int48_t),1);
		Util::convert_endian<io_endianness,Util::Endian::Native>(data);
		return *this;
	}
#endif
#ifdef __UINT64_TYPE__
	inline DataStream& operator<<(uint64_t data) {
		Util::convert_endian<Util::Endian::Native,io_endianness>(data);
		device.write(&data,sizeof(uint64_t),1);
		return *this;
	}
	inline DataStream& operator>>(uint64_t& data) {
		device.read(&data,sizeof(uint64_t),1);
		Util::convert_endian<io_endianness,Util::Endian::Native>(data);
		return *this;
	}
#endif
#ifdef __INT64_TYPE__
	inline DataStream& operator<<(int64_t data) {
		Util::convert_endian<Util::Endian::Native,io_endianness>(data);
		device.write(&data,sizeof(int64_t),1);
		return *this;
	}
	inline DataStream& operator>>(int64_t& data) {
		device.read(&data,sizeof(int64_t),1);
		Util::convert_endian<io_endianness,Util::Endian::Native>(data);
		return *this;
	}
#endif
	inline DataStream& operator<<(double data) {
		Util::convert_endian<Util::Endian::Native,io_endianness>(data);
		device.write(&data,sizeof(double),1);
		return *this;
	}
	inline DataStream& operator>>(double& data) {
		device.read(&data,sizeof(double),1);
		Util::convert_endian<io_endianness,Util::Endian::Native>(data);
		return *this;
	}
	inline DataStream& operator<<(char data) {
		device.write(&data,sizeof(char),1);
		return *this;
	}
	inline DataStream& operator>>(char& data) {
		device.read(&data,sizeof(char),1);
		return *this;
	}
	inline DataStream& operator<<(char8_t data) {
		device.write(&data,sizeof(char8_t),1);
		return *this;
	}
	inline DataStream& operator>>(char8_t& data) {
		device.read(&data,sizeof(char8_t),1);
		return *this;
	}
	inline DataStream& operator<<(char16_t data) {
		Util::convert_endian<Util::Endian::Native,io_endianness>(data);
		device.write(&data,sizeof(char16_t),1);
		return *this;
	}
	inline DataStream& operator>>(char16_t& data) {
		device.read(&data,sizeof(char16_t),1);
		Util::convert_endian<io_endianness,Util::Endian::Native>(data);
		return *this;
	}
	inline DataStream& operator<<(char32_t data) {
		Util::convert_endian<Util::Endian::Native,io_endianness>(data);
		device.write(&data,sizeof(char32_t),1);
		return *this;
	}
	inline DataStream& operator>>(char32_t& data) {
		device.read(&data,sizeof(char32_t),1);
		Util::convert_endian<io_endianness,Util::Endian::Native>(data);
		return *this;
	}
#ifdef __GNUC__
	inline DataStream& operator<<(unsigned __int128 data) {
		Util::convert_endian<Util::Endian::Native,io_endianness>(data);
		device.write(&data,sizeof(unsigned __int128),1);
		return *this;
	}
	inline DataStream& operator>>(unsigned __int128& data) {
		device.read(&data,sizeof(unsigned __int128),1);
		Util::convert_endian<io_endianness,Util::Endian::Native>(data);
		return *this;
	}
	inline DataStream& operator<<(__int128 data) {
		Util::convert_endian<Util::Endian::Native,io_endianness>(data);
		device.write(&data,sizeof(__int128),1);
		return *this;
	}
	inline DataStream& operator>>(__int128& data) {
		device.read(&data,sizeof(__int128),1);
		Util::convert_endian<io_endianness,Util::Endian::Native>(data);
		return *this;
	}
#endif
	template< class CharT, class Traits = std::char_traits<CharT>, class Allocator = std::allocator<CharT>>
	inline DataStream& operator<<(const std::basic_string<CharT, Traits, Allocator>& data) {
		*this << static_cast<uint32_t>( data.size () );
		if constexpr(sizeof(CharT) == sizeof(std::byte)) {
			device.write(data.data (), 1, data.size () );
			return *this;
		} else {
			return writeElements<CharT>(data.begin(), data.end(), false);
		}
	}
	template< class CharT, class Traits = std::char_traits<CharT>, class Allocator = std::allocator<CharT>>
	inline DataStream& operator>>(std::basic_string<CharT, Traits, Allocator>& data) {
		uint32_t size;
		*this >> size;
		data.resize(size);
		if constexpr(sizeof(CharT) == sizeof(std::byte)) {
			device.read(data.data (), 1, data.size () );
			return *this;
		} else {
			return readElementsInto<CharT>(data.begin(), data.end());
		}
	}
	template <typename T1, typename T2> inline DataStream& operator<<(const std::pair<T1,T2>& pair) {
		return *this << pair.first << pair.second;
	}
	template <typename T1, typename T2> inline DataStream& operator>>(std::pair<T1,T2>& pair) {
		return *this >> pair.first >> pair.second;
	}
	template <typename Element, typename Iterator> DataStream& writeElements(Iterator first, Iterator last, bool writeSize = true) {
		if(writeSize) {
			uint32_t elements = std::distance(first,last);
			*this << elements;
		}
		std::for_each(first,last, [this](const Element& iter) { *this << iter; } );
		return *this;
	}
	template <typename Element, typename Iterator> DataStream& readElementsInto(Iterator first, Iterator last) {
		std::for_each(first,last, [this](Element& iter) { *this >> iter; } );
		return *this;
	}
	template <typename Container> requires Util::VectorLike<Container>
	inline DataStream& operator<<(const Container& data) {
		*this << static_cast<uint32_t>( data.size () );
		if constexpr(sizeof(Container::value_type) == sizeof(std::byte)) {
			device.write(data.data (), 1, data.size () );
			return *this;
		} else {
			return writeElements<Container::value_type>(data.begin(), data.end(), false);
		}
	}
	template <typename Container> requires Util::VectorLike<Container>
	inline DataStream& operator>>(Container& data) {
		uint32_t size;
		*this >> size;
		data.resize(size);
		if constexpr(sizeof(typename Container::value_type) == sizeof(std::byte)) {
			device.read(data.data (), 1, data.size () );
			return *this;
		} else {
			return readElementsInto<typename Container::value_type>(data.begin(), data.end());
		}
	}
	template <typename Container> requires Util::MapLike<Container>
	inline DataStream& operator<<(const Container& data) {
		*this << static_cast<uint32_t>( data.size () );
		return writeElements<typename Container::value_type>(data.begin(), data.end(), false);
	}
	template <typename Container> requires Util::MapLike<Container>
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
	template <typename Container> requires Util::SequentialContainer<Container>
	inline DataStream& operator<<(const Container& data) {
		*this << static_cast<uint32_t>( data.size () );
		return writeElements<Container::value_type>(data.begin(), data.end(), false);
	}
	template <typename Container> requires Util::SequentialContainer<Container>
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
	template <typename T> inline DataStream& operator<<(const std::span<const T>& data) {
		//*this << static_cast<uint32_t>( data.size () );
		if constexpr(sizeof(T) == sizeof(std::byte)) {
			device.write(data.data (), 1, data.size () );
		} else {
			return writeElements<T>(data.begin(), data.end(), false);
		}
		return *this;
	}
	template <typename T> inline DataStream& operator>>(const std::span<T>& data) {
		if constexpr(sizeof(T) == sizeof(std::byte)) {
			device.read(data.data (), 1, data.size () );
			return *this;
		} else readElementsInto<T>(data.begin(), data.end());
	}
};

}
}
#endif // ELVDATASTREAM_HPP
