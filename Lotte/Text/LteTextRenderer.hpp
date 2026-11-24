#ifndef LTETEXTRENDERER_HPP
#define LTETEXTRENDERER_HPP
/**
 * @file LteTextRenderer.hpp
 * @brief SDF/MSDF/MSDFA text rendering over a `FontRepository`'s atlases.
 *
 * This is the rendering half of the text pipeline:
 *
 * - `TextLayout` positions glyphs and decorations (@ref LteTextLayout.hpp);
 * - `FontRepository` owns faces and their lazy `FontAtlas` pages;
 * - `TextRenderer` ensures the glyphs a layout needs, groups the quads by atlas
 *   page, and records one indexed draw per page run.
 *
 * @section lte_text_renderer_vertex One vertex format, all styles
 *
 * A glyph is a quad with a texture coordinate, a fill colour and a `params`
 * vector. `params.x` selects how the sampled texture is decoded
 * (`0` = SDF `.r`, `1` = MSDF median of `.rgb`, `2` = MSDFA median of `.rgb`,
 * `3` = solid colour, used for underline/strikethrough; its quad uses UV 0..1 and
 * a one-pixel AA margin so the shader anti-aliases the intended edges). `params.y`/`z` are the
 * on-screen distance range in pixels for each axis, and `params.w` is the bold
 * dilation in pixels. Because all four values travel with the vertex:
 *
 * - colour changes do not split a draw call (per-vertex colour);
 * - bold is a shader threshold shift, not another face or atlas;
 * - different SDF modes can share one pipeline and one batch;
 * - no uniform buffer or push constant has to be updated between runs.
 *
 * @section lte_text_renderer_runs Runs and ordering
 *
 * Quads are appended in layout order. A change of atlas page closes the current
 * run and starts a new one; decorations are appended afterwards with a 1x1 white
 * texture, so underlines sit on top of the glyphs. `prepare()` ensures new glyphs
 * and submits their uploads before the frame; `flush()` records one indexed draw
 * per run.
 *
 * @section lte_text_renderer_effects Extending the look
 *
 * The layout and the atlas know nothing about the shader. A stylized pass (outline,
 * shadow, glow, gradient fill) can therefore be added by:
 *
 * - writing a different fragment shader over the same `TextGuiVertex` inputs, or
 * - adding fields to `TextGuiVertex`/`params` and extending one branch of the
 *   shader, without touching `TextLayout`, `FontAtlas` or `FontRepository`.
 *
 * The per-vertex parameter block is deliberately small and explicit so a future
 * style layer can replace the bold/range values with a material or style index.
 *
 * @warning `flush()` must be the last call of the frame. The upload and vertex
 *          commands point into CPU caches owned by the renderer and its atlases.
 */
#include <Lotte/LotteLib.hpp>
#include <Lotte/GFX/LtePipeline.hpp>
#include <Lotte/GFX/LteResourceFactory.hpp>
#include <Lotte/GFX/LteWriteableTexture2D.hpp>
#include <Lotte/Gui/LteGuiRenderCache.hpp>
#include <Lotte/Text/LteFontAtlas.hpp>
#include <Lotte/Text/LteFontRepository.hpp>
#include <Lotte/Text/LteTextLayout.hpp>
#include <Kaldi/KldVertexFormat.hpp>
#include <glm/glm.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace Lotte {
namespace Text {

/**
 * @brief Vertex layout shared by glyphs and solid decoration quads.
 *
 * `params` carries `(mode, onScreenPxRangeX, onScreenPxRangeY, boldPixels)`;
 * mode `3` is a solid anti-aliased rectangle used by underline/strikethrough.
 * See the file documentation.
 */
struct TextGuiVertex {
	/** @brief Position in pixels (converted to clip space before upload). */
	glm::fvec2 position = glm::fvec2(0.0f);
	/** @brief Texture coordinate inside the atlas page. */
	glm::fvec2 uv = glm::fvec2(0.0f);
	/** @brief Fill colour; alpha multiplies the SDF coverage. */
	glm::fvec4 colour = glm::fvec4(1.0f);
	/** @brief Mode, distance range and bold dilation (see the file docs). */
	glm::fvec4 params = glm::fvec4(0.0f);

