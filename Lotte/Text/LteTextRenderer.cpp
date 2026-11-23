#include "LteTextRenderer.hpp"
#include <Kaldi/KaldiOperation.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace Lotte {
namespace Text {

namespace {

/** @brief Vertex stage shared by glyphs and solid decoration quads. */
constexpr std::string_view textVertexSource = R"hlsl(
struct VsInput {
    float2 position : POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
    float4 params : TEXCOORD1;
};

struct VsOutput {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
    float4 params : TEXCOORD1;
};

VsOutput main(VsInput input) {
    VsOutput output;
    output.position = float4(input.position, 0.0, 1.0);
    output.uv = input.uv;
    output.color = input.color;
    output.params = input.params;
    return output;
}
)hlsl";

/**
 * @brief Fragment stage for SDF, MSDF, MSDFA and solid quads.
 *
 * `params.x` selects the channel interpretation, `params.y`/`z` are the distance
 * range in screen pixels and `params.w` is the bold dilation in pixels. The
 * median is what reconstructs the contour from an MSDF; averaging would blur it.
 * MSDFA's alpha channel is preserved by the RGBA8 atlas and is available for a
 * glow/bevel pass; the default fill uses the RGB median for the contour.
 */
constexpr std::string_view textFragmentSource = R"hlsl(
[[vk::binding(0, 0)]] Texture2D<float4> textTexture : register(t0);
[[vk::binding(0, 1)]] SamplerState textSampler : register(s0);

struct VsOutput {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
    float4 params : TEXCOORD1;
};

float median3(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

float4 main(VsOutput input) : SV_Target {
    if (input.params.x > 2.5) {
        // Solid mode: underline and strikethrough quads.
        return input.color;
    }
    float4 raw = textTexture.Sample(textSampler, input.uv);
    float distance = (input.params.x < 0.5) ? raw.r : median3(raw.r, raw.g, raw.b);
    float pxRange = max(max(input.params.y, input.params.z), 1.0);
    float alpha = saturate((distance - 0.5) * pxRange + 0.5 + input.params.w);
    return float4(input.color.rgb, input.color.a * alpha);
}
)hlsl";

/** @brief Copies a Kaldi vertex descriptor's attributes into an owning vector. */
std::vector<Kld::AttributeDescriptor> attributesOf(const Kld::VertexDescriptor& descriptor)
{
	return std::vector<Kld::AttributeDescriptor>(descriptor.descriptors.begin(),
												 descriptor.descriptors.end());
}

} // namespace

TextRenderer::TextRenderer(Gfx::ResourceFactory& factory, FontRepository& repository,
						   TextRendererSettings settings)
	: factory_(&factory), repository_(&repository), settings_(settings)
{
	viewportWidth_ = std::max<std::uint16_t>(1, settings_.viewportWidth);
	viewportHeight_ = std::max<std::uint16_t>(1, settings_.viewportHeight);
	conventions_ = settings_.clipSpaceYAxis
					   ? Gfx::ClipSpaceConventions::fromYAxis(*settings_.clipSpaceYAxis)
					   : factory_->conventions();

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
	const std::array<Lotte::Gfx::ShaderStageSource, 2> stages = {{
		{ Kld::ShaderStage::Vertex, textVertexSource, "lotte_text.vs.hlsl" },
		{ Kld::ShaderStage::Fragment, textFragmentSource, "lotte_text.ps.hlsl" }
	}};
	shader_ = Lotte::Gfx::Pipeline(
		Lotte::Gfx::compileShaderProgram(nullptr, stages, options),
		attributesOf(TextGuiVertex::getVertexDescriptor()), sizeof(TextGuiVertex));
	input_ = shader_.vertexInput();
	descriptors_ = shader_.shaderDescriptors();

	sampledLayout_ = Kld::SampledTextureBindingLayout{
		0, Kld::TextureBindingType::Texture2D, 1, Kld::ShaderStage::Fragment
	};
	samplerLayout_ = Kld::SamplerBindingLayout{ 0, 1, Kld::ShaderStage::Fragment };
	bindings_ = Kld::PipelineBindingLayout{
		&sampledLayout_, 1,
		&samplerLayout_, 1,
		nullptr, 0,
		nullptr, 0,
		nullptr, 0
	};

	pipeline_ = factory_->createPipeline("LotteText", descriptors_.data(),
										static_cast<std::uint32_t>(descriptors_.size()),
										&input_, &bindings_);
	sampler_ = factory_->createSampler(Kld::SamplerFiltering::LINEAR,
									  Kld::SamplerWrapping::CLAMP_TO_EDGE);

	whiteTexture_ = Gfx::WriteableTexture2D::create(*factory_, 1, 1);
	whiteTexture_.fill(255, 255, 255, 255);
	whiteTexture_.recordUpdate();
}

