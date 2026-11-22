#ifndef ELVDATASERIALIZATIONSCHEMA_HPP
#define ELVDATASERIALIZATIONSCHEMA_HPP

#include <Elvavena/Util/ElvUtilGlobals.hpp>
#include <Elvavena/Util/ElvEndianness.hpp>
#include <Elvavena/Util/ElvContainerBasic.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace Elv {
namespace Io {

/**
 * @enum DataType
 * @brief Identifies the wire-level binary representation of a serialized field or element.
 *
 * Because binary serialization operates on raw byte widths and endianness rather than
 * arithmetic semantics, signed integers, unsigned integers, and IEEE 754 floating-point
 * values of equal bit width share the same primitive tag.
 */
enum class DataType : uint8_t {
	DUMMY = 0,  ///< Invalid sentinel tag representing an uninitialized or empty type (analogous to std::monostate).
	INT8,       ///< 8-bit primitive: represents int8_t, uint8_t, char, or bool.
	INT16,      ///< 16-bit primitive: represents int16_t or uint16_t.
	INT32,      ///< 32-bit primitive: represents int32_t, uint32_t, or 32-bit float.
	INT64,      ///< 64-bit primitive: represents int64_t, uint64_t, or 64-bit double.
	OBJECT,     ///< Nested struct or class described by an ObjectDefinition.
	VAR_STRING, ///< Variable-length UTF-8 or ASCII string, prefixed on the wire by a uint32_t byte length (including null terminator).
	NSTRING,    ///< Fixed-length UTF-8 or ASCII character buffer of exact byte size @c TypeDefinition::length.
	VAR_ARRAY,  ///< Contiguous dynamic array (e.g., std::vector), prefixed on the wire by a uint32_t element count.
	NARRAY,     ///< Fixed-size array of @c TypeDefinition::length elements spaced by @c TypeDefinition::elementStride bytes.
	LIST,       ///< Non-contiguous sequential container (e.g., std::list or std::deque), prefixed on the wire by a uint32_t element count.
	MAP,        ///< Associative key-value container (e.g., std::map or std::unordered_map), prefixed on the wire by a uint32_t pair count.
};

struct TypeDefinition;
struct ObjectDefinition;
struct CompositeOps;

/**
 * @struct TypeDefinition
 * @brief Recursive descriptor defining the binary wire format and in-memory layout of a type.
 *
 * Can represent standalone primitives, fixed-size buffers, nested objects, or arbitrarily
 * nested containers via its @c subType pointer.
 */
struct TypeDefinition {
	/// @brief Primary wire data category for this type.
	DataType dataType;

	/// @brief Memory stride (in bytes) of an individual element when this type is contained inside a DataType::NARRAY.
	uint32_t elementStride;

	/// @brief Fixed byte length for DataType::NSTRING, or fixed element count for DataType::NARRAY. Unused otherwise.
	uint32_t length;

	/// @brief Type-erased container v-table. Required for VAR_STRING, VAR_ARRAY, LIST, and MAP; nullptr otherwise.
	const CompositeOps* compOps;

	/**
	 * @brief Pointer to nested type metadata, interpreted according to @c dataType:
	 * - DataType::OBJECT: points to a const @c ObjectDefinition.
	 * - DataType::VAR_ARRAY, DataType::NARRAY, DataType::LIST: points to the element's const @c TypeDefinition.
	 * - DataType::MAP: points to a const @c MapTypedef (@c std::pair<TypeDefinition, TypeDefinition>) describing [KeyType, ValueType].
	 * - All other types: @c nullptr.
	 */
	const void* subType;
};

/**
 * @typedef MapTypedef
 * @brief Pair of TypeDefinitions representing the key type (@c first) and mapped value type (@c second) of a DataType::MAP.
 */
typedef std::pair<TypeDefinition, TypeDefinition> MapTypedef;

/**
 * @struct ObjectField
 * @brief Maps a single member variable within a C++ struct or class to its TypeDefinition.
 */
struct ObjectField {
	uint16_t offset;               ///< Byte offset of the member from the start of the enclosing object (e.g., via @c offsetof).
	const TypeDefinition* typeDef; ///< Pointer to the type descriptor governing how this field is serialized.
};

/**
 * @struct ObjectDefinition
 * @brief Top-level schema descriptor for a serializable C++ struct or class.
 */
struct MH_UTIL_API ObjectDefinition {
	const ObjectField* fields; ///< Pointer to a contiguous array of field descriptors.
	size_t fieldCount;         ///< Number of elements in the @c fields array.
	size_t stride;             ///< Total in-memory size of the object in bytes (i.e., @c sizeof(T)).

	/**
	 * @brief Serializes an instance of the described object into an I/O device.
	 * @param iodevice Target output device receiving the binary stream.
	 * @param endianness Byte order used to encode multi-byte primitives on the wire.
	 * @param data Pointer to the source C++ object instance in memory.
	 */
	void serialize(Device& iodevice, Util::Endian endianness, const void* data) const;

	/**
	 * @brief Deserializes an instance of the described object from an I/O device.
	 * @param iodevice Source input device providing the binary stream.
	 * @param endianness Byte order used to decode multi-byte primitives from the wire.
	 * @param data Pointer to the target C++ object instance to populate in-place.
	 */
	void deserialize(Device& iodevice, Util::Endian endianness, void* data) const;
};

/**
 * @struct CompositeOps
 * @brief Type-erased function table abstracting container operations for dynamic strings, arrays, lists, and maps.
 *
 * Decouples the binary serialization engine from C++ standard library or custom-allocator
 * container templates, avoiding header template bloat and ABI exposure.
 */
struct CompositeOps {
	/// @name Common Operations
	/// @{

