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

#ifndef DTGAMELAUNCHERCOMMON_H
#define DTGAMELAUNCHERCOMMON_H

#include "QzPluginLoader.h"

typedef FILE* fopen_t( const char*, const char* );
typedef int fclose_t( FILE* );
typedef size_t fwrite_t( const void*, size_t, size_t, FILE* );

static fopen_t* fopen_jmp;
static fclose_t* fclose_jmp;
static fwrite_t* fwrite_jmp;
static FILE* hwConfigFile = 0;
static FILE* repConfigFile = 0;

static bool strEndsWith( const char* str, const char* end ) {
    int endLen = strlen( end );
    return !static_cast< bool >( strncmp( str + strnlen( str, 255 ) - endLen, end, endLen ) );
}

FILE* fopen_d( const char* filename, const char* mode ) {
#ifdef Q_OS_LINUX
    const char* hwName = "/qzconfig.cfg";
    const char* repName = "/repconfig.cfg";
#elif defined Q_OS_WIN
    const char* hwName = "\\qzconfig.cfg";
    const char* repName = "\\repconfig.cfg";
#endif

    if ( strEndsWith( filename, hwName ) ) {
        hwConfigFile = fopen_jmp( filename, mode );
        return hwConfigFile;
    }
    else if ( strEndsWith( filename, repName ) ) {
        repConfigFile = fopen_jmp( filename, mode );
        return repConfigFile;
    }

    return fopen_jmp( filename, mode );
}

int fclose_d( FILE* stream ) {
    if ( stream == hwConfigFile ) {
        hwConfigFile = 0;
    }
    else if ( stream == repConfigFile ) {
        repConfigFile = 0;
    }

    return fclose_jmp( stream );
}

size_t fwrite_d( const void* ptr, size_t size, size_t count, FILE* stream ) {
    if ( stream == hwConfigFile || stream == repConfigFile ) {
        QString line = static_cast< const char* >( ptr );
        QRegExp cvarReg( "seta ([\\w]+) \"([^\"]*)" );
        const QHash< QString, QString >& settings = DtQzPluginLoader::getStoredCvars();

        if ( cvarReg.indexIn( line ) != -1 ) {
            if( settings.contains( cvarReg.cap( 1 ) ) ) {
                line = QString( "seta %1 \"%2\"\n" ).arg( cvarReg.cap( 1 ),
                                                          settings.value( cvarReg.cap( 1 ) ) );
                QByteArray setLine = line.toAscii();
                fwrite_jmp( setLine.constData(), 1, setLine.size(), stream );
                return count;
            }
            else if ( DtQzPluginLoader::isCvarAffected( cvarReg.cap( 1 ) ) ) {
                return count;
            }
        }
    }

    return fwrite_jmp( ptr, size, count, stream );
}

#endif

