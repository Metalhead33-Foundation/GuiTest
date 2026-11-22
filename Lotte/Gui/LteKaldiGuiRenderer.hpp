#ifndef LTEKALDIGUIRENDERER_HPP
#define LTEKALDIGUIRENDERER_HPP
/**
 * @file LteKaldiGuiRenderer.hpp
 * @brief The first concrete @ref Lotte::Gui::GuiRenderer: records into Kaldi.
 *
 * `LteGuiRenderer.hpp` defined the primitive vocabulary and the batching policy but
 * shipped no implementation. This is that implementation, kept deliberately small:
 *
 * - two pipelines only: **coloured** (`ColouredGuiVertex`, used for solid quads,
 *   triangles, gradients and lines) and **textured** (`ColouredTexturedGuiVertex`,
 *   used for every textured quad, tinted or not);
 * - positions are converted to clip space on the CPU, so neither shader needs a
 *   uniform buffer, push constants or a camera - the renderer owns the viewport
 *   size instead. The transform is
 *   @ref Lotte::Gfx::ClipSpaceConventions::pixelToClip taken from the factory's
 *   device, which is what keeps the same pixel-space UI upright on Vulkan (y-down
 *   clip space) and on OpenGL/Direct3D (y-up);
 * - geometry accumulates in @ref Lotte::Gui::GuiGeometryBatch objects and is
 *   recorded by one end-of-frame @ref flush(). A texture change closes the current
 *   textured *run* (a texture+sampler state plus its index range) and starts a new
 *   one, so draws are still grouped by state without recording anything mid-frame.
 *
 * @section lte_kaldi_gui_renderer_shaders Shaders
 *
 * The HLSL is embedded in the `.cpp` and compiled at construction time with
 * @ref Lotte::Gfx::compileShaderProgram (glslang's HLSL front-end, Vulkan SPIR-V).
 * The texture and sampler use the binding numbers the Vulkan backend documents
 * (`[[vk::binding(0, 0)]]` / `[[vk::binding(0, 1)]]`: set 0 is sampled textures,
 * set 1 is samplers), so the same pipeline also works on the GL backend, which
 * cross-compiles the blob with SPIRV-Cross.
 *
 * @section lte_kaldi_gui_renderer_frame Frame usage
 *
 * @code
 * renderer.beginFrame();                 // resets the CPU caches
 * renderer.render(someQuad);
 * cursor.render(renderer, mousePosition);
 * renderer.flush();                      // records uploads, state and draws
 * @endcode
 *
 * `flush()` must be called before the frame's command batch is submitted
 * (`KaldiContext::endFrame()`), because the upload commands point into the CPU
 * caches and those caches are only cleared by the next `beginFrame()`.
 */
#include <Lotte/LotteLib.hpp>
#include <Lotte/LteKld.hpp>
#include <Lotte/GFX/LtePipeline.hpp>
#include <Lotte/GFX/LteResourceFactory.hpp>
#include <Lotte/Gui/LteGuiRenderCache.hpp>
#include <Lotte/Gui/LteGuiRenderer.hpp>
#include <Lotte/Gui/LteGuiVertex.hpp>
#include <glm/glm.hpp>
#include <cstdint>
#include <optional>
#include <vector>

namespace Lotte {
namespace Gui {

/** @brief Construction-time settings of @ref KaldiGuiRenderer. */
struct KaldiGuiRendererSettings {
	/** @brief Initial framebuffer width in pixels; at least 1. */
	std::uint16_t viewportWidth = 1;
	/** @brief Initial framebuffer height in pixels; at least 1. */
	std::uint16_t viewportHeight = 1;
	/**
	 * @brief Clip-space y direction used by the pixel transform.
	 *
	 * Empty (the default) means "use the factory's conventions", which the factory
	 * derives from its device: `Down` on Vulkan, `Up` on OpenGL/Direct3D, `Up` when
	 * no device is attached. Set it explicitly only when rendering without a device
	 * (tests, offline recording).
	 */
	std::optional<Kld::ClipSpaceYAxis> clipSpaceYAxis;
};

/**
 * @brief Kaldi-backed GUI renderer.
 *
 * Owns the two pipelines, one sampler, and the CPU geometry caches. Every Kaldi
 * proxy it owns records its destroy command into the factory's dispatcher, so the
 * renderer must be destroyed before the factory's dispatcher and device.
 */
class MH_LOTTE_API KaldiGuiRenderer final : public GuiRenderer {
public:
	/**
	 * @brief Compiles the shaders and creates the pipelines.
	 * @param factory Factory used for every Kaldi resource.
	 * @param settings Initial viewport size.
	 * @throws std::runtime_error When a shader fails to compile.
	 */
	explicit KaldiGuiRenderer(Gfx::ResourceFactory& factory,
				  KaldiGuiRendererSettings settings = {});
	~KaldiGuiRenderer() override;

	KaldiGuiRenderer(const KaldiGuiRenderer&) = delete;
	KaldiGuiRenderer& operator=(const KaldiGuiRenderer&) = delete;
	KaldiGuiRenderer(KaldiGuiRenderer&&) = delete;
	KaldiGuiRenderer& operator=(KaldiGuiRenderer&&) = delete;

