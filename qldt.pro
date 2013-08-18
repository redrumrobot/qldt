CONFIG += ordered
TEMPLATE = subdirs

unix:SUBDIRS += src/unrar
win32:SUBDIRS += src/detours
SUBDIRS	+= src/p7zip src src/configeditor/src
