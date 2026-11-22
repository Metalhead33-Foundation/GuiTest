#ifndef LTECLIPSPACECONVENTIONS_HPP
#define LTECLIPSPACECONVENTIONS_HPP
/**
 * @file LteClipSpaceConventions.hpp
 * @brief All backend-specific fixed-function conventions, resolved once and applied silently.
 *
 * A vertex shader writes clip space; the fixed-function stage then interprets it,
 * and that interpretation is *not* portable between graphics APIs. Getting it wrong
 * is the classic "everything is upside down on Vulkan" bug, and it reappears in
 * three other disguises (winding, depth range, rectangle origin). This object is
 * the single place in Lotte that knows about all four, so no renderer, GUI widget
 * or 3D scene has to.
 *
 * | Question | Vulkan | OpenGL / OpenGL ES / Direct3D |
 * |---|---|---|
 * | Which end of the framebuffer is clip `y = +1`? | the bottom | the top |
 * | Which clip-space winding is front, for the same picture? | the opposite one | - |
 * | Depth range in clip space | `[0, 1]` | `[0, 1]` (the backend rewrites `z`) |
 * | Viewport/scissor rectangle origin | bottom-left after the backend's flip | bottom-left |
 *
 * @section lte_clip_space_contract The canonical Lotte space
 *
 * Every helper takes data in one canonical space and returns what the *current*
 * backend needs for an identical picture:
 *
 * - **2D** - pixels, origin at the target's top-left, `+y` pointing down, `z = 0`.
 *   Pass positions through @ref ClipSpaceConventions::pixelToClip.
 * - **3D** - right-handed camera looking down `-z`, `+y` up, depth in `[0, 1]`
 *   (the `_ZO` convention). Build the matrix with
 *   @ref ClipSpaceConventions::perspective or
 *   @ref ClipSpaceConventions::orthographic, or hand an existing y-up `[0, 1]`
 *   projection to @ref ClipSpaceConventions::projection.
 * - **Winding** - declare front faces the way the projection places them on screen
 *   (`CounterClockwise` for the usual case) and pass the value through
 *   @ref ClipSpaceConventions::frontFace.
 * - **Rectangles** - give viewport and scissor rectangles in top-left pixels and
 *   pass them through @ref ClipSpaceConventions::halRect.
 *
 * A `[-1, 1]` depth projection (for example `glm::perspectiveRH`) is *not* part of
 * the contract: Vulkan clips it away and the GL backend's `z` rewrite mangles it.
 * Use the `_ZO` builders above and the problem cannot occur.
 *
 * @section lte_clip_space_wiring How it reaches renderers
 *
 * Nobody reads `Kld::HalCapabilities` for this. A renderer takes a
 * @ref Lotte::Gfx::ResourceFactory, and
 * @ref Lotte::Gfx::ResourceFactory::conventions() has already derived the object
 * from the device attached to it:
 *
 * @code
 * const Lotte::Gfx::ClipSpaceConventions& space = factory.conventions();
 * // 2D: positions in top-left pixels
 * vertices.push_back({ space.pixelToClip({ 10.0f, 20.0f }, { 640.0f, 480.0f }), colour });
 * // 3D: a camera matrix that is correct on every backend
 * const glm::mat4 projection = space.perspective(glm::radians(60.0f), aspect, 0.1f, 500.0f);
 * const glm::mat4 viewProjection = projection * camera.viewMatrix();
 * @endcode
 *
 * Nothing above mentions a backend, and no user of Lotte ever sees one.
 */
#include <Lotte/LotteLib.hpp>
#include <Kaldi/KldDevice.hpp>
#include <Kaldi/KldHal.hpp>
#include <Kaldi/KaldiOperation.hpp>
#include <glm/glm.hpp>
#include <cstdint>

namespace Lotte {
namespace Gfx {

/** @brief Axis-aligned rectangle in pixels, origin at the target's top-left corner. */
struct PixelRect {
	/** @brief Left edge in pixels. */
	float x = 0.0f;
	/** @brief Top edge in pixels. */
	float y = 0.0f;
	/** @brief Width in pixels; may be negative to express a mirrored rect. */
	float width = 0.0f;
	/** @brief Height in pixels; may be negative to express a mirrored rect. */
	float height = 0.0f;

	/** @brief Bottom edge in pixels. */
	float bottom() const noexcept { return y + height; }
	/** @brief Right edge in pixels. */
	float right() const noexcept { return x + width; }
};

/**
 * @brief Resolved fixed-function conventions of one backend.
 *
 * Copyable, cheap (one enum) and deterministic: the same object always produces
 * the same mapping. Obtain it from
 * @ref Lotte::Gfx::ResourceFactory::conventions() instead of building it by hand.
 */
class MH_LOTTE_API ClipSpaceConventions {
public:
	/**
	 * @brief The OpenGL/Direct3D defaults (`y = +1` at the top).
	 *
	 * Also what a device reports before it has probed a backend, which keeps the
	 * pre-HAL-1.2 assumption and is correct for every API except Vulkan.
	 */
	ClipSpaceConventions() noexcept = default;

