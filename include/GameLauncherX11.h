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

#ifndef DTGAMELAUNCHERX11_H
#define DTGAMELAUNCHERX11_H

#include "GameLauncher.h"
#include "GameLauncherCommon.h"
#include "Detour.h"

#include <X11/X.h>
#include <X11/Xlib.h>

void DtGameLauncher::onDestroy() {
}

void DtGameLauncher::releaseInput() {
    uchar* XWarpPointerPtr = reinterpret_cast< uchar* >( dlsym( RTLD_NEXT, "XWarpPointer" ) );

    protectPage( XWarpPointerPtr, PROT_READ | PROT_WRITE | PROT_EXEC );
    *XWarpPointerPtr = 0xc3;
    protectPage( XWarpPointerPtr, PROT_READ | PROT_EXEC );
}

void DtGameLauncher::wrapIo() {
    /* Try to detour the libc IO functions to prevent
       changing some cvars in qzconfig.cfg and repconfig.cfg.

       This is done by code injection since QL plugin uses
       RTLD_DEEPBIND for shadowing libpng symbols.
    */

    if ( ioWrapped ) {
        return;
    }

    const char* libName = "/usr/lib/libc.so";
    void* dlHandle = openLibrary( libName );

    if ( !dlHandle ) {
        qDebug() << "Error: couldn't open" << libName;
        // Fedora's lib64 as fallback
        const char* libName = "/usr/lib64/libc.so";
        dlHandle = openLibrary( libName );
        if ( !dlHandle ) {
            qDebug() << "Error: couldn't open" << libName;
            return;
        }
    }

    if ( !allocateTrampMemory( dlHandle ) ) {
        qDebug() << "Error: couldn't allocate memory";
        return;
    }

#define DETOUR( sym ) \
    detour< sym##_t >( dlsym( dlHandle, #sym ), sym##_d, sym##_jmp );

    DETOUR( fopen );
    DETOUR( fclose );
    DETOUR( fwrite );

    ioWrapped = true;
}

#endif

