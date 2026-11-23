/**
 * @file ClipSpaceConventionsTests.cpp
 * @brief Verifies `Lotte::Gfx::ClipSpaceConventions`, the one place that knows how a
 *        backend interprets the fixed-function stage.
 *
 * Four things are easy to get silently wrong: the clip-space y direction, winding,
 * the depth range and the origin of viewport rectangles. These tests pin the
 * canonical Lotte space (top-left pixels for 2D, y-up `[0, 1]`-depth projections for
 * 3D) to the values each convention needs.
 */
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <Lotte/GFX/LteClipSpaceConventions.hpp>
#include <Lotte/GFX/LteResourceFactory.hpp>
#include <Lotte/LteKld.hpp>
#include <glm/glm.hpp>
#include <cmath>
#include <span>

namespace {

using Lotte::Gfx::ClipSpaceConventions;
using Lotte::Gfx::PixelRect;

/** @brief A device that reports a chosen clip-space y axis. */
class AxisDevice final : public Kld::Device {
public:
	explicit AxisDevice(Kld::ClipSpaceYAxis axis)
	{
		capabilities_.version = Kld::KaldiHalVersion1_2;
		capabilities_.clipSpaceYAxis = axis;
	}
	const Kld::HalCapabilities& capabilities() const override { return capabilities_; }
	Kld::HalStatus processCommandQueue(std::span<const Kld::GfxOp>) override
	{
		return Kld::HalOk;
	}
private:
	Kld::HalCapabilities capabilities_ = Kld::halCapabilitiesNone();
};

/** @brief Transforms a view-space point and divides by w, as the rasterizer does. */
glm::vec4 toNdc(const glm::mat4& projection, glm::vec3 point)
{
	const glm::vec4 clip = projection * glm::vec4(point, 1.0f);
	return clip / clip.w;
}

} // namespace

TEST_CASE("The clip-space y axis comes from the device, with GL/D3D as the fallback",
	  "[gui][clip-space]")
{
	CHECK(ClipSpaceConventions{}.yAxis() == Kld::ClipSpaceYAxis::Up);
	CHECK_FALSE(ClipSpaceConventions{}.yDown());

	AxisDevice vulkan(Kld::ClipSpaceYAxis::Down);
	AxisDevice gl(Kld::ClipSpaceYAxis::Up);
	CHECK(ClipSpaceConventions::fromDevice(&vulkan).yDown());
	CHECK_FALSE(ClipSpaceConventions::fromDevice(&gl).yDown());

	// No probed device: the pre-HAL-1.2 assumption, which only Vulkan contradicts.
	CHECK_FALSE(ClipSpaceConventions::fromDevice(nullptr).yDown());

	Kld::HalCapabilities capabilities = Kld::halCapabilitiesNone();
	CHECK_FALSE(ClipSpaceConventions::fromCapabilities(capabilities).yDown());
	capabilities.clipSpaceYAxis = Kld::ClipSpaceYAxis::Down;
	CHECK(ClipSpaceConventions::fromCapabilities(capabilities).yDown());
}

