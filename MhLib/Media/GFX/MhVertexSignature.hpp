#ifndef MHVERTEXSIGNATURE_HPP
#define MHVERTEXSIGNATURE_HPP
#include <cstdint>
#include <cstddef>
#include <utility>
namespace MH33 {
namespace GFX {

enum class VertexAttributeType : uint8_t {
	// Value types
	UINT8 = 0x00,
	UINT8_N = 0x08,
	SINT8 = 0x10,
	SINT8_N = 0x18,
	UINT16 = 0x20,
	UINT16_N = 0x28,
	SINT16 = 0x30,
	SINT16_N = 0x38,
	UINT32 = 0x40,
	UINT32_N = 0x48,
	SINT32 = 0x50,
	SINT32_N = 0x58,
	FLOAT16 = 0x60,
	FLOAT32 = 0x68,
	// Dimensions
	D1 = 0x00,
	D2 = 0x01,
	D3 = 0x02,
	D4 = 0x03,
	// Actual types
	UINT8x1 = (UINT8 | D1),
	UINT8x1_N = (UINT8_N | D1),
	UINT8x2 = (UINT8 | D2),
	UINT8x2_N = (UINT8_N | D2),
	UINT8x3 = (UINT8 | D3),
	UINT8x3_N = (UINT8_N | D3),
	UINT8x4 = (UINT8 | D4),
	UINT8x4_N = (UINT8_N | D4),
	SINT8x1 = (SINT8 | D1),
	SINT8x1_N = (SINT8_N | D1),
	SINT8x2 = (SINT8 | D2),
	SINT8x2_N = (SINT8_N | D2),
	SINT8x3 = (SINT8 | D3),
	SINT8x3_N = (SINT8_N | D3),
	SINT8x4 = (SINT8 | D4),
	SINT8x4_N = (SINT8_N | D4),
	UINT16x1 = (UINT16 | D1),
	UINT16x1_N = (UINT16_N | D1),
	UINT16x2 = (UINT16 | D2),
	UINT16x2_N = (UINT16_N | D2),
	UINT16x3 = (UINT16 | D3),
	UINT16x3_N = (UINT16_N | D3),
	UINT16x4 = (UINT16 | D4),
	UINT16x4_N = (UINT16_N | D4),
	SINT16x1 = (SINT16 | D1),
	SINT16x1_N = (SINT16_N | D1),
	SINT16x2 = (SINT16 | D2),
	SINT16x2_N = (SINT16_N | D2),
	SINT16x3 = (SINT16 | D3),
	SINT16x3_N = (SINT16_N | D3),
	SINT16x4 = (SINT16 | D4),
	SINT16x4_N = (SINT16_N | D4),
	UINT32x1 = (UINT32 | D1),
	UINT32x1_N = (UINT32_N | D1),
	UINT32x2 = (UINT32 | D2),
	UINT32x2_N = (UINT32_N | D2),
	UINT32x3 = (UINT32 | D3),
	UINT32x3_N = (UINT32_N | D3),
	UINT32x4 = (UINT32 | D4),
	UINT32x4_N = (UINT32_N | D4),
	SINT32x1 = (SINT32 | D1),
	SINT32x1_N = (SINT32_N | D1),
	SINT32x2 = (SINT32 | D2),
	SINT32x2_N = (SINT32_N | D2),
	SINT32x3 = (SINT32 | D3),
	SINT32x3_N = (SINT32_N | D3),
	SINT32x4 = (SINT32 | D4),
	SINT32x4_N = (SINT32_N | D4),
	FLOAT16x1 = (FLOAT16 | D1),
	FLOAT16x2 = (FLOAT16 | D2),
	FLOAT16x3 = (FLOAT16 | D3),
	FLOAT16x4 = (FLOAT16 | D4),
	FLOAT32x1 = (FLOAT32 | D1),
	FLOAT32x2 = (FLOAT32 | D2),
	FLOAT32x3 = (FLOAT32 | D3),
	FLOAT32x4 = (FLOAT32 | D4)
};

struct VertexAttribute {
	VertexAttributeType type;
	size_t stride;
	uintptr_t offset;
};

typedef std::pair<VertexAttribute*,size_t> VertexAttributeList;

}
}
#endif // MHVERTEXSIGNATURE_HPP
