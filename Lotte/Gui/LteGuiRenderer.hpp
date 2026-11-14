#ifndef LTEGUIRENDERER_HPP
#define LTEGUIRENDERER_HPP
/**
 * @file LteGuiRenderer.hpp
 * @brief The GUI draw-command vocabulary and the renderer interface Lotte expects.
 *
 * Salvaged from `GUI/MhGuiRenderer.hpp` and
 * `GFX/Advanced/MhHardwareAcceleratedGuiRenderer.{hpp,cpp}` on the
 * `NewBeginning` branch. Both were an abstract `Renderer` with one `render()`
 * overload per primitive plus a `flush()`, and both were implemented by batching
 * geometry per (pipeline, texture) and flushing when that state changed.
 *
 * What is salvaged: the primitive vocabulary (widgets produce these, so it is a
 * useful contract) and the batching *policy* documented below.
 *
 * What is not salvaged: the implementation. That version hard-coded four cache
 * members, a `lastRenderType`/`lastTexture` state machine, and a
 * `uintptr_t texturePtr` that it `reinterpret_cast` to a texture object - which
 * both hid the type and made the renderer impossible to use without also having
 * its exact texture class. The reusable pieces live in
 * `LteGuiRenderCache.hpp` (@ref Lotte::Gui::GuiGeometryBatch) and
 * `LteGuiVertex.hpp`.
 *
 * @section lte_gui_renderer_missing What a concrete renderer still has to do
 *
 * 1. Map each primitive onto a batch: quads and triangles into a coloured
 *    triangle batch, textured variants into a textured triangle batch, lines into
 *    a line batch.
 * 2. Sort or group submissions by pipeline state and texture handle, because a
 *    flush is required on every state change. The original flushed on every
 *    change of render type or texture and then re-drew the batches in a fixed
 *    order, which loses the caller's ordering; a layer/order key fixes that.
 * 3. Own the pipelines, samplers and the viewport/scissor setup, and record
 *    `Kld::OpBindPipeline`/`OpSetViewport` around the draws.
 * 4. Convert gradient primitives: `Kld::OpDrawTriangles` takes no per-corner
 *    colour, so gradient variants are expanded into per-vertex colours before
 *    upload (`ColouredTexturedGuiVertex` already carries a colour).
 * 5. Round-trip text: `Lotte::Text::TextLayout` + `appendGlyphQuads` +
 *    `Lotte::Gui::appendGlyphVertices` already produce the vertex data; the
 *    renderer only needs to upload it and bind the atlas texture.
 *
 * @warning No concrete `GuiRenderer` exists yet. This header is the contract and
 *          the vocabulary, not a working renderer.
 */

#include <Lotte/LotteLib.hpp>
#include <Kaldi/KaldiOperation.hpp>
#include <glm/glm.hpp>
#include <cstdint>
#include <span>

namespace Lotte {
namespace Gui {

/** @brief Texture and sampler pair bound for a textured draw. */
struct GuiTextureBinding {
	/** @brief Sampled texture handle, or 0 when untextured. */
	Kld::HandleId texture = 0;
	/** @brief Sampler handle, or 0 to use the pipeline default. */
	Kld::HandleId sampler = 0;
};

/** @brief Axis-aligned, single-coloured quad. */
struct ColouredQuad {
	/** @brief Top-left corner in pixels. */
	glm::fvec2 topLeft = glm::fvec2(0.0f);
	/** @brief Bottom-right corner in pixels. */
	glm::fvec2 bottomRight = glm::fvec2(0.0f);
	/** @brief Fill colour. */
	glm::fvec4 colour = glm::fvec4(1.0f);
};

/** @brief Axis-aligned quad with one colour per corner. */
struct GradientQuad {
	/** @brief Top-left corner in pixels. */
	glm::fvec2 topLeft = glm::fvec2(0.0f);
	/** @brief Bottom-right corner in pixels. */
	glm::fvec2 bottomRight = glm::fvec2(0.0f);
	/** @brief Corner colours in order top-left, top-right, bottom-left, bottom-right. */
	glm::fvec4 colour[4] = { glm::fvec4(1.0f), glm::fvec4(1.0f), glm::fvec4(1.0f), glm::fvec4(1.0f) };
};

/** @brief Triangle with a single colour. */
struct ColouredTriangle {
	/** @brief Corner positions in pixels. */
	glm::fvec2 position[3] = { glm::fvec2(0.0f), glm::fvec2(0.0f), glm::fvec2(0.0f) };
	/** @brief Fill colour. */
	glm::fvec4 colour = glm::fvec4(1.0f);
};

/** @brief Triangle with one colour per corner. */
struct GradientTriangle {
	/** @brief Corner positions in pixels. */
	glm::fvec2 position[3] = { glm::fvec2(0.0f), glm::fvec2(0.0f), glm::fvec2(0.0f) };
	/** @brief Corner colours. */
	glm::fvec4 colour[3] = { glm::fvec4(1.0f), glm::fvec4(1.0f), glm::fvec4(1.0f) };
};

/** @brief Single-coloured line segment. */
struct ColouredLine {
	/** @brief Endpoints in pixels. */
	glm::fvec2 position[2] = { glm::fvec2(0.0f), glm::fvec2(0.0f) };
	/** @brief Line colour. */
	glm::fvec4 colour = glm::fvec4(1.0f);
	/** @brief Line width in pixels. */
	float thickness = 1.0f;
};

/** @brief Line segment with one colour per endpoint. */
struct GradientLine {
	/** @brief Endpoints in pixels. */
	glm::fvec2 position[2] = { glm::fvec2(0.0f), glm::fvec2(0.0f) };
	/** @brief Endpoint colours. */
	glm::fvec4 colour[2] = { glm::fvec4(1.0f), glm::fvec4(1.0f) };
	/** @brief Line width in pixels. */
	float thickness = 1.0f;
};

/** @brief Textured quad with no tint. */
struct TexturedQuad {
	/** @brief Top-left corner in pixels. */
	glm::fvec2 topLeft = glm::fvec2(0.0f);
	/** @brief Bottom-right corner in pixels. */
	glm::fvec2 bottomRight = glm::fvec2(0.0f);
	/** @brief Texture coordinates of the top-left and bottom-right corners. */
	glm::fvec2 uv[2] = { glm::fvec2(0.0f), glm::fvec2(1.0f) };
	/** @brief Texture to sample. */
	GuiTextureBinding binding;
};

/** @brief Textured quad with a single tint. */
struct ColouredTexturedQuad {
	/** @brief Top-left corner in pixels. */
	glm::fvec2 topLeft = glm::fvec2(0.0f);
	/** @brief Bottom-right corner in pixels. */
	glm::fvec2 bottomRight = glm::fvec2(0.0f);
	/** @brief Texture coordinates of the top-left and bottom-right corners. */
	glm::fvec2 uv[2] = { glm::fvec2(0.0f), glm::fvec2(1.0f) };
	/** @brief Texture to sample. */
	GuiTextureBinding binding;
	/** @brief Tint applied to the sampled texture. */
	glm::fvec4 colour = glm::fvec4(1.0f);
};

/** @brief Textured quad with one tint per corner. */
struct GradientTexturedQuad {
	/** @brief Top-left corner in pixels. */
	glm::fvec2 topLeft = glm::fvec2(0.0f);
	/** @brief Bottom-right corner in pixels. */
	glm::fvec2 bottomRight = glm::fvec2(0.0f);
	/** @brief Texture coordinates of the top-left and bottom-right corners. */
	glm::fvec2 uv[2] = { glm::fvec2(0.0f), glm::fvec2(1.0f) };
	/** @brief Texture to sample. */
	GuiTextureBinding binding;
	/** @brief Corner tints in order top-left, top-right, bottom-left, bottom-right. */
	glm::fvec4 colour[4] = { glm::fvec4(1.0f), glm::fvec4(1.0f), glm::fvec4(1.0f), glm::fvec4(1.0f) };
};

/**
 * @brief Interface a GUI renderer must implement.
 *
 * Implementations may record into a Kaldi command buffer, may defer to a
 * `GuiGeometryBatch`, and may reorder submissions, but they must preserve the
 * relative order of primitives that share a pipeline and texture.
 */
class GuiRenderer
{
public:
	/** @brief Releases the renderer through the base interface. */
	virtual ~GuiRenderer() = default;

