#ifndef LTEMESH_HPP
#define LTEMESH_HPP
/**
 * @file LteMesh.hpp
 * @brief Higher-level mesh concept over Kaldi vertex and index buffers.
 *
 * Salvaged from `MhLib/Media/GFX/MhMesh.hpp` on the `NewBeginning` branch, which
 * defined abstract `UnindexedMesh`/`IndexedMesh` classes with
 * `getVertexCount()`, `getNativeHandle()`, `ensureSize()`, and
 * `access(std::function<void(void*,size_t)>)` upload callbacks. That design
 * existed because every backend owned its own buffer object; Kaldi already
 * models buffers as handles plus `OpCreateBufferObject` / `OpUpdateBufferObject`,
 * so the same concept needs far less machinery:
 *
 * - @ref Lotte::Gfx::MeshView is the mesh *concept*: two buffer handles, the
 *   vertex layout, element counts and index width. It is a plain aggregate,
 *   trivially copyable, and owns nothing - which is what a renderer or a
 *   command-list builder wants to hold.
 * - @ref Lotte::Gfx::MeshBuilder is the builder *and* owner: it keeps the CPU
 *   caches, names draw groups (@ref Lotte::Gfx::MeshDrawRange), records upload
 *   commands, and owns the Kaldi buffer proxies so they are destroyed through
 *   RAII.
 *
 * The `access(std::function<...>)` indirection is gone: uploads are recorded
 * from a `std::vector` cache, and the callback only made sense while buffers were
 * mapped directly. Buffer mapping remains available in Kaldi
 * (`OpMapBufferObject`) when a mesh has to be filled from a worker thread.
 *
 * Command payloads point into the CPU caches, so a cache must stay alive and
 * unchanged until the backend has consumed the recorded commands - the same rule
 * the whole Kaldi facade follows.
 */

#include <Lotte/LotteLib.hpp>
#include <Lotte/LteKld.hpp>
#include <Kaldi/KldCommandBuffer.hpp>
#include <Kaldi/KldVertexFormat.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace Lotte {
namespace Gfx {

/** @brief A contiguous draw range inside a mesh's index buffer. */
struct MeshDrawRange {
	/** @brief First index to draw. */
	std::uint32_t indexOffset = 0;
	/** @brief Number of indices to draw. */
	std::uint32_t indexCount = 0;
	/** @brief Vertex offset added to every index. */
	std::uint32_t vertexOffset = 0;
	/** @brief Number of instances for instanced draws. */
	std::uint32_t instanceCount = 1;
};

/** @brief Non-owning view of a GPU-resident mesh. */
struct MeshView {
	/** @brief Vertex buffer handle. */
	Kld::HandleId vertexBuffer = 0;
	/** @brief Index buffer handle. */
	Kld::HandleId indexBuffer = 0;
	/** @brief Vertex layout the buffers were filled with. */
	const Kld::VertexDescriptor* layout = nullptr;
	/** @brief Number of vertices in the vertex buffer. */
	std::uint32_t vertexCount = 0;
	/** @brief Number of indices in the index buffer. */
	std::uint32_t indexCount = 0;
	/** @brief Width of one index. */
	Kld::IndexType indexType = Kld::IndexType::Uint32;