	/** @brief Updates the pixel-to-clip-space transform for the next frame. */
	void setViewportSize(std::uint16_t width, std::uint16_t height) noexcept;
	/** @brief The current viewport size in pixels. */
	glm::fvec2 viewportSize() const noexcept;
	/** @brief The conventions the pixel transform follows. */
	const Gfx::ClipSpaceConventions& conventions() const noexcept { return conventions_; }

	/**
	 * @brief Resets the per-frame CPU caches.
	 *
	 * Call once at the start of every frame, before the first `render()` call. The
	 * previous frame's caches are only released here, after their submission has
	 * completed.
	 */
	void beginFrame();

	/// @name GuiRenderer
	///@{
	void render(const ColouredQuad& target) override;
	void render(const GradientQuad& target) override;
	void render(const ColouredTriangle& target) override;
	void render(const GradientTriangle& target) override;
	void render(const ColouredLine& target) override;
	void render(const GradientLine& target) override;
	void render(const TexturedQuad& target) override;
	void render(const ColouredTexturedQuad& target) override;
	void render(const GradientTexturedQuad& target) override;
	/**
	 * @brief Records every pending batch, in the order textured, coloured, lines.
	 *
	 * Safe to call when nothing is pending.
	 *
	 * @warning `flush()` is the **last** renderer call of a frame. The upload
	 *          commands point into the CPU vertex caches, so appending more geometry
	 *          afterwards could reallocate those caches and invalidate the pointer
	 *          before the frame is submitted. Call @ref beginFrame() to start the
	 *          next frame's cache.
	 */
	void flush() override;
	///@}

private:
	/** @brief One texture-state run inside the textured batch. */
	struct TexturedRun {
		/** @brief Texture and sampler bound for the run. */
		GuiTextureBinding binding;
		/** @brief First index of the run. */
		std::uint32_t indexBegin = 0;
		/** @brief Number of indices in the run. */
		std::uint32_t indexCount = 0;
	};

	/** @brief Pixel position to the backend's clip space (see @ref Gfx::ClipSpaceConventions). */
	glm::fvec2 toNdc(glm::fvec2 pixel) const noexcept;
	/** @brief Appends four coloured corners as two triangles. */
	void appendColouredQuad(glm::fvec2 topLeft, glm::fvec2 bottomRight,
				const glm::fvec4 (&colours)[4]);
	/** @brief Appends a tinted/plain textured quad, starting a run on a texture change. */
	void appendTexturedQuad(glm::fvec2 topLeft, glm::fvec2 bottomRight,
				const glm::fvec2 (&uv)[2], const GuiTextureBinding& binding,
				const glm::fvec4 (&colours)[4]);
	/** @brief Records viewport, scissor and blend state. */
	void recordDrawState();
	/** @brief Records the pending textured runs. */
	void flushTextured();
	/** @brief Records the pending coloured triangle run. */
	void flushColoured();
	/** @brief Records the pending line run. */
	void flushLines();

	/** @brief Non-owning resource factory. */
	Gfx::ResourceFactory& factory_;

	/** @brief Coloured pipeline: owning shader container, descriptors and Kaldi proxy. */
	Gfx::Pipeline colourShader_;
	std::vector<Kld::ShaderBinaryDescriptor> colourDescriptors_;
	Kld::VertexDescriptor colourInput_ {};
	Lotte::Pipeline colourPipeline_;

	/** @brief Textured pipeline: owning shader container, descriptors and Kaldi proxy. */
	Gfx::Pipeline texturedShader_;
	std::vector<Kld::ShaderBinaryDescriptor> texturedDescriptors_;
	Kld::VertexDescriptor texturedInput_ {};
	Kld::SampledTextureBindingLayout texturedSampledLayout_ {};
	Kld::SamplerBindingLayout texturedSamplerLayout_ {};
	Kld::PipelineBindingLayout texturedBindings_ {};
	Lotte::Pipeline texturedPipeline_;

	/** @brief Sampler used by every textured draw. */
	Lotte::Sampler sampler_;
	/** @brief Straight-alpha blend state applied before each run. */
	Kld::OpSetBlendState blend_ {};

	/** @brief CPU geometry caches. */
	GuiGeometryBatch<ColouredGuiVertex> colourBatch_;
	GuiGeometryBatch<ColouredGuiVertex> lineBatch_;
	GuiGeometryBatch<ColouredTexturedGuiVertex> texturedBatch_;

	/** @brief Index offset already recorded this frame, per coloured batch. */
	std::uint32_t colourDrawn_ = 0;
	std::uint32_t lineDrawn_ = 0;
	/** @brief Texture-state runs accumulated for this frame's textured batch. */
	std::vector<TexturedRun> texturedRuns_;
	/** @brief Texture the next appended quad belongs to. */
	GuiTextureBinding activeTexture_ {};

	/** @brief Viewport size in pixels, never zero. */
	std::uint16_t viewportWidth_ = 1;
	std::uint16_t viewportHeight_ = 1;
	/** @brief Backend conventions used by @ref toNdc() and the draw state. */
	Gfx::ClipSpaceConventions conventions_;
};

} // namespace Gui
} // namespace Lotte

#endif // LTEKALDIGUIRENDERER_HPP
