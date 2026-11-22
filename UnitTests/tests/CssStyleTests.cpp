/**
 * @file CssStyleTests.cpp
 * @brief CSS for MML: `Text/LteCssStyle` and the MML element-observer hook it uses.
 *
 * The whole file is inert when the build could not find libcss, so the suite keeps
 * working on hosts without it. What is asserted here is the contract the header
 * promises: element/class/id selectors, the cascade against MML's built-in element
 * defaults, inheritance including `em` resolution, unit conversion, decoration
 * propagation, and the `class=`/`id=` tag grammar.
 */
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <Lotte/Text/LteCssStyle.hpp>
#include <Lotte/Text/LteMml.hpp>
#include <Lotte/Text/LteRichText.hpp>
#include <string>

#ifdef LTE_WITH_LIBCSS

namespace {

using Lotte::Text::CssMetrics;

/** @brief Runs @p mml through @p css and returns the runs. */
Lotte::Text::TextRunList render(const std::string& mml, const std::string& css,
				const Lotte::Text::TextAttributes& base = {})
{
	const Lotte::Text::CssStylesheet sheet(css, CssMetrics{});
	return Lotte::Text::styleMarkup(mml, sheet, base);
}

/** @brief Returns the attributes of the run that contains @p needle. */
Lotte::Text::TextAttributes attributesOf(const Lotte::Text::TextRunList& runs,
					 const std::u32string& needle)
{
	for (const Lotte::Text::TextRun& run : runs) {
		if (run.text.find(needle) != std::u32string::npos) return run.attributes;
	}
	return {};
}


} // namespace

TEST_CASE("MML tags are elements for CSS, with their usual defaults", "[text][css]")
{
	// No author rules at all: `<b>`/`<u>` must still mean what MML says they mean,
	// which is what the built-in (user-agent) stylesheet is for.
	const Lotte::Text::TextRunList runs = render("<b>bold</b> plain <u>under</u>", "");
	CHECK(attributesOf(runs, U"bold").bold);
	CHECK_FALSE(attributesOf(runs, U"plain").bold);
	CHECK(attributesOf(runs, U"under").underline);
	CHECK_FALSE(attributesOf(runs, U"under").bold);
}

TEST_CASE("Author rules override the element defaults", "[text][css]")
{
	const Lotte::Text::TextRunList runs =
		render("<b>not bold</b><i>slanted</i>", "b { font-weight: normal } i { font-style: normal }");
	CHECK_FALSE(attributesOf(runs, U"not bold").bold);
	CHECK_FALSE(attributesOf(runs, U"slanted").italic);
}

TEST_CASE("Type, class and id selectors all match MML tags", "[text][css]")
{
	const std::string css =
		".warn { color: #ff0000 }\n"
		"#loud { color: #00ff00 }\n"
		"b { color: #0000ff }\n";
	const Lotte::Text::TextRunList runs =
		render("<b>blue</b><b class=warn>red</b><b class=warn id=loud>green</b>", css);

	const auto blue = attributesOf(runs, U"blue").colour;
	CHECK(blue.r == Catch::Approx(0.0f));
	CHECK(blue.b == Catch::Approx(1.0f));
	// A class beats the type selector (higher specificity).
	const auto red = attributesOf(runs, U"red").colour;
	CHECK(red.r == Catch::Approx(1.0f));
	CHECK(red.b == Catch::Approx(0.0f));
	// An id beats a class.
	const auto green = attributesOf(runs, U"green").colour;
	CHECK(green.g == Catch::Approx(1.0f));
	CHECK(green.r == Catch::Approx(0.0f));
}

TEST_CASE("The test fixture never reaches the text", "[text][css]")
{
	// `class=`/`id=` are parameters in either spelling; they must not leak.
	const Lotte::Text::TextRunList runs = render("[b class=warn]x[/b]", ".warn { color: #123456 }");
	REQUIRE(runs.size() >= 1);
	CHECK(runs.back().text == std::u32string(U"x"));
}

TEST_CASE("Inherited properties travel down the element stack", "[text][css]")
{
	const std::string css = "p { color: #00ff00; font-size: 10px }";
	const Lotte::Text::TextRunList runs = render("<p>outer<b>inner</b></p>", css);

	CHECK(attributesOf(runs, U"outer").colour.g == Catch::Approx(1.0f));
	// `<b>` sets no colour of its own, so it inherits the paragraph's.
	CHECK(attributesOf(runs, U"inner").colour.g == Catch::Approx(1.0f));
	CHECK(attributesOf(runs, U"inner").bold);
	CHECK(attributesOf(runs, U"outer").size == Catch::Approx(10.0f));
}

