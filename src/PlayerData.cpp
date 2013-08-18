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

#include "PlayerData.h"

#ifdef Q_OS_LINUX
#ifdef __x86_64__
    const char* pluginFileName = "npquakelive.x64.so";

    const char* firefoxUserAgent = "Mozilla/5.0 (X11; U; Linux x86_64; es-US; rv:1.9.1.9) Gecko/20100402 Ubuntu/9.10 (karmic) Firefox/3.5.9";
#else
    const char* pluginFileName = "npquakelive.i386.so";

    const char* firefoxUserAgent = "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9.1.9) Gecko/20100315 Ubuntu/9.10 (karmic) Firefox/3.5.9";
#endif
    const char* firefox3ProfilesPath = "/.mozilla/firefox";
#elif defined Q_OS_WIN
    const char* pluginFileName = "";
    const char* firefox3ProfilesPath = "/Application Data/Mozilla/Firefox";

    const char* firefoxUserAgent = "Mozilla/5.0 (Windows; U; Windows NT 6.1; en-US; rv:1.9.1.9) Gecko/20100315 Firefox/3.5.9";
#endif

const char* baseSubDir = "baseq3";
const char* demoSubDir = "demos";

#define M( w, h ) << QSize( w, h )

const QList< QSize > qaModes = QList< QSize >()
    M( 320, 240 )   M( 400, 300 )   M( 512, 384 )
    M( 640, 480 )   M( 800, 600 )   M( 960, 720 )
    M( 1024, 768 )  M( 1152, 864 )  M( 1280, 1024 )
    M( 1600, 1200 ) M( 2048, 1536 ) M( 856, 480 );

const QList< QSize > qzModes = QList< QSize >()
    M( 320, 240 )    M( 400, 300 )   M( 512, 384 )
    M( 640, 360 )    M( 640, 400 )   M( 640, 480 )
    M( 800, 450 )    M( 852, 480 )   M( 800, 500 )
    M( 800, 600 )    M( 1024, 640 )  M( 1024, 576 )
    M( 1024, 768 )   M( 1152, 864 )  M( 1280, 720 )
    M( 1280, 768 )   M( 1280, 800 )  M( 1280, 1024 )
    M( 1440, 900 )   M( 1600, 900 )  M( 1600, 1000 )
    M( 1680, 1050 )  M( 1600, 1200 ) M( 1920, 1080 )
    M( 1920, 1200 )  M( 1920, 1440 ) M( 2048, 1536 )
    M( 2560, 1600 );
