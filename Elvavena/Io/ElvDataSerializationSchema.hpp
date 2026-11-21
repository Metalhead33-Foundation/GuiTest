#ifndef ELVDATASERIALIZATIONSCHEMA_HPP
#define ELVDATASERIALIZATIONSCHEMA_HPP
#include <Elvavena/Util/ElvUtilGlobals.hpp>
#include <Elvavena/Util/ElvEndianness.hpp>
#include <Elvavena/Util/ElvContainerBasic.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <cstdint>
#include <utility>

namespace Elv {
namespace Io {

// Because this is a binary serialization primitive, we don't need to account for signedness or floats
enum class DataType : uint8_t {
	DUMMY = 0, //! Invalid dummy data tag to represent an invalid default. Think of it as an equivalent of std::monostate.
	INT8, //! Can represent both int8_t and uint8_t, as well as booleans
	INT16, //! Can represent both int16_t and uint16_t
	INT32, //! Can represent both int32_t and uint32_t, as well as 32-bit floating point values
	INT64, //! Can represent both int64_t and uint64_t, as well as 64-bit floating point values
	OBJECT, //! Represents a struct.
	VAR_STRING, //! Represents a variable-size UTF8 or ASCII string. Length is a uint32_t.
	NSTRING, //! Represents a fixed-size UTF8 or ASCII string.
	VAR_ARRAY, //! Represents an std::vector. Length is a uint32_t.
	NARRAY, //! Represents a fixed-size array.
	LIST, //! Represents an std::list or any other non-sequential non-associative array. Length is a uint32_t.
	MAP, //! Represents an associative array - std::map or std::unordered_map - of two element types. Essentially a VAR_ARRAY of <Key,Value> pairs.
};

struct TypeDefinition;
struct ObjectDefinition;
struct CompositeOps;

struct TypeDefinition {
	//! The type of data being used.
	DataType dataType;
	//! Relevant for any and all non-primitive data, and only truly relevant when the parent is DataType::NARRAY.
	uint32_t elementStride;
	//! Used only for DataType::NSTRING and DataType::NARRAY
	uint32_t length;
	//! Only relvant if any of the data tags is DataType::VAR_STRING, DataType::VAR_ARRAY, DataType::LIST or DataType::MAP. Otherwise all null pointers.
	const CompositeOps* compOps;
	/*!
	 * If dataType == DataType::OBJECT, this points to an ObjectDefinition.
	 * If dataType == DataType::VAR_ARRAY, dataType == DataType::NARRAY or dataType == DataType::LIST, this points to the element's type definition.
	 * If dataType == DataType::MAP, this points to a 2-element TypeDefinition array [KeyType, ValueType]
	 */
	const void* subType;
};
typedef std::pair<TypeDefinition,TypeDefinition> MapTypedef;

struct ObjectField {
	uint16_t offset;
	const TypeDefinition* typeDef;
};

struct MH_UTIL_API ObjectDefinition {
	const ObjectField* fields;
	size_t fieldCount;
	size_t stride;
	void serialize(Device& iodevice, Util::Endian endianness, const void* data) const;
	void deserialize(Device& iodevice, Util::Endian endianness, void* data) const;
};

struct CompositeOps {
	// --- Common / Read Operations ---

	// Returns element count (for strings, byte length excluding null terminator).
	size_t (*size)(const void* container);

	// Clears the container before deserialization begins.
	void (*clear)(void* container);

	// Optional optimization: pre-allocates capacity for VAR_STRING, VAR_ARRAY, or unordered_map.
	// Can be nullptr for std::list or std::map.
	void (*reserve)(void* container, size_t count);

	// --- Sequential / String Operations (VAR_STRING, VAR_ARRAY, LIST) ---

	// Serialization: invokes `visitor(const void* elem, void* userCtx)` for each element.
	// Using a visitor avoids iterator allocation/type-erasure overhead for std::list and std::map!
	// (For VAR_STRING, can either return raw char* via a helper or visit/resize directly).
	void (*forEach)(const void* container, void (*visitor)(const void* elem, void* ctx), void* ctx);

	// Deserialization: appends a default-constructed element and returns a pointer to it
	// so the deserializer can populate it directly in-place (zero extra copies!).
	void* (*emplaceBackDefault)(void* container);

	// --- String Fast-Path (Optional, for VAR_STRING) ---
	// Avoids calling emplaceBackDefault N times for a 10KB string.
	const char* (*stringData)(const void* str);
	char* (*stringResize)(void* str, size_t newSize);

	// --- Associative Operations (MAP) ---

	// Serialization: invokes `visitor(const void* key, const void* val, void* ctx)` for each pair.
	void (*forEachKV)(const void* map, void (*visitor)(const void* key, const void* val, void* ctx), void* ctx);

