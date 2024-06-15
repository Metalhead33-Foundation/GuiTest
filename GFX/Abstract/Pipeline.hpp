#ifndef PIPELINE_H
#define PIPELINE_H
#include <MhLib/Util/MhGlobals.hpp>
#include "Texture.hpp"
#include "UniformBuffer.hpp"
#include "VertexBuffer.hpp"
namespace MH33 {
namespace GFX {

enum class RenderType {
	POINTS,
	LINES,
	LINES_ADJACENCY,
	LINE_STRIPS,
	LINE_STRIP_ADJACENCY,
	TRIANGLES,
	TRIANGLES_ADJACENCY,
	TRIANGLE_STRIP,
	TRIANGLE_STRIP_ADJACENCY,
	TRIANGLE_FAN,
	PATCHES
};

enum class ShaderModuleType {
	VERTEX_SHADER,
	GEOMETRY_SHADER,
	TESSELLATION_CONTROL_SHADER,
	TESSELLATION_EVALUATION_SHADER,
	PIXEL_SHADER,
	COMPUTE_SHADER
};

struct ShaderModuleCreateInfo {
	ShaderModuleType shaderType;
	std::vector<std::byte> source;
};
struct ShaderModuleCreateInfoRef {
	ShaderModuleType shaderType;
	std::span<std::byte> source;
};

DEFINE_CLASS(Pipeline)
class Pipeline : public GfxResource {
public:
	virtual ~Pipeline() = default;
	virtual void bind() const = 0;
	virtual unsigned getBindingPoint(const std::string& uniformName) const = 0;
	virtual unsigned getUniformBlockIndex(const std::string& uniformName) const = 0;
	virtual void setUniform(unsigned bindingPoint, StorageBuffer& buffer, uint8_t unit) const = 0;
	virtual void setUniform(unsigned bindingPoint, Texture2D& texture, uint8_t unit) const = 0;
	virtual void setUniform(unsigned bindingPoint, TextureArray2D& texture, uint8_t unit) const = 0;
	virtual void setUniform(unsigned bindingPoint, Texture3D& texture, uint8_t unit) const = 0;
	virtual void setUniform(unsigned bindingPoint, Cubemap& texture, uint8_t unit) const = 0;
	virtual void draw(UnindexedVertexBuffer& vertices, RenderType geometryType, size_t offset = 0, size_t count = 0) = 0;
	virtual void draw(IndexedVertexBuffer& vertices, RenderType geometryType, size_t offset = 0, size_t count = 0) = 0;
	virtual void drawInstanced(uint32_t instances, UnindexedVertexBuffer& vertices, RenderType geometryType, size_t offset, size_t count) = 0;
	virtual void drawInstanced(uint32_t instances, IndexedVertexBuffer& vertices, RenderType geometryType, size_t count) = 0;
};

}
}
#endif // PIPELINE_H
