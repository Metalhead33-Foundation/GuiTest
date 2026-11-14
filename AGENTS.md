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

Whenever calling `git`, always use `--no-pager`.
