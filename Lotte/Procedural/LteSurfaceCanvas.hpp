#ifndef LTESURFACECANVAS_HPP
#define LTESURFACECANVAS_HPP
/**
 * @file LteSurfaceCanvas.hpp
 * @brief Layered albedo+height painting and one deferred lighting pass.
 *
 * The salvage of the `Canvas`/`paint()`/`shade()` core of
 * `Sandbox/slop_advice/gui_procedural_generation/claude/ui_forge.cpp`. The idea the
 * file is built on is the one worth keeping: **nothing is pre-lit**. A layer writes
 * albedo plus a height delta derived from the shape's signed distance, and a single
 * final pass turns the accumulated height field into normals, lights it from one
 * direction, adds specular, cheap ambient occlusion and a drop shadow.
 *
 * That is why one painter can produce a raised panel, a sunken inventory well and a
 * gilded bevel from the same code: raised and sunken only differ in the sign of the
 * height delta, and metal only differs in the specular response the material asks
 * for.
 *
 * What was dropped from the original:
 *
 * - the built-in PNG writer and the on-disk element dump (Euphemy already has
 *   encoders; `toDecodeTarget` hands the pixels over),
 * - the `atlas.json` emitter (a theme schema and a JSON loader replace it),
 * - the per-run `std::vector` allocations inside the painter's inner loop.
 *
 * @note GPU plan: `SurfaceCanvas::shade` is a per-pixel gather over the height
 *       field, so it maps directly onto a compute shader later; the CPU version stays
 *       the reference image and the editor-time path.
 */
