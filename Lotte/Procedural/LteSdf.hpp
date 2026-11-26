#ifndef LTESDF_HPP
#define LTESDF_HPP
/**
 * @file LteSdf.hpp
 * @brief Signed distance fields and the shape algebra a procedural texture needs.
 *
 * Ported from the `rbox`/`circle`/`capsule`/`poly`/`uni`/`inset`/`outline`/`rough`
 * helpers of `Sandbox/slop_advice/gui_procedural_generation/claude/ui_forge.cpp`,
 * with the file/atlas writing removed (that belongs to the caller, and the project
 * already has `Euph::Media` encoders).
 *
 * A shape is a callable `float(glm::fvec2)`: negative inside, positive outside, and
 * the value is the distance to the nearest edge in pixels. That one convention is
 * what makes the set composable - a bevel is a lerp over the distance, a stroke is
 * `|d| - thickness`, an erosion is `d + k` - and it is why the whole texturing
 * pipeline can be a stack of painted layers instead of per-element drawing code.
 *
 * Everything here is `inline` on purpose: these are tiny, allocation-free helpers
 * that a material calls millions of times, so they must be inlinable into the
 * painter's hot loop rather than behind an exported symbol.
 *
 * @note GPU plan: an SDF is exactly what a compute shader evaluates per pixel, so
 *       this header is the CPU reference each future shader is checked against.
 */
