#include "MhHardwareAcceleratedFont.hpp"
#include <MhLib/Io/MhFile.hpp>
#include <MhLib/Media/Image/MhPNG.hpp>
namespace MH33 {
namespace GFX {

static void DEBUG_SAVE_GLYPH(MH33::TXT::Font::Character& character, const std::span<const std::byte>& bytes) {
	if(!(character.size.x && character.size.y)) return;
	std::stringstream sstrm;
	sstrm << "debug/" << character.offset.x << 'x' << character.offset.y << ".png";
	MH33::Io::File outfile(sstrm.str(),MH33::Io::Mode::WRITE);
	MH33::Image::DecodeTarget out;
	out.format = MH33::Image::Format::R8U;
	out.frames.resize(1);
	out.frames[0].width = character.size.x;
	out.frames[0].height = character.size.y;
	out.frames[0].stride = character.size.x;
	out.frames[0].imageData.resize(bytes.size());
	std::memcpy(out.frames[0].imageData.data(),bytes.data(),bytes.size());
	MH33::Image::PNG::encode(outfile,character.size.x,character.size.y,0,8, out.frames[0].imageData, 0.5f);
}
void Font::debugSaveTexture()
{
	std::stringstream sstrm;
	sstrm << "debug/font_texture-" << fontFace->family_name << '-' << int(isBold) << '_' << int(isSdf) << ".png";
	MH33::Io::File outfile(sstrm.str(),MH33::Io::Mode::WRITE);
	tex->saveTo(outfile);
}

const MH33::GFX::AttributeDescriptor LineVertex::attributes[] = {
	{ .SemanticName = "POS", .SemanticIndex = 0, .type = MH33::GFX::PrimitiveType::F32x2, .offset = offsetof(LineVertex,POS) }
};
const MH33::GFX::AttributeDescriptor GlyphVertex::attributes[] = {
	{ .SemanticName = "POS", .SemanticIndex = 0, .type = MH33::GFX::PrimitiveType::F32x2, .offset = offsetof(GlyphVertex,POS) },
	{ .SemanticName = "TEX", .SemanticIndex = 0, .type = MH33::GFX::PrimitiveType::F32x2, .offset = offsetof(GlyphVertex,TEX) }
};

const MH33::GFX::VertexDescriptor LineVertex::vertexDescriptor = {
	.stride = sizeof(LineVertex),
	.descriptors = attributes
};

const MH33::GFX::VertexDescriptor GlyphVertex::vertexDescriptor = {
	.stride = sizeof(GlyphVertex),
	.descriptors = attributes
};


void Font::insertCharacterIntoBackend(Character& character, const std::span<const std::byte>& bytes, const glm::ivec2& glyphSize, const glm::ivec2& glyphBearing, unsigned int glyphAdvance, const glm::ivec2& glyphOffset, const glm::ivec2& intendedCorner)
{
	if((glyphOffset.x >= tex->getWidth() || intendedCorner.x >= tex->getWidth())) {
		if(tex->getWidth() < 8192) {
			tex->resize(tex->getWidth() * 2, tex->getHeight());
		} else {
			textureOffset.x = 1;
			textureOffset.y += maxCharSizeSoFar.y + 1;
			character.offset = textureOffset + glm::ivec2(1, 0);
		}
	}
	//DEBUG_SAVE_GLYPH(character, bytes);
	tex->blit(bytes,MH33::GFX::TextureFormat::R8U,textureOffset,glyphSize);
	textureOffset.x += glyphSize.x + 1;
}

void Font::queueLineForRendering(TXT::TextRenderState& state, const glm::fvec2& endA, const glm::fvec2& endB)
{
	renderingContext->lineCache.push_back( { .POS = endA } );
	renderingContext->lineCache.push_back( { .POS = endB } );
}
static const unsigned quadIds[] = { 0, 1, 2, 1, 2, 3 };
void Font::queueGlyphForRendering(TXT::TextRenderState& state, const Character& character, const glm::fvec2& pos1, const glm::fvec2& pos2, float xdiff)
{
	const glm::fvec2 tc0(static_cast<float>(character.offset.x) * tex->getWidthR(), static_cast<float>(character.offset.y) * tex->getHeightR());
	const glm::fvec2 tc1 = tc0 + glm::vec2(static_cast<float>(character.size.x) * tex->getWidthR(), static_cast<float>(character.size.y) * tex->getHeightR());
	renderingContext->glyphCache.push_back({ .POS = glm::fvec2(std::min(pos1.x,pos2.x)+xdiff,-1.0f * std::min(pos1.y,pos2.y)),
																	   .TEX = glm::fvec2(std::min(tc0.x,tc1.x),std::min(tc0.y,tc1.y))  });
	renderingContext->glyphCache.push_back({ .POS = glm::fvec2(std::max(pos1.x,pos2.x)+xdiff,-1.0f * std::min(pos1.y,pos2.y)),
																	   .TEX = glm::fvec2(std::max(tc0.x,tc1.x),std::min(tc0.y,tc1.y))  });
	renderingContext->glyphCache.push_back({ .POS = glm::fvec2(std::min(pos1.x,pos2.x),-1.0f * std::max(pos1.y,pos2.y)),
																	   .TEX = glm::fvec2(std::min(tc0.x,tc1.x),std::max(tc0.y,tc1.y))  });
	renderingContext->glyphCache.push_back({ .POS = glm::fvec2(std::max(pos1.x,pos2.x),-1.0f * std::max(pos1.y,pos2.y)),
																	   .TEX = glm::fvec2(std::max(tc0.x,tc1.x),std::max(tc0.y,tc1.y))  });
	for(const auto& it : quadIds) {
		renderingContext->indexCache.push_back(renderingContext->maxIndex + it);
	}
	renderingContext->maxIndex += 4;
}

void Font::flushQueue(TXT::TextRenderState& state)
{
	renderingContext->texture = tex.get();
	renderingContext->flush(state);
}

void Font::flushTexture()
{
	tex->update();
	//debugSaveTexture();
}

Font::Font(pTextRenderingContext renderingContext, ResourceFactory& resFact, Io::uDevice&& iodev, const TXT::sFreeTypeSystem& system, unsigned fontSize, bool bold, bool isSdf)
	: MH33::TXT::Font(std::move(iodev), system, fontSize, bold, isSdf), tex(resFact.createWriteableTexture2D(TextureFormat::R8U,256,256)), renderingContext(renderingContext)
{

}

static constexpr const unsigned EXPECTED_MAX_GLYPHS_ONSCREEN = 2048;
static constexpr const unsigned EXPECTED_MAX_GLYPH_VERTICES = EXPECTED_MAX_GLYPHS_ONSCREEN * 4;
static constexpr const unsigned EXPECTED_MAX_GLYPH_INDICES = EXPECTED_MAX_GLYPHS_ONSCREEN * 6;
static constexpr const unsigned EXPECTED_MAX_GLYPH_LINES = EXPECTED_MAX_GLYPHS_ONSCREEN * 2;

void FontRepository::init(const ConstModuleCreateInfoList& textPipelineCreator, const ConstModuleCreateInfoList& linePipelineCreator)
{
	renderingContext.glyphCache.reserve(EXPECTED_MAX_GLYPH_VERTICES);
	renderingContext.indexCache.reserve(EXPECTED_MAX_GLYPH_INDICES);
	renderingContext.lineCache.reserve(EXPECTED_MAX_GLYPH_LINES);
	renderingContext.maxIndex = 0;
	renderingContext.glyphVertexBuffer = MH33::GFX::uIndexedVertexBuffer(resourceFactory->createIndexedVertexBuffer(VertexBufferUsageClass::Dynamic, &GlyphVertex::vertexDescriptor, EXPECTED_MAX_GLYPH_VERTICES * sizeof(GlyphVertex), EXPECTED_MAX_GLYPH_INDICES));
	renderingContext.lineVertexBuffer = MH33::GFX::uUnindexedVertexBuffer(resourceFactory->createUnindexedVertexBuffer(VertexBufferUsageClass::Dynamic, &LineVertex::vertexDescriptor, EXPECTED_MAX_GLYPH_LINES * sizeof(LineVertex)));
	renderingContext.uniformBuffer = MH33::GFX::uStorageBuffer(resourceFactory->createStorageBuffer(MH33::GFX::StorageBufferType::UNIFORM_BUFFER,sizeof(UniformForTextRendering)));
	renderingContext.glyphPipeline = MH33::GFX::uPipeline(resourceFactory->createPipeline(textPipelineCreator,&GlyphVertex::vertexDescriptor));
	renderingContext.linePipeline = MH33::GFX::uPipeline(resourceFactory->createPipeline(linePipelineCreator,&LineVertex::vertexDescriptor));
	renderingContext.uniformBindingPoint1 = renderingContext.glyphPipeline->getBindingPoint("fontRenderingData");
	renderingContext.textureBindingPoint = renderingContext.glyphPipeline->getBindingPoint("glyphTexture");
	renderingContext.uniformIndex1 = renderingContext.glyphPipeline->getUniformBlockIndex("fontRenderingData");
	renderingContext.uniformBindingPoint2 = renderingContext.linePipeline->getBindingPoint("fontRenderingData");
	renderingContext.uniformIndex2 = renderingContext.linePipeline->getUniformBlockIndex("fontRenderingData");
	renderingContext.uniformBuffer->ensureDataSize(sizeof(UniformForTextRendering));
}

TXT::sFont FontRepository::createFont(Io::uDevice&& iodev, const TXT::sFreeTypeSystem& system, unsigned fontSize, bool bold)
{
	return std::make_shared<Font>(&renderingContext,*resourceFactory,std::move(iodev), system, fontSize, bold, true);
}

FontRepository::FontRepository(const Io::sSystem& iosys, pResourceFactory resourceFactory, const ConstModuleCreateInfoList& textPipelineCreator, const ConstModuleCreateInfoList& linePipelineCreator)
	: MH33::TXT::FontRepository(iosys), resourceFactory(resourceFactory)
{
	init(textPipelineCreator, linePipelineCreator);
}

FontRepository::FontRepository(Io::sSystem&& iosys, pResourceFactory resourceFactory, const ConstModuleCreateInfoList& textPipelineCreator, const ConstModuleCreateInfoList& linePipelineCreator)
	: MH33::TXT::FontRepository(std::move(iosys)), resourceFactory(resourceFactory)
{
	init(textPipelineCreator, linePipelineCreator);
}

void TextRenderingContext::flush(TXT::TextRenderState& state)
{
	if( !(glyphCache.size() || lineCache.size() ) ) {
		return;
	}
	localUniform.clr = state.colour;
	uniformBuffer->bind();
	uniformBuffer->setData(MH33::Util::as_const_byte_span(localUniform),0);
	if(glyphCache.size()) {
		glyphVertexBuffer->bindData();
		glyphVertexBuffer->ensureDataSize(glyphCache.size() * sizeof(GlyphVertex));
		glyphVertexBuffer->setDataT<GlyphVertex>(glyphCache);
		glyphVertexBuffer->bindIndices();
		glyphVertexBuffer->ensureIndexCount(indexCache.size());
		glyphVertexBuffer->setIndices(indexCache);
		glyphVertexBuffer->unbindData();
		glyphVertexBuffer->unbindIndices();
		glyphPipeline->bind();
		glyphPipeline->setUniform(uniformIndex1,*uniformBuffer,uniformIndex1);
		glyphPipeline->setUniform(textureBindingPoint,*texture, 0);
		glyphPipeline->draw(*glyphVertexBuffer, MH33::GFX::RenderType::TRIANGLES, 0, indexCache.size());
		maxIndex = 0;
		glyphCache.clear();
		indexCache.clear();
	}
	if(lineCache.size()) {
		lineVertexBuffer->bind();
		lineVertexBuffer->ensureDataSize(lineCache.size() * sizeof(LineVertex));
		lineVertexBuffer->setDataT<LineVertex>(lineCache);
		lineVertexBuffer->unbind();
		linePipeline->bind();
		linePipeline->setUniform(uniformIndex2,*uniformBuffer,uniformIndex2);
		linePipeline->draw(*lineVertexBuffer, MH33::GFX::RenderType::LINES, 0, lineCache.size());
		lineCache.clear();
	}
}

}
}