TEST_CASE("em sizes resolve against the parent's computed size", "[text][css]")
{
	const std::string css = "p { font-size: 10px } b { font-size: 1.5em }";
	const Lotte::Text::TextRunList runs = render("<p><b>big</b></p>", css);
	CHECK(attributesOf(runs, U"big").size == Catch::Approx(15.0f));
}

TEST_CASE("Absolute units and keywords become pixels", "[text][css]")
{
	const Lotte::Text::TextRunList points =
		render("<b>pt</b>", "b { font-size: 12pt }");
	// 12pt at 96dpi is 16px.
	CHECK(attributesOf(points, U"pt").size == Catch::Approx(16.0f));

	const Lotte::Text::TextRunList keyword =
		render("<b>large</b>", "b { font-size: large }");
	// libcss resolves the keyword itself, using its own table (large = 1.125 medium).
	CHECK(attributesOf(keyword, U"large").size == Catch::Approx(16.0f * 1.125f));
}

TEST_CASE("text-decoration is propagated like a browser paints it", "[text][css]")
{
	const Lotte::Text::TextRunList runs = render("<u>a<b>b</b></u>", "");
	CHECK(attributesOf(runs, U"a").underline);
	// CSS does not inherit text-decoration, but the line is painted through `<b>`.
	const auto inner = attributesOf(runs, U"b");
	CHECK(inner.underline);
	CHECK(inner.bold);
}

TEST_CASE("font-family keeps the first named family, lowercased", "[text][css]")
{
	const Lotte::Text::TextRunList runs =
		render("<font>x</font>", "font { font-family: 'DejaVu Sans', serif }");
	CHECK(attributesOf(runs, U"x").familyName == "dejavu sans");
}

TEST_CASE("The base attributes seed the root and survive unknown properties")
{
	Lotte::Text::TextAttributes base;
	base.colour = Lotte::Text::Colour(0.25f, 0.5f, 0.75f, 1.0f);
	base.size = 21.0f;
	base.familyName = "dejavu serif";

	// Everything below is layout-only: parsed by libcss, invisible to MML.
	const std::string css = "p { margin: 4px; padding: 2px; background: red; text-align: center }";
	const Lotte::Text::TextRunList runs = render("<p>plain</p>", css, base);
	const auto attributes = attributesOf(runs, U"plain");

	CHECK(attributes.colour.r == Catch::Approx(0.25f).margin(1.0f / 255.0f));
	CHECK(attributes.colour.g == Catch::Approx(0.5f).margin(1.0f / 255.0f));
	CHECK(attributes.colour.b == Catch::Approx(0.75f).margin(1.0f / 255.0f));
	CHECK(attributes.size == Catch::Approx(21.0f));
	CHECK(attributes.familyName == "dejavu serif");
}

TEST_CASE("Structural tags still break lines under CSS", "[text][css]")
{
	const Lotte::Text::TextRunList runs = render("<p>one<br>two</p>", "p { color: red }");
	std::u32string all;
	for (const auto& run : runs) all += run.text;
	CHECK(all == std::u32string(U"one\ntwo\n"));
}

TEST_CASE("Without a stylesheet the parser still honours the class= grammar", "[text][mml]")
{
	// A document that carries CSS parameters must parse identically when styling is
	// not attached: `class=`/`id=` are simply ignored by the built-in handlers.
	Lotte::Text::RichTextProcessor processor;
	Lotte::Text::MmlParser parser(&processor);
	parser.parse("[b class=warn]x[/b]");
	processor.flush();
	REQUIRE(processor.getRuns().size() == 1);
	CHECK(processor.getRuns()[0].text == std::u32string(U"x"));
	CHECK(processor.getRuns()[0].attributes.bold);
}

TEST_CASE("A stylesheet can be inspected and reused", "[text][css]")
{
	const Lotte::Text::CssStylesheet sheet("b { color: red }");
	CHECK_FALSE(sheet.hadParseProblem());
	CHECK_FALSE(Lotte::Text::CssStylesheet::markupDefaults().empty());

	// Two documents, one sheet: the sheet holds no per-document state.
	const auto first = Lotte::Text::styleMarkup("<b>a</b>", sheet);
	const auto second = Lotte::Text::styleMarkup("<b>b</b>", sheet);
	CHECK(attributesOf(first, U"a").colour.r == Catch::Approx(1.0f));
	CHECK(attributesOf(second, U"b").colour.r == Catch::Approx(1.0f));
}

#endif // LTE_WITH_LIBCSS
