TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
TARGET = PakTool

SOURCES += main.cpp \
    CFourCC.cpp \
    TropicalFreeze/CTropicalFreezePak.cpp \
    StringUtil.cpp \
    Prime/CDependencyParser.cpp \
    Prime/CPrimePak.cpp \
    Corruption/CCorruptionPak.cpp \
    Compression.cpp \
    TropicalFreeze/DecompressLZSS.cpp \
    MREA.cpp

HEADERS += \
    types.h \
    CFourCC.h \
    TropicalFreeze/SSectionHeader.h \
    TropicalFreeze/CTropicalFreezePak.h \
    TropicalFreeze/SRFRMHeader.h \
    TropicalFreeze/SFileID_128.h \
    StringUtil.h \
    PakList.h \
    Prime/CDependencyParser.h \
    Prime/CPrimePak.h \
    PakEnum.h \
    SAreaHeader.h \
    Corruption/CCorruptionPak.h \
    Compression.h \
    TropicalFreeze/DecompressLZSS.h \
    MREA.h


LIBS += -LE:/C++/Libraries/zlib/lib/ -lzdll
INCLUDEPATH += E:/C++/Libraries/zlib/include
DEPENDPATH += E:/C++/Libraries/zlib/include
PRE_TARGETDEPS += E:/C++/Libraries/zlib/lib/zdll.lib

win32:CONFIG(release, debug|release): LIBS += -LE:/C++/Libraries/FileIO/lib/ -lFileIO
else:win32:CONFIG(debug, debug|release): LIBS += -LE:/C++/Libraries/FileIO/lib/ -lFileIOd
INCLUDEPATH += E:/C++/Libraries/FileIO/include
DEPENDPATH += E:/C++/Libraries/FileIO/include
CONFIG(release, debug|release): PRE_TARGETDEPS += E:/C++/Libraries/FileIO/lib/FileIO.lib
CONFIG(debug, debug|release): PRE_TARGETDEPS += E:/C++/Libraries/FileIO/lib/FileIOd.lib

CONFIG(release, debug|release): LIBS += -LE:/C++/Libraries/boost_1_56_0/lib32-msvc-12.0 -llibboost_filesystem-vc120-mt-1_56
CONFIG(debug, debug|release): LIBS += -LE:/C++/Libraries/boost_1_56_0/lib32-msvc-12.0 -llibboost_filesystem-vc120-mt-gd-1_56
INCLUDEPATH += E:/C++/Libraries/boost_1_56_0

LIBS += -LE:/C++/Libraries/lzo-2.08/lib/ -llzo-2.08
INCLUDEPATH += E:/C++/Libraries/lzo-2.08/include
DEPENDPATH += E:/C++/Libraries/lzo-2.08/include
PRE_TARGETDEPS += E:/C++/Libraries/lzo-2.08/lib/lzo-2.08.lib

CONFIG(release, debug|release): LIBS += -LE:/C++/Libraries/hashlib++/lib/ -lhashlib++
CONFIG(debug, debug|release): LIBS += -LE:/C++/Libraries/hashlib++/lib/ -lhashlib++d
INCLUDEPATH += E:/C++/Libraries/hashlib++/include
DEPENDPATH += E:/C++/Libraries/hashlib++/include
