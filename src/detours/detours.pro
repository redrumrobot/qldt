TARGET          =  detours
TEMPLATE        =  lib
CONFIG          =  staticlib debug_and_release
QT              = 

INCLUDEPATH += .

SOURCES += creatwth.cpp  \
    detours.cpp          \
    disasm.cpp           \
    image.cpp            \
    modules.cpp

DEFINES += WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x403 DETOURS_X86=1 _X86_
QMAKE_CXXFLAGS_RELEASE = -nologo -W4 -WX -Zi -MTd -Gy -Gm- -Zl -Gs -O1