TextRenderer::~TextRenderer() = default;

void TextRenderer::setViewportSize(std::uint16_t width, std::uint16_t height) noexcept
{
	viewportWidth_ = std::max<std::uint16_t>(1, width);
	viewportHeight_ = std::max<std::uint16_t>(1, height);
}

glm::fvec2 TextRenderer::viewportSize() const noexcept
{
	return glm::fvec2(float(viewportWidth_), float(viewportHeight_));
}

void TextRenderer::beginFrame()
{
	batch_.clear();
	runs_.clear();
	activeTexture_ = 0;
	queuedGlyphs_ = 0;
}

bool TextRenderer::prepare(const TextLayout& layout)
{
	std::vector<FontAtlas*> atlases;
	for (const PlacedGlyph& glyph : layout.glyphs) {
		if (glyph.faceIndex >= layout.faces.size()) {
			continue;
		}
		const PreprocessedFontFace* face = layout.faces[glyph.faceIndex];
		if (face == nullptr) {
			continue;
		}
		FontAtlas* atlas = repository_->atlasFor(face);
		if (atlas == nullptr) {
			continue;
		}
		// ensureGlyph() records create/update work into the dispatcher; the caller
		// submits it before beginFrame().
		atlas->ensureGlyph(glyph.codePoint);
		if (std::find(atlases.begin(), atlases.end(), atlas) == atlases.end()) {
			atlases.push_back(atlas);
		}
	}
	bool recorded = false;
	for (FontAtlas* atlas : atlases) {
		if (atlas->recordUpdates()) {
			recorded = true;
		}
	}
	return recorded;
}

glm::fvec2 TextRenderer::toNdc(glm::fvec2 pixel) const noexcept
{
	return conventions_.pixelToClip(pixel, viewportSize());
}

float TextRenderer::effectiveDistanceRange(const PreprocessedFontFace& face, bool horizontal)
{
	const float stored = horizontal ? face.distanceRangeX : face.distanceRangeY;
	if (stored > 0.0f) {
		return stored;
	}
	const std::uint32_t storedSize = face.storedBitmapSize();
	if (storedSize > 0 && face.bitmapLogicalSize > 0) {
		const float nominal = float(face.bitmapPadding) * float(storedSize)
							  / float(face.bitmapLogicalSize);
		if (nominal > 0.0f) {
			return nominal;
		}
	}
	return storedSize > 0 ? float(storedSize) * 0.5f : 1.0f;
}

void TextRenderer::useTexture(Kld::HandleId texture)
{
	if (!runs_.empty() && activeTexture_ == texture) {
		return;
	}
	activeTexture_ = texture;
	PageRun run;
	run.texture = texture;
	run.indexBegin = static_cast<std::uint32_t>(batch_.getIndices().size());
	runs_.push_back(run);
}

void TextRenderer::appendGlyphQuad(const GlyphQuad& quad, float mode, glm::fvec2 pxRange,
								   float boldPixels, const glm::fvec4& colour)
{
	const glm::fvec4 params{ mode, pxRange.x, pxRange.y, boldPixels };
	const glm::fvec2 topLeft = toNdc(quad.position[0]);
	const glm::fvec2 topRight = toNdc(quad.position[1]);
	const glm::fvec2 bottomLeft = toNdc(quad.position[2]);
	const glm::fvec2 bottomRight = toNdc(quad.position[3]);
	batch_.appendQuad(
		TextGuiVertex{ topLeft, quad.uv[0], colour, params },
		TextGuiVertex{ topRight, quad.uv[1], colour, params },
		TextGuiVertex{ bottomLeft, quad.uv[2], colour, params },
		TextGuiVertex{ bottomRight, quad.uv[3], colour, params });
	runs_.back().indexCount += 6;
}

