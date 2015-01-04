CONFIG += ordered
TEMPLATE = subdirs

unix:SUBDIRS += src/unrar
SUBDIRS	+= src/p7zip src src/configeditor/src
