#include "LteKaldiGuiRenderer.hpp"
#include <Kaldi/KaldiOperation.hpp>
#include <algorithm>
#include <array>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace Lotte {
namespace Gui {

namespace {

/**
 * @brief Vertex stage of the coloured pipeline.
 *
 * Positions arrive in clip space because @ref KaldiGuiRenderer::toNdc() already
 * applied the viewport transform. That keeps the pipeline free of uniforms, so a
 * GUI batch never has to update a constant buffer before drawing it.
 */
constexpr std::string_view colourVertexSource = R"hlsl(
struct VsInput {
    float2 position : POSITION;
    float4 color : COLOR;
};

struct VsOutput {
    float4 position : SV_Position;
    float4 color : COLOR;
};

VsOutput main(VsInput input) {
    VsOutput output;
    output.position = float4(input.position, 0.0, 1.0);
    output.color = input.color;
    return output;
}
)hlsl";

/** @brief Fragment stage of the coloured pipeline: pass the interpolated colour through. */
constexpr std::string_view colourFragmentSource = R"hlsl(
struct VsOutput {
    float4 position : SV_Position;
    float4 color : COLOR;
};

float4 main(VsOutput input) : SV_Target {
    return input.color;
}
)hlsl";

/** @brief Vertex stage of the textured pipeline. */
constexpr std::string_view texturedVertexSource = R"hlsl(
struct VsInput {
    float2 position : POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
};

struct VsOutput {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
};

VsOutput main(VsInput input) {
    VsOutput output;
    output.position = float4(input.position, 0.0, 1.0);
    output.uv = input.uv;
    output.color = input.color;
    return output;
}
)hlsl";

/**
 * @brief Fragment stage of the textured pipeline.
 *
 * The bindings follow the Kaldi -> Vulkan mapping: set 0 holds sampled textures and
 * set 1 holds samplers, with the portable slot number as the binding.
 */
constexpr std::string_view texturedFragmentSource = R"hlsl(
[[vk::binding(0, 0)]] Texture2D<float4> guiTexture : register(t0);
[[vk::binding(0, 1)]] SamplerState guiSampler : register(s0);

struct VsOutput {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
};

float4 main(VsOutput input) : SV_Target {
    return guiTexture.Sample(guiSampler, input.uv) * input.color;
}
)hlsl";

/** @brief Copies a Kaldi vertex descriptor's attributes into an owning vector. */
std::vector<Kld::AttributeDescriptor> attributesOf(const Kld::VertexDescriptor& descriptor)
{
	return std::vector<Kld::AttributeDescriptor>(descriptor.descriptors.begin(),
						     descriptor.descriptors.end());
}

} // namespace