	/**
	 * @brief Derives the conventions from a probed capability snapshot.
	 * @param capabilities Snapshot whose `clipSpaceYAxis` is honoured.
	 */
	static ClipSpaceConventions fromCapabilities(const Kld::HalCapabilities& capabilities) noexcept;
	/**
	 * @brief Derives the conventions from a device, or the OpenGL/Direct3D defaults.
	 * @param device Any device; `nullptr` selects the defaults.
	 */
	static ClipSpaceConventions fromDevice(const Kld::Device* device) noexcept;
	/**
	 * @brief Forces a specific y axis, for offline recording and tests.
	 * @param axis Convention to assume.
	 */
	static ClipSpaceConventions fromYAxis(Kld::ClipSpaceYAxis axis) noexcept;

	/** @brief The clip-space y direction this object was built for. */
	Kld::ClipSpaceYAxis yAxis() const noexcept { return axis_; }
	/** @brief True for Vulkan-style clip space (`y = +1` at the bottom). */
	bool yDown() const noexcept { return axis_ == Kld::ClipSpaceYAxis::Down; }

	/**
	 * @brief 2D: maps a top-left pixel position to clip space.
	 * @param pixel Position in pixels, origin at the target's top-left corner.
	 * @param viewport Target size in pixels.
	 * @return Clip-space position; `z = 0` is implicit and valid on every backend.
	 *
	 * A non-positive viewport (which happens between window creation and the first
	 * resize) collapses to the origin instead of producing infinities.
	 */
	glm::fvec2 pixelToClip(glm::fvec2 pixel, glm::fvec2 viewport) const noexcept;

	/**
	 * @brief 3D: adapts a y-up, `[0, 1]`-depth projection to the backend.
	 * @param yUpProjection Projection with `+y` up and depth in `[0, 1]`
	 *        (`glm::perspectiveRH_ZO`, `glm::orthoRH_ZO`, ...).
	 * @return The same projection, y-mirrored when the backend needs it.
	 *
	 * Only the y row is touched; the depth axis is already portable because the GL
	 * backend rewrites clip `z` in the shader.
	 */
	glm::mat4 projection(const glm::mat4& yUpProjection) const noexcept;

	/**
	 * @brief 3D: builds a right-handed, y-up, `[0, 1]`-depth perspective projection.
	 * @param fovYRadians Vertical field of view in radians; larger than 0.
	 * @param aspect Width/height ratio; larger than 0.
	 * @param nearPlane Distance to the near plane; larger than 0.
	 * @param farPlane Distance to the far plane; larger than @p nearPlane.
	 */
	glm::mat4 perspective(float fovYRadians, float aspect, float nearPlane,
			      float farPlane) const noexcept;
	/**
	 * @brief 3D: builds a right-handed, y-up, `[0, 1]`-depth orthographic projection.
	 * @param left Left edge in view space.
	 * @param right Right edge in view space.
	 * @param bottom Bottom edge in view space.
	 * @param top Top edge in view space.
	 * @param nearPlane Distance to the near plane; larger than 0.
	 * @param farPlane Distance to the far plane; larger than @p nearPlane.
	 */
	glm::mat4 orthographic(float left, float right, float bottom, float top, float nearPlane,
			       float farPlane) const noexcept;

	/**
	 * @brief 3D: keeps a declared winding meaning the same thing on screen.
	 * @param declared Winding of front faces as the y-up projection would produce it.
	 * @return The winding to hand to Kaldi's rasterizer state.
	 *
	 * Mirroring the projection for a y-down backend also mirrors the triangle order,
	 * so the declared winding has to be swapped with it; otherwise back faces become
	 * front faces on Vulkan only.
	 */
	Kld::FrontFace frontFace(Kld::FrontFace declared) const noexcept;

	/**
	 * @brief Converts a top-left pixel rectangle into the HAL's bottom-left rectangle.
	 * @param topLeft Rectangle in pixels, origin at the target's top-left corner.
	 * @param targetHeight Height of the render target in pixels.
	 * @return The same rectangle in HAL window space, ready for `SetViewport`/`SetScissor`.
	 *
	 * The HAL documents the OpenGL rectangle convention for both commands, so this
	 * conversion is identical on every backend - but it is still needed whenever a
	 * layout hands you a rectangle in top-left pixels (a 3D view inside a 2D frame).
	 */
	PixelRect halRect(PixelRect topLeft, float targetHeight) const noexcept;

private:
	explicit ClipSpaceConventions(Kld::ClipSpaceYAxis axis) noexcept
		: axis_(axis)
	{
	}

	/** @brief Resolved clip-space y direction. */
	Kld::ClipSpaceYAxis axis_ = Kld::ClipSpaceYAxis::Up;
};

} // namespace Gfx
} // namespace Lotte

#endif // LTECLIPSPACECONVENTIONS_HPP
