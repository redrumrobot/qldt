/*
Copyright 2010-2011 Ed Bow <edxbow@gmail.com>

This file is part of Quake Live - Demo Tools (QLDT).

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#ifndef DTPLAYERDATA_H
#define DTPLAYERDATA_H

#include <qglobal.h>
#include <QSize>
#include <QString>
#include <QList>

#define QZ_DONTCHANGE -3
#define QZ_DESKTOP -2
#define QZ_640x480 5
#define QZ_800x600 9
#define QZ_1024x640 10
#define QZ_1024x768 12

#define QA_640x480 3
#define QA_800x600 4
#define QA_1024x768 6

extern const char* pluginFileName;
extern const char* firefox3ProfilesPath;
extern const char* baseSubDir;
extern const char* demoSubDir;

extern const char* firefoxUserAgent;

extern const QList< QSize > qaModes;
extern const QList< QSize > qzModes;

#endif // DTPLAYERDATA_H
