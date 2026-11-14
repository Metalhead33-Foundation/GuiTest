#ifndef LTEGUIRENDERCACHE_HPP
#define LTEGUIRENDERCACHE_HPP
/**
 * @file LteGuiRenderCache.hpp
 * @brief CPU-side geometry batches that record Kaldi draw commands.
 *
 * Salvaged from `GFX/Advanced/MhHardwareAcceleratedRenderCache.hpp` on the
 * `NewBeginning` branch, which defined `CachedPipeline`, `CachedPipelineWithTexture`,
 * `IndexedCachedPipeline` and `IndexedCachedPipelineWithTexture` - four near-identical
 * templates that each owned a vertex buffer, an index buffer, a CPU cache and a
 * texture pointer, and offered `commitData()` / `clearData()` / `draw()`.
 *
 * This version keeps the useful part of that idea and drops the rest:
 *
 * - **One template, not four.** The variants differed only in whether they were
 *   indexed and whether they bound a texture. Indexing is a property of the
 *   batch (all GUI geometry is indexed: 4 vertices + 6 indices per quad), and
 *   texture binding belongs to the draw call, not to the vertex storage. So there
 *   is a single @ref Lotte::Gui::GuiGeometryBatch.
 * - **RAII buffers.** The buffers are `Lotte::Buffer` proxies, so they record
 *   their own destroy command when the batch is destroyed; the old caches held a
 *   raw handle and required manual cleanup.
 * - **Recording, not drawing.** `recordUpload()` and `recordDraw()` append
 *   commands to a caller-owned `CommandBuffer`, which is how Kaldi works. The old
 *   `draw()` called into a device immediately, the coupling that made the original
 *   renderer untestable.
 * - **`primitiveCount` became `Range`.** The old caches stored one
 *   `primitiveCount` and always drew from index 0. A range lets one batch hold
 *   several sub-draws (one per texture or pipeline state) without re-uploading.
 *
 * @warning The GPU path is untested: there is no working Kaldi backend yet. The
 *          command sequence is written against the HAL contract in
 *          `Kaldi/KaldiOperation.hpp` and will be validated with the first
 *          backend. The CPU side (caches, ranges, index generation) is complete.
 */

#include <Lotte/LotteLib.hpp>
#include <Lotte/LteKld.hpp>
#include <Kaldi/KldCommandBuffer.hpp>
#include <cstdint>
#include <optional>
#include <vector>

namespace Lotte {
namespace Gui {

/**
 * @brief Vertex/index cache that records upload and draw commands for Kaldi.
 * @tparam VertexType One of the `Lotte::Gui` vertex structs.
 *
 * @section lte_gui_batch_usage Usage
 *
 * 1. Append geometry into @ref getVertices() and @ref getIndices() while building
 *    a frame (use @ref appendQuad() for the common case).
 * 2. Call @ref recordUpload() once per frame, before the first draw that uses the
 *    batch.
 * 3. Call @ref recordBind() and @ref recordDraw() whenever the geometry must be
 *    submitted.
 * 4. Call @ref clear() when the CPU cache is consumed; the GPU buffers stay
 *    allocated and are reused next frame.
 */
template <typename VertexType>
class GuiGeometryBatch
{
public:
	/** @brief Sub-range of the uploaded index buffer. */
	struct Range {
		/** @brief First index to draw. */
		std::uint32_t indexOffset = 0;
		/** @brief Number of indices to draw. */
		std::uint32_t indexCount = 0;
		/** @brief Vertex offset added to every index. */
		std::uint32_t vertexOffset = 0;
	};
private:
	std::optional<Buffer> vertexBuffer;
	std::optional<Buffer> indexBuffer;
	std::uint32_t vertexCapacityBytes = 0;
	std::uint32_t indexCapacityBytes = 0;
	Range resident;
	std::vector<VertexType> vertexCache;
	std::vector<std::uint32_t> indexCache;
public:
	/** @brief Creates an empty batch with no GPU buffers. */
	GuiGeometryBatch() = default;

	GuiGeometryBatch(const GuiGeometryBatch&) = delete;
	GuiGeometryBatch& operator=(const GuiGeometryBatch&) = delete;
	GuiGeometryBatch(GuiGeometryBatch&&) = default;
	GuiGeometryBatch& operator=(GuiGeometryBatch&&) = default;

	/** @brief Returns the CPU vertex cache. */
	std::vector<VertexType>& getVertices() { return vertexCache; }
	/** @brief Returns the CPU vertex cache. */
	const std::vector<VertexType>& getVertices() const { return vertexCache; }
	/** @brief Returns the CPU index cache. */
	std::vector<std::uint32_t>& getIndices() { return indexCache; }
	/** @brief Returns the CPU index cache. */
	const std::vector<std::uint32_t>& getIndices() const { return indexCache; }