	// Deserialization: allocates temporary default-constructed Key and Value objects,
	// passes their pointers to `populate(void* key, void* val, void* ctx)` so the deserializer
	// can fill them, inserts/moves them into the map, and cleans up the temporaries.
	void (*insertWith)(void* map, void (*populate)(void* key, void* val, void* ctx), void* ctx);
};

// 1. VectorLike (handles std::vector, std::string, custom-allocator vectors/strings)
template <Util::VectorLike T>
const CompositeOps* getCompositeOps() {
	static constexpr CompositeOps ops = {
		/* size */ [](const void* c) -> size_t {
			return static_cast<const T*>(c)->size();
		},
		/* clear */ [](void* c) {
			static_cast<T*>(c)->clear();
		},
		/* reserve */ [](void* c, size_t n) {
			static_cast<T*>(c)->reserve(n);
		},
		/* forEach */ [](const void* c, void (*visit)(const void*, void*), void* ctx) {
			for (const auto& item : *static_cast<const T*>(c)) {
				visit(&item, ctx);
			}
		},
		/* emplaceBackDefault */ [](void* c) -> void* {
			auto* vec = static_cast<T*>(c);
			vec->push_back(typename T::value_type{});
			return &vec->back();
		},
		/* stringData (enabled automatically if value_type is 1-byte char) */
		[]() -> const char* (*)(const void*) {
			if constexpr (sizeof(typename T::value_type) == 1 && std::is_integral_v<typename T::value_type>) {
				return [](const void* c) -> const char* {
					return reinterpret_cast<const char*>(static_cast<const T*>(c)->data());
				};
			} else {
				return nullptr;
			}
		}(),
		/* stringResize */
		[]() -> char* (*)(void*, size_t) {
			if constexpr (sizeof(typename T::value_type) == 1 && std::is_integral_v<typename T::value_type>) {
				return [](void* c, size_t n) -> char* {
					auto* vec = static_cast<T*>(c);
					vec->resize(n);
					return reinterpret_cast<char*>(vec->data());
				};
			} else {
				return nullptr;
			}
		}(),
		/* forEachKV */ nullptr,
		/* insertWith */ nullptr
	};
	return &ops;
}

// 2. SequentialContainer (handles std::list, std::deque, etc.)
template <Util::SequentialContainer T>
const CompositeOps* getCompositeOps() {
	static constexpr CompositeOps ops = {
		/* size */ [](const void* c) -> size_t {
			return static_cast<const T*>(c)->size();
		},
		/* clear */ [](void* c) {
			static_cast<T*>(c)->clear();
		},
		/* reserve */ nullptr,
		/* forEach */ [](const void* c, void (*visit)(const void*, void*), void* ctx) {
			for (const auto& item : *static_cast<const T*>(c)) {
				visit(&item, ctx);
			}
		},
		/* emplaceBackDefault */ [](void* c) -> void* {
			auto* seq = static_cast<T*>(c);
			auto it = seq->insert(seq->end(), typename T::value_type{});
			return &(*it);
		},
		/* stringData */ nullptr,
		/* stringResize */ nullptr,
		/* forEachKV */ nullptr,
		/* insertWith */ nullptr
	};
	return &ops;
}

// 3. MapLike (handles std::map, std::unordered_map, custom-allocator maps)
template <Util::MapLike T>
const CompositeOps* getCompositeOps() {
	static constexpr CompositeOps ops = {
		/* size */ [](const void* c) -> size_t {
			return static_cast<const T*>(c)->size();
		},
		/* clear */ [](void* c) {
			static_cast<T*>(c)->clear();
		},
		/* reserve */ [](void* c, size_t n) {
			if constexpr (requires(T& m, size_t count) { m.reserve(count); }) {
				static_cast<T*>(c)->reserve(n);
			}
		},
		/* forEach */ nullptr,
		/* emplaceBackDefault */ nullptr,
		/* stringData */ nullptr,
		/* stringResize */ nullptr,
		/* forEachKV */ [](const void* c, void (*visit)(const void*, const void*, void*), void* ctx) {
			for (const auto& [k, v] : *static_cast<const T*>(c)) {
				visit(&k, &v, ctx);
			}
		},
		/* insertWith */ [](void* c, void (*populate)(void*, void*, void*), void* ctx) {
			typename T::key_type key{};
			typename T::mapped_type val{};
			populate(&key, &val, ctx);
			static_cast<T*>(c)->emplace(std::move(key), std::move(val));
		}
	};
	return &ops;
}

template <typename T>
consteval DataType deduceDataType() {
	if constexpr (std::same_as<T, bool> || sizeof(T) == 1 && std::is_arithmetic_v<T>) return DataType::INT8;
	else if constexpr (sizeof(T) == 2 && std::is_arithmetic_v<T>) return DataType::INT16;
	else if constexpr (sizeof(T) == 4 && std::is_arithmetic_v<T>) return DataType::INT32;
	else if constexpr (sizeof(T) == 8 && std::is_arithmetic_v<T>) return DataType::INT64;
	else if constexpr (std::same_as<T, std::string>) return DataType::VAR_STRING;
	else if constexpr (Util::VectorLike<T>) return DataType::VAR_ARRAY;
	else if constexpr (Util::SequentialContainer<T>) return DataType::LIST;
	else if constexpr (Util::MapLike<T>) return DataType::MAP;
	else return DataType::OBJECT;
}

}
}
#endif // ELVDATASERIALIZATIONSCHEMA_HPP
