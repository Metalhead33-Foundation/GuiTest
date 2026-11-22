/**
 * @file KaldiCommandBufferTests.cpp
 * @brief Validates the object-oriented facade in `Kaldi/KldCommandBuffer.hpp`.
 *
 * The facade is pure client-side bookkeeping - handle pools, a double-buffered
 * command vector and RAII proxies - so it is testable without a backend: a
 * recording `Kld::Device` captures what `submit()` would hand to a real one.
 */
#include <catch2/catch_test_macros.hpp>
#include <Kaldi/KldCommandBuffer.hpp>
#include <Kaldi/KldDevice.hpp>
#include <Kaldi/KldHal.hpp>
#include <span>
#include <vector>

namespace {

/** @brief A device that remembers the last command span instead of executing it. */
class RecordingDevice final : public Kld::Device {
public:
	const Kld::HalCapabilities& capabilities() const override { return capabilities_; }

	Kld::HalStatus processCommandQueue(std::span<const Kld::GfxOp> commands) override
	{
		submitted.assign(commands.begin(), commands.end());
		return Kld::HalOk;
	}

	/** @brief Copy of the last submitted span. */
	std::vector<Kld::GfxOp> submitted;

private:
	Kld::HalCapabilities capabilities_ = Kld::halCapabilitiesNone();
};

/** @brief Copies the read buffer of @p dispatcher (what `submit()` would see). */
std::vector<Kld::GfxOp> readBufferOf(const Kld::CommandDispatcher<>& dispatcher)
{
	std::vector<Kld::GfxOp> ops;
	dispatcher.getCommandBuffer().onReadBuffer(
		[&ops](const auto& vec) { ops.assign(vec.begin(), vec.end()); });
	return ops;
}

} // namespace

TEST_CASE("Handles are unique across every resource kind", "[kaldi][facade]")
{
	Kld::CommandDispatcher<> dispatcher;
	const Kld::HandleId buffer = dispatcher.acquireId(Kld::ResourceHandleKind::Buffer);
	const Kld::HandleId texture = dispatcher.acquireId(Kld::ResourceHandleKind::Texture);
	const Kld::HandleId sampler = dispatcher.acquireId(Kld::ResourceHandleKind::Sampler);
	const Kld::HandleId image = dispatcher.acquireId(Kld::ResourceHandleKind::Image);

	CHECK(buffer != 0);
	// The backends reject a handle that is already live under *any* kind, so the
	// pools must not be independent.
	CHECK(buffer != texture);
	CHECK(buffer != sampler);
	CHECK(buffer != image);
	CHECK(texture != sampler);
	CHECK(texture != image);
	CHECK(sampler != image);

	dispatcher.freeId(Kld::ResourceHandleKind::Texture, texture);
	CHECK(dispatcher.acquireId(Kld::ResourceHandleKind::Fence) == texture);
}

TEST_CASE("The dispatcher records into the write buffer only", "[kaldi][facade]")
{
	Kld::CommandDispatcher<> dispatcher;
	Kld::OopFacade::setViewport(dispatcher, 0.0f, 0.0f, 64.0f, 64.0f, 0.0f, 1.0f);
	CHECK(readBufferOf(dispatcher).empty());

	dispatcher.getCommandBuffer().swapBuffers();
	const std::vector<Kld::GfxOp> ops = readBufferOf(dispatcher);
	REQUIRE(ops.size() == 1);
	CHECK(ops[0].opcode == Kld::Opcode::SetViewport);
	CHECK(ops[0].opt.opSetViewport.width == 64.0f);
}

TEST_CASE("Proxies record their create and their destroy command", "[kaldi][facade]")
{
	Kld::CommandDispatcher<> dispatcher;
	Kld::HandleId id = 0;
	{
		auto buffer = Kld::OopFacade::createBuffer(dispatcher, 64);
		id = buffer.id();
		CHECK(id != 0);
		// The create command names the handle the proxy wraps.
		dispatcher.getCommandBuffer().swapBuffers();
		const std::vector<Kld::GfxOp> created = readBufferOf(dispatcher);
		REQUIRE(created.size() == 1);
		CHECK(created[0].opcode == Kld::Opcode::CreateBufferObject);
		CHECK(created[0].opt.opCreateBufferObject.id == id);
		CHECK(created[0].opt.opCreateBufferObject.size == 64);
	}
	dispatcher.getCommandBuffer().swapBuffers();
	const std::vector<Kld::GfxOp> ops = readBufferOf(dispatcher);
	REQUIRE(ops.size() == 1);
	CHECK(ops[0].opcode == Kld::Opcode::DestroyBufferObject);
	CHECK(ops[0].opt.opDestroy.id == id);
	// The id went back to the pool, so the next acquisition reuses it.
	CHECK(dispatcher.acquireId(Kld::ResourceHandleKind::Image) == id);
}

