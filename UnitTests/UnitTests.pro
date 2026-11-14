TEMPLATE = app
CONFIG += console c++2a link_pkgconfig
CONFIG -= qt

TARGET = UnitTests

# Catch2 is consumed from the system installation (headers in <catch2/...>, shared
# libraries providing the framework and main()). The pkg-config module is the portable
# way to reach both; it expands to "-lCatch2Main -lCatch2".
PKGCONFIG += catch2-with-main

# Link to our local libraries
LIBS += -L$$OUT_PWD/../Elvavena -lElvavena
LIBS += -L$$OUT_PWD/../Euphemy -lEuphemy
LIBS += -L$$OUT_PWD/../Lotte -lLotte
LIBS += -L$$OUT_PWD/../Haruka -lHaruka

INCLUDEPATH += $$PWD/..
INCLUDEPATH += $$PWD

SOURCES += \
    tests/AssetRuntimeTests.cpp \
    tests/CompressionTests.cpp \
    tests/ConfigTests.cpp \
    tests/FontRoundTripTests.cpp \
    tests/ImageCodecTests.cpp

HEADERS += \
    support/TestSupport.hpp

DISTFILES += README.md