#include <Lotte/LotteLib.hpp>
#include <Euphemy/Util/EuphNoise.hpp>
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace Lotte {
namespace Procedural {

/** @brief A signed distance function: negative inside, positive outside, in pixels. */
using SdfFunction = std::function<float(glm::fvec2)>;

/**
 * @brief Rounded axis-aligned box.
 * @param topLeft Top-left corner in pixels.
 * @param bottomRight Bottom-right corner in pixels.
 * @param radius Corner radius in pixels; values larger than half the box are clamped.
 * @return The distance to the box's outline.
 */
inline SdfFunction roundedBox(glm::fvec2 topLeft, glm::fvec2 bottomRight, float radius = 0.0f)
{
	return [=](glm::fvec2 point) {
		const glm::fvec2 half = (bottomRight - topLeft) * 0.5f;
		const float clampedRadius = std::min({ radius, half.x, half.y });
		const glm::fvec2 centre = (topLeft + bottomRight) * 0.5f;
		// The offset has to survive *unclamped* into the interior term: a point deep inside
		// the box must report a real negative distance. Clamping each component to zero
		// first makes every interior point return exactly `-radius`, which turns an
		// `outline()` of this box into a full-canvas fill and flattens every bevel.
		const glm::fvec2 offset = glm::abs(point - centre) - (half - glm::fvec2(clampedRadius));
		const glm::fvec2 outward = glm::max(offset, glm::fvec2(0.0f));
		return glm::length(outward) + std::min(std::max(offset.x, offset.y), 0.0f) -
			   clampedRadius;
	};
}

/**
 * @brief Circle.
 * @param centre Centre in pixels.
 * @param radius Radius in pixels.
 * @return The distance to the circle's outline.
 */
inline SdfFunction circle(glm::fvec2 centre, float radius)
{
	return [=](glm::fvec2 point) { return glm::length(point - centre) - radius; };
}

/**
 * @brief Capsule (thick line segment).
 * @param from First endpoint in pixels.
 * @param to Second endpoint in pixels.
 * @param radius Half thickness in pixels.
 * @return The distance to the capsule's outline.
 */
inline SdfFunction capsule(glm::fvec2 from, glm::fvec2 to, float radius)
{
return [=](glm::fvec2 point) {
const glm::fvec2 segment = to - from;
const float lengthSquared = glm::dot(segment, segment);
const float t = lengthSquared > 0.0f
? std::clamp(glm::dot(point - from, segment) / lengthSquared, 0.0f, 1.0f)
: 0.0f;
return glm::length(point - (from + segment * t)) - radius;
};
}

/**
 * @brief Convex or concave polygon (even-odd inside test, distance to the closest edge).
 * @param vertices Polygon corners in pixels, in order.
 * @return The distance to the polygon's outline.
 * @note Fewer than three vertices produce a shape that is empty everywhere, which is
 *       what a degenerate polygon should look like.
 */
inline SdfFunction polygon(std::span<const glm::fvec2> vertices)
{
	const std::vector<glm::fvec2> corners(vertices.begin(), vertices.end());
	return [corners](glm::fvec2 point) {
		if (corners.size() < 3) return 1.0e9f;
		float squaredDistance = glm::dot(point - corners[0], point - corners[0]);
		float sign = 1.0f;
		for (std::size_t i = 0, j = corners.size() - 1; i < corners.size(); j = i, ++i) {
			const glm::fvec2 edge = corners[j] - corners[i];
			const glm::fvec2 toPoint = point - corners[i];
			const float lengthSquared = glm::dot(edge, edge);
			const float t = lengthSquared > 0.0f
				? std::clamp(glm::dot(toPoint, edge) / lengthSquared, 0.0f, 1.0f)
				: 0.0f;
			const glm::fvec2 projected = toPoint - edge * t;
			squaredDistance = std::min(squaredDistance, glm::dot(projected, projected));
			const bool c0 = point.y >= corners[i].y;
			const bool c1 = point.y < corners[j].y;
			const bool c2 = edge.x * toPoint.y > edge.y * toPoint.x;
			if ((c0 && c1 && c2) || (!c0 && !c1 && !c2)) sign = -sign;
		}
		return sign * std::sqrt(squaredDistance);
	};
}

/**
 * @brief Diamond (a four-corner polygon), used by the gem bevels.
 * @param centre Centre in pixels.
 * @param halfWidth Horizontal half extent in pixels.
 * @param halfHeight Vertical half extent in pixels.
 * @return The distance to the diamond's outline.
 */
inline SdfFunction diamond(glm::fvec2 centre, float halfWidth, float halfHeight)
{
const std::vector<glm::fvec2> corners = {
{ centre.x, centre.y - halfHeight },
{ centre.x + halfWidth, centre.y },
{ centre.x, centre.y + halfHeight },
{ centre.x - halfWidth, centre.y },
};
return polygon(std::span<const glm::fvec2>(corners));
}

/**
 * @brief Star polygon, used by the compass tile.
 * @param centre Centre in pixels.
 * @param outerRadius Radius of the tips in pixels.
 * @param innerRadius Radius of the notches in pixels.
 * @param points Number of tips; at least one.
 * @return The distance to the star's outline.
 */
inline SdfFunction star(glm::fvec2 centre, float outerRadius, float innerRadius,
std::uint32_t points = 4)
{
	std::vector<glm::fvec2> corners;
	const std::uint32_t tips = std::max<std::uint32_t>(points, 1u);
	for (std::uint32_t index = 0; index < tips * 2; ++index) {
		const float angle = -1.5707963f + static_cast<float>(index) * 3.14159265f /
										static_cast<float>(tips);
		const float radius = (index % 2 == 0) ? outerRadius : innerRadius;
		corners.push_back(centre + glm::fvec2(std::cos(angle), std::sin(angle)) * radius);
	}
	return polygon(std::span<const glm::fvec2>(corners));
}

/**
 * @brief Union of two shapes.
 * @param first First shape.
 * @param second Second shape.
 * @return The pointwise minimum of the two distances.
 */
inline SdfFunction unite(SdfFunction first, SdfFunction second)
{
	return [first = std::move(first), second = std::move(second)](glm::fvec2 point) {
		return std::min(first(point), second(point));
	};
}

/**
 * @brief Erodes or dilates a shape.
 * @param shape Shape to offset.
 * @param distance Positive shrinks (inset), negative grows (outset).
 * @return The offset distance field.
 */
inline SdfFunction inset(SdfFunction shape, float distance)
{
	return [shape = std::move(shape), distance](glm::fvec2 point) {
		return shape(point) + distance;
	};
}

/**
 * @brief Stroke along a shape's outline.
 * @param shape Shape whose outline is stroked.
 * @param thickness Stroke width in pixels.
 * @param offset Pixels to move inside (positive) or outside (negative) the outline.
 * @return The distance to the stroke band.
 */
inline SdfFunction outline(SdfFunction shape, float thickness, float offset = 0.0f)
{
return [shape = std::move(shape), thickness, offset](glm::fvec2 point) {
return std::abs(shape(point) + offset) - thickness * 0.5f;
};
}

/**
 * @brief Perturbs an outline with noise so it reads as hand-hewn rather than vector.
 * @param shape Shape to roughen.
 * @param amplitude Maximum displacement in pixels.
 * @param frequency Noise frequency per pixel.
 * @param seed Hash seed of the noise.
 * @return The perturbed distance field.
 */
inline SdfFunction roughen(SdfFunction shape, float amplitude, float frequency, std::uint32_t seed)
{
return [shape = std::move(shape), amplitude, frequency, seed](glm::fvec2 point) {
// Gradient noise, not value noise: a chipped edge wants no lattice plateau, and the
// fine octave is capped near 2 px so the outline does not alias into speckle.
const float macro = Euph::Util::Noise::latticeGradientFbm2D(point.x * frequency,
point.y * frequency, seed, 3) - 0.5f;
const float micro = Euph::Util::Noise::latticeGradient2D(point.x * 0.45f, point.y * 0.45f,
seed + 5u) - 0.5f;
return shape(point) + (macro * 2.0f + micro * 0.5f) * amplitude;
};
}

} // namespace Procedural
} // namespace Lotte

#endif // LTESDF_HPP