	/** @brief Returns true when the view can be bound and drawn. */
	bool isValid() const { return vertexBuffer && indexBuffer && layout; }
};

/**
 * @brief Records the buffer bindings of a mesh.
 * @param commands Dispatcher that receives the commands.
 * @param mesh Mesh to bind.
 * @param binding Vertex buffer binding slot.
 */
inline void recordBind(CommandBuffer& commands, const MeshView& mesh, std::uint32_t binding = 0)
{
	commands.push({ Kld::Opcode::BindVertexBuffer, { .opBindVertexBuffer = {
		binding, mesh.vertexBuffer, 0, std::uint32_t(mesh.layout ? mesh.layout->stride : 0), 0 } } });
	commands.push({ Kld::Opcode::BindIndexBuffer, { .opBindIndexBuffer = { mesh.indexBuffer, 0, mesh.indexType } } });
}

/**
 * @brief Records one indexed draw of a mesh range.
 * @param commands Dispatcher that receives the command.
 * @param mesh Mesh to draw; must already be bound.
 * @param range Range to draw.
 */
inline void recordDraw(CommandBuffer& commands, const MeshView& mesh, const MeshDrawRange& range)
{
	if (!mesh.isValid() || !range.indexCount) return;
	KldOopDetail::drawIndexedTriangles(commands, range.indexOffset, range.indexCount, range.vertexOffset);
}

/**
 * @brief Records one instanced indexed draw of a mesh range.
 * @param commands Dispatcher that receives the command.
 * @param mesh Mesh to draw; must already be bound.
 * @param range Range to draw.
 * @param firstInstance Index of the first instance.
 */
inline void recordDrawInstanced(CommandBuffer& commands, const MeshView& mesh, const MeshDrawRange& range,
								std::uint32_t firstInstance = 0)
{
	if (!mesh.isValid() || !range.indexCount) return;
	KldOopDetail::drawIndexedTrianglesInstanced(commands, range.indexOffset, range.indexCount,
												range.vertexOffset, range.instanceCount, firstInstance);
}

/**
 * @brief Builds and owns a mesh, recording its upload commands.
 * @tparam VertexType Vertex struct exposing `getVertexDescriptor()`.
 *
 * @section lte_mesh_builder_usage Usage
 *
 * 1. Append vertices and indices into @ref getVertices() / @ref getIndices()
 *    (or use @ref appendVertices() / @ref appendIndices()).
 * 2. Call @ref endGroup() after each drawable group to record a
 *    @ref Lotte::Gfx::MeshDrawRange.
 * 3. Call @ref recordUpload() once, then @ref recordBind() and
 *    @ref recordDraw() per group.
 * 4. @ref clear() drops the CPU caches but keeps the GPU buffers for reuse.
 */
template <typename VertexType>
class MeshBuilder
{
private:
	std::optional<Buffer> vertexBuffer;
	std::optional<Buffer> indexBuffer;
	std::uint32_t vertexCapacityBytes = 0;
	std::uint32_t indexCapacityBytes = 0;
	std::vector<VertexType> vertexCache;
	std::vector<std::uint32_t> indexCache;
	std::vector<MeshDrawRange> ranges;
	std::uint32_t groupIndexStart = 0;
	std::uint32_t groupVertexStart = 0;
public:
	/** @brief Returns the Kaldi layout of the vertex type. */
	static const Kld::VertexDescriptor& getVertexDescriptor() { return VertexType::getVertexDescriptor(); }

	/** @brief Returns the CPU vertex cache. */
	std::vector<VertexType>& getVertices() { return vertexCache; }
	/** @brief Returns the CPU vertex cache. */
	const std::vector<VertexType>& getVertices() const { return vertexCache; }
	/** @brief Returns the CPU index cache. */
	std::vector<std::uint32_t>& getIndices() { return indexCache; }
	/** @brief Returns the CPU index cache. */
	const std::vector<std::uint32_t>& getIndices() const { return indexCache; }
	/** @brief Returns the draw groups recorded so far. */
	const std::vector<MeshDrawRange>& getRanges() const { return ranges; }

	/**
	 * @brief Appends vertices and returns the index of the first appended one.
	 * @param vertices Vertex data to copy.
	 * @param count Number of vertices.
	 * @return Base vertex index, for use when appending indices.
	 */
	std::uint32_t appendVertices(const VertexType* vertices, std::size_t count)
	{
		const std::uint32_t base = std::uint32_t(vertexCache.size());
		vertexCache.insert(vertexCache.end(), vertices, vertices + count);
		return base;
	}

