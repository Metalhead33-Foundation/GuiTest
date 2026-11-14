#ifndef LTEGUIVERTEX_HPP
#define LTEGUIVERTEX_HPP
/**
 * @file LteGuiVertex.hpp
 * @brief GUI vertex formats and their Kaldi vertex descriptors.
 *
 * Salvaged from `Renderer/Shared/GuiVertexTypes.hpp` (`GuiTest-legacy`) and
 * `GFX/Advanced/MhHardwareAcceleratedGuiRenderer.hpp` (`NewBeginning`). The old
 * tree declared the descriptors as `static const` class members, which forced a
 * `.cpp` per vertex type and made the header non-header-only; here each type
 * exposes `getVertexDescriptor()` returning a function-local static, so the
 * array and descriptor are built once and the header stands alone.
 *
 * Three formats cover everything a 2D GUI draws:
 *
 * | Type | Contents | Used for |
 * |---|---|---|
 * | @ref Lotte::Gui::ColouredGuiVertex | position + colour | solid quads, triangles, lines |
 * | @ref Lotte::Gui::TexturedGuiVertex | position + uv | plain textured quads, glyphs without per-glyph colour |
 * | @ref Lotte::Gui::ColouredTexturedGuiVertex | position + uv + colour | tinted sprites, glyph quads |
 *
 * Positions are in pixels, `y` growing downwards, matching
 * @ref Lotte::Text::PlacedGlyph; the viewport transform belongs to the renderer,
 * not to the vertex data.
 */

#include <Lotte/LotteLib.hpp>
#include <Lotte/Text/LteFontAtlas.hpp>
#include <Lotte/Text/LteTextLayout.hpp>
#include <Kaldi/KldVertexFormat.hpp>
#include <glm/glm.hpp>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Lotte {
namespace Gui {

/** @brief Position + colour vertex for solid geometry. */
struct ColouredGuiVertex {
	/** @brief Position in pixels. */
	glm::fvec2 position = glm::fvec2(0.0f);
	/** @brief Premultiplied-or-straight colour, as the pipeline expects. */
	glm::fvec4 colour = glm::fvec4(1.0f);
	/** @brief Returns the Kaldi layout for this vertex type. */
	static const Kld::VertexDescriptor& getVertexDescriptor()
	{
		static const Kld::AttributeDescriptor attributes[] = {
			Kld::AttributeDescriptor{ "POSITION", 0, Kld::PrimitiveType::F32x2, offsetof(ColouredGuiVertex, position) },
			Kld::AttributeDescriptor{ "COLOR", 0, Kld::PrimitiveType::F32x4, offsetof(ColouredGuiVertex, colour) }
		};
		static const Kld::VertexDescriptor descriptor{ sizeof(ColouredGuiVertex), attributes };
		return descriptor;
	}
};

/** @brief Position + texture coordinate vertex for untinted sprites. */
struct TexturedGuiVertex {
	/** @brief Position in pixels. */
	glm::fvec2 position = glm::fvec2(0.0f);
	/** @brief Normalized texture coordinate. */
	glm::fvec2 uv = glm::fvec2(0.0f);
	/** @brief Returns the Kaldi layout for this vertex type. */
	static const Kld::VertexDescriptor& getVertexDescriptor()
	{
		static const Kld::AttributeDescriptor attributes[] = {
			Kld::AttributeDescriptor{ "POSITION", 0, Kld::PrimitiveType::F32x2, offsetof(TexturedGuiVertex, position) },
			Kld::AttributeDescriptor{ "TEXCOORD", 0, Kld::PrimitiveType::F32x2, offsetof(TexturedGuiVertex, uv) }
		};
		static const Kld::VertexDescriptor descriptor{ sizeof(TexturedGuiVertex), attributes };
		return descriptor;
	}
};

/** @brief Position + texture coordinate + colour vertex for tinted sprites and glyphs. */
struct ColouredTexturedGuiVertex {
	/** @brief Position in pixels. */
	glm::fvec2 position = glm::fvec2(0.0f);
	/** @brief Normalized texture coordinate. */
	glm::fvec2 uv = glm::fvec2(0.0f);
	/** @brief Tint applied to the sampled texture. */
	glm::fvec4 colour = glm::fvec4(1.0f);
	/** @brief Returns the Kaldi layout for this vertex type. */
	static const Kld::VertexDescriptor& getVertexDescriptor()
	{
		static const Kld::AttributeDescriptor attributes[] = {
			Kld::AttributeDescriptor{ "POSITION", 0, Kld::PrimitiveType::F32x2, offsetof(ColouredTexturedGuiVertex, position) },
			Kld::AttributeDescriptor{ "TEXCOORD", 0, Kld::PrimitiveType::F32x2, offsetof(ColouredTexturedGuiVertex, uv) },
			Kld::AttributeDescriptor{ "COLOR", 0, Kld::PrimitiveType::F32x4, offsetof(ColouredTexturedGuiVertex, colour) }
		};
		static const Kld::VertexDescriptor descriptor{ sizeof(ColouredTexturedGuiVertex), attributes };
		return descriptor;
	}
};

/**
 * @brief Expands glyph quads into tinted vertices, resolving colours from the layout.
 * @param layout Layout that produced @a quads; supplies the per-glyph colours.
 * @param quads Quads from @ref Lotte::Text::appendGlyphQuads().
 * @param vertices Destination vertex list; appended to.
 * @param indices Optional index list; when non-null, two triangles per quad are
 *        appended with the correct vertex offset.
 * @return Number of quads converted.
 */
inline std::size_t appendGlyphVertices(const Text::TextLayout& layout, const std::vector<Text::GlyphQuad>& quads,
									   std::vector<ColouredTexturedGuiVertex>& vertices,
									   std::vector<std::uint32_t>* indices = nullptr)
{
	std::size_t converted = 0;
	for (const Text::GlyphQuad& quad : quads) {
		const std::uint32_t base = std::uint32_t(vertices.size());
		const glm::fvec4 colour = quad.styleIndex < layout.styles.size()
									? layout.styles[quad.styleIndex].colour
									: glm::fvec4(0.0f, 0.0f, 0.0f, 1.0f);
		for (int corner = 0; corner < 4; ++corner) {
			vertices.push_back(ColouredTexturedGuiVertex{ quad.position[corner], quad.uv[corner], colour });
		}
		if (indices) {
			indices->push_back(base + 0);
			indices->push_back(base + 1);
			indices->push_back(base + 2);
			indices->push_back(base + 1);
			indices->push_back(base + 2);
			indices->push_back(base + 3);
		}
		++converted;
	}
	return converted;
}

}
}

#endif // LTEGUIVERTEX_HPP
