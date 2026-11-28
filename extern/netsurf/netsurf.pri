# Vendored NetSurf libraries: libcss plus the two libraries it needs.
#
# `extern/netsurf` is a checkout of the NetSurf stack (libcss, libparserutils,
# libwapcaplet) together with its buildsystem. It is built by that buildsystem into the
# project's *build* directory and linked statically, so the CSS the engine runs is the one
# this repository pins -- not whatever version the host distribution happens to ship, which
# is what `pkg-config` would otherwise resolve to.
#
# Include this from every project that needs the CSS engine (Lotte and UnitTests): the
# `LTE_WITH_LIBCSS` define and the include path have to agree everywhere, or a header-only
# declaration changes shape between the library and its users.
#
# Escape hatch for a distribution build that wants to share the system library:
#     LOTTE_USE_SYSTEM_CSS=1 qmake ...

NETSURF_ROOT = $$PWD
NETSURF_VENDORED = false

# `$$(...)` reads the environment at qmake time; keep it in a variable first, because an
# unset variable expands to nothing and `isEmpty()` then has no argument at all.
NETSURF_SYSTEM_CSS = $$(LOTTE_USE_SYSTEM_CSS)

isEmpty(NETSURF_SYSTEM_CSS) {
    exists($$NETSURF_ROOT/libcss/Makefile): NETSURF_VENDORED = true
}

equals(NETSURF_VENDORED, true) {
    message("NetSurf: building the vendored libcss from $$NETSURF_ROOT")

    # One prefix per build tree, shared by every subproject that includes this file, and
    # inside it every object the buildsystem produces.
    NETSURF_PREFIX = $$clean_path($$OUT_PWD/../netsurf)
    NETSURF_STAMP = $$NETSURF_PREFIX/.installed

    netsurf_vendored.target = $$NETSURF_STAMP
    netsurf_vendored.commands = $$QMAKE_MKDIR $$shell_quote($$NETSURF_PREFIX) && \
        MAKE=$(MAKE) sh $$shell_quote($$NETSURF_ROOT/build_vendored.sh) \
        $$shell_quote($$NETSURF_PREFIX)
    netsurf_vendored.depends = $$shell_quote($$NETSURF_ROOT/build_vendored.sh)
    QMAKE_EXTRA_TARGETS += netsurf_vendored
    PRE_TARGETDEPS += $$NETSURF_STAMP

    INCLUDEPATH += $$NETSURF_PREFIX/include
    # Static archives, in dependency order: libcss needs both of the others.
    LIBS += -L$$NETSURF_PREFIX/lib -lcss -lparserutils -lwapcaplet
    DEFINES += LTE_WITH_LIBCSS
} else {
    packagesExist(libcss) {
        message("NetSurf: falling back to the system libcss from pkg-config")
        PKGCONFIG += libcss
        DEFINES += LTE_WITH_LIBCSS
    } else {
        message("NetSurf: no libcss at all, LTE_WITH_LIBCSS stays off")
    }
}
