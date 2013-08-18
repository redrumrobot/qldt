TARGET          =   qldtce
TEMPLATE        =   app
QT              +=  core gui
CONFIG          +=  debug_and_release
TRANSLATIONS    =   res/lang/ce_ru.ts

include(../../qtsingleapp/src/qtsingleapplication.pri)

INCLUDEPATH += ../../../include ../include
DEPENDPATH += ../../../include ../include

DESTDIR = ../../../

RESOURCES += qldtce.qrc lang.qrc
OTHER_FILES += ../../res/style/cfgeditor.qss

SOURCES += qldtce.cpp               \
    GameConfigEditor.cpp            \
    SyntaxHighlighter.cpp           \
    ConfigEditorData.cpp            \
    ../../OptionsDialog.cpp         \
    ../../EditorOptionsDialog.cpp   \
    ../../Table.cpp                 \
    ../../TableDelegate.cpp         \
    ../../Crypt.cpp                 \
    ../../ClearLineEdit.cpp         \
    ../../PlayerData.cpp            \
    ../../TabWidget.cpp             \
    ../../Config.cpp                \
    ../../About.cpp

HEADERS  += GameConfigEditor.h  \
    SyntaxHighlighter.h         \
    EditorOptionsDialog.h       \
    ConfigEditorData.h          \
    OptionsDialog.h             \
    Table.h                     \
    TableDelegate.h             \
    Crypt.h                     \
    ClearLineEdit.h             \
    PlayerData.h                \
    TabWidget.h                 \
    Config.h                    \
    About.h

unix {
    QMAKE_CXXFLAGS += -pedantic -Wno-long-long

    app.path = /usr/share/applications
    app.files = ./desktop/qldtce.desktop

    mime.path = /usr/share/mime/packages
    mime.files = ./desktop/qldtce.xml

    target.path = /usr/local/bin

    INSTALLS += app mime target
}

win32 {
    RC_FILE = qldtce.rc
}

