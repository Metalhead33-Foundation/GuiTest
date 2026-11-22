/**
 * @file KaldiGuiRendererTests.cpp
 * @brief End-to-end check of `Lotte::Gui::KaldiGuiRenderer`'s clip-space transform.
 *
 * The renderer never touches `Kld::HalCapabilities` itself: it asks the resource
 * factory for its @ref Lotte::Gfx::ClipSpaceConventions and applies them while
 * building the vertex cache. These tests drive a device that reports a chosen axis,
 * record one quad, and read the uploaded vertices back - so they cover the whole
 * chain from "which backend is this" to "which clip-space numbers reach the GPU"
 * without needing one. The helper's own matrix/winding/rectangle behaviour is
 * covered by `ClipSpaceConventionsTests.cpp`; the live frame is smoke-tested by the
 * Sandbox against the Vulkan and OpenGL backends.
 */
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <Lotte/Gui/LteKaldiGuiRenderer.hpp>
#include <optional>
#include <span>
#include <vector>

namespace {

/** @brief A device that records submissions and reports a chosen clip-space y axis. */
class AxisRecordingDevice final : public Kld::Device {
public:
	explicit AxisRecordingDevice(Kld::ClipSpaceYAxis axis)
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

/** @brief Copies the dispatcher's write buffer. */
std::vector<Kld::GfxOp> writeOps(Lotte::CommandBuffer& dispatcher)
{
	std::vector<Kld::GfxOp> ops;
	dispatcher.getCommandBuffer().onWriteBuffer(
		[&ops](const auto& vec) { ops.assign(vec.begin(), vec.end()); });
	return ops;
}

/**
 * @brief Renders one coloured quad and returns the first uploaded vertex.
 *
 * The renderer uploads its CPU vertex cache through `CreateBufferObject`, so the
 * recorded payload is exactly the clip-space geometry that would reach the GPU.
 * @param axis Axis the fake device reports.
 * @param overrideAxis Optional explicit settings override, which must win.
 */
Lotte::Gui::ColouredGuiVertex firstUploadedVertex(
	Kld::ClipSpaceYAxis axis,
	std::optional<Kld::ClipSpaceYAxis> overrideAxis = std::nullopt)
{
	Lotte::CommandBuffer dispatcher;
	AxisRecordingDevice device(axis);
	Lotte::Gfx::ResourceFactory factory(dispatcher, &device);
	Lotte::Gui::KaldiGuiRendererSettings settings{ 640, 480 };
	settings.clipSpaceYAxis = overrideAxis;
	Lotte::Gui::KaldiGuiRenderer renderer(factory, settings);

	Lotte::Gui::ColouredQuad quad;
	quad.topLeft = glm::fvec2(10.0f, 20.0f);
	quad.bottomRight = glm::fvec2(110.0f, 70.0f);
	quad.colour = glm::fvec4(1.0f);
	renderer.beginFrame();
	renderer.render(quad);
	renderer.flush();

	for (const Kld::GfxOp& op : writeOps(dispatcher)) {
		if (op.opcode == Kld::Opcode::CreateBufferObject &&
		    op.opt.opCreateBufferObject.policy == Kld::BUFFER_USAGE_VERTEX &&
		    op.opt.opCreateBufferObject.data != nullptr) {
			return static_cast<const Lotte::Gui::ColouredGuiVertex*>(
				       op.opt.opCreateBufferObject.data)[0];
		}
	}
	return Lotte::Gui::ColouredGuiVertex{};
}

} // namespace

TEST_CASE("The renderer takes its clip-space y axis from the device", "[gui][clip-space]")
{
	// Vulkan: pixel (10, 20) becomes x = 2*10/640 - 1, y = 2*20/480 - 1 (y-down).
	const Lotte::Gui::ColouredGuiVertex vulkanVertex =
		firstUploadedVertex(Kld::ClipSpaceYAxis::Down);
	CHECK(vulkanVertex.position.x == Catch::Approx(2.0f * 10.0f / 640.0f - 1.0f));
	CHECK(vulkanVertex.position.y == Catch::Approx(2.0f * 20.0f / 480.0f - 1.0f));
	// The top-left pixel must be above the bottom-right pixel in clip space too.
	CHECK(vulkanVertex.position.y < 0.0f);

	// OpenGL/Direct3D: the same pixel maps to the mirrored y.
	const Lotte::Gui::ColouredGuiVertex glVertex =
		firstUploadedVertex(Kld::ClipSpaceYAxis::Up);
	CHECK(glVertex.position.x == Catch::Approx(vulkanVertex.position.x));
	CHECK(glVertex.position.y == Catch::Approx(-vulkanVertex.position.y));
	CHECK(glVertex.position.y > 0.0f);
}

TEST_CASE("The settings switch wins over the device axis", "[gui][clip-space]")
{
	// A Vulkan device rendering with an explicit y-up override is the offline
	// recording / test hook: it must produce the mirrored geometry, not the device's.
	const Lotte::Gui::ColouredGuiVertex deviceAxis =
		firstUploadedVertex(Kld::ClipSpaceYAxis::Down);
	const Lotte::Gui::ColouredGuiVertex overridden =
		firstUploadedVertex(Kld::ClipSpaceYAxis::Down, Kld::ClipSpaceYAxis::Up);

	CHECK(overridden.position.y == Catch::Approx(-deviceAxis.position.y));
	CHECK(overridden.position.y > 0.0f);
	CHECK(overridden.position.x == Catch::Approx(deviceAxis.position.x));
}

