#ifndef KALDIOPERATION_HPP
#define KALDIOPERATION_HPP
#include <cstdint>
#include <Euphemy/Media/Image/EuphImageType.hpp>
#include <Kaldi/KldVertexFormat.hpp>
namespace Kld {

typedef Euph::Media::Image::Format TextureFormat;

typedef uint32_t HandleId;

enum class Opcode : uint16_t {
	CreateBufferObject,
	DestroyBufferObject,
	CreateVertexArrayObject,
	CreateTexture1D,
	CreateTexture2D,
	CreateTexture3D,
	DestroyTexture,
	CreateSampler,
	DestroySampler
	// More to come later!
};
struct OpDestroy {
	HandleId id; // Just a dumb id.
};
struct OpCreateBufferObject {
	HandleId id; // Preallocated by the backend
	uint32_t size; // In bytes
	uint32_t policy;
	// Reserved for now. Could be made similar to the [GLenum target]
	// - e.g. GL_ARRAY_BUFFER, GL_UNIFORM_BUFFER, GL_ELEMENT_ARRAY_BUFFER, etc. -
	// and [GLenum usage]
	// - eg  GL_STREAM_DRAW, GL_STATIC_DRAW, GL_DYNAMIC_COPY, etc. -
	// parameters of glBufferData
	// Unsure about D3D and Vulkan interpretations.
	const void* data; // nullptr if you just want to reserve space
};
struct OpCreateVertexArrayObject {
	HandleId id; // Preallocated by the backend
	const VertexDescriptor* descriptor;
	uint32_t elementCount;
};
struct OpCreateTexture1D {
	HandleId id; // Preallocated by the backend
	TextureFormat format;
	uint32_t width;
	const void* pixels; // nullptr if you just want to reserve space
	// Also, be very careful - because we execute commands asychronously, you must ensure that pointer remains valid until the command is executed
	// Maybe I'll look for a workaround in the future, or force syncing and return promises.
};
struct OpCreateTexture2D {
	HandleId id; // Preallocated by the backend
	TextureFormat format;
	uint8_t mipLevels;
	bool generateMipmaps; // If TRUE, then mipLevels is the number of mipmaps to generate. If FALSE, mipLevels is the number of mipmaps already generated.
	uint16_t width;
	uint16_t height;
	const void* pixels; // nullptr if you just want to reserve space
	// Same problems as with OpCreateTexture1D
};
struct OpCreateTexture3D {
	HandleId id; // Preallocated by the backend
	TextureFormat format;
	bool generateMipmaps;
	uint16_t width;
	uint16_t height;
	uint16_t depth;
	const void* pixels; // nullptr if you just want to reserve space
	// Same problems as with OpCreateTexture1D
};
enum class SamplerWrapping : uint8_t {
	CLAMP_TO_EDGE,
	CLAMP_TO_BORDER,
	MIRRORED_REPEAT,
	REPEAT
};
enum class SamplerFiltering : uint8_t {
	NEAREST,
	LINEAR,
	NEAREST_MIPMAP_NEAREST,
	LINEAR_MIPMAP_NEAREST,
	NEAREST_MIPMAP_LINEAR,
	LINEAR_MIPMAP_LINEAR
};
struct OpCreateSampler {
	HandleId id; // Preallocated by the backend
	SamplerFiltering filtering;
	SamplerWrapping wrapping[3];
	float maxAnisotropy;
	float maxLod;
	float minLod;
	float lodBias;
};

struct GfxOp {
	Opcode opcode;
	union {
		OpDestroy opDestroy;
		OpCreateBufferObject opCreateBufferObject;
		OpCreateVertexArrayObject opCreateVertexArrayObject;
		OpCreateTexture1D opCreateTexture1D;
		OpCreateTexture2D opCreateTexture2D;
		OpCreateTexture3D opCreateTexture3D;
		OpCreateSampler opCreateSampler;
		// More to come later!
	} opt;
};

}
#endif // KALDIOPERATION_HPP
