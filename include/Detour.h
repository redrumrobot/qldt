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

#ifndef DTDETOUR_H
#define DTDETOUR_H

#ifdef __x86_64__
#include "hde64.h"
#else
#include "hde32.h"
#endif

#include "Data.h"

#include <QFile>

#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>

static void protectPage( void* ptr, int prot ) {
    void* page = reinterpret_cast< void* >( reinterpret_cast< quint64 >( ptr ) &
                                            ~static_cast< quint64 >( getpagesize() - 1 ) );
    mprotect( page, getpagesize(), prot );
}

static void* openLibrary( const char* libPath ) {
    QFile lib( libPath );

    if ( !lib.open( QFile::ReadOnly ) ) {
        return 0;
    }

    quint32 header = 0;
    lib.read( reinterpret_cast< char* >( &header ), 4 );

    void* dlHandle = 0;
    const quint32 elfMagic = 0x464c457f;

    if ( header == elfMagic ) {
        dlHandle = dlopen( libPath, RTLD_NOW );
    }
    else {
        lib.seek( 0 );

        while ( !lib.atEnd() ) {
            QString line = lib.readLine();

            if ( line.startsWith( "GROUP ( " ) ) {
                QString libName = line.split( ' ' ).at( 2 );

                if ( QFile::exists( libName ) ) {
                    dlHandle = dlopen( libName.toAscii().constData(), RTLD_NOW );
                }

                break;
            }
        }
    }

    lib.close();

    return dlHandle;
}

static void* trampPageAddress;
static uchar* trampAddress;

static void getAddressTable( QVector< quint64 >& table ) {
    QFile maps( "/proc/self/maps" );

    if ( !maps.open( QFile::ReadOnly ) ) {
        return;
    }

    QString line = maps.readLine();
    bool ok;

    while ( !line.isEmpty() ) {
        QStringList region = line.split( ' ' ).at( 0 ).split( '-' );
        table.append( region.at( 0 ).toULong( &ok, 16 ) );
        table.append( region.at( 1 ).toULong( &ok, 16 ) );
        line = maps.readLine();
    }

    maps.close();
}

static bool allocateTrampMemory( void* dlHandle ) {
    quint64 libAddr = *reinterpret_cast< quintptr* >( dlHandle );

    QVector< quint64 > table;
    getAddressTable( table );

    int i = 0;

    while ( table.at( i ) != libAddr || table.at( i + 1 ) == libAddr ) {
        ++i;
    }

    int pageSize = getpagesize();
    const int minSize = 0x400;

    if ( pageSize < minSize ) {
        int pages = minSize / pageSize;

        if ( minSize % pageSize ) {
            ++pages;
        }

        pageSize *= pages;
    }

    quint64 fromAddr = table.at( i + 1 );
    quint64 lastAddr = fromAddr;
    trampPageAddress = 0;

    for ( ; i > 0; i -= 2 ) {
        quint64 rangeEnd = table.at( i + 1 );
        qint64 freeSize = lastAddr - rangeEnd;
        quint64 distance = fromAddr - rangeEnd;

        if ( freeSize >= pageSize && distance < 0x7fffffff ) {
            trampPageAddress = reinterpret_cast< void* >( lastAddr - pageSize );
            break;
        }

        lastAddr = table.at( i );
    }

    trampAddress = static_cast< uchar* >( trampPageAddress );

    return ( trampPageAddress &&
             mmap( trampPageAddress, getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC,
                   MAP_PRIVATE | MAP_ANON, -1, 0 ) );
}

static const unsigned int jumpSize = 5;

static quint32 getTrampLength( void* funcPtr ) {
    quint32 trampLen = 0;
    char* ptr = static_cast< char* >( funcPtr );

    do {
        quint32 len =
#ifdef __x86_64__
        hde64_length( ptr );
#else
        hde32_length( ptr );
#endif
        ptr += len;
        trampLen += len;
    } while ( trampLen < jumpSize );

    return trampLen;
}

static qint32 offset( void* from, void* to ) {
    return reinterpret_cast< quint64 >( to ) - reinterpret_cast< quint64 >( from ) - jumpSize;
}

template < class T >
static void detour( void* funcPtr, T func, T*& func_jmp ) {
    quint32 len = getTrampLength( funcPtr );
    qint32 addr = offset( trampAddress, funcPtr );
    func_jmp = pointer_cast< T* >( trampAddress );

    memcpy( trampAddress, funcPtr, len );
    trampAddress += len;
    *trampAddress++ = 0xe9;
    *reinterpret_cast< quint32* >( trampAddress ) = addr;
    trampAddress += sizeof( quint32 );

#ifdef __x86_64__
    addr = offset( funcPtr, trampAddress );
    *trampAddress++ = 0x48;
    *trampAddress++ = 0xb8;
    *reinterpret_cast< quint64* >( trampAddress ) = reinterpret_cast< quint64 >( func );
    trampAddress += sizeof( quint64 );
    *trampAddress++ = 0xff;
    *trampAddress++ = 0xe0;
#else
    addr = offset( funcPtr, pointer_cast< void* >( func ) );
#endif

    protectPage( funcPtr, PROT_READ | PROT_WRITE | PROT_EXEC );
    uchar* ptr = static_cast< uchar* >( funcPtr );
    *ptr++ = 0xe9;
    *reinterpret_cast< quint32* >( ptr ) = addr;
    protectPage( funcPtr, PROT_READ | PROT_EXEC );
}

#endif