TEST_CASE("Top-left pixels map onto the clip space of the backend", "[gui][clip-space]")
{
	const glm::fvec2 viewport(640.0f, 480.0f);
	const ClipSpaceConventions vulkan = ClipSpaceConventions::fromYAxis(Kld::ClipSpaceYAxis::Down);
	const ClipSpaceConventions gl = ClipSpaceConventions::fromYAxis(Kld::ClipSpaceYAxis::Up);

	// Vulkan: NDC -1 is the top of the framebuffer, +1 the bottom.
	CHECK(vulkan.pixelToClip({ 0.0f, 0.0f }, viewport).y == Catch::Approx(-1.0f));
	CHECK(vulkan.pixelToClip({ 0.0f, 480.0f }, viewport).y == Catch::Approx(1.0f));
	// OpenGL/Direct3D: the same pixels mirror vertically.
	CHECK(gl.pixelToClip({ 0.0f, 0.0f }, viewport).y == Catch::Approx(1.0f));
	CHECK(gl.pixelToClip({ 0.0f, 480.0f }, viewport).y == Catch::Approx(-1.0f));

	// x is backend-independent and always left-to-right.
	CHECK(vulkan.pixelToClip({ 0.0f, 0.0f }, viewport).x == Catch::Approx(-1.0f));
	CHECK(gl.pixelToClip({ 640.0f, 0.0f }, viewport).x == Catch::Approx(1.0f));
	// The centre stays the centre under either convention.
	CHECK(vulkan.pixelToClip({ 320.0f, 240.0f }, viewport).y == Catch::Approx(0.0f));
	CHECK(gl.pixelToClip({ 320.0f, 240.0f }, viewport).y == Catch::Approx(0.0f));

	// A degenerate viewport (between window creation and the first resize) collapses
	// instead of producing infinities.
	const glm::fvec2 collapsed = vulkan.pixelToClip({ 10.0f, 10.0f }, glm::fvec2(0.0f));
	CHECK(collapsed.x == Catch::Approx(0.0f));
	CHECK(collapsed.y == Catch::Approx(0.0f));
}

TEST_CASE("A y-up projection is mirrored only for a y-down backend", "[gui][clip-space]")
{
	const ClipSpaceConventions vulkan = ClipSpaceConventions::fromYAxis(Kld::ClipSpaceYAxis::Down);
	const ClipSpaceConventions gl = ClipSpaceConventions::fromYAxis(Kld::ClipSpaceYAxis::Up);
	const glm::mat4 yUp = glm::mat4(1.0f);

	CHECK(gl.projection(yUp) == yUp);

	const glm::mat4 mirrored = vulkan.projection(yUp);
	CHECK(mirrored[0][0] == Catch::Approx(1.0f));
	CHECK(mirrored[1][1] == Catch::Approx(-1.0f));
	CHECK(mirrored[2][2] == Catch::Approx(1.0f));
	CHECK(mirrored[3][3] == Catch::Approx(1.0f));
	// Only the y row changes, so depth and x survive untouched.
	CHECK(mirrored[2][3] == Catch::Approx(yUp[2][3]));
	CHECK(mirrored[3][2] == Catch::Approx(yUp[3][2]));

	// "Above the camera axis" points one way for an y-up backend and the other way
	// for Vulkan, whose -1 is the top of the screen.
	CHECK(gl.projection(yUp)[1][1] > 0.0f);
	CHECK(vulkan.projection(yUp)[1][1] < 0.0f);
}

TEST_CASE("Built projections are y-up with a [0, 1] depth range", "[gui][clip-space]")
{
	const ClipSpaceConventions vulkan = ClipSpaceConventions::fromYAxis(Kld::ClipSpaceYAxis::Down);
	const ClipSpaceConventions gl = ClipSpaceConventions::fromYAxis(Kld::ClipSpaceYAxis::Up);
	const float nearPlane = 0.5f;
	const float farPlane = 100.0f;

	for (const ClipSpaceConventions* space : { &gl, &vulkan }) {
		const glm::mat4 projection =
			space->perspective(glm::radians(90.0f), 1.0f, nearPlane, farPlane);

		// Depth is [0, 1] under both conventions: Vulkan demands exactly that, and the
		// GL backend rewrites z in the shader. A [-1, 1] matrix would be clipped away.
		CHECK(toNdc(projection, glm::vec3(0.0f, 0.0f, -nearPlane)).z == Catch::Approx(0.0f));
		CHECK(toNdc(projection, glm::vec3(0.0f, 0.0f, -farPlane)).z == Catch::Approx(1.0f));

		// 90 degree vertical FOV at aspect 1: the near plane is 2 units tall, so a
		// point one unit above the axis at distance 1 lands exactly on the edge.
		const float aboveY = toNdc(projection, glm::vec3(0.0f, 1.0f, -1.0f)).y;
		const float belowY = toNdc(projection, glm::vec3(0.0f, -1.0f, -1.0f)).y;
		CHECK(std::abs(aboveY) == Catch::Approx(1.0f));
		CHECK(std::abs(belowY) == Catch::Approx(1.0f));
		// "Above" is +1 for OpenGL/Direct3D and -1 for Vulkan; "below" mirrors it.
		CHECK((aboveY > 0.0f) == !space->yDown());
		CHECK((belowY < 0.0f) == !space->yDown());
	}

	// Orthographic follows the same rules.
	const glm::mat4 ortho = vulkan.orthographic(-8.0f, 8.0f, -8.0f, 8.0f, 0.0f, 1.0f);
	CHECK(toNdc(ortho, glm::vec3(0.0f, 4.0f, -1.0f)).y == Catch::Approx(-0.5f));
	CHECK(toNdc(ortho, glm::vec3(0.0f, 0.0f, -0.5f)).z == Catch::Approx(0.5f));
	CHECK(toNdc(ortho, glm::vec3(8.0f, 0.0f, -1.0f)).x == Catch::Approx(1.0f));
}