	/** @brief Returns true when there is nothing to upload. */
	bool empty() const { return vertexCache.empty() || indexCache.empty(); }

	/** @brief Drops the CPU caches and forgets the uploaded range. */
	void clear()
	{
		vertexCache.clear();
		indexCache.clear();
		resident = Range{};
	}

	/**
	 * @brief Appends a quad and its two triangles.
	 * @param topLeft Top-left corner vertex.
	 * @param topRight Top-right corner vertex.
	 * @param bottomLeft Bottom-left corner vertex.
	 * @param bottomRight Bottom-right corner vertex.
	 */
	void appendQuad(const VertexType& topLeft, const VertexType& topRight,
					const VertexType& bottomLeft, const VertexType& bottomRight)
	{
		const std::uint32_t base = std::uint32_t(vertexCache.size());
		vertexCache.push_back(topLeft);
		vertexCache.push_back(topRight);
		vertexCache.push_back(bottomLeft);
		vertexCache.push_back(bottomRight);
		indexCache.push_back(base + 0);
		indexCache.push_back(base + 1);
		indexCache.push_back(base + 2);
		indexCache.push_back(base + 1);
		indexCache.push_back(base + 2);
		indexCache.push_back(base + 3);
	}

	/**
	 * @brief Appends a line segment as two vertices and two indices.
	 * @param from First endpoint.
	 * @param to Second endpoint.
	 */
	void appendLine(const VertexType& from, const VertexType& to)
	{
		const std::uint32_t base = std::uint32_t(vertexCache.size());
		vertexCache.push_back(from);
		vertexCache.push_back(to);
		indexCache.push_back(base + 0);
		indexCache.push_back(base + 1);
	}

	/**
	 * @brief Records buffer creation/resize/upload for the current CPU caches.
	 * @param commands Dispatcher that receives the commands.
	 * @note The command payload points at the CPU cache, so the cache must stay
	 *       alive and unmodified until the backend consumes the queue.
	 */
	void recordUpload(CommandBuffer& commands)
	{
		if (empty()) return;
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

		resident = Range{ 0, std::uint32_t(indexCache.size()), 0 };
	}

	/**
	 * @brief Records the vertex and index buffer bindings.
	 * @param commands Dispatcher whose buffers are being bound.
	 * @param binding Vertex buffer binding slot.
	 */
	void recordBind(CommandBuffer& commands, std::uint32_t binding = 0) const
	{
		(void)commands;
		if (vertexBuffer) vertexBuffer->bindVertex(binding, 0, std::uint32_t(sizeof(VertexType)));
		if (indexBuffer) indexBuffer->bindIndex(0, Kld::IndexType::Uint32);
	}

	/**
	 * @brief Records an indexed triangle draw.
	 * @param commands Dispatcher that receives the command.
	 * @param range Index range to draw; defaults to everything that was uploaded.
	 */
	void recordDraw(CommandBuffer& commands, Range range = Range{}) const
	{
		const Range effective = (range.indexCount || range.vertexOffset) ? range : resident;
		if (!vertexBuffer || !indexBuffer || !effective.indexCount) return;
		KldOopDetail::drawIndexedTriangles(commands, effective.indexOffset, effective.indexCount, effective.vertexOffset);
	}

	/**
	 * @brief Records an indexed line draw.
	 * @param commands Dispatcher that receives the command.
	 * @param thickness Line width in pixels.
	 * @param range Index range to draw; defaults to everything that was uploaded.
	 */
	void recordDrawLines(CommandBuffer& commands, float thickness = 1.0f, Range range = Range{}) const
	{
		const Range effective = (range.indexCount || range.vertexOffset) ? range : resident;
		if (!vertexBuffer || !indexBuffer || !effective.indexCount) return;
		KldOopDetail::drawIndexedLines(commands, effective.indexOffset, effective.indexCount, effective.vertexOffset, thickness);
	}

	/** @brief Returns the range that is currently resident on the GPU. */
	const Range& getResidentRange() const { return resident; }
	/** @brief Returns true when GPU buffers exist for this batch. */
	bool isUploaded() const { return vertexBuffer.has_value() && indexBuffer.has_value(); }
	/** @brief Returns the vertex buffer handle, or 0 when none exists. */
	Kld::HandleId getVertexBufferHandle() const { return vertexBuffer ? vertexBuffer->id() : Kld::HandleId(0); }
	/** @brief Returns the index buffer handle, or 0 when none exists. */
	Kld::HandleId getIndexBufferHandle() const { return indexBuffer ? indexBuffer->id() : Kld::HandleId(0); }
};

}
}

#endif // LTEGUIRENDERCACHE_HPP
