#ifndef MHHARDWAREACCELERATEDFONT_HPP
#define MHHARDWAREACCELERATEDFONT_HPP
#include <MhLib/Text/MhFont.hpp>
#include <MhLib/Text/MhFontRepository.hpp>
#include <GFX/Abstract/Texture.hpp>
#include <GFX/Abstract/GfxResourceFactory.hpp>
#include <GFX/Abstract/VertexFormat.hpp>

namespace MH33 {
namespace GFX {

struct LineVertex {
	glm::vec2 POS;
	static const MH33::GFX::AttributeDescriptor attributes[];
	static const MH33::GFX::VertexDescriptor vertexDescriptor;
};
struct GlyphVertex {
	glm::vec2 POS;
	glm::vec2 TEX;
	static const MH33::GFX::AttributeDescriptor attributes[];
	static const MH33::GFX::VertexDescriptor vertexDescriptor;
};

struct STD140 UniformForTextRendering {
	glm::fvec4 clr;
};

DEFINE_STRUCT(TextRenderingContext)
struct TextRenderingContext {
	pTexture2D texture;
	UniformForTextRendering localUniform;
	std::vector<GlyphVertex> glyphCache;
	std::vector<uint32_t> indexCache;
	std::vector<LineVertex> lineCache;
	uint32_t maxIndex;
	MH33::GFX::uIndexedVertexBuffer glyphVertexBuffer;
	MH33::GFX::uUnindexedVertexBuffer lineVertexBuffer;
	MH33::GFX::uStorageBuffer uniformBuffer;
	MH33::GFX::uPipeline glyphPipeline,linePipeline;
	unsigned uniformBindingPoint1,uniformBindingPoint2;
	unsigned uniformIndex1,uniformIndex2;
	unsigned textureBindingPoint;
	void flush(TXT::TextRenderState& state);
};

class Font : public MH33::TXT::Font {
private:
	sWriteableTexture2D tex;
	pTextRenderingContext renderingContext;
protected:
	void insertCharacterIntoBackend(Character& character, const std::span<const std::byte>& bytes, const glm::ivec2& glyphSize, const glm::ivec2& glyphBearing, unsigned int glyphAdvance, const glm::ivec2& glyphOffset, const glm::ivec2& intendedCorner) override;
	void queueLineForRendering(TXT::TextRenderState& state, const glm::fvec2& endA, const glm::fvec2& endB) override;
	void queueGlyphForRendering(TXT::TextRenderState& state, const Character& character, const glm::fvec2& pos1, const glm::fvec2& pos2, float xdiff) override;
	void flushQueue(TXT::TextRenderState& state) override;
public:
	Font(pTextRenderingContext renderingContext, ResourceFactory& resFact, MH33::Io::uDevice&& iodev, const TXT::sFreeTypeSystem& system, unsigned fontSize = 48, bool bold = false, bool isSdf = false);
};
/*
typedef std::span<ShaderModuleCreateInfo> MutableModuleCreateInfoList;
typedef std::span<const ShaderModuleCreateInfo> ConstModuleCreateInfoList;
typedef std::span<const ShaderModuleCreateInfoRef> ConstModuleCreateInfoRefList;
*/

class FontRepository : public MH33::TXT::FontRepository {
private:
	pResourceFactory resourceFactory;
	TextRenderingContext renderingContext;
	void init(const ConstModuleCreateInfoList& textPipelineCreator, const ConstModuleCreateInfoList& linePipelineCreator);
protected:
	TXT::sFont createFont(Io::uDevice&& iodev, const TXT::sFreeTypeSystem& system, unsigned fontSize, bool bold) override;
public:
	FontRepository(const MH33::Io::sSystem& iosys, pResourceFactory resourceFactory, const ConstModuleCreateInfoList& textPipelineCreator, const ConstModuleCreateInfoList& linePipelineCreator);
	FontRepository(MH33::Io::sSystem&& iosys, pResourceFactory resourceFactory, const ConstModuleCreateInfoList& textPipelineCreator, const ConstModuleCreateInfoList& linePipelineCreator);
};

}
}
#endif // MHHARDWAREACCELERATEDFONT_HPP
