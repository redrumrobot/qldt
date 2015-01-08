TARGET                  =   qldt
TEMPLATE                =   app
QT                      =   core gui network xml sql
CONFIG                  +=  debug_and_release
TRANSLATIONS            =   res/lang/ru.ts

include(qtsingleapp/src/qtsingleapplication.pri)

INCLUDEPATH += ../include                           \
    ../include/firefox                              \
    ../include/firefox/modules/plugin/base/public   \
    ../include/firefox/js/src                       \
    zlib/contrib/minizip                            \
    unrar                                           \
    hde                                             \
    p7zip/src                                       \
    p7zip/include

DEPENDPATH += ../include                            \
    ../include/firefox                              \
    ../include/firefox/modules/plugin/base/public   \
    ../include/firefox/js/src                       \
    zlib/contrib/minizip                            \
    unrar                                           \
    hde                                             \
    detours                                         \
    p7zip/src                                       \
    p7zip/include

DESTDIR	= ../

RESOURCES += qldt.qrc
OTHER_FILES += res/style/main.qss   \
    res/style/carbon_player.qss     \
    res/style/black_player.qss      \
    res/style/cfgeditor.qss

HEADERS += MainWindow.h     \
    Huffman.h               \
    AbstractProtocol.h      \
    Demo.h                  \
    Dm68.h                  \
    Dm73.h                  \
    Config.h                \
    DemoTable.h             \
    MainTable.h             \
    ScanTable.h             \
    TabWidget.h             \
    Table.h                 \
    TablesWidget.h          \
    MainTabWidget.h         \
    FindFragsPanel.h        \
    EditTab.h               \
    DemoPartsWidget.h       \
    TableDelegate.h         \
    Data.h                  \
    DemoData.h              \
    Version.h               \
    QuakeEnums.h            \
    QuakeCommon.h           \
    MainWidget.h            \
    ScanWidget.h            \
    Config.h                \
    ProgressDialog.h        \
    PlayerData.h            \
    PlayerWindow.h          \
    GameLauncher.h          \
    Task.h                  \
    OptionsDialog.h         \
    MainOptionsDialog.h     \
    EditorOptionsDialog.h   \
    Crypt.h                 \
    DirTree.h               \
    FileDialog.h            \
    Archiver.h              \
    crypt.h                 \
    ioapi.h                 \
    unzip.h                 \
    zip.h                   \
    dll.hpp                 \
    unrar.h                 \
    7z.h                    \
    About.h                 \
    CommandsWidget.h        \
    ClearLineEdit.h         \
    XmlReader.h             \
    XmlWriter.h             \
    FindText.h              \
    FindTextDialog.h        \
    FormatDialog.h          \
    FindFragsTable.h        \
    FindTextTable.h         \
    Dm73MapFix.h            \
    ConvertDialog.h         \
    HelpDialog.h            \
    HuffmanStatic.h         \
    DemoDatabase.h          \
    FindDemoDialog.h        \
    FindDemo.h              \
    FindDemoTable.h

SOURCES += qldt.cpp           \
    MainWindow.cpp            \
    Huffman.cpp               \
    AbstractProtocol.cpp      \
    Demo.cpp                  \
    Dm68.cpp                  \
    Dm73.cpp                  \
    DemoTable.cpp             \
    MainTable.cpp             \
    ScanTable.cpp             \
    TabWidget.cpp             \
    Table.cpp                 \
    TablesWidget.cpp          \
    MainTabWidget.cpp         \
    FindFragsPanel.cpp        \
    EditTab.cpp               \
    DemoPartsWidget.cpp       \
    TableDelegate.cpp         \
    Data.cpp                  \
    MainWidget.cpp            \
    ScanWidget.cpp            \
    Config.cpp                \
    ProgressDialog.cpp        \
    PlayerData.cpp            \
    PlayerWindow.cpp          \
    GameLauncher.cpp          \
    Task.cpp                  \
    OptionsDialog.cpp         \
    MainOptionsDialog.cpp     \
    EditorOptionsDialog.cpp   \
    Crypt.cpp                 \
    DirTree.cpp               \
    FileDialog.cpp            \
    Archiver.cpp              \
    ioapi.c                   \
    unzip.c                   \
    zip.c                     \
    7z.cpp                    \
    About.cpp                 \
    CommandsWidget.cpp        \
    ClearLineEdit.cpp         \
    XmlReader.cpp             \
    XmlWriter.cpp             \
    FindText.cpp              \
    FindTextDialog.cpp        \
    FormatDialog.cpp          \
    FindFragsTable.cpp        \
    FindTextTable.cpp         \
    Dm73MapFix.cpp            \
    ConvertDialog.cpp         \
    HelpDialog.cpp            \
    DemoDatabase.cpp          \
    FindDemoDialog.cpp        \
    FindDemo.cpp              \
    FindDemoTable.cpp

linux-g++-64 {
    HEADERS += hde64.h
    SOURCES += hde64.c
}

linux-g++-32 {
    HEADERS += hde32.h
    SOURCES += hde32.c
}

unix {
    INCLUDEPATH += ./p7zip/src/linux/Common
    DEPENDPATH += ./p7zip/src/linux/Common

    DEFINES += ENV_UNIX

    LIBS += -lz ./unrar/libunrar.a ./p7zip/libp7zip.a -lX11 -ldl
    QMAKE_CXXFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS_RELEASE += -O3
    QMAKE_CXXFLAGS += -pedantic -Wno-long-long

    doc.path = /usr/local/share/qldt/
    doc.files = CHANGELOG COPYING VERSION README.TXT

    pixmaps.path = /usr/share/pixmaps
    pixmaps.files = ./desktop/qldt_logo.png ./desktop/quakelive_demo.png ./desktop/quake3_demo.png ./desktop/quakelive_cfg.png

    app.path = /usr/share/applications
    app.files = ./desktop/qldt.desktop

    mime.path = /usr/share/mime/packages
    mime.files = ./desktop/qldt.xml

    target.path = /usr/local/bin

    INSTALLS += doc pixmaps app mime target
}

win32 {
    LIBS += -loleaut32 ./p7zip/p7zip.lib -luser32 -L../ -lzlibwapi -lunrar
    RC_FILE = qldt.rc
    QMAKE_CXXFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS_RELEASE += -MP2 -Ox
    DEFINES += _CRT_SECURE_NO_WARNINGS _UNICODE
}