	/** @brief Returns the Kaldi layout for this vertex type. */
	static const Kld::VertexDescriptor& getVertexDescriptor()
	{
		static const Kld::AttributeDescriptor attributes[] = {
			Kld::AttributeDescriptor{ "POSITION", 0, Kld::PrimitiveType::F32x2, offsetof(TextGuiVertex, position) },
			Kld::AttributeDescriptor{ "TEXCOORD", 0, Kld::PrimitiveType::F32x2, offsetof(TextGuiVertex, uv) },
			Kld::AttributeDescriptor{ "COLOR", 0, Kld::PrimitiveType::F32x4, offsetof(TextGuiVertex, colour) },
			Kld::AttributeDescriptor{ "TEXCOORD", 1, Kld::PrimitiveType::F32x4, offsetof(TextGuiVertex, params) }
		};
		static const Kld::VertexDescriptor descriptor{ sizeof(TextGuiVertex), attributes };
		return descriptor;
	}
};

/** @brief Construction-time settings of @ref TextRenderer. */
struct TextRendererSettings {
	/** @brief Initial framebuffer width in pixels; at least 1. */
	std::uint16_t viewportWidth = 1;
	/** @brief Initial framebuffer height in pixels; at least 1. */
	std::uint16_t viewportHeight = 1;
	/**
	 * @brief Clip-space y direction.
	 *
	 * Empty uses the factory's device conventions, exactly like the GUI renderer.
	 */
	std::optional<Kld::ClipSpaceYAxis> clipSpaceYAxis;
	/** @brief Bold dilation as a fraction of the run's em size. */
	float boldStrength = 0.04f;
};

/**
 * @brief Records glyph and decoration draws for a @ref TextLayout.
 *
 * The renderer borrows the factory and the repository. Both must outlive it.
 */
class MH_LOTTE_API TextRenderer
{
public:
	/**
	 * @brief Compiles the text shader and creates the pipeline and sampler.
	 * @param factory Factory used for every Kaldi resource.
	 * @param repository Repository that owns the faces and atlases.
	 * @param settings Initial viewport and style settings.
	 * @throws std::runtime_error When the shader fails to compile.
	 */
	TextRenderer(Gfx::ResourceFactory& factory, FontRepository& repository,
				 TextRendererSettings settings = {});
	~TextRenderer();

	TextRenderer(const TextRenderer&) = delete;
	TextRenderer& operator=(const TextRenderer&) = delete;
	TextRenderer(TextRenderer&&) = delete;
	TextRenderer& operator=(TextRenderer&&) = delete;

	/** @brief Updates the pixel-to-clip-space transform for the next frame. */
	void setViewportSize(std::uint16_t width, std::uint16_t height) noexcept;
	/** @brief The current viewport size in pixels. */
	glm::fvec2 viewportSize() const noexcept;
	/** @brief The conventions the pixel transform follows. */
	const Gfx::ClipSpaceConventions& conventions() const noexcept { return conventions_; }

	/** @brief Resets the per-frame CPU caches and run list. */
	void beginFrame();

	/**
	 * @brief Ensures the layout's glyphs and records their atlas uploads.
	 *
	 * Call **before** the frame begins (before `KaldiContext::beginFrame()` on a
	 * backend with render passes) and submit the recorded commands when this
	 * returns `true`. The method is idempotent: already-cached glyphs cost a hash
	 * lookup and record nothing.
	 * @param layout Layout whose glyphs will be rendered this frame.
	 * @return `true` when texture creation/upload commands were recorded and have
	 *         to be submitted before drawing.
	 * @throws std::invalid_argument When a glyph's face is not owned by the
	 *         repository.
	 */
	bool prepare(const TextLayout& layout);

