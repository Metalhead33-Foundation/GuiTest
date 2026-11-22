/**
 * @file BackendConfigTests.cpp
 * @brief End-to-end test of the Kaldi config contract and its `.ini` glue.
 *
 * The chain under test is the one a real engine uses:
 *
 *   GL::ContextConfig  <-  GL::ContextConfigProvider  <-  Lotte::Conf::ConfigurationReader
 *                                                              ^
 *                                        Euph::Conf::Configuration (.ini text)
 *
 * `KaldiGLCommon/GlContextConfig.cpp` is compiled into this binary (see
 * `UnitTests.pro`) so the GL provider is exercised without linking a GL library.
 */
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <Kaldi/KldConfig.hpp>
#include <KaldiGLCommon/GlContextConfig.hpp>
#include <KaldiGLCommon/GlDeviceConfig.hpp>
#include <Lotte/Conf/LteBackendConfig.hpp>
#include <Euphemy/Config/EuphConfiguration.hpp>
#include <string>
#include <string_view>

namespace {

/** @brief Section name used by every test here. */
constexpr std::string_view graphicsSection = "graphics";

/** @brief Writes the provider defaults and returns the configuration. */
Euph::Conf::Configuration makeDefaults()
{
	GL::ContextConfig config = GL::defaultContextConfig();
	GL::ContextConfigProvider provider(config);
	Euph::Conf::Configuration ini;
	Lotte::Conf::writeBackendDefaults(ini, provider, Kld::ContextPurpose::MainWindow, graphicsSection);
	return ini;
}

/** @brief Shorthand for the `graphics` section. */
Euph::Conf::ConfigurationSection& graphics(Euph::Conf::Configuration& ini)
{
	return ini.getSections()[std::string(graphicsSection)];
}

}

TEST_CASE("Provider defaults are written with typed keys", "[config][backend]")
{
	Euph::Conf::Configuration ini = makeDefaults();
	Euph::Conf::ConfigurationSection& section = graphics(ini);

	CHECK(section.getInteger("iWidth") == 1280);
	CHECK(section.getInteger("iHeight") == 720);
	CHECK(section.getInteger("iFramesInFlight") == 2);
	CHECK(section.getString("sPresentMode") == std::string_view("fifo"));
	CHECK(section.getBool("bVSync"));
	CHECK(section.getBool("bSrgbFramebuffer"));

	// Desktop defaults from the GL provider.
	CHECK(section.getString("sGlApi") == std::string_view("auto"));
	CHECK(section.getInteger("iGlVersionMajor") == 4);
	CHECK(section.getInteger("iGlVersionMinor") == 3);
	CHECK(section.getBool("bGlCoreProfile"));
	CHECK(section.getBool("bGlForwardCompatible"));
	CHECK_FALSE(section.getBool("bGlDebugContext", false));
	CHECK(section.getInteger("iGlSamples") == 0);
	CHECK(section.getInteger("iGlDepthBits") == 24);
	CHECK(section.getInteger("iGlStencilBits") == 8);
	CHECK(section.getInteger("iGlScreen") == -1);
}

TEST_CASE("Every described field's key prefix matches its value type", "[config][backend]")
{
	GL::ContextConfig config = GL::defaultContextConfig();
	GL::ContextConfigProvider provider(config);
	const std::span<const Kld::ConfigField> fields = provider.fields(Kld::ContextPurpose::MainWindow);
	REQUIRE(fields.size() >= Kld::commonConfigFields().size());
	for (const Kld::ConfigField& field : fields) {
		REQUIRE(field.key != nullptr);
		CHECK(field.key[0] == Kld::configKeyPrefix(field.type));
	}
}

TEST_CASE("Applying a configuration clamps values to the advertised range", "[config][backend]")
{
	Euph::Conf::Configuration ini = makeDefaults();
	Euph::Conf::ConfigurationSection& section = graphics(ini);
	section.setInt("iGlSamples", 999);     // maximum is 16
	section.setInt("iGlVersionMajor", 99); // maximum is 5
	section.setInt("iGlScreen", 42);       // maximum is 15
	section.setString("sGlApi", "glx");
	section.setBool("bGlCoreProfile", 0u);

	GL::ContextConfig config = GL::defaultContextConfig();
	GL::ContextConfigProvider provider(config);
	REQUIRE(Lotte::Conf::applyBackendConfig(ini, provider, graphicsSection));

	CHECK(config.sampleCount == 16);
	CHECK(config.versionMajor == 5);
	CHECK(config.screen == 15);
	CHECK(config.api == GL::ContextApi::GLX);
	CHECK_FALSE(config.coreProfile);
}

TEST_CASE("A missing section keeps the backend defaults", "[config][backend]")
{
	Euph::Conf::Configuration ini;
	GL::ContextConfig config = GL::defaultContextConfig();
	const GL::ContextConfig original = config;
	GL::ContextConfigProvider provider(config);

	CHECK_FALSE(Lotte::Conf::applyBackendConfig(ini, provider, "does-not-exist"));
	CHECK(config.versionMajor == original.versionMajor);
	CHECK(config.api == original.api);
}

