TARGET          =   p7zip
TEMPLATE        =   lib
CONFIG          +=  staticlib
QT              = 

INCLUDEPATH += myWindows include_windows
DEPENDPATH += myWindows include_windows

QMAKE_CFLAGS_SHLIB  =  
QMAKE_CFLAGS_STATIC_LIB  =  
QMAKE_CFLAGS =
QMAKE_CFLAGS_RELEASE = -O
QMAKE_CFLAGS_WARN_ON = 
QMAKE_CXXFLAGS_SHLIB =  
QMAKE_CXXFLAGS_STATIC_LIB =
QMAKE_CXXFLAGS_RELEASE = -O
QMAKE_CXXFLAGS_WARN_ON = 
QMAKE_CXXFLAGS =

linux-g++-64 {
    QMAKE_CFLAGS += -m64
    QMAKE_CXXFLAGS += -m64
}

linux-g++-32 {
    QMAKE_CFLAGS += -m32
    QMAKE_CXXFLAGS += -m32
}

DEFINES = _FILE_OFFSET_BITS=64  \
    _LARGEFILE_SOURCE           \
    _NO_CRYPTO                  \
    COMPRESS_MT                 \
    COMPRESS_MF_MT              \
    _NO_CRYPTO                  \
    BREAK_HANDLER               \
    BENCH_MT                    \
    ENV_UNIX                    \
    NDEBUG

DESTDIR = ../../
OBJECTS_DIR = build

SOURCES += \
myWindows/wine_date_and_time.cpp \
Windows/DLL.cpp \
Windows/FileDir.cpp \
Windows/FileFind.cpp \
Windows/FileIO.cpp \
Windows/FileName.cpp \
Windows/PropVariant.cpp \
Windows/PropVariantConversions.cpp \
Common/IntToString.cpp \
Common/MyWindows.cpp \
Common/MyString.cpp \
Common/StringConvert.cpp \
Common/MyVector.cpp \
Common/Wildcard.cpp \
7zip/Common/FileStreams.cpp \
C/Threads.c