	/**
	 * @brief Queues every drawable glyph and decoration of @p layout.
	 *
	 * Glyphs must have been ensured by @ref prepare(); a glyph that is not in its
	 * atlas yet is skipped for this frame. The returned quads are recorded by
	 * @ref flush().
	 * @param layout Layout to render; its faces must still be alive.
	 * @return Number of queued glyph quads.
	 * @throws std::invalid_argument When a glyph's face is not owned by the
	 *         repository.
	 */
	std::size_t render(const TextLayout& layout);

	/**
	 * @brief Records one draw per queued page run.
	 *
	 * Safe to call when nothing is pending.
	 */
	void flush();

	/** @brief Number of glyph quads queued since the last @ref beginFrame. */
	std::size_t queuedGlyphCount() const noexcept { return queuedGlyphs_; }
	/** @brief Number of page runs queued since the last @ref beginFrame. */
	std::size_t queuedRunCount() const noexcept { return runs_.size(); }

private:
	/** @brief One atlas page (or the white texture) and its index range. */
	struct PageRun {
		/** @brief Texture bound for the run. */
		Kld::HandleId texture = 0;
		/** @brief First index of the run. */
		std::uint32_t indexBegin = 0;
		/** @brief Number of indices in the run. */
		std::uint32_t indexCount = 0;
	};

	/** @brief Pixel position to the backend's clip space. */
	glm::fvec2 toNdc(glm::fvec2 pixel) const noexcept;
	/** @brief Starts a run when the bound texture changes. */
	void useTexture(Kld::HandleId texture);
	/** @brief Appends one glyph quad and its six indices. */
	void appendGlyphQuad(const GlyphQuad& quad, float mode, glm::fvec2 pxRange,
						 float boldPixels, const glm::fvec4& colour);
	/** @brief Appends one solid, anti-aliased quad for a decoration line. */
	void appendDecorationQuad(glm::fvec2 from, glm::fvec2 to, float thickness,
							  const glm::fvec4& colour);
	/** @brief Records viewport, scissor and blend state. */
	void recordDrawState();
	/** @brief Returns the effective distance range of a face on one axis. */
	static float effectiveDistanceRange(const PreprocessedFontFace& face, bool horizontal);

	/** @brief Non-owning factory. */
	Gfx::ResourceFactory* factory_ = nullptr;
	/** @brief Repository that owns the faces and atlases. */
	FontRepository* repository_ = nullptr;

	/** @brief Owning shader container and its descriptors. */
	Gfx::Pipeline shader_;
	std::vector<Kld::ShaderBinaryDescriptor> descriptors_;
	Kld::VertexDescriptor input_ {};
	Kld::SampledTextureBindingLayout sampledLayout_ {};
	Kld::SamplerBindingLayout samplerLayout_ {};
	Kld::PipelineBindingLayout bindings_ {};
	Lotte::Pipeline pipeline_;
	Lotte::Sampler sampler_;
	Kld::OpSetBlendState blend_ {};
	/** @brief 1x1 opaque texture used by solid decoration quads. */
	Gfx::WriteableTexture2D whiteTexture_;

	/** @brief CPU geometry cache. */
	Gui::GuiGeometryBatch<TextGuiVertex> batch_;
	/** @brief Page runs in append order. */
	std::vector<PageRun> runs_;
	/** @brief Texture of the run currently being filled. */
	Kld::HandleId activeTexture_ = 0;

	/** @brief Viewport size in pixels, never zero. */
	std::uint16_t viewportWidth_ = 1;
	std::uint16_t viewportHeight_ = 1;
	/** @brief Backend conventions used by @ref toNdc(). */
	Gfx::ClipSpaceConventions conventions_;
	/** @brief Style settings. */
	TextRendererSettings settings_;
	/** @brief Glyph quads queued this frame. */
	std::size_t queuedGlyphs_ = 0;
};

} // namespace Text
} // namespace Lotte

#endif // LTETEXTRENDERER_HPP

