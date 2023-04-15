#ifndef SOFTMESH_HPP
#define SOFTMESH_HPP
#include <MhLib/Media/GFX/MhMesh.hpp>
#include <vector>
#include <span>
#include <mutex>

namespace Renderer {
namespace Software {

DEFINE_CLASS(UnindexedMesh)
class UnindexedMesh : public MH33::GFX::UnindexedMesh
{
private:
	size_t vertexCount;
	std::vector<std::byte> buffer;
	const MH33::GFX::VertexDescriptor* vertexDescriptor;
	mutable std::mutex mutex;
public:
	UnindexedMesh(const MH33::GFX::VertexDescriptor* vertexDescriptor);
	UnindexedMesh(const MH33::GFX::VertexDescriptor* vertexDescriptor, const std::span<const std::byte>& bytes, size_t vertexCount);
	size_t getVertexCount() const override;
	MH33::GFX::Handle getNativeHandle() override;
	MH33::GFX::ConstHandle getNativeHandle() const override;
	const MH33::GFX::VertexDescriptor* getVertexDescriptor() const override;
	void access(const std::function<void (void*, size_t)>& accessor) override;
	void access(const std::function<void (const void*, size_t)>& accessor) const override;
	void ensureSize(size_t count) override;
};
DEFINE_CLASS(IndexedMesh)
class IndexedMesh : public MH33::GFX::IndexedMesh
{
private:
	size_t vertexCount;
	size_t indexCount; // Kept separate because we may expand the buffer beyond what we're actually using
	std::vector<std::byte> vertexBuffer;
	std::vector<uint32_t> indexBuffer;
	const MH33::GFX::VertexDescriptor* vertexDescriptor;
	mutable std::mutex mutex;
public:
	IndexedMesh(const MH33::GFX::VertexDescriptor* vertexDescriptor);
	IndexedMesh(const MH33::GFX::VertexDescriptor* vertexDescriptor, const std::span<const std::byte>& bytes, size_t vertexCount, const std::span<const uint32_t>& indices);
	size_t getVertexCount() const override;
	MH33::GFX::Handle getVertexNativeHandle() override;
	MH33::GFX::ConstHandle getVertexNativeHandle() const override;
	size_t getIndexCount() const override;
	MH33::GFX::Handle getIndexNativeHandle() override;
	MH33::GFX::ConstHandle getIndexNativeHandle() const override;
	const MH33::GFX::VertexDescriptor* getVertexDescriptor() const override;
	void accessVertices(const std::function<void (void*, size_t)>& accessor) override;
	void accessVertices(const std::function<void (const void*, size_t)>& accessor) const override;
	void accessIndices(const std::function<void (std::span<uint32_t>&)>& accessor) override;
	void accessIndices(const std::function<void (const std::span<const uint32_t>&)>& accessor) const override;
	void ensureVertexCount(size_t count) override;
	void ensureIndexCount(size_t count) override;
};

}
}

#endif // SOFTMESH_HPP
