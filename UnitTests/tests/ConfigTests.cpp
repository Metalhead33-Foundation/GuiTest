/**
 * @file ConfigTests.cpp
 * @brief Round-trip tests for Euphemy's INI-style configuration format.
 *
 * The format stores the value *type* in the first letter of the key (`s`, `i`, `u`, `f`, `b`), which
 * is what the parser uses to decode the text; the accessor then has to be asked for the matching
 * type. `Configuration` can be written to a stream and parsed back, which is its only serialization,
 * so these tests check that typed values survive that trip unchanged.
 */
#include <catch2/catch_test_macros.hpp>
#include <Euphemy/Config/EuphConfiguration.hpp>
#include <sstream>
#include <string>

namespace {

/** @brief Builds a configuration using every typed setter the section offers. */
Euph::Conf::Configuration makeConfiguration()
{
	Euph::Conf::Configuration config;
	Euph::Conf::ConfigurationSection& graphics = config.getSections()["graphics"];
	graphics.setInt("iWidth", -1920);
	graphics.setUint("uHeight", 1080u);
	graphics.setFloat("fScale", 1.25f);
	graphics.setString("sMode", "windowed");
	graphics.setBool("bVSync", 1u);

	Euph::Conf::ConfigurationSection& audio = config.getSections()["audio"];
	audio.setUint("uSampleRate", 48000u);
	audio.setFloat("fVolume", 0.75f);
	return config;
}

}

TEST_CASE("Configuration survives a text round-trip", "[config][roundtrip]")
{
	Euph::Conf::Configuration original = makeConfiguration();

	std::ostringstream stream;
	stream << original;
	REQUIRE_FALSE(stream.str().empty());

	Euph::Conf::Configuration reparsed(stream.str());
	REQUIRE(reparsed.getSections().size() == original.getSections().size());
	REQUIRE(reparsed.getSections().count("graphics") == 1);
	REQUIRE(reparsed.getSections().count("audio") == 1);

	Euph::Conf::ConfigurationSection& graphics = reparsed.getSections().at("graphics");
	CHECK(graphics.getInteger("iWidth") == -1920);
	CHECK(graphics.getUInteger("uHeight") == 1080u);
	CHECK(graphics.getFloat("fScale") == 1.25f);
	CHECK(graphics.getString("sMode") == std::string_view("windowed"));
	CHECK(graphics.getBool("bVSync"));

	Euph::Conf::ConfigurationSection& audio = reparsed.getSections().at("audio");
	CHECK(audio.getUInteger("uSampleRate") == 48000u);
	CHECK(audio.getFloat("fVolume") == 0.75f);
}

TEST_CASE("Configuration caches the supplied default for a missing key", "[config]")
{
	Euph::Conf::Configuration config;
	Euph::Conf::ConfigurationSection& section = config.getSections()["missing"];

	CHECK(section.getString("sKey", "fallback") == std::string_view("fallback"));
	CHECK(section.getInteger("iKey", 42) == 42);
	CHECK(section.getUInteger("uKey", 7u) == 7u);
	CHECK(section.getFloat("fKey", 0.5f) == 0.5f);
	CHECK(section.getBool("bKey", true));
	// The defaults were stored, so a second lookup finds them.
	CHECK(section.getInteger("iKey") == 42);
}

TEST_CASE("Configuration parses a hand-written document", "[config]")
{
	static const std::string document =
		"[net]\n"
		"sHost = example.org\n"
		"uPort = 27015\n"
		"bEnabled = 1\n";

	Euph::Conf::Configuration config(document);
	REQUIRE(config.getSections().count("net") == 1);
	Euph::Conf::ConfigurationSection& net = config.getSections().at("net");
	CHECK(net.getString("sHost") == std::string_view("example.org"));
	CHECK(net.getUInteger("uPort") == 27015u);
	CHECK(net.getBool("bEnabled"));
}

TEST_CASE("Configuration rejects keys without a type prefix", "[config]")
{
	// The parser derives the value type from the first letter of the key, so a key that does not
	// start with s/b/f/i/u cannot be represented.
	REQUIRE_THROWS(Euph::Conf::Configuration(std::string("[net]\nhost = example.org\n")));
}