	/**
	 * @brief Returns the number of elements in the container (or byte length excluding null terminator for strings).
	 * @param container Pointer to the const container instance.
	 * @return Current element or character count.
	 */
	size_t (*size)(const void* container);

	/**
	 * @brief Removes all elements from the container prior to deserialization.
	 * @param container Pointer to the mutable container instance.
	 */
	void (*clear)(void* container);

	/**
	 * @brief Pre-allocates storage capacity when supported by the underlying container.
	 * @param container Pointer to the mutable container instance.
	 * @param count Expected number of elements to reserve space for.
	 * @note May be @c nullptr for node-based containers such as @c std::list or @c std::map.
	 */
	void (*reserve)(void* container, size_t count);
	/// @}

	/// @name Sequential Container Operations (VAR_STRING, VAR_ARRAY, LIST)
	/// @{

	/**
	 * @brief Iterates over every element in a sequential container and invokes a visitor callback.
	 * @param container Pointer to the const sequential container.
	 * @param visitor Callback invoked with a pointer to each element and the opaque @p ctx pointer.
	 * @param ctx Caller-supplied context forwarded to @p visitor.
	 */
	void (*forEach)(const void* container, void (*visitor)(const void* elem, void* ctx), void* ctx);

	/**
	 * @brief Appends a default-constructed element to the end of the container and returns its address.
	 * @param container Pointer to the mutable sequential container.
	 * @return Pointer to the newly constructed element for in-place deserialization.
	 */
	void* (*emplaceBackDefault)(void* container);
	/// @}

	/// @name String Fast-Path Operations (VAR_STRING)
	/// @{

	/**
	 * @brief Returns a pointer to the contiguous character buffer of a string container.
	 * @param str Pointer to the const string instance.
	 * @return Pointer to the underlying character data, or @c nullptr if not a byte-string container.
	 */
	const char* (*stringData)(const void* str);

	/**
	 * @brief Resizes a string container to @p newSize characters and returns a mutable pointer to its buffer.
	 * @param str Pointer to the mutable string instance.
	 * @param newSize Target character length (excluding null terminator).
	 * @return Mutable pointer to the resized character buffer, or @c nullptr if not a byte-string container.
	 */
	char* (*stringResize)(void* str, size_t newSize);
	/// @}

	/// @name Associative Container Operations (MAP)
	/// @{

	/**
	 * @brief Iterates over every key-value pair in an associative container and invokes a visitor callback.
	 * @param map Pointer to the const map container.
	 * @param visitor Callback invoked with pointers to each key, mapped value, and the opaque @p ctx pointer.
	 * @param ctx Caller-supplied context forwarded to @p visitor.
	 */
	void (*forEachKV)(const void* map, void (*visitor)(const void* key, const void* val, void* ctx), void* ctx);

	/**
	 * @brief Constructs temporary key and value instances, populates them via callback, and inserts them into the map.
	 * @param map Pointer to the mutable map container.
	 * @param populate Callback invoked with mutable pointers to the temporary key and value objects to deserialize into.
	 * @param ctx Caller-supplied context forwarded to @p populate.
	 */
	void (*insertWith)(void* map, void (*populate)(void* key, void* val, void* ctx), void* ctx);
	/// @}
};

/**
 * @brief Generates a static CompositeOps v-table for contiguous random-access containers (e.g., std::vector, std::string).
 *
 * Automatically enables @c stringData and @c stringResize fast-paths when @c T::value_type is a 1-byte integral type.
 *
 * @tparam T A container type satisfying @c Util::VectorLike.
 * @return Pointer to a static constexpr @c CompositeOps instance tailored to @p T.
 */
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

/**
 * @brief Generates a static CompositeOps v-table for non-contiguous sequential containers (e.g., std::list, std::deque).
 *
 * @tparam T A container type satisfying @c Util::SequentialContainer.
 * @return Pointer to a static constexpr @c CompositeOps instance tailored to @p T.
 */
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

/**
 * @brief Generates a static CompositeOps v-table for associative key-value containers (e.g., std::map, std::unordered_map).
 *
 * @tparam T A container type satisfying @c Util::MapLike.
 * @return Pointer to a static constexpr @c CompositeOps instance tailored to @p T.
 */
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

/**
 * @brief Deduces the corresponding DataType enum tag for a C++ type at compile time.
 *
 * @tparam T The C++ field or element type to inspect.
 * @return The matching @c DataType enumerator for @p T.
 */
template <typename T>
consteval DataType deduceDataType() {
	if constexpr (std::same_as<T, bool> || (sizeof(T) == 1 && std::is_arithmetic_v<T>)) return DataType::INT8;
	else if constexpr (sizeof(T) == 2 && std::is_arithmetic_v<T>) return DataType::INT16;
	else if constexpr (sizeof(T) == 4 && std::is_arithmetic_v<T>) return DataType::INT32;
	else if constexpr (sizeof(T) == 8 && std::is_arithmetic_v<T>) return DataType::INT64;
	else if constexpr (std::same_as<T, std::string>) return DataType::VAR_STRING;
	else if constexpr (Util::VectorLike<T>) return DataType::VAR_ARRAY;
	else if constexpr (Util::SequentialContainer<T>) return DataType::LIST;
	else if constexpr (Util::MapLike<T>) return DataType::MAP;
	else return DataType::OBJECT;
}

} // namespace Io
} // namespace Elv

#endif // ELVDATASERIALIZATIONSCHEMA_HPP