TEST_CASE("A configuration survives serialization and re-applies identically", "[config][backend]")
{
	Euph::Conf::Configuration ini = makeDefaults();
	graphics(ini).setString("sGlApi", "egl");
	graphics(ini).setInt("iGlSamples", 4);
	graphics(ini).setInt("iGlVersionMinor", 1);

	const std::string text = Lotte::Conf::serializeConfiguration(ini);
	REQUIRE_FALSE(text.empty());

	Euph::Conf::Configuration reparsed = Lotte::Conf::parseConfiguration(text);
	GL::ContextConfig config = GL::defaultContextConfig();
	GL::ContextConfigProvider provider(config);
	REQUIRE(Lotte::Conf::applyBackendConfig(reparsed, provider, graphicsSection));

	CHECK(config.api == GL::ContextApi::EGL);
	CHECK(config.sampleCount == 4);
	CHECK(config.versionMinor == 1);
}

TEST_CASE("The provider recommends purpose-specific common defaults", "[config][backend]")
{
	GL::ContextConfig config = GL::defaultContextConfig();
	GL::ContextConfigProvider provider(config);

	const Kld::DeviceConfig main = provider.description(Kld::ContextPurpose::MainWindow);
	CHECK(main.width == 1280);
	CHECK(main.height == 720);
	CHECK(main.vsync);

	const Kld::DeviceConfig offscreen = provider.description(Kld::ContextPurpose::Offscreen);
	CHECK(offscreen.width == 1024);
	CHECK(offscreen.height == 1024);
	CHECK_FALSE(offscreen.vsync);

	const Kld::DeviceConfig debug = provider.description(Kld::ContextPurpose::DebugView);
	CHECK(debug.width == 640);
	CHECK(debug.height == 360);
	CHECK_FALSE(debug.srgbFramebuffer);
}

TEST_CASE("Config helpers reject values outside the choice list", "[config][backend]")
{
	const Kld::ConfigField field{ "sMode", Kld::ConfigValueType::StringValue, 0, 0, "fifo", "immediate|fifo", "help" };
	CHECK(Kld::configFieldAllows(field, "fifo"));
	CHECK(Kld::configFieldAllows(field, "immediate"));
	CHECK_FALSE(Kld::configFieldAllows(field, "mailbox"));
	CHECK(Kld::configFieldAllows(Kld::commonConfigFields().front(), "anything"));
	CHECK(Kld::configValueTypeName(Kld::ConfigValueType::FloatValue) == std::string_view("float"));
	CHECK(Kld::configKeyPrefix(Kld::ConfigValueType::IntegerValue) == 'i');
}

TEST_CASE("The GL device provider adds executor keys to the context keys", "[config][backend]")
{
	GL::DeviceConfig config = GL::defaultDeviceConfig();
	GL::DeviceConfigProvider provider(config);
	const std::span<const Kld::ConfigField> fields = provider.fields(Kld::ContextPurpose::MainWindow);
	const auto hasKey = [&fields](std::string_view key) {
		for (const Kld::ConfigField& field : fields) {
			if (key == field.key) {
				return true;
			}
		}
		return false;
	};
	CHECK(hasKey("sGlApi"));
	CHECK(hasKey("bGlDebugLabels"));
	CHECK(hasKey("bGlForceNearestFiltering"));
	CHECK(hasKey("bGlForceTrilinear"));
	CHECK(hasKey("fGlTextureLodBias"));
	CHECK(hasKey("fGlMaxAnisotropy"));
	CHECK(hasKey("iGlMaxTextureSize"));
	for (const Kld::ConfigField& field : fields) {
		REQUIRE(field.key != nullptr);
		CHECK(field.key[0] == Kld::configKeyPrefix(field.type));
	}
}

TEST_CASE("Applying a device configuration sets and clamps the executor policy", "[config][backend]")
{
	Euph::Conf::Configuration ini;
	Euph::Conf::ConfigurationSection& section = ini.getSections()["graphics"];
	section.setBool("bGlDebugLabels", false);
	section.setBool("bGlForceNearestFiltering", true);
	section.setBool("bGlForceTrilinear", true);
	section.setFloat("fGlTextureLodBias", 0.25F);
	section.setFloat("fGlMaxAnisotropy", 99.0F); // outside the advertised range
	section.setUint("iGlMaxTextureSize", 4096);

	GL::DeviceConfig config = GL::defaultDeviceConfig();
	GL::DeviceConfigProvider provider(config);
	Lotte::Conf::ConfigurationReader reader(ini, "graphics");
	REQUIRE(provider.apply(reader));
	CHECK_FALSE(config.executor.debugLabels);
	CHECK(config.executor.forceNearestFiltering);
	CHECK(config.executor.forceTrilinear);
	CHECK(config.executor.textureLodBias == Catch::Approx(0.25F));
	CHECK(config.executor.maxAnisotropy == Catch::Approx(16.0F));
	CHECK(config.executor.maxTextureSize == 4096);
	// The context half is still applied by the composed provider.
	CHECK(config.context.versionMajor == 4);
}

TEST_CASE("The device provider reports the executor anisotropy in its description", "[config][backend]")
{
	GL::DeviceConfig config = GL::defaultDeviceConfig();
	config.executor.maxAnisotropy = 8.0F;
	GL::DeviceConfigProvider provider(config);
	const Kld::DeviceConfig description = provider.description(Kld::ContextPurpose::MainWindow);
	CHECK(description.maxAnisotropy == Catch::Approx(8.0F));
	CHECK(description.width == 1280);
	CHECK(description.generateMipmaps);
}