	/**
	 * @brief Appends indices.
	 * @param indices Index data to copy; the caller applies the vertex base.
	 * @param count Number of indices.
	 */
	void appendIndices(const std::uint32_t* indices, std::size_t count)
	{
		indexCache.insert(indexCache.end(), indices, indices + count);
	}

	/**
	 * @brief Closes the current draw group.
	 * @return The recorded range; empty when nothing was appended since the last call.
	 */
	MeshDrawRange endGroup()
	{
		MeshDrawRange range;
		range.indexOffset = groupIndexStart;
		range.indexCount = std::uint32_t(indexCache.size()) - groupIndexStart;
		range.vertexOffset = groupVertexStart;
		groupIndexStart = std::uint32_t(indexCache.size());
		groupVertexStart = std::uint32_t(vertexCache.size());
		if (range.indexCount) ranges.push_back(range);
		return range;
	}

	/**
	 * @brief Records creation/resize/upload of the current CPU caches.
	 * @param commands Dispatcher that receives the commands.
	 */
	void recordUpload(CommandBuffer& commands)
	{
		if (vertexCache.empty() || indexCache.empty()) return;
		const std::uint32_t vertexBytes = std::uint32_t(vertexCache.size() * sizeof(VertexType));
		const std::uint32_t indexBytes = std::uint32_t(indexCache.size() * sizeof(std::uint32_t));

		if (!vertexBuffer) {
			vertexBuffer.emplace(KldOopDetail::createBuffer(commands, vertexBytes, Kld::BUFFER_USAGE_VERTEX, vertexCache.data()));
			vertexCapacityBytes = vertexBytes;
		} else {
			if (vertexBytes > vertexCapacityBytes) {
				vertexBuffer->resize(vertexBytes);
				vertexCapacityBytes = vertexBytes;
			}
			vertexBuffer->update(0, vertexBytes, vertexCache.data());
		}

		if (!indexBuffer) {
			indexBuffer.emplace(KldOopDetail::createBuffer(commands, indexBytes, Kld::BUFFER_USAGE_INDEX, indexCache.data()));
			indexCapacityBytes = indexBytes;
		} else {
			if (indexBytes > indexCapacityBytes) {
				indexBuffer->resize(indexBytes);
				indexCapacityBytes = indexBytes;
			}
			indexBuffer->update(0, indexBytes, indexCache.data());
		}
	}

	/** @brief Returns a view of the uploaded mesh. */
	MeshView getMeshView() const
	{
		MeshView view;
		if (vertexBuffer) view.vertexBuffer = vertexBuffer->id();
		if (indexBuffer) view.indexBuffer = indexBuffer->id();
		view.layout = &getVertexDescriptor();
		view.vertexCount = std::uint32_t(vertexCache.size());
		view.indexCount = std::uint32_t(indexCache.size());
		return view;
	}

	/** @brief Records the mesh bindings. */
	void recordBind(CommandBuffer& commands, std::uint32_t binding = 0) const
	{
		const MeshView view = getMeshView();
		if (view.isValid()) Gfx::recordBind(commands, view, binding);
	}

	/**
	 * @brief Records the draw of one group.
	 * @param commands Dispatcher that receives the command.
	 * @param range Group to draw.
	 */
	void recordDraw(CommandBuffer& commands, const MeshDrawRange& range) const
	{
		const MeshView view = getMeshView();
		if (view.isValid()) Gfx::recordDraw(commands, view, range);
	}

	/** @brief Records every group in order. */
	void recordDrawAll(CommandBuffer& commands) const
	{
		for (const MeshDrawRange& range : ranges) recordDraw(commands, range);
	}

	/** @brief Drops the CPU caches and recorded groups; GPU buffers stay allocated. */
	void clear()
	{
		vertexCache.clear();
		indexCache.clear();
		ranges.clear();
		groupIndexStart = 0;
		groupVertexStart = 0;
	}

	/** @brief Returns true when GPU buffers exist for this mesh. */
	bool isUploaded() const { return vertexBuffer.has_value() && indexBuffer.has_value(); }
};

}
}

#endif // LTEMESH_HPP

