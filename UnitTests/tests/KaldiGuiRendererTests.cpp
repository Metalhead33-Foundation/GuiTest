/**
 * @file KaldiGuiRendererTests.cpp
 * @brief CPU-visible parts of `Lotte::Gui::KaldiGuiRenderer`.
 *
 * Without a GPU the only testable piece is the pixel-to-clip-space transform, and
 * it happens to be the piece that goes wrong: Vulkan's clip space is y-down while
 * OpenGL's and Direct3D's is y-up. `Kld::HalCapabilities::clipSpaceYAxis` reports
 * which one a backend uses, and the renderer must follow it. The recorded frame
 * itself is smoke-tested by the Sandbox against the Vulkan backend.
 */
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <Lotte/Gui/LteKaldiGuiRenderer.hpp>
#include <span>
#include <vector>

TEST_CASE("Pixel positions map to the clip space of the backend", "[gui][clip-space]")
{
	using Lotte::Gui::pixelToClipSpace;
	const glm::fvec2 viewport(640.0f, 480.0f);

	// Vulkan (y-down): NDC -1 is the top of the framebuffer, +1 the bottom.
	const glm::fvec2 vulkanTopLeft =
		pixelToClipSpace(glm::fvec2(0.0f, 0.0f), viewport, Kld::ClipSpaceYAxis::Down);
	CHECK(vulkanTopLeft.x == Catch::Approx(-1.0f));
	CHECK(vulkanTopLeft.y == Catch::Approx(-1.0f));
	const glm::fvec2 vulkanBottomRight = pixelToClipSpace(glm::fvec2(640.0f, 480.0f),
							      viewport,
							      Kld::ClipSpaceYAxis::Down);
	CHECK(vulkanBottomRight.x == Catch::Approx(1.0f));
	CHECK(vulkanBottomRight.y == Catch::Approx(1.0f));
	// The center stays the center under either convention.
	CHECK(pixelToClipSpace(glm::fvec2(320.0f, 240.0f), viewport, Kld::ClipSpaceYAxis::Down).y
	      == Catch::Approx(0.0f));

	// OpenGL/Direct3D (y-up): the same pixels mirror vertically.
	const glm::fvec2 glTopLeft =
		pixelToClipSpace(glm::fvec2(0.0f, 0.0f), viewport, Kld::ClipSpaceYAxis::Up);
	CHECK(glTopLeft.x == Catch::Approx(-1.0f));
	CHECK(glTopLeft.y == Catch::Approx(1.0f));
	const glm::fvec2 glBottomRight =
		pixelToClipSpace(glm::fvec2(640.0f, 480.0f), viewport, Kld::ClipSpaceYAxis::Up);
	CHECK(glBottomRight.x == Catch::Approx(1.0f));
	CHECK(glBottomRight.y == Catch::Approx(-1.0f));

	// x is backend-independent; only the y sign flips.
	CHECK(pixelToClipSpace(glm::fvec2(160.0f, 60.0f), viewport, Kld::ClipSpaceYAxis::Down).x
	      == Catch::Approx(pixelToClipSpace(glm::fvec2(160.0f, 60.0f), viewport,
						Kld::ClipSpaceYAxis::Up).x));
	CHECK(pixelToClipSpace(glm::fvec2(160.0f, 60.0f), viewport, Kld::ClipSpaceYAxis::Down).y
	      == Catch::Approx(-pixelToClipSpace(glm::fvec2(160.0f, 60.0f), viewport,
						 Kld::ClipSpaceYAxis::Up).y));
}

TEST_CASE("A zero-sized viewport yields a neutral clip-space position",
	  "[gui][clip-space]")
{
	using Lotte::Gui::pixelToClipSpace;
	const glm::fvec2 origin =
		pixelToClipSpace(glm::fvec2(10.0f, 10.0f), glm::fvec2(0.0f, 0.0f),
				 Kld::ClipSpaceYAxis::Down);
	CHECK(origin.x == Catch::Approx(0.0f));
	CHECK(origin.y == Catch::Approx(0.0f));
}

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
 */
Lotte::Gui::ColouredGuiVertex firstUploadedVertex(Kld::ClipSpaceYAxis axis)
{
	Lotte::CommandBuffer dispatcher;
	AxisRecordingDevice device(axis);
	Lotte::Gfx::ResourceFactory factory(dispatcher, &device);
	Lotte::Gui::KaldiGuiRenderer renderer(
		factory, Lotte::Gui::KaldiGuiRendererSettings{ 640, 480 });

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

