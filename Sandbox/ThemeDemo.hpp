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
#include <string>

namespace Sandbox {

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
