#ifndef ELVNETFIELDDELTA_HPP
#define ELVNETFIELDDELTA_HPP
/**
 * @file ElvNetFieldDelta.hpp
 * @brief Schema-driven fixed-field delta codec.
 *
 * `schemaFieldDeltaOps()` is the zero-code opt-in for components described by an
 * `ObjectDefinition`: it writes a one-bit changed mask per field, then the raw
 * bytes of changed fields. Variable-length fields and padding-sensitive blobs
 * should provide their own `FieldDeltaOps` table instead.
 */
#include <Elvavena/Net/ElvNetReplicationSchema.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace Elv {
namespace Net {

/**
 * @brief Returns the fixed byte size of one schema field, or zero when variable.
 * @param type Type descriptor.
 */
inline std::size_t schemaFieldSize(const Io::TypeDefinition* type) noexcept {
	if (type == nullptr) return 0;
	switch (type->dataType) {
	case Io::DataType::INT8:
		return 1;
	case Io::DataType::INT16:
		return 2;
	case Io::DataType::INT32:
		return 4;
	case Io::DataType::INT64:
		return 8;
	case Io::DataType::NSTRING:
		return type->length;
	case Io::DataType::NARRAY:
		return static_cast<std::size_t>(type->length)
			* (type->elementStride != 0 ? type->elementStride
				: schemaFieldSize(static_cast<const Io::TypeDefinition*>(type->subType)));
	case Io::DataType::OBJECT:
		return type->subType != nullptr
			? static_cast<const Io::ObjectDefinition*>(type->subType)->stride : 0;
	default:
		return 0;
	}
}

namespace detail {

/** @brief Returns whether every schema field is fixed-size. */
inline bool schemaFieldsFixed(const Io::ObjectDefinition& schema) noexcept {
	for (std::size_t index = 0; index < schema.fieldCount; ++index) {
		if (schemaFieldSize(schema.fields[index].typeDef) == 0) return false;
	}
	return true;
}

/** @brief Compares every fixed field. */
inline bool schemaFieldChanged(const void* current, const void* baseline, const void* context) {
	const auto* schema = static_cast<const Io::ObjectDefinition*>(context);
	if (schema == nullptr || current == nullptr || baseline == nullptr) return false;
	if (!schemaFieldsFixed(*schema)) {
		return std::memcmp(current, baseline, schema->stride) != 0;
	}
	for (std::size_t index = 0; index < schema->fieldCount; ++index) {
		const Io::ObjectField& field = schema->fields[index];
		const std::size_t size = schemaFieldSize(field.typeDef);
		if (std::memcmp(static_cast<const std::byte*>(current) + field.offset,
			static_cast<const std::byte*>(baseline) + field.offset, size) != 0) {
			return true;
		}
	}
	return false;
}

/** @brief Encodes a field mask plus changed field bytes. */
inline bool schemaFieldEncode(Io::BitWriter& writer, const void* current, const void* baseline,
	const void* context) {
	const auto* schema = static_cast<const Io::ObjectDefinition*>(context);
	if (schema == nullptr || current == nullptr || baseline == nullptr
		|| !schemaFieldsFixed(*schema) || schema->fieldCount > writer.remainingBits()) {
		return false;
	}
	for (std::size_t index = 0; index < schema->fieldCount; ++index) {
		const Io::ObjectField& field = schema->fields[index];
		const std::size_t size = schemaFieldSize(field.typeDef);
		const bool changed = std::memcmp(static_cast<const std::byte*>(current) + field.offset,
			static_cast<const std::byte*>(baseline) + field.offset, size) != 0;
		if (!writer.writeBit(changed)) return false;
	}
	if (!writer.alignToByte()) return false;
	for (std::size_t index = 0; index < schema->fieldCount; ++index) {
		const Io::ObjectField& field = schema->fields[index];
		const std::size_t size = schemaFieldSize(field.typeDef);
		if (std::memcmp(static_cast<const std::byte*>(current) + field.offset,
			static_cast<const std::byte*>(baseline) + field.offset, size) == 0) {
			continue;
		}
		if (!writer.writeBytes(static_cast<const std::byte*>(current) + field.offset, size)) {
			return false;
		}
	}
	return true;
}

/** @brief Reconstructs a component from baseline plus a field delta. */
inline bool schemaFieldDecode(Io::BitReader& reader, void* target, const void* baseline,
	const void* context) {
	const auto* schema = static_cast<const Io::ObjectDefinition*>(context);
	if (schema == nullptr || target == nullptr || baseline == nullptr
		|| !schemaFieldsFixed(*schema) || schema->fieldCount > reader.remainingBits()) {
		return false;
	}
	std::memcpy(target, baseline, schema->stride);
	std::vector<bool> changed(schema->fieldCount, false);
	for (std::size_t index = 0; index < schema->fieldCount; ++index) {
		std::uint64_t maskBit = 0;
		if (!reader.readBits(maskBit, 1)) return false;
		changed[index] = maskBit != 0;
	}
	if (!reader.alignToByte()) return false;
	for (std::size_t index = 0; index < schema->fieldCount; ++index) {
		if (!changed[index]) continue;
		const Io::ObjectField& field = schema->fields[index];
		const std::size_t size = schemaFieldSize(field.typeDef);
		if (!reader.readBytes(static_cast<std::byte*>(target) + field.offset, size)) {
			return false;
		}
	}
	return true;
}

} // namespace detail

/**
 * @brief Returns the schema-driven fixed-field delta table.
 *
 * Set `ComponentDefinition::fieldDeltaContext` to the matching
 * `ObjectDefinition*`.
 */
inline const FieldDeltaOps* schemaFieldDeltaOps() noexcept {
	static const FieldDeltaOps ops {
		&detail::schemaFieldChanged,
		&detail::schemaFieldEncode,
		&detail::schemaFieldDecode
	};
	return &ops;
}

} // namespace Net
} // namespace Elv
#endif // ELVNETFIELDDELTA_HPP
