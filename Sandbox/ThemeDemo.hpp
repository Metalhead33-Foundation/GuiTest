#ifndef SANDBOX_THEMEDEMO_HPP
#define SANDBOX_THEMEDEMO_HPP
/**
 * @file ThemeDemo.hpp
 * @brief The `--theme` CLI: parse a theme file, generate its atlases, dump them.
 *
 * This is the end-to-end smoke test for the pieces added around the theme work:
 * the JSON loader (`ThemeJson`) parses into `Lotte::Gui::ThemeDefinition`, the
 * procedural generator (Lotte) builds the atlas the definition asks for, Euphemy's
 * PNG encoder writes it out, and a `DefinitionTheme` resolves skins from the result.
 * It deliberately runs without a GPU so it can be used from a terminal or CI.
 */
#include <Elvavena/Io/ElvIoSystem.hpp>
#include <Lotte/Gui/LteThemeDefinition.hpp>
#include <Lotte/Procedural/LteGuiAtlas.hpp>
#include <string>

namespace Sandbox {

/**
 * @brief Reads a whole virtual file.
 * @param io Filesystem to read from.
 * @param path Virtual path of the file.
 * @return The file's bytes, or an empty string when it could not be opened.
 *
 * Shared by the `--theme` CLI and the game loop; a missing theme file is not an error
 * in either of them (the sandbox then simply runs unthemed).
 */
std::string readVirtualFile(Elv::Io::System& io, const std::string& path);

/**
 * @brief Turns a definition's procedural entry into generator settings.
 * @param atlas Atlas entry whose seed and scale are honoured.
 * @return Builder settings with every element size scaled by the entry's percentage.
 *
 * Shared by the CLI and the game loop so `--theme` and the running sandbox generate the
 * same atlas from the same document.
 */
Lotte::Procedural::GuiAtlasSettings atlasSettingsFor(const Lotte::Gui::ThemeAtlasDefinition& atlas);

/**
 * @brief Runs the theme demonstration.
 * @param io Filesystem the theme file is read from (the sandbox PhysFS mount).
 * @param themePath Virtual path of the theme document.
 * @param outputDirectory Physical directory the generated atlas PNGs are written to.
 * @return `0` on success, non-zero when the document could not be read or parsed.
 */
int runThemeDemo(Elv::Io::System& io, const std::string& themePath,
                 const std::string& outputDirectory);

} // namespace Sandbox

#endif // SANDBOX_THEMEDEMO_HPP
