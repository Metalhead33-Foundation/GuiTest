/**
 * @file WriteableTexture2DTests.cpp
 * @brief Dirty-rectangle bookkeeping and upload recording of `WriteableTexture2D`.
 *
 * The class sits between a CPU pixel buffer and Kaldi's `CreateTexture2D` /
 * `UpdateTexture2D` commands, so the tests inspect the recorded command buffer
 * instead of submitting anything: no device is needed to verify how many uploads a
 * change produces, and that is the part a GPU cannot tell us.
 */
#include <catch2/catch_test_macros.hpp>
#include <Lotte/GFX/LteWriteableTexture2D.hpp>
#include <Lotte/LteKld.hpp>
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <Euphemy/Media/Image/EuphPixelFormat.hpp>
#include <cstdint>
#include <span>
#include <vector>

namespace {

/** @brief Copies the dispatcher's write buffer (what the next submit would see). */
std::vector<Kld::GfxOp> writeOps(Lotte::CommandBuffer& dispatcher)
{
	std::vector<Kld::GfxOp> ops;
	dispatcher.getCommandBuffer().onWriteBuffer(
		[&ops](const auto& vec) { ops.assign(vec.begin(), vec.end()); });
	return ops;
}

/** @brief Counts how many recorded commands use @p opcode. */
std::size_t countOps(Lotte::CommandBuffer& dispatcher, Kld::Opcode opcode)
{
	std::size_t count = 0;
	for (const Kld::GfxOp& op : writeOps(dispatcher)) {
		if (op.opcode == opcode) {
			++count;
		}
	}
	return count;
}

/** @brief Builds a BGRA8 image so the converter's channel swap is exercised. */
Euph::Media::Image::DecodeTarget makeBgraImage(unsigned width, unsigned height)
{
	using namespace Euph::Media::Image;
	DecodeTarget target;
	target.setFormat(Format::BGRA8U);
	Frame& frame = target.addFrame(width, height);
	const std::span<PixelBGRA_U8> pixels = frame.asDataSpan<PixelBGRA_U8>();
	for (PixelBGRA_U8& pixel : pixels) {
		pixel.b = 30;
		pixel.g = 20;
		pixel.r = 10;
		pixel.a = 200;
	}
	return target;
}

} // namespace

TEST_CASE("Creating a surface queues the texture and one full-width upload",
	  "[gui][texture]")
{
	Lotte::CommandBuffer dispatcher;
	Lotte::Gfx::ResourceFactory factory(dispatcher);
	Lotte::Gfx::WriteableTexture2D surface =
		Lotte::Gfx::WriteableTexture2D::create(factory, 8, 4);

	REQUIRE(surface.isValid());
	CHECK(surface.width() == 8);
	CHECK(surface.height() == 4);
	CHECK(surface.rowBytes() == 32);
	CHECK(surface.byteSize() == 128);
	CHECK(surface.handle() != 0);
	CHECK(surface.dirtyRegion().empty());
	CHECK(countOps(dispatcher, Kld::Opcode::CreateTexture2D) == 1);
	CHECK(countOps(dispatcher, Kld::Opcode::UpdateTexture2D) == 1);
	// The initial content is transparent, and it is uploaded as one command.
	bool sawFullUpload = false;
	for (const Kld::GfxOp& op : writeOps(dispatcher)) {
		if (op.opcode != Kld::Opcode::UpdateTexture2D) {
			continue;
		}
		CHECK(op.opt.opUpdateTexture2D.format == Kld::TextureFormat::RGBA8U);
		CHECK(op.opt.opUpdateTexture2D.x == 0);
		CHECK(op.opt.opUpdateTexture2D.y == 0);
		CHECK(op.opt.opUpdateTexture2D.width == 8);
		CHECK(op.opt.opUpdateTexture2D.height == 4);
		sawFullUpload = true;
	}
	CHECK(sawFullUpload);
}

