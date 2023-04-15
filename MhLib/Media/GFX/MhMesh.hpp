#ifndef MHMESH_HPP
#define MHMESH_HPP
#include <MhLib/Media/GFX/MhGFXHandle.hpp>
#include <MhLib/Media/GFX/MhVertexFormat.hpp>
#include <functional>
namespace MH33 {
namespace GFX {

DEFINE_CLASS(UnindexedMesh)
class UnindexedMesh
{
public:
	virtual ~UnindexedMesh() = default;
	virtual size_t getVertexCount() const = 0;
	virtual Handle getNativeHandle() = 0;
	virtual ConstHandle getNativeHandle() const = 0;
	virtual const VertexDescriptor* getVertexDescriptor() const = 0;
	virtual void access(const std::function<void(void*,size_t)>& accessor) = 0;
	virtual void access(const std::function<void(const void*,size_t)>& accessor) const = 0;
	virtual void ensureSize(size_t count) = 0;
};
DEFINE_CLASS(IndexedMesh)
class IndexedMesh
{
public:
	virtual ~IndexedMesh() = default;
	virtual size_t getVertexCount() const = 0;
	virtual Handle getVertexNativeHandle() = 0;
	virtual ConstHandle getVertexNativeHandle() const = 0;
	virtual size_t getIndexCount() const = 0;
	virtual Handle getIndexNativeHandle() = 0;
	virtual ConstHandle getIndexNativeHandle() const = 0;
	virtual const VertexDescriptor* getVertexDescriptor() const = 0;
	virtual void accessVertices(const std::function<void(void*,size_t)>& accessor) = 0;
	virtual void accessVertices(const std::function<void(const void*,size_t)>& accessor) const = 0;
	virtual void accessIndices(const std::function<void(std::span<uint32_t>&)>& accessor) = 0;
	virtual void accessIndices(const std::function<void(const std::span<const uint32_t>&)>& accessor) const = 0;
	virtual void ensureVertexCount(size_t count) = 0;
	virtual void ensureIndexCount(size_t count) = 0;
};

}
}
#endif // MHMESH_HPP