KaldiGuiRenderer::KaldiGuiRenderer(Gfx::ResourceFactory& factory,
				   KaldiGuiRendererSettings settings)
	: factory_(factory)
{
	viewportWidth_ = std::max<std::uint16_t>(1, settings.viewportWidth);
	viewportHeight_ = std::max<std::uint16_t>(1, settings.viewportHeight);

	// Vulkan's clip space is y-down and OpenGL/Direct3D's is y-up. The factory has
	// already resolved that (and the winding/depth/rectangle consequences of it)
	// from the device, so the renderer just follows along; the setting is only an
	// override for device-less recording.
	conventions_ = settings.clipSpaceYAxis ? Gfx::ClipSpaceConventions::fromYAxis(
							 *settings.clipSpaceYAxis)
					       : factory_.conventions();

	// Straight-alpha blending: the GUI authors PNGs with straight alpha, and the
	// shader deliberately does not premultiply.
	blend_ = Kld::OpSetBlendState{
		true,
		Kld::BlendFactor::SrcAlpha,
		Kld::BlendFactor::OneMinusSrcAlpha,
		Kld::BlendOp::Add,
		Kld::BlendFactor::One,
		Kld::BlendFactor::OneMinusSrcAlpha,
		Kld::BlendOp::Add,
		static_cast<std::uint8_t>(Kld::COLOR_WRITE_ALL)
	};

	const Lotte::Gfx::ShaderCompileOptions options {};
	const std::array<Lotte::Gfx::ShaderStageSource, 2> colourStages = {{
		{ Kld::ShaderStage::Vertex, colourVertexSource, "lotte_gui_colour.vs.hlsl" },
		{ Kld::ShaderStage::Fragment, colourFragmentSource, "lotte_gui_colour.ps.hlsl" }
	}};
	const std::array<Lotte::Gfx::ShaderStageSource, 2> texturedStages = {{
		{ Kld::ShaderStage::Vertex, texturedVertexSource, "lotte_gui_textured.vs.hlsl" },
		{ Kld::ShaderStage::Fragment, texturedFragmentSource, "lotte_gui_textured.ps.hlsl" }
	}};

	colourShader_ = Lotte::Gfx::Pipeline(
		Lotte::Gfx::compileShaderProgram(nullptr, colourStages, options),
		attributesOf(ColouredGuiVertex::getVertexDescriptor()),
		sizeof(ColouredGuiVertex));
	texturedShader_ = Lotte::Gfx::Pipeline(
		Lotte::Gfx::compileShaderProgram(nullptr, texturedStages, options),
		attributesOf(ColouredTexturedGuiVertex::getVertexDescriptor()),
		sizeof(ColouredTexturedGuiVertex));

	// The descriptors and vertex inputs are views into the shader containers, so
	// they are built after the containers have their final address and stay members
	// until the recorded create commands have been submitted.
	colourInput_ = colourShader_.vertexInput();
	colourDescriptors_ = colourShader_.shaderDescriptors();
	texturedInput_ = texturedShader_.vertexInput();
	texturedDescriptors_ = texturedShader_.shaderDescriptors();

	texturedSampledLayout_ = Kld::SampledTextureBindingLayout{
		0, Kld::TextureBindingType::Texture2D, 1, Kld::ShaderStage::Fragment
	};
	texturedSamplerLayout_ = Kld::SamplerBindingLayout{ 0, 1, Kld::ShaderStage::Fragment };
	texturedBindings_ = Kld::PipelineBindingLayout{
		&texturedSampledLayout_, 1,
		&texturedSamplerLayout_, 1,
		nullptr, 0,
		nullptr, 0,
		nullptr, 0
	};

	colourPipeline_ = factory_.createPipeline(
		"LotteGuiColour", colourDescriptors_.data(),
		static_cast<std::uint32_t>(colourDescriptors_.size()), &colourInput_);
	texturedPipeline_ = factory_.createPipeline(
		"LotteGuiTextured", texturedDescriptors_.data(),
		static_cast<std::uint32_t>(texturedDescriptors_.size()), &texturedInput_,
		&texturedBindings_);
	sampler_ = factory_.createSampler(Kld::SamplerFiltering::LINEAR,
					  Kld::SamplerWrapping::CLAMP_TO_EDGE);
}

KaldiGuiRenderer::~KaldiGuiRenderer() = default;

void KaldiGuiRenderer::setViewportSize(std::uint16_t width, std::uint16_t height) noexcept
{
	viewportWidth_ = std::max<std::uint16_t>(1, width);
	viewportHeight_ = std::max<std::uint16_t>(1, height);
}

glm::fvec2 KaldiGuiRenderer::viewportSize() const noexcept
{
	return glm::fvec2(float(viewportWidth_), float(viewportHeight_));
}

void KaldiGuiRenderer::beginFrame()
{
	colourBatch_.clear();
	lineBatch_.clear();
	texturedBatch_.clear();
	colourDrawn_ = 0;
	lineDrawn_ = 0;
	texturedRuns_.clear();
	activeTexture_ = GuiTextureBinding{};
}

glm::fvec2 KaldiGuiRenderer::toNdc(glm::fvec2 pixel) const noexcept
{
	return conventions_.pixelToClip(pixel, viewportSize());
}

void KaldiGuiRenderer::appendColouredQuad(glm::fvec2 topLeft, glm::fvec2 bottomRight,
					  const glm::fvec4 (&colours)[4])
{
	const glm::fvec2 topLeftNdc = toNdc(topLeft);
	const glm::fvec2 bottomRightNdc = toNdc(bottomRight);
	colourBatch_.appendQuad(
		ColouredGuiVertex{ topLeftNdc, colours[0] },
		ColouredGuiVertex{ glm::fvec2(bottomRightNdc.x, topLeftNdc.y), colours[1] },
		ColouredGuiVertex{ glm::fvec2(topLeftNdc.x, bottomRightNdc.y), colours[2] },
		ColouredGuiVertex{ bottomRightNdc, colours[3] });
}