#include <Lotte/LotteLib.hpp>
#include <Lotte/GFX/LteImageConvert.hpp>
#include <Lotte/Procedural/LteSdf.hpp>
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <glm/glm.hpp>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace Lotte {
namespace Procedural {

/** @brief One material's answer for one pixel. */
struct MaterialSample {
	/** @brief Linear RGB albedo before lighting. */
	glm::fvec3 albedo = glm::fvec3(1.0f);
	/** @brief Grain height added on top of the structural height. */
	float micro = 0.0f;
	/** @brief Metalness in `[0, 1]`; tints the specular highlight warm. */
	float metal = 0.0f;
	/** @brief Broad-white specular amount in `[0, 1]`. */
	float shine = 0.0f;
	/** @brief Specular exponent (Blinn-Phong); higher is glossier. */
	float gloss = 20.0f;
};

/**
 * @brief Paints one layer: `(position, signed distance) -> material`.
 *
 * The signed distance is passed in so a material can fade at the edge (frosting,
 * burn, gem depth) without recomputing the shape.
 */
using SurfaceMaterial = std::function<MaterialSample(glm::fvec2 position, float signedDistance)>;

/** @brief How a layer contributes to the height field. */
struct PaintOptions {
	/** @brief Width in pixels of the chamfered edge. */
	float bevel = 3.0f;
	/** @brief Height gained per pixel of inward distance, up to @ref bevel. */
	float slope = 0.85f;
	/** @brief `+1` for a raised layer, `-1` for a sunken one. */
	float sign = 1.0f;
	/** @brief Constant height added to every covered pixel. */
	float lift = 0.0f;
};

/** @brief One accumulated texel. */
struct SurfacePixel {
	/** @brief Accumulated albedo, straight alpha. */
	glm::fvec3 albedo = glm::fvec3(0.0f);
	/** @brief Coverage in `[0, 1]`. */
	float alpha = 0.0f;
	/** @brief Structural height. */
	float height = 0.0f;
	/** @brief Grain height. */
	float micro = 0.0f;
	/** @brief Metalness in `[0, 1]`. */
	float metal = 0.0f;
	/** @brief Broad specular amount in `[0, 1]`. */
	float shine = 0.0f;
	/** @brief Specular exponent. */
	float gloss = 20.0f;
	/** @brief Additive emissive light. */
	glm::fvec3 emissive = glm::fvec3(0.0f);
};

/** @brief Lighting settings of @ref SurfaceCanvas::shade. */
struct ShadeOptions {
	/** @brief Composite a blurred, offset copy of the alpha into a drop shadow. */
	bool dropShadow = true;
	/** @brief Direction towards the key light; normalized on use. */
	glm::fvec3 lightDirection = glm::fvec3(-0.5f, -0.65f, 0.75f);
	/** @brief Fraction of light that reaches a surface facing away from the key light. */
	float ambient = 0.30f;
	/** @brief Weight of @ref SurfacePixel::micro in the normal's height field. */
float microScale = 2.0f;
/** @brief Opacity of the drop shadow in `[0, 1]`. */
float shadowAlpha = 0.62f;
/** @brief Drop-shadow offset in pixels. */
glm::ivec2 shadowOffset = glm::ivec2(-3, -4);
};

/**
 * @brief Per-pixel albedo, height and material buffer a painter writes into.
 *
 * Painting is order-dependent (a later layer covers an earlier one), shading is not;
 * that split is what lets the expensive part (normals, blur, lighting) run once.
 */
class MH_LOTTE_API SurfaceCanvas
{
public:
/** @brief Creates an empty canvas; @ref isValid is false. */
SurfaceCanvas() = default;
/**
 * @brief Creates a transparent canvas.
 * @param width Width in pixels.
 * @param height Height in pixels.
 */
SurfaceCanvas(std::uint32_t width, std::uint32_t height);

/** @brief Width in pixels. */
std::uint32_t width() const noexcept { return widthValue; }
/** @brief Height in pixels. */
std::uint32_t height() const noexcept { return heightValue; }
/** @brief True when the canvas has pixels. */
bool isValid() const noexcept { return widthValue != 0 && heightValue != 0; }

/**
 * @brief One texel.
 * @param x Column; out of range is undefined.
 * @param y Row; out of range is undefined.
 */
SurfacePixel& at(std::uint32_t x, std::uint32_t y) noexcept
{
return texels[std::size_t(y) * widthValue + x];
}
/**
 * @brief One texel.
 * @param x Column; out of range is undefined.
 * @param y Row; out of range is undefined.
 */
const SurfacePixel& at(std::uint32_t x, std::uint32_t y) const noexcept
{
return texels[std::size_t(y) * widthValue + x];
}
/** @brief All texels, row-major. */
std::span<SurfacePixel> pixels() noexcept { return texels; }
/** @brief All texels, row-major. */
std::span<const SurfacePixel> pixels() const noexcept { return texels; }

/**
 * @brief Paints one shape with one material, accumulating over what is there.
 * @param shape Signed distance field; only pixels with `d < 0.5` are touched.
 * @param material Material evaluated per covered pixel.
 * @param options Height ramp and raised/sunken sign.
 */
void paint(const SdfFunction& shape, const SurfaceMaterial& material,
const PaintOptions& options = {});

/**
 * @brief Adds a soft additive glow around a shape.
 * @param shape Shape whose outside the glow fades over.
 * @param colour Linear RGB of the light.
 * @param radius Falloff distance in pixels.
 * @param intensity Peak intensity at the edge.
 */
void glow(const SdfFunction& shape, glm::fvec3 colour, float radius, float intensity);

/**
 * @brief Composites another canvas into this one with premultiplied alpha.
 * @param source Canvas to stamp.
 * @param destination Top-left destination pixel; may be negative.
 */
void blit(const SurfaceCanvas& source, glm::ivec2 destination);

/**
 * @brief Turns the height field into a lit RGBA8 image.
 * @param options Light direction, ambient level, shadow toggle.
 * @return The shaded image, ready for upload or for a Euphemy encoder.
 */
Gfx::Rgba8Image shade(const ShadeOptions& options = {}) const;

private:
/** @brief Canvas width in pixels. */
std::uint32_t widthValue = 0;
/** @brief Canvas height in pixels. */
std::uint32_t heightValue = 0;
/** @brief Row-major texels. */
std::vector<SurfacePixel> texels;
};

/**
 * @brief Wraps an RGBA8 surface as a Euphemy decode target.
 * @param image Tightly packed RGBA8 image.
 * @return A single-frame `RGBA8U` decode target that the PNG/TGA/DDS encoders accept.
 */
MH_LOTTE_API Euph::Media::Image::DecodeTarget toDecodeTarget(const Gfx::Rgba8Image& image);

} // namespace Procedural
} // namespace Lotte

#endif // LTESURFACECANVAS_HPP
