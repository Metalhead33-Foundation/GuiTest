#ifndef MHPIPELINE_HPP
#define MHPIPELINE_HPP
#include <MhLib/Media/GFX/MhMesh.hpp>
#include <MhLib/Media/GFX/MhTexture.hpp>
#include <MhLib/Media/GFX/MhUniformBuffer.hpp>
#include <MhLib/Media/GFX/MhMesh.hpp>
namespace MH33 {
namespace GFX {

DEFINE_CLASS(Pipeline)
class Pipeline {
public:
	virtual ~Pipeline() = default;
	virtual Handle getNativeHandle() = 0;
	virtual ConstHandle getNativeHandle() const = 0;
	virtual void setTexture(int id, const Texture2D& texture) = 0;
	virtual void setTexture(int id, const Texture3D& texture) = 0;
	virtual void setTexture(int id, const ArrayTexture2D& texture) = 0;
	virtual void setUniformBuffer(int id, const UniformBuffer& texture) = 0;
	virtual int getId(const std::string& uniformName) = 0;
	virtual void drawTriangles(UnindexedMesh& mesh) = 0;
	virtual void drawTriangles(IndexedMesh& mesh) = 0;
	virtual void drawLines(UnindexedMesh& mesh, float thickness = 1.0f) = 0;
	virtual void drawLines(IndexedMesh& mesh, float thickness = 1.0f) = 0;
};

}
}
#endif // MHPIPELINE_HPP
