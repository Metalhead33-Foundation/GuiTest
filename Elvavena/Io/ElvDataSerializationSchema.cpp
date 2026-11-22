#include "ElvDataSerializationSchema.hpp"
#include <Elvavena/Io/ElvDataStream.hpp>
namespace Elv {
namespace Io {

#define SERIALIZATION_HELPER_HEADER_DEF template <Util::Endian io_endianness = Util::Endian::Big, DeviceLike IoType = Device>
#define SERIALIZATION_HELPER_HEADER template <Util::Endian io_endianness, DeviceLike IoType>
#define STREAMER DataStream<io_endianness,IoType>
#define SERIALIZATION_STRUCT SerializationHelpers<io_endianness,IoType>

SERIALIZATION_HELPER_HEADER_DEF struct SerializationHelpers {
	typedef DataStream<io_endianness, IoType> Stream;
	static void serializeObject(Stream& stream, const ObjectDefinition& objectdef, const void* data);
	static void deserializeObject(Stream& stream, const ObjectDefinition& objectdef, void* data);
	static void serializeMap(Stream& stream, const MapTypedef& objectdef, const CompositeOps& compops, const void* data);
	static void deserializeMap(Stream& stream, const MapTypedef& objectdef, const CompositeOps& compops, void* data);
	static void serializeSequence(Stream& stream, const TypeDefinition& objectdef, const CompositeOps& compops, const void* data);
	static void deserializeSequence(Stream& stream, const TypeDefinition& objectdef, const CompositeOps& compops, void* data);
	static void serialize(Stream& stream, const TypeDefinition& typedefin, const void* data) {
		switch (typedefin.dataType) {
			case DataType::DUMMY:
				break;
			case DataType::INT8:
				stream << *static_cast<const uint8_t*>(data);
				break;
			case DataType::INT16:
				stream << *static_cast<const uint16_t*>(data);
				break;
			case DataType::INT32:
				stream << *static_cast<const uint32_t*>(data);
				break;
			case DataType::INT64:
				stream << *static_cast<const uint64_t*>(data);
				break;
			case DataType::OBJECT:
				serializeObject(stream, *static_cast<const ObjectDefinition*>(typedefin.subType), data);
				break;
			case DataType::VAR_STRING:
				{
				// Doesn't include the null terminator, so we add one.
				const auto size = typedefin.compOps->size(data) + 1;
				// The actual string.
				const auto str = typedefin.compOps->stringData(data);
				stream << static_cast<uint32_t>(size);
				uint8_t nullchar = 0;
				stream.device.write(str, 1, size-1);
				stream.device.write(&nullchar, 1, 1);
				break;
				}
			case DataType::NSTRING:
				stream.device.write(data, 1, typedefin.length);
				break;
			case DataType::VAR_ARRAY:
				serializeSequence(stream, *static_cast<const TypeDefinition*>(typedefin.subType), *typedefin.compOps, data);
				break;
			case DataType::NARRAY: {
					const auto* elemType = static_cast<const TypeDefinition*>(typedefin.subType);
					const auto* bytes = static_cast<const uint8_t*>(data);
					for(uint32_t i = 0; i < typedefin.length; ++i) {
						serialize(stream, *elemType, bytes + (i * typedefin.elementStride));
					}
					break;
				}
			case DataType::LIST:
				serializeSequence(stream, *static_cast<const TypeDefinition*>(typedefin.subType), *typedefin.compOps, data);
				break;
			case DataType::MAP:
				serializeMap(stream, *static_cast<const MapTypedef*>(typedefin.subType), *typedefin.compOps, data);
				break;
		}
	};
	static void deserialize(Stream& stream, const TypeDefinition& typedefin, void* data) {
		switch (typedefin.dataType) {
			case DataType::DUMMY:
				break;
			case DataType::INT8:
				stream >> *static_cast<uint8_t*>(data);
				break;
			case DataType::INT16:
				stream >> *static_cast<uint16_t*>(data);
				break;
			case DataType::INT32:
				stream >> *static_cast<uint32_t*>(data);
				break;
			case DataType::INT64:
				stream >> *static_cast<uint64_t*>(data);
				break;
			case DataType::OBJECT:
				deserializeObject(stream, *static_cast<const ObjectDefinition*>(typedefin.subType), data);
				break;
			case DataType::VAR_STRING:
			{
				// This len includes the null terminator!
				uint32_t len;
				stream >> len;
				if(len) {
					uint8_t dummy;
					char* str = typedefin.compOps->stringResize(data, len-1);
					stream.device.read(str, 1, len-1);
					stream.device.read(&dummy, 1, 1);
				} else {
					typedefin.compOps->clear(data);
				}
				break;
			}
			case DataType::NSTRING:
				stream.device.read(data, 1, typedefin.length);
				break;
			case DataType::VAR_ARRAY:
				deserializeSequence(stream, *static_cast<const TypeDefinition*>(typedefin.subType), *typedefin.compOps, data);
				break;
			case DataType::NARRAY:
			{
				const auto* elemType = static_cast<const TypeDefinition*>(typedefin.subType);
				auto* bytes = static_cast<uint8_t*>(data);
				for(uint32_t i = 0; i < typedefin.length; ++i) {
					deserialize(stream, *elemType, bytes + (i * typedefin.elementStride));
				}
				break;
			}
			case DataType::LIST:
				deserializeSequence(stream, *static_cast<const TypeDefinition*>(typedefin.subType), *typedefin.compOps, data);
				break;
			case DataType::MAP:
				deserializeMap(stream, *static_cast<const MapTypedef*>(typedefin.subType), *typedefin.compOps, data);
				break;
		}
	};
};
SERIALIZATION_HELPER_HEADER void SERIALIZATION_STRUCT::serializeObject(STREAMER& stream, const ObjectDefinition& objectdef, const void* data)
{
	const uint8_t* bytes = static_cast<const uint8_t*>(data);
	for(size_t i = 0; i < objectdef.fieldCount; ++i) {
		const auto& fielddef = objectdef.fields[i];
		SERIALIZATION_STRUCT::serialize(stream, *fielddef.typeDef, bytes + fielddef.offset);
	}
}
SERIALIZATION_HELPER_HEADER void SERIALIZATION_STRUCT::deserializeObject(STREAMER& stream, const ObjectDefinition& objectdef, void* data)
{
	uint8_t* bytes = static_cast<uint8_t*>(data);
	for(size_t i = 0; i < objectdef.fieldCount; ++i) {
		const auto& fielddef = objectdef.fields[i];
		SERIALIZATION_STRUCT::deserialize(stream, *fielddef.typeDef, bytes + fielddef.offset);
	}
}
SERIALIZATION_HELPER_HEADER void SERIALIZATION_STRUCT::serializeMap(STREAMER& stream, const MapTypedef& objectdef, const CompositeOps& compops, const void* data)
{
	const uint32_t size = static_cast<uint32_t>(compops.size(data));
	stream << size;

	struct Ctx { STREAMER& stream; const MapTypedef& mapDef; };
	Ctx ctx{ stream, objectdef };

	compops.forEachKV(data, [](const void* key, const void* val, void* rawCtx) {
		auto* c = static_cast<Ctx*>(rawCtx);
		SERIALIZATION_STRUCT::serialize(c->stream, c->mapDef.first, key);
		SERIALIZATION_STRUCT::serialize(c->stream, c->mapDef.second, val);
	}, &ctx);
}
SERIALIZATION_HELPER_HEADER void SERIALIZATION_STRUCT::deserializeMap(STREAMER& stream, const MapTypedef& objectdef, const CompositeOps& compops, void* data)
{
	uint32_t size;
	stream >> size;

	compops.clear(data);
	if (compops.reserve) {
		compops.reserve(data, size);
	}

	struct Ctx { STREAMER& stream; const MapTypedef& mapDef; };
	Ctx ctx{ stream, objectdef };

	for (uint32_t i = 0; i < size; ++i) {
		compops.insertWith(data, [](void* key, void* val, void* rawCtx) {
			auto* c = static_cast<Ctx*>(rawCtx);
			SERIALIZATION_STRUCT::deserialize(c->stream, c->mapDef.first, key);
			SERIALIZATION_STRUCT::deserialize(c->stream, c->mapDef.second, val);
		}, &ctx);
	}
}
SERIALIZATION_HELPER_HEADER void SERIALIZATION_STRUCT::serializeSequence(STREAMER& stream, const TypeDefinition& objectdef, const CompositeOps& compops, const void* data)
{
	const uint32_t size = static_cast<uint32_t>(compops.size(data));
	stream << size;

	struct Ctx { STREAMER& stream; const TypeDefinition& elemDef; };
	Ctx ctx{ stream, objectdef };

	compops.forEach(data, [](const void* elem, void* rawCtx) {
		auto* c = static_cast<Ctx*>(rawCtx);
		SERIALIZATION_STRUCT::serialize(c->stream, c->elemDef, elem);
	}, &ctx);
}
SERIALIZATION_HELPER_HEADER void SERIALIZATION_STRUCT::deserializeSequence(STREAMER& stream, const TypeDefinition& objectdef, const CompositeOps& compops, void* data)
{
	uint32_t size;
	stream >> size;

	compops.clear(data);
	if (compops.reserve) {
		compops.reserve(data, size);
	}

	for (uint32_t i = 0; i < size; ++i) {
		void* elemPtr = compops.emplaceBackDefault(data);
		deserialize(stream, objectdef, elemPtr);
	}
}

void ObjectDefinition::serialize(Device& iodevice, Util::Endian endianness, const void* data) const
{
	switch (endianness) {
		case Util::Endian::Little: {
			DataStream<Util::Endian::Little, Device> stream(iodevice);
			SerializationHelpers<Util::Endian::Little, Device>::serializeObject(stream, *this, data);
			break;
		}
		case Util::Endian::Big: {
			DataStream<Util::Endian::Big, Device> stream(iodevice);
			SerializationHelpers<Util::Endian::Big, Device>::serializeObject(stream, *this, data);
			break;
		}
	}
}

void ObjectDefinition::deserialize(Device& iodevice, Util::Endian endianness, void* data) const
{
	switch (endianness) {
		case Util::Endian::Little: {
			DataStream<Util::Endian::Little, Device> stream(iodevice);
			SerializationHelpers<Util::Endian::Little, Device>::deserializeObject(stream, *this, data);
			break;
		}
		case Util::Endian::Big: {
			DataStream<Util::Endian::Big, Device> stream(iodevice);
			SerializationHelpers<Util::Endian::Big, Device>::deserializeObject(stream, *this, data);
			break;
		}
	}
}

}
}