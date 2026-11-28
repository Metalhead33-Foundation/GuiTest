**Never EVER put Makefiles, executables, libraries, .o files into any folder outside "build"! The "build" folder exists for a good reason! This is a QMake project!**

**Call `make` with an explicit absolute path.** In a terminal the shell rewrites `argv[0]`, so
   GNU make derives its `MAKE` variable from the Cursor binary and recursive `make` spawns new Cursor
   windows. Work around it with:

   ```bash
   env MAKE=/usr/bin/make /usr/bin/make -j16
   ```

   The same rewrite affects `QMAKE`, so a changed `.pro` file or a new subproject
   must be regenerated explicitly with `/usr/bin/qmake6`, and recursive make must
   inherit the override:

   ```bash
   env MAKE=/usr/bin/make /usr/bin/make -j16 QMAKE=/usr/bin/qmake6
   ```

**Call the compiler through `env` too.** The `argv[0]` rewrite hits the GCC driver as well: a bare
   `/usr/bin/g++ ...` typed into the Cursor terminal resolves its own program prefix (where `cc1plus`,
   `cc1`, `as`, `ld` live) from the rewritten `argv[0]`, so it looks next to the Cursor AppImage instead
   of `/usr/libexec/gcc/...` and dies with:

   ```
   Cursor-2.1.46-x86_64.AppImage: fatal error: cannot execute 'cc1plus': posix_spawnp: No such file or directory
   compilation terminated.
   ```

   The tell-tale sign is that the diagnostic is prefixed with `Cursor-…-x86_64.AppImage:` instead of
   `g++:` (the trailing errno text is localized, e.g. Hungarian "Nincs ilyen fájl vagy könyvtár"). It
   affects every GCC on the machine (`g++`, `g++-11`, `g++-16`, …), not just one version.
   `env` re-executes the real binary with a correct `argv[0]`, so prefix it on every *direct* compiler
   call:

   ```bash
   env /usr/bin/g++ -std=gnu++2a -I. -c foo.cpp -o /tmp/foo.o
   ```

   Equivalents, if `env` is not an option: `COMPILER_PATH=/usr/libexec/gcc/x86_64-pc-linux-gnu/15/` or
   `-B/usr/libexec/gcc/x86_64-pc-linux-gnu/15/` on the command line. The version directory must match
   the active driver (`/usr/bin/g++ -dumpversion`, `gcc-config -c` → `x86_64-pc-linux-gnu-15`).
   `GCC_EXEC_PREFIX` does **not** work for this. Compiler invocations spawned by `make` (the generated
   Makefiles use a bare `CXX = g++`) are unaffected — only the first process the terminal execs is
   rewritten — which is why the normal build works while a manual `g++` does not.

   Scratch/probe programs that link the built libraries (keep them in `/tmp`, never in the repo):

   ```bash
   env /usr/bin/g++ -std=gnu++2a -I. /tmp/probe.cpp \
     -Lbuild/Desktop-Debug/{Haruka,Euphemy,Elvavena} \
     -lHaruka -lEuphemy -lElvavena -lopenmpt -o /tmp/probe
   LD_LIBRARY_PATH=build/Desktop-Debug/Elvavena:build/Desktop-Debug/Euphemy:build/Desktop-Debug/Lotte:build/Desktop-Debug/Haruka /tmp/probe
   ```

   For a probe that uses SDL, add `-ISandbox $(pkg-config --cflags --libs sdl2)` and, when it must
   exercise the real driver code, compile `Sandbox/HarukaContext.cpp` alongside it. The Catch2 suite
   also needs the Kaldi stub libraries on the library path:

   ```bash
   cd build/Desktop-Debug && \
   LD_LIBRARY_PATH=Elvavena:Euphemy:Lotte:Haruka:KaldiVK:Kaldi ./UnitTests/UnitTests
   ```

**Documentation is checked, not remembered.** A header, a note or a module README is part of
   the generated reference: `doxygen Doxyfile` writes `build/doxygen/html/` and must report
   nothing, and `UnitTests "[docs]"` checks the Markdown tree (note ids, folder indexes,
   module pages, "References is last"). Run both after touching a header or a `.md` file;
   `docs/DOCUMENTATION.md` is the policy they enforce. The render never writes into the
   source tree.

Whenever calling `git`, always use `--no-pager`.

**JSON is an application concern, not a library one.** `nlohmann_json` is a dependency of
   `Sandbox` and `UnitTests` only; no library in this tree may include it or any other parser.
   A loader parses a document into the plain C++ structs the library already exposes
   (`Lotte::Gui::ThemeDefinition` is the reference example), and every fixed vocabulary is an
   enum with one name table (`parseThemeElement`, `guiAtlasPartName`, ...) - never a
   `std::map<std::string, ...>` inside a loader. The reasoning is in `docs/PROCEDURAL.md` §1.

**The same holds for YAML, and no user-visible string lives in C++.** `libyaml` is a dependency
   of `Sandbox` (and of the tests that compile its loader) only: `Sandbox/LocalizationYaml.cpp`
   fills a plain `Euph::Loc::Localization`, which is what a library is allowed to expose.
   Captions, labels and menu entries are keys looked up at runtime; a missing key renders as the
   key itself, never as an empty widget, and a new caption is a new line under
   `Sandbox/localization/<language>/` - not a string literal next to the code that shows it.

**CSS is libcss's job, not a second parser.** Theme typography and box hints are CSS
   declarations resolved through `libcss` (`Lotte::Gui::resolveThemeCss` ->
   `Text::computeElementStyle`); the library wraps them in a universal rule and reads the
   computed style back. Never hand-parse `font-size`/`color`/`padding` in a loader, and
   never resolve a theme's CSS per frame: `DefinitionTheme` resolves it once when it is
   built. Without `LTE_WITH_LIBCSS` a block is reported and ignored, never fatal.

**Noise is a Euphemy concern.** Every generator lives in `EuphNoise.hpp`; `Lotte/Procedural/*`
   composes them and never rolls its own - no local hash, no hand-rolled grating, no private
   smoothstep. Gradient noise (`latticeGradient2D`, `latticeGradientFbm2D`) is the default for
   a surface, value noise keeps its place where the lattice *is* the effect, the finest octave
   stays at or above ~2 px per cycle, and a stretched field gets exactly one octave. The
   parchment hatch and both of its causes are written up in `docs/PROCEDURAL.md` §5.

**A GUI layout is written on a virtual canvas, never in window pixels.** The design size is a
   `Lotte::Gui::ViewportSettings::canvas` (800x600 in the sandbox) and
   `Lotte::Gui::GuiViewport` is the only place a real resolution may enter: it turns canvas
   units into pixels with one uniform factor (`fit` keeps the whole canvas and letterboxes,
   `fill` covers the window and crops). Widget rectangles, font sizes and mouse positions all
   pass through it, so "GUI scale" is one number rather than a per-element concern, and a script
   that never reads `GUI.Layout.resolution` works on every window. A widget's box is stored in
   *canvas* units by `Sandbox::GuiScene` and re-derived on every viewport change - never cache a
   pixel rectangle in a script. A layout may use `0..1` fractions (`UI.x(0.5)`) instead of
   numbers, but it must not branch on the window size to fake scaling.