TEST_CASE("Partial dirty rectangles upload one command per row", "[gui][texture]")
{
	Lotte::CommandBuffer dispatcher;
	Lotte::Gfx::ResourceFactory factory(dispatcher);
	Lotte::Gfx::WriteableTexture2D surface =
		Lotte::Gfx::WriteableTexture2D::create(factory, 8, 4);
	const std::size_t uploadsBefore = countOps(dispatcher, Kld::Opcode::UpdateTexture2D);

	surface.setPixel(3, 2, 1, 2, 3, 4);
	const Lotte::Gfx::WriteableTexture2D::DirtyRegion single = surface.dirtyRegion();
	CHECK(single.x == 3);
	CHECK(single.y == 2);
	CHECK(single.width == 1);
	CHECK(single.height == 1);
	surface.recordUpdate();
	CHECK_FALSE(surface.hasDirtyRegion());
	CHECK(countOps(dispatcher, Kld::Opcode::UpdateTexture2D) == uploadsBefore + 1);

	// Two rows inside the surface: one command per row because neither spans it.
	surface.markDirty(2, 1, 3, 2);
	const Lotte::Gfx::WriteableTexture2D::DirtyRegion wide = surface.dirtyRegion();
	CHECK(wide.x == 2);
	CHECK(wide.y == 1);
	CHECK(wide.width == 3);
	CHECK(wide.height == 2);
	surface.recordUpdate();
	CHECK(countOps(dispatcher, Kld::Opcode::UpdateTexture2D) == uploadsBefore + 3);
}

TEST_CASE("A dirty rectangle spanning the full width is one command",
	  "[gui][texture]")
{
	Lotte::CommandBuffer dispatcher;
	Lotte::Gfx::ResourceFactory factory(dispatcher);
	Lotte::Gfx::WriteableTexture2D surface =
		Lotte::Gfx::WriteableTexture2D::create(factory, 8, 4);
	const std::size_t uploadsBefore = countOps(dispatcher, Kld::Opcode::UpdateTexture2D);

	surface.markDirty(0, 1, 8, 2);
	surface.recordUpdate();
	CHECK(countOps(dispatcher, Kld::Opcode::UpdateTexture2D) == uploadsBefore + 1);
}

TEST_CASE("blitRgba8 clips to the surface and marks only what it wrote",
	  "[gui][texture]")
{
	Lotte::CommandBuffer dispatcher;
	Lotte::Gfx::ResourceFactory factory(dispatcher);
	Lotte::Gfx::WriteableTexture2D surface =
		Lotte::Gfx::WriteableTexture2D::create(factory, 4, 4);

	std::vector<std::uint8_t> source(std::size_t(2) * 2 * 4);
	for (std::size_t index = 0; index < source.size(); index += 4) {
		source[index + 0] = 0x11;
		source[index + 1] = 0x22;
		source[index + 2] = 0x33;
		source[index + 3] = 0x44;
	}
	// Stamping at (3, 3) only lands one pixel inside the surface.
	REQUIRE(surface.blitRgba8(source.data(), 2, 2, 3, 3));
	const Lotte::Gfx::WriteableTexture2D::DirtyRegion region = surface.dirtyRegion();
	CHECK(region.x == 3);
	CHECK(region.y == 3);
	CHECK(region.width == 1);
	CHECK(region.height == 1);

	const std::uint8_t* pixels = surface.pixels();
	REQUIRE(pixels != nullptr);
	const std::size_t offset = (std::size_t(3) * 4 + 3) * 4;
	CHECK(pixels[offset + 0] == 0x11);
	CHECK(pixels[offset + 1] == 0x22);
	CHECK(pixels[offset + 2] == 0x33);
	CHECK(pixels[offset + 3] == 0x44);

	// Entirely outside the surface: nothing is written, nothing becomes dirty.
	surface.clearDirty();
	CHECK_FALSE(surface.blitRgba8(source.data(), 2, 2, 4, 0));
	CHECK_FALSE(surface.hasDirtyRegion());
}

TEST_CASE("A decoded BGRA image becomes RGBA on the CPU side", "[gui][texture]")
{
	Lotte::CommandBuffer dispatcher;
	Lotte::Gfx::ResourceFactory factory(dispatcher);
	const Euph::Media::Image::DecodeTarget decoded = makeBgraImage(2, 2);
	Lotte::Gfx::WriteableTexture2D surface =
		Lotte::Gfx::WriteableTexture2D::fromDecodeTarget(factory, decoded);

	REQUIRE(surface.isValid());
	CHECK(surface.width() == 2);
	CHECK(surface.height() == 2);
	const std::uint8_t* pixels = surface.pixels();
	REQUIRE(pixels != nullptr);
	for (std::size_t index = 0; index < 4 * 4; index += 4) {
		CHECK(pixels[index + 0] == 10);
		CHECK(pixels[index + 1] == 20);
		CHECK(pixels[index + 2] == 30);
		CHECK(pixels[index + 3] == 200);
	}
	CHECK(countOps(dispatcher, Kld::Opcode::UpdateTexture2D) == 1);
}


