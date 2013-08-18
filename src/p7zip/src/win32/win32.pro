TARGET          =   p7zip
TEMPLATE        =   lib
CONFIG          +=  staticlib
QT              = 

INCLUDEPATH += Windows
DEPENDPATH += Windows

QMAKE_CFLAGS_SHLIB  =  
QMAKE_CFLAGS_STATIC_LIB  =  
QMAKE_CFLAGS =
QMAKE_CFLAGS_RELEASE = -Gr -nologo -WX -EHsc -Gy -GR- -W3 -GS- -Zc:forScope -Zc:wchar_t- -O1 -MD
QMAKE_CFLAGS_WARN_ON = 
QMAKE_CXXFLAGS_SHLIB =  
QMAKE_CXXFLAGS_STATIC_LIB =
QMAKE_CXXFLAGS_RELEASE = -Gr -nologo -WX -EHsc -Gy -GR- -W3 -GS- -Zc:forScope -Zc:wchar_t- -O1 -MD
QMAKE_CXXFLAGS_WARN_ON = 
QMAKE_CXXFLAGS =

DEFINES += _FILE_OFFSET_BITS=64  \
    _LARGEFILE_SOURCE           \
    _NO_CRYPTO                  \
    COMPRESS_MT                 \
    COMPRESS_MF_MT              \
    BREAK_HANDLER               \
    BENCH_MT

DESTDIR = ../../
OBJECTS_DIR = build

DEFINES += NDEBUG _WIN32 _UNICODE

SOURCES += \
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