void KaldiGuiRenderer::appendTexturedQuad(glm::fvec2 topLeft, glm::fvec2 bottomRight,
					  const glm::fvec2 (&uv)[2],
					  const GuiTextureBinding& binding,
					  const glm::fvec4 (&colours)[4])
{
	// A texture (or sampler) change closes the current run and starts a new one.
	// Nothing is recorded here: every run is drawn by the single end-of-frame
	// flush(), which is also what keeps the recorded upload pointers valid.
	if (texturedRuns_.empty() || activeTexture_.texture != binding.texture ||
	    activeTexture_.sampler != binding.sampler) {
		activeTexture_ = binding;
		TexturedRun run;
		run.binding = binding;
		run.indexBegin = static_cast<std::uint32_t>(texturedBatch_.getIndices().size());
		texturedRuns_.push_back(run);
	}
	const glm::fvec2 topLeftNdc = toNdc(topLeft);
	const glm::fvec2 bottomRightNdc = toNdc(bottomRight);
	texturedBatch_.appendQuad(
		ColouredTexturedGuiVertex{ topLeftNdc, uv[0], colours[0] },
		ColouredTexturedGuiVertex{ glm::fvec2(bottomRightNdc.x, topLeftNdc.y),
					   glm::fvec2(uv[1].x, uv[0].y), colours[1] },
		ColouredTexturedGuiVertex{ glm::fvec2(topLeftNdc.x, bottomRightNdc.y),
					   glm::fvec2(uv[0].x, uv[1].y), colours[2] },
		ColouredTexturedGuiVertex{ bottomRightNdc, uv[1], colours[3] });
	texturedRuns_.back().indexCount += 6;
}

void KaldiGuiRenderer::render(const ColouredQuad& target)
{
	const glm::fvec4 colours[4] = { target.colour, target.colour, target.colour,
					target.colour };
	appendColouredQuad(target.topLeft, target.bottomRight, colours);
}

void KaldiGuiRenderer::render(const GradientQuad& target)
{
	appendColouredQuad(target.topLeft, target.bottomRight, target.colour);
}

void KaldiGuiRenderer::render(const ColouredTriangle& target)
{
	const std::uint32_t base = static_cast<std::uint32_t>(colourBatch_.getVertices().size());
	for (int corner = 0; corner < 3; ++corner) {
		colourBatch_.getVertices().push_back(
			ColouredGuiVertex{ toNdc(target.position[corner]), target.colour });
		colourBatch_.getIndices().push_back(base + static_cast<std::uint32_t>(corner));
	}
}

void KaldiGuiRenderer::render(const GradientTriangle& target)
{
	const std::uint32_t base = static_cast<std::uint32_t>(colourBatch_.getVertices().size());
	for (int corner = 0; corner < 3; ++corner) {
		colourBatch_.getVertices().push_back(
			ColouredGuiVertex{ toNdc(target.position[corner]), target.colour[corner] });
		colourBatch_.getIndices().push_back(base + static_cast<std::uint32_t>(corner));
	}
}

void KaldiGuiRenderer::render(const ColouredLine& target)
{
	lineBatch_.appendLine(ColouredGuiVertex{ toNdc(target.position[0]), target.colour },
			      ColouredGuiVertex{ toNdc(target.position[1]), target.colour });
}

void KaldiGuiRenderer::render(const GradientLine& target)
{
	lineBatch_.appendLine(ColouredGuiVertex{ toNdc(target.position[0]), target.colour[0] },
			      ColouredGuiVertex{ toNdc(target.position[1]), target.colour[1] });
}

void KaldiGuiRenderer::render(const TexturedQuad& target)
{
	const glm::fvec4 white(1.0f);
	const glm::fvec4 colours[4] = { white, white, white, white };
	appendTexturedQuad(target.topLeft, target.bottomRight, target.uv, target.binding,
			   colours);
}

