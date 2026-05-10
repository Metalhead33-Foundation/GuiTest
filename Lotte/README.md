# Lotte

Lotte is currently an empty project placeholder.

It is intended to become a graphics library built on top of Elvavena and Euphemy.

When development starts, this folder will host the Lotte library implementation and supporting project files.

## Build Notes

Kaldi is used by Lotte as a header-only dependency. Do not add `-lKaldi` or `libKaldi` to Lotte project files; there is no Kaldi library target to link against.

This is a qmake project. Makefiles are generated artifacts and are intentionally ignored by the repository. Regenerate them from the existing shadow build directory, for example:

```sh
cd build/Desktop-Debug
qmake6 ../../Project.pro
```

Do not edit source-tree Makefiles to fix Lotte build behavior; update the `.pro` or `.pri` files and regenerate.