TEST_CASE("Moving a proxy transfers the destroy command", "[kaldi][facade]")
{
	Kld::CommandDispatcher<> dispatcher;
	Kld::HandleId id = 0;
	{
		auto source = Kld::OopFacade::createSampler(
			dispatcher, Kld::SamplerFiltering::LINEAR,
			{ Kld::SamplerWrapping::REPEAT, Kld::SamplerWrapping::REPEAT,
			  Kld::SamplerWrapping::REPEAT },
			1.0f, 1000.0f, 0.0f, 0.0f);
		id = source.id();
		auto moved = std::move(source);
		CHECK(source.id() == 0);
		CHECK(moved.id() == id);
		// Only the destination is alive, so only one destroy command appears.
		dispatcher.getCommandBuffer().swapBuffers();
		readBufferOf(dispatcher);
	}
	dispatcher.getCommandBuffer().swapBuffers();
	const std::vector<Kld::GfxOp> ops = readBufferOf(dispatcher);
	REQUIRE(ops.size() == 1);
	CHECK(ops[0].opcode == Kld::Opcode::DestroySampler);
	CHECK(ops[0].opt.opDestroy.id == id);
}

TEST_CASE("mergeCommandBuffers appends sources in order", "[kaldi][facade]")
{
	Kld::CommandDispatcher<> destination;
	Kld::CommandDispatcher<> first;
	Kld::CommandDispatcher<> second;
	Kld::OopFacade::setViewport(first, 1.0f, 1.0f, 8.0f, 8.0f, 0.0f, 1.0f);
	Kld::OopFacade::setScissor(second, 0, 0, 4, 4);
	first.getCommandBuffer().swapBuffers();
	second.getCommandBuffer().swapBuffers();

	Kld::mergeCommandBuffers(destination, first);
	Kld::mergeCommandBuffers(destination, { &second });
	destination.getCommandBuffer().swapBuffers();

	const std::vector<Kld::GfxOp> ops = readBufferOf(destination);
	REQUIRE(ops.size() == 2);
	CHECK(ops[0].opcode == Kld::Opcode::SetViewport);
	CHECK(ops[1].opcode == Kld::Opcode::SetScissor);
}

TEST_CASE("DeferredDestroyQueue records the destroy command of its kind", "[kaldi][facade]")
{
	Kld::CommandDispatcher<> dispatcher;
	const Kld::HandleId texture = dispatcher.acquireId(Kld::ResourceHandleKind::Texture);
	const Kld::HandleId image = dispatcher.acquireId(Kld::ResourceHandleKind::Image);
	Kld::DeferredDestroyQueue deferred;
	deferred.retire(Kld::ResourceHandleKind::Texture, texture);
	deferred.retire(Kld::ResourceHandleKind::Image, image);
	CHECK(deferred.size() == 2);
	deferred.flushInto(dispatcher);
	CHECK(deferred.size() == 0);

	dispatcher.getCommandBuffer().swapBuffers();
	const std::vector<Kld::GfxOp> ops = readBufferOf(dispatcher);
	REQUIRE(ops.size() == 2);
	CHECK(ops[0].opcode == Kld::Opcode::DestroyTexture);
	CHECK(ops[0].opt.opDestroy.id == texture);
	CHECK(ops[1].opcode == Kld::Opcode::DestroyImage);
	CHECK(ops[1].opt.opDestroyImage.id == image);
}

TEST_CASE("submit hands the read buffer to the device", "[kaldi][facade]")
{
	RecordingDevice device;
	Kld::CommandDispatcher<> dispatcher;
	auto target = Kld::OopFacade::createImage2D(dispatcher, Kld::TextureFormat::RGBA8U, 32, 32, 1,
						    Kld::IMAGE_USAGE_COLOR_ATTACHMENT);
	target.read2D(0, 0, 0, 0, 32, 32, nullptr, nullptr);

	dispatcher.getCommandBuffer().swapBuffers();
	const Kld::HalStatus status = dispatcher.submit(device);
	CHECK(status.ok());
	REQUIRE(device.submitted.size() == 2);
	CHECK(device.submitted[0].opcode == Kld::Opcode::CreateImage2D);
	CHECK(device.submitted[0].opt.opCreateImage2D.id == target.id());
	CHECK(device.submitted[1].opcode == Kld::Opcode::ReadImage2D);
	CHECK(device.submitted[1].opt.opReadImage2D.image == target.id());
}