void KaldiGuiRenderer::render(const ColouredTexturedQuad& target)
{
	const glm::fvec4 colours[4] = { target.colour, target.colour, target.colour,
					target.colour };
	appendTexturedQuad(target.topLeft, target.bottomRight, target.uv, target.binding,
			   colours);
}

void KaldiGuiRenderer::render(const GradientTexturedQuad& target)
{
	appendTexturedQuad(target.topLeft, target.bottomRight, target.uv, target.binding,
			   target.colour);
}

void KaldiGuiRenderer::recordDrawState()
{
	CommandBuffer& commands = factory_.commands();
	// The GUI fills its target, so this is a trivial conversion - but routing it
	// through the conventions keeps sub-viewports (a 3D scene inside a 2D frame)
	// correct without the caller knowing that the HAL measures rectangles from the
	// bottom edge.
	const Gfx::PixelRect viewport = conventions_.halRect(
		Gfx::PixelRect{ 0.0f, 0.0f, float(viewportWidth_), float(viewportHeight_) },
		float(viewportHeight_));
	KldOopDetail::setViewport(commands, viewport.x, viewport.y, viewport.width,
				  viewport.height, 0.0f, 1.0f);
	KldOopDetail::setScissor(commands, static_cast<std::uint32_t>(viewport.x),
				 static_cast<std::uint32_t>(viewport.y),
				 static_cast<std::uint32_t>(viewport.width),
				 static_cast<std::uint32_t>(viewport.height));
	KldOopDetail::setBlendState(commands, blend_);
}

void KaldiGuiRenderer::flushTextured()
{
	const std::uint32_t total = static_cast<std::uint32_t>(texturedBatch_.getIndices().size());
	if (total == 0 || texturedRuns_.empty()) {
		texturedRuns_.clear();
		return;
	}
	CommandBuffer& commands = factory_.commands();
	recordDrawState();
	texturedPipeline_.bind();
	texturedBatch_.recordUpload(commands);
	texturedBatch_.recordBind(commands);
	for (const TexturedRun& run : texturedRuns_) {
		if (run.indexCount == 0 || run.binding.texture == 0) {
			continue;
		}
		commands.push({ Kld::Opcode::BindTexture,
				{ .opBindTexture = { 0u, run.binding.texture,
						     Kld::TextureBindingType::Texture2D } } });
		sampler_.bind(0);
		texturedBatch_.recordDraw(
			commands, GuiGeometryBatch<ColouredTexturedGuiVertex>::Range{
					  run.indexBegin, run.indexCount, 0 });
	}
	texturedRuns_.clear();
}

void KaldiGuiRenderer::flushColoured()
{
	const std::uint32_t total = static_cast<std::uint32_t>(colourBatch_.getIndices().size());
	if (total <= colourDrawn_) {
		colourDrawn_ = total;
		return;
	}
	CommandBuffer& commands = factory_.commands();
	recordDrawState();
	colourPipeline_.bind();
	colourBatch_.recordUpload(commands);
	colourBatch_.recordBind(commands);
	colourBatch_.recordDraw(
		commands, GuiGeometryBatch<ColouredGuiVertex>::Range{
				  colourDrawn_, total - colourDrawn_, 0 });
	colourDrawn_ = total;
}

void KaldiGuiRenderer::flushLines()
{
	const std::uint32_t total = static_cast<std::uint32_t>(lineBatch_.getIndices().size());
	if (total <= lineDrawn_) {
		lineDrawn_ = total;
		return;
	}
	CommandBuffer& commands = factory_.commands();
	recordDrawState();
	colourPipeline_.bind();
	lineBatch_.recordUpload(commands);
	lineBatch_.recordBind(commands);
	lineBatch_.recordDrawLines(
		commands, 1.0f,
		GuiGeometryBatch<ColouredGuiVertex>::Range{ lineDrawn_,
							   total - lineDrawn_, 0 });
	lineDrawn_ = total;
}

void KaldiGuiRenderer::flush()
{
	// Textured geometry is flushed first so a cursor or a glyph always sits on top
	// of the coloured layers recorded in the same frame. Finer ordering needs the
	// layer key the renderer contract describes; it does not exist yet.
	flushTextured();
	flushColoured();
	flushLines();
}

} // namespace Gui
} // namespace Lotte