void TextRenderer::appendDecorationQuad(glm::fvec2 from, glm::fvec2 to, float thickness,
										const glm::fvec4& colour)
{
	if (to.x <= from.x) {
		return;
	}
	const float half = std::max(0.5f, thickness) * 0.5f;
	const glm::fvec2 topLeft(from.x, from.y - half);
	const glm::fvec2 topRight(to.x, from.y - half);
	const glm::fvec2 bottomLeft(from.x, from.y + half);
	const glm::fvec2 bottomRight(to.x, from.y + half);
	const glm::fvec4 params{ 3.0f, 0.0f, 0.0f, 0.0f };
	const glm::fvec2 uv(0.0f);
	batch_.appendQuad(
		TextGuiVertex{ toNdc(topLeft), uv, colour, params },
		TextGuiVertex{ toNdc(topRight), uv, colour, params },
		TextGuiVertex{ toNdc(bottomLeft), uv, colour, params },
		TextGuiVertex{ toNdc(bottomRight), uv, colour, params });
	runs_.back().indexCount += 6;
}

std::size_t TextRenderer::render(const TextLayout& layout)
{
	const std::size_t before = queuedGlyphs_;

	for (const PlacedGlyph& glyph : layout.glyphs) {
		if (glyph.faceIndex >= layout.faces.size() || glyph.styleIndex >= layout.styles.size()) {
			continue;
		}
		const PreprocessedFontFace* face = layout.faces[glyph.faceIndex];
		if (face == nullptr) {
			continue;
		}
		FontAtlas* atlas = repository_->atlasFor(face);
		if (atlas == nullptr) {
			continue;
		}
		GlyphQuad quad;
		if (!buildGlyphQuad(glyph, *atlas, quad)) {
			continue;
		}

		const TextStyle& style = layout.styles[glyph.styleIndex];
		const float tileSize = float(atlas->tileSize());
		const glm::fvec2 pxRange(effectiveDistanceRange(*face, true) * glyph.size.x / tileSize,
								 effectiveDistanceRange(*face, false) * glyph.size.y / tileSize);
		const bool synthesizeBold = style.bold && !repository_->isBoldVariant(face);
		const float bold = synthesizeBold ? glyph.emSize * settings_.boldStrength : 0.0f;
		const int modeValue = static_cast<int>(face->type);
		const float mode = static_cast<float>(modeValue >= 0 && modeValue <= 2 ? modeValue : 0);

		useTexture(atlas->pageTexture(quad.page));
		appendGlyphQuad(quad, mode, pxRange, bold, style.colour);
		++queuedGlyphs_;
	}

	for (const DecorationLine& line : layout.lines) {
		if (line.styleIndex >= layout.styles.size()) {
			continue;
		}
		const TextStyle& style = layout.styles[line.styleIndex];
		const float thickness = line.thickness > 0.0f ? line.thickness : 1.0f;
		useTexture(whiteTexture_.handle());
		appendDecorationQuad(line.from, line.to, thickness, style.colour);
	}

	return queuedGlyphs_ - before;
}

void TextRenderer::recordDrawState()
{
	CommandBuffer& commands = factory_->commands();
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

void TextRenderer::flush()
{
	if (batch_.empty()) {
		runs_.clear();
		return;
	}
	CommandBuffer& commands = factory_->commands();
	recordDrawState();
	pipeline_.bind();
	batch_.recordUpload(commands);
	batch_.recordBind(commands);
	sampler_.bind(0);
	for (const PageRun& run : runs_) {
		if (run.texture == 0 || run.indexCount == 0) {
			continue;
		}
		commands.push({ Kld::Opcode::BindTexture,
						{ .opBindTexture = { 0u, run.texture, Kld::TextureBindingType::Texture2D } } });
		batch_.recordDraw(commands,
						  Gui::GuiGeometryBatch<TextGuiVertex>::Range{
							  run.indexBegin, run.indexCount, 0 });
	}
	runs_.clear();
}

} // namespace Text
} // namespace Lotte