	/** @brief Submits a solid quad. */
	virtual void render(const ColouredQuad& target) = 0;
	/** @brief Submits a corner-coloured quad. */
	virtual void render(const GradientQuad& target) = 0;
	/** @brief Submits a solid triangle. */
	virtual void render(const ColouredTriangle& target) = 0;
	/** @brief Submits a corner-coloured triangle. */
	virtual void render(const GradientTriangle& target) = 0;
	/** @brief Submits a solid line. */
	virtual void render(const ColouredLine& target) = 0;
	/** @brief Submits a gradient line. */
	virtual void render(const GradientLine& target) = 0;
	/** @brief Submits an untinted textured quad. */
	virtual void render(const TexturedQuad& target) = 0;
	/** @brief Submits a tinted textured quad. */
	virtual void render(const ColouredTexturedQuad& target) = 0;
	/** @brief Submits a corner-tinted textured quad. */
	virtual void render(const GradientTexturedQuad& target) = 0;

	/// @name Bulk submission
	///@{
	/** @brief Submits every quad in the span. */
	virtual void render(const std::span<const ColouredQuad>& targets)
	{
		for (const ColouredQuad& target : targets) render(target);
	}
	/** @brief Submits every quad in the span. */
	virtual void render(const std::span<const GradientQuad>& targets)
	{
		for (const GradientQuad& target : targets) render(target);
	}
	/** @brief Submits every triangle in the span. */
	virtual void render(const std::span<const ColouredTriangle>& targets)
	{
		for (const ColouredTriangle& target : targets) render(target);
	}
	/** @brief Submits every triangle in the span. */
	virtual void render(const std::span<const GradientTriangle>& targets)
	{
		for (const GradientTriangle& target : targets) render(target);
	}
	/** @brief Submits every line in the span. */
	virtual void render(const std::span<const ColouredLine>& targets)
	{
		for (const ColouredLine& target : targets) render(target);
	}
	/** @brief Submits every line in the span. */
	virtual void render(const std::span<const GradientLine>& targets)
	{
		for (const GradientLine& target : targets) render(target);
	}
	/** @brief Submits every textured quad in the span. */
	virtual void render(const std::span<const TexturedQuad>& targets)
	{
		for (const TexturedQuad& target : targets) render(target);
	}
	/** @brief Submits every tinted quad in the span. */
	virtual void render(const std::span<const ColouredTexturedQuad>& targets)
	{
		for (const ColouredTexturedQuad& target : targets) render(target);
	}
	/** @brief Submits every corner-tinted quad in the span. */
	virtual void render(const std::span<const GradientTexturedQuad>& targets)
	{
		for (const GradientTexturedQuad& target : targets) render(target);
	}
	///@}

	/**
	 * @brief Emits all batched geometry.
	 *
	 * Called once per frame, and possibly earlier when the implementation has to
	 * change pipeline state. Implementations must be callable when nothing is
	 * pending.
	 */
	virtual void flush() = 0;
};

}
}

#endif // LTEGUIRENDERER_HPP
