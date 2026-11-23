#include "LteClipSpaceConventions.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace Lotte {
namespace Gfx {

ClipSpaceConventions ClipSpaceConventions::fromCapabilities(
	const Kld::HalCapabilities& capabilities) noexcept
{
	return ClipSpaceConventions(capabilities.clipSpaceYAxis);
}

ClipSpaceConventions ClipSpaceConventions::fromDevice(const Kld::Device* device) noexcept
{
	// No device means "not probed yet", which the HAL defines as the OpenGL/Direct3D
	// convention - the same assumption every pre-1.2 consumer made.
	return device != nullptr ? fromCapabilities(device->capabilities()) : ClipSpaceConventions{};
}

ClipSpaceConventions ClipSpaceConventions::fromYAxis(Kld::ClipSpaceYAxis axis) noexcept
{
	return ClipSpaceConventions(axis);
}

glm::fvec2 ClipSpaceConventions::pixelToClip(glm::fvec2 pixel, glm::fvec2 viewport) const noexcept
{
	if (viewport.x <= 0.0f || viewport.y <= 0.0f) {
		return glm::fvec2(0.0f);
	}
	const float x = 2.0f * pixel.x / viewport.x - 1.0f;
	const float y = 2.0f * pixel.y / viewport.y - 1.0f;
	// `y` above is already expressed for a y-down backend (pixel 0 -> -1, the top
	// under Vulkan). A y-up backend needs the mirrored value instead.
	return yDown() ? glm::fvec2(x, y) : glm::fvec2(x, -y);
}

glm::mat4 ClipSpaceConventions::projection(const glm::mat4& yUpProjection) const noexcept
{
	if (!yDown()) {
		return yUpProjection;
	}
	// glm is column-major, so `m[column][row]`: row 1 is the clip-space y row.
	glm::mat4 mirrored = yUpProjection;
	mirrored[0][1] = -mirrored[0][1];
	mirrored[1][1] = -mirrored[1][1];
	mirrored[2][1] = -mirrored[2][1];
	mirrored[3][1] = -mirrored[3][1];
	return mirrored;
}

glm::mat4 ClipSpaceConventions::perspective(float fovYRadians, float aspect, float nearPlane,
					    float farPlane) const noexcept
{
	return projection(glm::perspectiveRH_ZO(fovYRadians, aspect, nearPlane, farPlane));
}

glm::mat4 ClipSpaceConventions::orthographic(float left, float right, float bottom, float top,
					     float nearPlane, float farPlane) const noexcept
{
	return projection(glm::orthoRH_ZO(left, right, bottom, top, nearPlane, farPlane));
}

Kld::FrontFace ClipSpaceConventions::frontFace(Kld::FrontFace declared) const noexcept
{
	if (!yDown()) {
		return declared;
	}
	return declared == Kld::FrontFace::CounterClockwise ? Kld::FrontFace::Clockwise
							   : Kld::FrontFace::CounterClockwise;
}

PixelRect ClipSpaceConventions::halRect(PixelRect topLeft, float targetHeight) const noexcept
{
	const PixelRect hal { topLeft.x, targetHeight - topLeft.bottom(), topLeft.width,
			      topLeft.height };
	return hal;
}

} // namespace Gfx
} // namespace Lotte
