#ifndef MHVERTEXFORMAT_H
#define MHVERTEXFORMAT_H
#include <cstdint>
#include <string>
#include <span>

#if defined (_MSC_VER)
#define STD140 __declspec(align(16))
#elif defined(__GNUC__) || defined(__clang__)
#define STD140 __attribute__((aligned(16)))
#else
#define STD140
#endif

// __declspec(align(16)) - MVC
// __attribute__((aligned(16))) - GCC

namespace MH33 {
namespace GFX {

enum PrimitiveType : uint8_t {
	// Basics
	INVALID = 0x00,
	SIZE_1 = 0x01,
	SIZE_2 = 0x02,
	SIZE_3 = 0x03,
	SIZE_4 = 0x04,
	SIZE_MASK = 0x07,
	TYPE_HALF_FLOAT = 0x08, // 16-bit half-float
	TYPE_FLOAT = 0x10,		// 32-bit float
	TYPE_DOUBLE = 0x18,		// 64-bit double
	TYPE_UBYTE = 0x20,		// 8-bit unsigned integer
	TYPE_SBYTE = 0x28,		// 8-bit signed integer
	TYPE_USHORT = 0x30,		// 16-bit unsigned integer
	TYPE_SSHORT = 0x38,		// 16-bit signed integer
	TYPE_UINT = 0x40,		// 32-bit unsigned integer
	TYPE_SINT = 0x48,		// 32-bit signed integer
	TYPE_MASK = 0x78,
	IS_NORMALIZED = 0x80, // Only relevant for integers - ignored for floats
	// Floats
	F32x1 = ( SIZE_1 | TYPE_FLOAT ),
	F32x2 = ( SIZE_2 | TYPE_FLOAT ),
	F32x3 = ( SIZE_3 | TYPE_FLOAT ),
	F32x4 = ( SIZE_4 | TYPE_FLOAT ),
	F1x16 = ( SIZE_1 | TYPE_HALF_FLOAT ),
	F2x16 = ( SIZE_2 | TYPE_HALF_FLOAT ),
	F3x16 = ( SIZE_3 | TYPE_HALF_FLOAT ),
	F4x16 = ( SIZE_4 | TYPE_HALF_FLOAT ),
	F1x64 = ( SIZE_1 | TYPE_DOUBLE ),
	F2x64 = ( SIZE_2 | TYPE_DOUBLE ),
	F3x64 = ( SIZE_3 | TYPE_DOUBLE ),
	F4x64 = ( SIZE_4 | TYPE_DOUBLE ),
	// Non-normalized integers
	U8x1 = ( SIZE_1 | TYPE_UBYTE ),
	U8x2 = ( SIZE_2 | TYPE_UBYTE ),
	U8x3 = ( SIZE_3 | TYPE_UBYTE ),
	U8x4 = ( SIZE_4 | TYPE_UBYTE ),
	S8x1 = ( SIZE_1 | TYPE_SBYTE ),
	S8x2 = ( SIZE_2 | TYPE_SBYTE ),
	S8x3 = ( SIZE_3 | TYPE_SBYTE ),
	S8x4 = ( SIZE_4 | TYPE_SBYTE ),
	U16x1 = ( SIZE_1 | TYPE_USHORT ),
	U16x2 = ( SIZE_2 | TYPE_USHORT ),
	U16x3 = ( SIZE_3 | TYPE_USHORT ),
	U16x4 = ( SIZE_4 | TYPE_USHORT ),
	S16x1 = ( SIZE_1 | TYPE_SSHORT ),
	S16x2 = ( SIZE_2 | TYPE_SSHORT ),
	S16x3 = ( SIZE_3 | TYPE_SSHORT ),
	S16x4 = ( SIZE_4 | TYPE_SSHORT ),
	U32x1 = ( SIZE_1 | TYPE_UINT ),
	U32x2 = ( SIZE_2 | TYPE_UINT ),
	U32x3 = ( SIZE_3 | TYPE_UINT ),
	U32x4 = ( SIZE_4 | TYPE_UINT ),
	S32x1 = ( SIZE_1 | TYPE_SINT ),
	S32x2 = ( SIZE_2 | TYPE_SINT ),
	S32x3 = ( SIZE_3 | TYPE_SINT ),
	S32x4 = ( SIZE_4 | TYPE_SINT ),
	// Normalized integers
	UN8x1 = ( SIZE_1 | TYPE_UBYTE | IS_NORMALIZED ),
	UN8x2 = ( SIZE_2 | TYPE_UBYTE | IS_NORMALIZED ),
	UN8x3 = ( SIZE_3 | TYPE_UBYTE | IS_NORMALIZED ),
	UN8x4 = ( SIZE_4 | TYPE_UBYTE | IS_NORMALIZED ),
	SN8x1 = ( SIZE_1 | TYPE_SBYTE | IS_NORMALIZED ),
	SN8x2 = ( SIZE_2 | TYPE_SBYTE | IS_NORMALIZED ),
	SN8x3 = ( SIZE_3 | TYPE_SBYTE | IS_NORMALIZED ),
	SN8x4 = ( SIZE_4 | TYPE_SBYTE | IS_NORMALIZED ),
	UN16x1 = ( SIZE_1 | TYPE_USHORT | IS_NORMALIZED ),
	UN16x2 = ( SIZE_2 | TYPE_USHORT | IS_NORMALIZED ),
	UN16x3 = ( SIZE_3 | TYPE_USHORT | IS_NORMALIZED ),
	UN16x4 = ( SIZE_4 | TYPE_USHORT | IS_NORMALIZED ),
	SN16x1 = ( SIZE_1 | TYPE_SSHORT | IS_NORMALIZED ),
	SN16x2 = ( SIZE_2 | TYPE_SSHORT | IS_NORMALIZED ),
	SN16x3 = ( SIZE_3 | TYPE_SSHORT | IS_NORMALIZED ),
	SN16x4 = ( SIZE_4 | TYPE_SSHORT | IS_NORMALIZED ),
	UN32x1 = ( SIZE_1 | TYPE_UINT | IS_NORMALIZED ),
	UN32x2 = ( SIZE_2 | TYPE_UINT | IS_NORMALIZED ),
	UN32x3 = ( SIZE_3 | TYPE_UINT | IS_NORMALIZED ),
	UN32x4 = ( SIZE_4 | TYPE_UINT | IS_NORMALIZED ),
	SN32x1 = ( SIZE_1 | TYPE_SINT | IS_NORMALIZED ),
	SN32x2 = ( SIZE_2 | TYPE_SINT | IS_NORMALIZED ),
	SN32x3 = ( SIZE_3 | TYPE_SINT | IS_NORMALIZED ),
	SN32x4 = ( SIZE_4 | TYPE_SINT | IS_NORMALIZED )
};
struct AttributeDescriptor {
	std::string SemanticName;	// Only relevnat for D3D afaik
	unsigned SemanticIndex;	// Only relevnat for D3D afaik
	PrimitiveType type; // Maps to a DXGI_FORMAT in D3D, to a VkFormat in
			// Vulkan, is split into GLenum (type), GLint (size) and
			// GLboolean (normalized) in OpenGL
	uintptr_t offset; // Converted to uint32_t for Vulkan, void* for OpenGL, UINT
		   // for D3D
};
struct VertexDescriptor {
	size_t stride; // The size of a whole vertex.
	std::span<const AttributeDescriptor> descriptors;
};


}
}
#endif // MHVERTEXFORMAT_H
