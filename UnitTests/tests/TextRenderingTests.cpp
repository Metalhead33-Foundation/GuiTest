/**
 * @file TextRenderingTests.cpp
 * @brief CPU tests for the font repository, lazy glyph atlas and text renderer.
 *
 * The atlas and renderer only record Kaldi commands, so these tests inspect the
 * command buffer like `WriteableTexture2DTests` does: no device or GPU is needed to
 * verify packing, UVs, vertex parameters, run splitting and uploads.
 */
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <Euphemy/Io/EuphMemoryDevice.hpp>
#include <Euphemy/Media/Image/EuphPNG.hpp>
#include <Euphemy/Media/Image/EuphPixelFormat.hpp>
#include <Lotte/GFX/LteClipSpaceConventions.hpp>
#include <Lotte/GFX/LteResourceFactory.hpp>
#include <Lotte/LteKld.hpp>
#include <Lotte/Text/LteFontAtlas.hpp>
#include <Lotte/Text/LteFontRepository.hpp>
#include <Lotte/Text/LteTextLayout.hpp>
#include <Lotte/Text/LteTextRenderer.hpp>
#include <cstddef>
#include <cstdint>
#include <glm/vec2.hpp>
#include <memory>
#include <span>
#include <stdexcept>
#include <vector>

namespace {

using Lotte::Text::DistanceType;
using Lotte::Text::FontStyle;
using Lotte::Text::PreprocessedFontFace;
using Lotte::Text::SDFType;
using Lotte::Text::StoredCharacter;

/** @brief Copies the dispatcher's write buffer (what the next submit would see). */
std::vector<Kld::GfxOp> writeOps(Lotte::CommandBuffer& dispatcher)
{
	std::vector<Kld::GfxOp> ops;
	dispatcher.getCommandBuffer().onWriteBuffer(
		[&ops](const auto& vec) { ops.assign(vec.begin(), vec.end()); });
	return ops;
}

/** @brief Collects the index offsets of recorded indexed triangle draws (in bytes). */
std::vector<std::uint32_t> drawIndexOffsets(Lotte::CommandBuffer& dispatcher)
{
	std::vector<std::uint32_t> offsets;
	for (const Kld::GfxOp& op : writeOps(dispatcher)) {
		if (op.opcode == Kld::Opcode::DrawIndexedTriangles) {
			offsets.push_back(op.opt.opDrawIndexedTriangles.indexOffset);
		}
	}
	return offsets;
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

/** @brief Encodes a uniform @p size x @p size RGBA8 tile as PNG. */
std::vector<std::byte> makeSdfPayload(unsigned size, std::uint8_t red, std::uint8_t green,
									  std::uint8_t blue, std::uint8_t alpha = 255)
{
	using namespace Euph::Media::Image;
	DecodeTarget target;
	target.setFormat(Format::RGBA8U);
	Frame& frame = target.addFrame(size, size);
	const std::span<PixelRGBA_U8> pixels = frame.asDataSpan<PixelRGBA_U8>();
	for (PixelRGBA_U8& pixel : pixels) {
		pixel.r = red;
		pixel.g = green;
		pixel.b = blue;
		pixel.a = alpha;
	}
	Euph::Io::MemoryDevice device(Elv::Io::Mode::READ_WRITE);
	PNG::encode(device, target, 0.0f);
	REQUIRE(device.size() > 0);
	REQUIRE(device.seek(0, Elv::Io::SeekOrigin::SET) == 0);
	std::vector<std::byte> bytes(device.size());
	REQUIRE(device.read(bytes.data(), 1, bytes.size()) == bytes.size());
	return bytes;
}

/** @brief Creates a small synthetic face with one shared PNG payload per glyph. */
std::shared_ptr<PreprocessedFontFace> makeFace(const std::string& family, SDFType type = SDFType::SDF)
{
	auto face = std::make_shared<PreprocessedFontFace>();
	face->familyName = family;
	face->type = type;
	face->distType = DistanceType::Manhattan;
	face->bitmapSize = 8;
	face->bitmapLogicalSize = 16;
	face->bitmapPadding = 2;
	face->ascender = 14.0f;
	face->descender = -2.0f;
	face->faceHeight = 16.0f;
	face->maxAdvance = 9.0f;
	face->unitsPerEm = 1000;
	face->distanceRangeX = 2.0f;
	face->distanceRangeY = 2.0f;
	return face;
}

/** @brief Adds a drawable 8x8 glyph with the given payload. */
void addGlyph(PreprocessedFontFace& face, char32_t codePoint, const std::vector<std::byte>& payload)
{
	StoredCharacter glyph;
	glyph.valid = true;
	glyph.width = 8;
	glyph.height = 8;
	glyph.bearingX = 1;
	glyph.bearingY = 8;
	glyph.metricWidth = 8.0f;
	glyph.metricHeight = 8.0f;
	glyph.horiBearingX = 1.0f;
	glyph.horiBearingY = 8.0f;
	glyph.horiAdvance = 9.0f;
	glyph.advanceX = 9.0f;
	glyph.sdf = payload;
	face.glyphs[static_cast<std::uint32_t>(codePoint)] = std::move(glyph);
}

/** @brief Finds the uploaded text vertex buffer and its element count. */
const Lotte::Text::TextGuiVertex* uploadedTextVertices(Lotte::CommandBuffer& dispatcher,
													   std::size_t& vertexCount)
{
	vertexCount = 0;
	Kld::HandleId vertexBuffer = 0;
	const void* data = nullptr;
	std::uint32_t byteSize = 0;
	const std::vector<Kld::GfxOp> ops = writeOps(dispatcher);
	for (const Kld::GfxOp& op : ops) {
		if (op.opcode != Kld::Opcode::CreateBufferObject ||
			op.opt.opCreateBufferObject.policy != Kld::BUFFER_USAGE_VERTEX) {
			continue;
		}
		vertexBuffer = op.opt.opCreateBufferObject.id;
		data = op.opt.opCreateBufferObject.data;
		byteSize = op.opt.opCreateBufferObject.size;
	}
	if (vertexBuffer == 0 || data == nullptr) {
		return nullptr;
	}
	// Later frames update the same buffer instead of recreating it.
	for (const Kld::GfxOp& op : ops) {
		if (op.opcode == Kld::Opcode::UpdateBufferObject &&
			op.opt.opUpdateBufferObject.id == vertexBuffer &&
			op.opt.opUpdateBufferObject.offset == 0) {
			data = op.opt.opUpdateBufferObject.data;
			byteSize = op.opt.opUpdateBufferObject.size;
		}
	}
	if (data == nullptr || byteSize < sizeof(Lotte::Text::TextGuiVertex)) {
		return nullptr;
	}
	vertexCount = byteSize / sizeof(Lotte::Text::TextGuiVertex);
	return static_cast<const Lotte::Text::TextGuiVertex*>(data);
}

} // namespace

TEST_CASE("The font repository resolves families, styles and lazily created atlases",
		  "[text][repository]")
{
	Lotte::CommandBuffer dispatcher;
	Lotte::Gfx::ResourceFactory factory(dispatcher);
	Lotte::Text::FontRepository repository(factory);

	const auto regular = makeFace("Alpha");
	const auto bold = makeFace("Alpha Bold");
	const auto spaced = makeFace("Space Face");
	repository.addFace("Alpha", regular);
	repository.addFace("Alpha Bold", "Alpha", bold, FontStyle{ true, false });
	repository.addFace(spaced);

	CHECK(repository.faceCount() == 3);
	CHECK(repository.faceFor("alpha", false) == regular.get());
	CHECK(repository.faceFor("ALPHA", true) == bold.get());
	// MML strips whitespace inside a tag; the repository matches that.
	CHECK(repository.faceFor("spaceface") == spaced.get());
	CHECK(repository.faceFor(" SPACE  FACE ") == spaced.get());
	CHECK(repository.isBoldVariant(bold.get()));
	CHECK_FALSE(repository.isBoldVariant(regular.get()));
	// Unknown families fall back to the first registered face.
	CHECK(repository.faceFor("Unknown", false) == regular.get());
	CHECK(repository.defaultFaceId() == "Alpha");

	CHECK(repository.atlasCount() == 0);
	Lotte::Text::FontAtlas* atlas = repository.atlasFor(regular.get());
	REQUIRE(atlas != nullptr);
	CHECK(repository.atlasCount() == 1);
	// Creating the atlas is cheap: pages appear only when a glyph is stamped.
	CHECK(atlas->pageCount() == 0);
	CHECK(repository.atlasFor(regular.get()) == atlas);
	CHECK(repository.atlasForFamily("alpha", true) == repository.atlasFor(bold.get()));
	CHECK(repository.atlasFor("Alpha Bold") == repository.atlasFor(bold.get()));

	{
		const auto unregistered = makeFace("Other");
		CHECK_THROWS_AS(repository.atlasFor(unregistered.get()), std::invalid_argument);
	}
	CHECK_THROWS_AS(repository.setDefaultFaceId("missing"), std::invalid_argument);
}

TEST_CASE("The atlas shelf-packs pages, maps UVs and records uploads", "[text][atlas]")
{
	const std::vector<std::byte> payload = makeSdfPayload(8, 200, 200, 200);
	auto face = makeFace("AtlasFace");
	for (std::uint32_t index = 0; index < 10; ++index) {
		addGlyph(*face, char32_t(U'A' + index), payload);
	}

	Lotte::CommandBuffer dispatcher;
	Lotte::Gfx::ResourceFactory factory(dispatcher);
	Lotte::Text::FontAtlasSettings settings;
	settings.pageWidth = 32;
	settings.pageHeight = 32;
	settings.gutter = 1;
	settings.maxPages = 0;
	Lotte::Text::FontAtlas atlas(factory, face, settings);

	CHECK(atlas.tileSize() == 8);
	CHECK(atlas.pageCount() == 0);
	REQUIRE(atlas.ensureGlyph(U'A'));
	CHECK(atlas.pageCount() == 1);
	CHECK(atlas.pageTexture(0) != 0);
	CHECK(countOps(dispatcher, Kld::Opcode::CreateTexture2D) == 1);

	const Lotte::Text::FontAtlasGlyph* first = atlas.findGlyph(U'A');
	REQUIRE(first != nullptr);
	CHECK(first->page == 0);
	CHECK(first->x == 1); // gutter
	CHECK(first->y == 1);
	glm::fvec2 uv0(0.0f);
	glm::fvec2 uv1(0.0f);
	REQUIRE(atlas.mapGlyph(U'A', uv0, uv1));
	CHECK(uv0.x == Catch::Approx(1.0f / 32.0f));
	CHECK(uv0.y == Catch::Approx(1.0f / 32.0f));
	CHECK(uv1.x == Catch::Approx(9.0f / 32.0f));
	CHECK(uv1.y == Catch::Approx(9.0f / 32.0f));

	REQUIRE(atlas.ensureGlyph(U'B'));
	const Lotte::Text::FontAtlasGlyph* second = atlas.findGlyph(U'B');
	REQUIRE(second != nullptr);
	CHECK(second->page == 0);
	CHECK(second->x == 11); // 8 tile + 2 gutter
	CHECK(second->y == 1);

	CHECK_FALSE(atlas.ensureGlyph(0x1F600));
	CHECK_FALSE(atlas.ensureGlyph(0x1F600));
	CHECK(atlas.findGlyph(0x1F600) == nullptr);

	// 3x3 cells per page; the tenth glyph opens a second page.
	for (std::uint32_t index = 0; index < 10; ++index) {
		REQUIRE(atlas.ensureGlyph(char32_t(U'A' + index)));
	}
	CHECK(atlas.glyphCount() == 10);
	CHECK(atlas.pageCount() == 2);
	CHECK(atlas.pageTexture(0) != 0);
	CHECK(atlas.pageTexture(1) != 0);
	CHECK(atlas.pageTexture(0) != atlas.pageTexture(1));
	const Lotte::Gfx::WriteableTexture2D* surface = atlas.pageSurface(0);
	REQUIRE(surface != nullptr);
	CHECK(surface->width() == 32);
	CHECK(surface->height() == 32);
	CHECK(surface->pixels() != nullptr);
	CHECK(atlas.pageSurface(2) == nullptr);
	REQUIRE(atlas.mapGlyph(U'A', uv0, uv1));
	CHECK(atlas.pageForGlyph(U'A') == 0);
	const Lotte::Text::FontAtlasGlyph* spill = atlas.findGlyph(char32_t(U'A' + 9));
	REQUIRE(spill != nullptr);
	CHECK(spill->page == 1);

	const std::size_t uploadsBefore = countOps(dispatcher, Kld::Opcode::UpdateTexture2D);
	CHECK(atlas.recordUpdates());
	CHECK(countOps(dispatcher, Kld::Opcode::UpdateTexture2D) > uploadsBefore);
	// A second call has no dirty pages left.
	const std::size_t uploadsAfter = countOps(dispatcher, Kld::Opcode::UpdateTexture2D);
	CHECK_FALSE(atlas.recordUpdates());
	CHECK(countOps(dispatcher, Kld::Opcode::UpdateTexture2D) == uploadsAfter);
}

TEST_CASE("ensureGlyphs only loads the glyphs that belong to its face", "[text][atlas]")
{
	const std::vector<std::byte> payload = makeSdfPayload(8, 128, 128, 128);
	auto firstFace = makeFace("First");
	auto secondFace = makeFace("Second");
	addGlyph(*firstFace, U'A', payload);
	addGlyph(*secondFace, U'B', payload);

	Lotte::CommandBuffer dispatcher;
	Lotte::Gfx::ResourceFactory factory(dispatcher);
	Lotte::Text::FontAtlasSettings settings;
	settings.pageWidth = 32;
	settings.pageHeight = 32;
	Lotte::Text::FontAtlas atlas(factory, firstFace, settings);

	Lotte::Text::TextLayout layout;
	layout.faces = { firstFace.get(), secondFace.get() };
	Lotte::Text::PlacedGlyph placed;
	placed.codePoint = U'A';
	placed.faceIndex = 0;
	layout.glyphs.push_back(placed);
	placed.codePoint = U'B';
	placed.faceIndex = 1;
	layout.glyphs.push_back(placed);

	CHECK(atlas.ensureGlyphs(layout) == 1);
	CHECK(atlas.findGlyph(U'A') != nullptr);
	CHECK(atlas.findGlyph(U'B') == nullptr);
	CHECK(atlas.ensureGlyphs(layout) == 0);
}

TEST_CASE("The text renderer emits SDF vertices, colours, bold and decorations",
		  "[text][renderer]")
{
	const std::vector<std::byte> payload = makeSdfPayload(8, 220, 220, 220);
	auto face = makeFace("RenderFace");
	addGlyph(*face, U'A', payload);
	addGlyph(*face, U'B', payload);

	Lotte::CommandBuffer dispatcher;
	Lotte::Gfx::ResourceFactory factory(dispatcher);
	Lotte::Text::FontRepository repository(factory);
	repository.addFace("RenderFace", face);

	Lotte::Text::TextRendererSettings rendererSettings;
	rendererSettings.viewportWidth = 640;
	rendererSettings.viewportHeight = 480;
	rendererSettings.clipSpaceYAxis = Kld::ClipSpaceYAxis::Down;
	Lotte::Text::TextRenderer renderer(factory, repository, rendererSettings);

	Lotte::Text::TextAttributes attributes;
	attributes.familyName = "RenderFace";
	attributes.size = 14.0f; // em = L - P = 14, so the run scale is 1
	attributes.colour = glm::fvec4(1.0f, 0.5f, 0.25f, 1.0f);
	attributes.underline = true;

	Lotte::Text::TextLayoutOptions options;
	options.origin = glm::fvec2(10.0f, 20.0f);
	auto lookup = [&repository](const std::string& family, bool bold) {
		return repository.faceFor(family, bold);
	};
	const Lotte::Text::TextLayout layout =
		Lotte::Text::layoutRuns({ Lotte::Text::TextRun{ U"AB", attributes } }, lookup, options);
	REQUIRE(layout.glyphs.size() == 2);
	REQUIRE(layout.lines.size() == 1);
	REQUIRE(layout.faces.size() == 1);

	CHECK(renderer.prepare(layout));
	renderer.beginFrame();
	CHECK(renderer.render(layout) == 2);
	CHECK(renderer.queuedRunCount() == 2); // one atlas page + the white decoration texture
	renderer.flush();

	std::size_t vertexCount = 0;
	const Lotte::Text::TextGuiVertex* vertices = uploadedTextVertices(dispatcher, vertexCount);
	REQUIRE(vertices != nullptr);
	REQUIRE(vertexCount == 12); // two glyph quads and one decoration quad

	const glm::fvec4 expectedParams{ 0.0f, 2.0f * (layout.glyphs[0].size.x / 8.0f),
									 2.0f * (layout.glyphs[0].size.y / 8.0f), 0.0f };
	CHECK(vertices[0].params.x == Catch::Approx(0.0f));
	CHECK(vertices[0].params.y == Catch::Approx(expectedParams.y));
	CHECK(vertices[0].params.z == Catch::Approx(expectedParams.z));
	CHECK(vertices[0].params.w == Catch::Approx(0.0f));
	CHECK(vertices[0].colour == attributes.colour);

	const Lotte::Gfx::ClipSpaceConventions conventions =
		Lotte::Gfx::ClipSpaceConventions::fromYAxis(Kld::ClipSpaceYAxis::Down);
	const glm::fvec2 expectedPosition = conventions.pixelToClip(
		layout.glyphs[0].position, glm::fvec2(640.0f, 480.0f));
	CHECK(vertices[0].position.x == Catch::Approx(expectedPosition.x));
	CHECK(vertices[0].position.y == Catch::Approx(expectedPosition.y));

	// The decoration quad follows the two glyph quads and carries 0..1 UVs so the
	// shader can anti-alias its edges from screen-space derivatives.
	CHECK(vertices[8].params.x == Catch::Approx(3.0f));
	CHECK(vertices[8].uv == glm::fvec2(0.0f, 0.0f));
	CHECK(vertices[9].uv == glm::fvec2(1.0f, 0.0f));
	CHECK(vertices[10].uv == glm::fvec2(0.0f, 1.0f));
	CHECK(vertices[11].uv == glm::fvec2(1.0f, 1.0f));
	CHECK(vertices[8].colour == attributes.colour);

	CHECK(countOps(dispatcher, Kld::Opcode::BindTexture) == 2);
	CHECK(countOps(dispatcher, Kld::Opcode::DrawIndexedTriangles) == 2);
	const std::vector<std::uint32_t> offsets = drawIndexOffsets(dispatcher);
	REQUIRE(offsets.size() == 2);
	// Two glyph quads * 6 indices, converted to a byte offset for Kaldi.
	CHECK(offsets[0] == 0);
	CHECK(offsets[1] == 2u * 6u * std::uint32_t(sizeof(std::uint32_t)));

	// Bold is a per-vertex dilation of 4% of the em size.
	Lotte::Text::TextAttributes boldAttributes = attributes;
	boldAttributes.bold = true;
	boldAttributes.underline = false;
	const Lotte::Text::TextLayout boldLayout =
		Lotte::Text::layoutRuns({ Lotte::Text::TextRun{ U"A", boldAttributes } }, lookup, options);
	CHECK_FALSE(renderer.prepare(boldLayout)); // already cached from the first layout
	renderer.beginFrame();
	REQUIRE(renderer.render(boldLayout) == 1);
	renderer.flush();
	const Lotte::Text::TextGuiVertex* boldVertices = uploadedTextVertices(dispatcher, vertexCount);
	REQUIRE(boldVertices != nullptr);
	REQUIRE(vertexCount == 4);
	CHECK(boldVertices[0].params.w == Catch::Approx(14.0f * 0.04f));

	// A real bold face must not be dilated a second time by the shader.
	auto boldFace = makeFace("RenderFace Bold");
	addGlyph(*boldFace, U'A', payload);
	repository.addFace("RenderFace Bold", "RenderFace", boldFace, FontStyle{ true, false });
	const Lotte::Text::TextLayout realBoldLayout =
		Lotte::Text::layoutRuns({ Lotte::Text::TextRun{ U"A", boldAttributes } }, lookup, options);
	REQUIRE(realBoldLayout.faces.size() == 1);
	CHECK(realBoldLayout.faces.front() == boldFace.get());
	CHECK(realBoldLayout.styles.front().bold);
	CHECK(renderer.prepare(realBoldLayout));
	renderer.beginFrame();
	REQUIRE(renderer.render(realBoldLayout) == 1);
	renderer.flush();
	const Lotte::Text::TextGuiVertex* realBoldVertices =
		uploadedTextVertices(dispatcher, vertexCount);
	REQUIRE(realBoldVertices != nullptr);
	REQUIRE(vertexCount == 4);
	CHECK(realBoldVertices[0].params.w == Catch::Approx(0.0f));
}

TEST_CASE("An MSDF face selects the median mode and pages split the draw runs", "[text][renderer]")
{
	const std::vector<std::byte> payload = makeSdfPayload(8, 200, 100, 50);
	auto face = makeFace("MsdfFace", SDFType::MSDF);
	for (std::uint32_t index = 0; index < 10; ++index) {
		addGlyph(*face, char32_t(U'A' + index), payload);
	}

	Lotte::CommandBuffer dispatcher;
	Lotte::Gfx::ResourceFactory factory(dispatcher);
	Lotte::Text::FontAtlasSettings atlasSettings;
	atlasSettings.pageWidth = 32;
	atlasSettings.pageHeight = 32;
	Lotte::Text::FontRepository repository(factory, atlasSettings);
	repository.addFace("MsdfFace", face);

	Lotte::Text::TextRendererSettings rendererSettings;
	rendererSettings.viewportWidth = 640;
	rendererSettings.viewportHeight = 480;
	rendererSettings.clipSpaceYAxis = Kld::ClipSpaceYAxis::Down;
	Lotte::Text::TextRenderer renderer(factory, repository, rendererSettings);

	Lotte::Text::TextAttributes attributes;
	attributes.familyName = "MsdfFace";
	attributes.size = 14.0f;
	auto lookup = [&repository](const std::string& family, bool bold) {
		return repository.faceFor(family, bold);
	};
	const Lotte::Text::TextLayout layout = Lotte::Text::layoutRuns(
		{ Lotte::Text::TextRun{ U"ABCDEFGHIJ", attributes } }, lookup);
	REQUIRE(layout.glyphs.size() == 10);

	CHECK(renderer.prepare(layout));
	renderer.beginFrame();
	CHECK(renderer.render(layout) == 10);
	// Nine cells fit on a 32x32 page; the tenth starts page 2.
	CHECK(renderer.queuedRunCount() == 2);
	renderer.flush();
	CHECK(countOps(dispatcher, Kld::Opcode::BindTexture) == 2);
	CHECK(countOps(dispatcher, Kld::Opcode::DrawIndexedTriangles) == 2);
	const std::vector<std::uint32_t> offsets = drawIndexOffsets(dispatcher);
	REQUIRE(offsets.size() == 2);
	// Nine glyphs on the first page => 54 indices before the second run.
	CHECK(offsets[0] == 0);
	CHECK(offsets[1] == 9u * 6u * std::uint32_t(sizeof(std::uint32_t)));

	std::size_t vertexCount = 0;
	const Lotte::Text::TextGuiVertex* vertices = uploadedTextVertices(dispatcher, vertexCount);
	REQUIRE(vertices != nullptr);
	REQUIRE(vertexCount == 40);
	CHECK(vertices[0].params.x == Catch::Approx(1.0f)); // MSDF median
	CHECK(vertices[4].params.x == Catch::Approx(1.0f));
}

TEST_CASE("Decorations cover only their own run", "[text][layout]")
{
	using namespace Lotte::Text;

	auto face = makeFace("DecorationFace");
	const std::vector<std::byte> empty;
	addGlyph(*face, U'A', empty);
	addGlyph(*face, U'B', empty);
	addGlyph(*face, U'C', empty);
	StoredCharacter space;
	space.valid = true;
	space.horiAdvance = 9.0f;
	space.advanceX = 9.0f;
	face->glyphs[static_cast<std::uint32_t>(U' ')] = space;

	TextAttributes regular;
	regular.size = 14.0f;
	TextAttributes underline = regular;
	underline.underline = true;
	TextAttributes strike = regular;
	strike.strikethrough = true;

	const TextRunList runs = {
		TextRun{ U"A", regular },
		TextRun{ U"B", underline },
		TextRun{ U"C", strike }
	};
	TextLayoutOptions options;
	options.origin = glm::fvec2(10.0f, 20.0f);
	auto lookup = [&face](const std::string&, bool) { return face.get(); };
	const TextLayout layout = layoutRuns(runs, lookup, options);

	REQUIRE(layout.glyphs.size() == 3);
	REQUIRE(layout.lines.size() == 2);
	// The underline starts after "A" (10 + 9) and ends after "B"; the strike
	// starts after "B" and ends after "C".
	CHECK(layout.lines[0].from.x == Catch::Approx(19.0f));
	CHECK(layout.lines[0].to.x == Catch::Approx(28.0f));
	CHECK(layout.lines[1].from.x == Catch::Approx(28.0f));
	CHECK(layout.lines[1].to.x == Catch::Approx(37.0f));
	REQUIRE(layout.lines[0].styleIndex < layout.styles.size());
	REQUIRE(layout.lines[1].styleIndex < layout.styles.size());
	CHECK(layout.styles[layout.lines[0].styleIndex].underline);
	CHECK_FALSE(layout.styles[layout.lines[0].styleIndex].strikethrough);
	CHECK(layout.styles[layout.lines[1].styleIndex].strikethrough);
	CHECK_FALSE(layout.styles[layout.lines[1].styleIndex].underline);

	// Extra word spacing is applied after the space advance, not as a glyph tweak.
	TextLayoutOptions spacedOptions = options;
	spacedOptions.wordSpacing = 3.0f;
	const TextLayout spaced = layoutRuns({ TextRun{ U"A B", regular } }, lookup, spacedOptions);
	CHECK(spaced.endOffset.x == Catch::Approx(10.0f + 9.0f + 9.0f + 3.0f + 9.0f));
}

TEST_CASE("One layout can mix faces and atlas pages", "[text][renderer]")
{
	using namespace Lotte::Text;

	const std::vector<std::byte> payloadA = makeSdfPayload(8, 200, 200, 200);
	const std::vector<std::byte> payloadB = makeSdfPayload(8, 50, 120, 200);
	auto faceA = makeFace("Family A");
	auto faceB = makeFace("Family B");
	addGlyph(*faceA, U'A', payloadA);
	addGlyph(*faceB, U'B', payloadB);

	Lotte::CommandBuffer dispatcher;
	Lotte::Gfx::ResourceFactory factory(dispatcher);
	Lotte::Text::FontRepository repository(factory);
	repository.addFace(faceA);
	repository.addFace(faceB);

	Lotte::Text::TextRendererSettings settings;
	settings.viewportWidth = 640;
	settings.viewportHeight = 480;
	settings.clipSpaceYAxis = Kld::ClipSpaceYAxis::Down;
	Lotte::Text::TextRenderer renderer(factory, repository, settings);

	Lotte::Text::TextAttributes attributesA;
	attributesA.familyName = "Family A";
	attributesA.size = 14.0f;
	Lotte::Text::TextAttributes attributesB = attributesA;
	attributesB.familyName = "Family B";
	attributesB.colour = glm::fvec4(0.25f, 0.5f, 1.0f, 1.0f);

	auto lookup = [&repository](const std::string& family, bool bold) {
		return repository.faceFor(family, bold);
	};
	const Lotte::Text::TextLayout layout = Lotte::Text::layoutRuns(
		{ Lotte::Text::TextRun{ U"A", attributesA },
		  Lotte::Text::TextRun{ U"B", attributesB } },
		lookup);
	REQUIRE(layout.glyphs.size() == 2);
	REQUIRE(layout.faces.size() == 2);
	CHECK(layout.glyphs[0].faceIndex != layout.glyphs[1].faceIndex);

	CHECK(renderer.prepare(layout));
	renderer.beginFrame();
	CHECK(renderer.render(layout) == 2);
	CHECK(renderer.queuedRunCount() == 2); // one page per face
	renderer.flush();
	CHECK(countOps(dispatcher, Kld::Opcode::BindTexture) == 2);
	CHECK(countOps(dispatcher, Kld::Opcode::DrawIndexedTriangles) == 2);
	const std::vector<std::uint32_t> offsets = drawIndexOffsets(dispatcher);
	REQUIRE(offsets.size() == 2);
	CHECK(offsets[0] == 0);
	CHECK(offsets[1] == 6u * std::uint32_t(sizeof(std::uint32_t))); // one glyph per face

	std::size_t vertexCount = 0;
	const Lotte::Text::TextGuiVertex* vertices = uploadedTextVertices(dispatcher, vertexCount);
	REQUIRE(vertices != nullptr);
	REQUIRE(vertexCount == 8);
	CHECK(vertices[0].colour == attributesA.colour);
	CHECK(vertices[4].colour == attributesB.colour);
}