TEST_CASE("Winding is swapped together with a mirrored projection", "[gui][clip-space]")
{
	const ClipSpaceConventions vulkan = ClipSpaceConventions::fromYAxis(Kld::ClipSpaceYAxis::Down);
	const ClipSpaceConventions gl = ClipSpaceConventions::fromYAxis(Kld::ClipSpaceYAxis::Up);

	CHECK(gl.frontFace(Kld::FrontFace::CounterClockwise) == Kld::FrontFace::CounterClockwise);
	CHECK(gl.frontFace(Kld::FrontFace::Clockwise) == Kld::FrontFace::Clockwise);

	// Mirroring y reverses the triangle order, so the declaration has to follow it.
	CHECK(vulkan.frontFace(Kld::FrontFace::CounterClockwise) == Kld::FrontFace::Clockwise);
	CHECK(vulkan.frontFace(Kld::FrontFace::Clockwise) == Kld::FrontFace::CounterClockwise);
}

TEST_CASE("Top-left rectangles become HAL bottom-left rectangles", "[gui][clip-space]")
{
	const ClipSpaceConventions space = ClipSpaceConventions::fromYAxis(Kld::ClipSpaceYAxis::Up);
	const PixelRect hal = space.halRect(PixelRect{ 10.0f, 20.0f, 100.0f, 50.0f }, 480.0f);

	CHECK(hal.x == Catch::Approx(10.0f));
	CHECK(hal.y == Catch::Approx(480.0f - 70.0f));
	CHECK(hal.width == Catch::Approx(100.0f));
	CHECK(hal.height == Catch::Approx(50.0f));
	CHECK(hal.bottom() == Catch::Approx(480.0f - 20.0f));

	// A full-target rectangle is the identity, which is why the GUI renderer can use
	// the same path for its own viewport.
	const PixelRect full = space.halRect(PixelRect{ 0.0f, 0.0f, 640.0f, 480.0f }, 480.0f);
	CHECK(full.x == Catch::Approx(0.0f));
	CHECK(full.y == Catch::Approx(0.0f));
	CHECK(full.width == Catch::Approx(640.0f));
	CHECK(full.height == Catch::Approx(480.0f));
}

TEST_CASE("The resource factory hands its conventions to renderers", "[gui][clip-space]")
{
	Lotte::CommandBuffer dispatcher;
	AxisDevice vulkan(Kld::ClipSpaceYAxis::Down);
	Lotte::Gfx::ResourceFactory factory(dispatcher, &vulkan);
	CHECK(factory.conventions().yDown());

	// Swapping the backend has to swap the conventions with it; otherwise a late
	// device selection would leave a y-flipped UI behind.
	AxisDevice gl(Kld::ClipSpaceYAxis::Up);
	factory.setDevice(&gl);
	CHECK_FALSE(factory.conventions().yDown());

	factory.setDevice(nullptr);
	CHECK_FALSE(factory.conventions().yDown());
}
