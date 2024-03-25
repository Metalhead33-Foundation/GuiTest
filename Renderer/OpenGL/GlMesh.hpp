#ifndef MESH_HPP
#define MESH_HPP
#include <MhLib/Media/GFX/MhMesh.hpp>
#include <GL/GlBuffer.hpp>
#include <GL/GlVAO.hpp>
namespace GL {

class UnindexedMesh : public MH33::GFX::UnindexedMesh
{
private:
	mutable Buffer buffer;
	Vao vao;
	const MH33::GFX::VertexDescriptor* descriptor;
public:
	UnindexedMesh(const MH33::GFX::VertexDescriptor* descriptor, bool streaming);
	size_t getVertexCount() const override;
	MH33::GFX::Handle getNativeHandle() override;
	MH33::GFX::ConstHandle getNativeHandle() const override;
	const MH33::GFX::VertexDescriptor* getVertexDescriptor() const override;
	void access(const std::function<void (void*, size_t)>& accessor) override;
	void access(const std::function<void (const void*, size_t)>& accessor) const override;
	void ensureSize(size_t count) override;
	const Buffer& getBuffer() const;
	Buffer& getBuffer();
	const Vao& getVao() const;
	Vao& getVao();
};

class IndexedMesh : public MH33::GFX::IndexedMesh
{
private:
	mutable Buffer vertexBuffer, indexBuffer;
	Vao vao;
	const MH33::GFX::VertexDescriptor* descriptor;
public:
	IndexedMesh(const MH33::GFX::VertexDescriptor* descriptor, bool streaming);
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
	const Buffer& getVertexBuffer() const;
	const Buffer& getIndexBuffer() const;
	const Vao& getVao() const;
	Buffer& getVertexBuffer();
	Buffer& getIndexBuffer();
	Vao& getVao();
};

}
#endif // MESH_HPP
