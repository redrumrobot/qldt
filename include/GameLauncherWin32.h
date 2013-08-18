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

#ifndef DTGAMELAUNCHERWIN32_H
#define DTGAMELAUNCHERWIN32_H

#include "GameLauncher.h"
#include "GameLauncherCommon.h"
#include "Data.h"
#include "detours.h"

HWND DtGameLauncher::playerWindow;
bool DtGameLauncher::sendKey;

void DtGameLauncher::onDestroy() {
    removeInputHooks();
}

LRESULT CALLBACK qzKeyboardProc( int nCode, WPARAM wParam, LPARAM lParam ) {
    if ( nCode < 0 ) {
        return CallNextHookEx( 0, nCode, wParam, lParam );
    }

    if ( nCode == HC_NOREMOVE ) {
        SendMessage( DtGameLauncher::playerWindow, ( lParam >> 31 ) ? WM_KEYUP : WM_KEYDOWN,
                     wParam, lParam );
    }

    if ( DtGameLauncher::sendKey ) {
        return CallNextHookEx( 0, nCode, wParam, lParam );
    }

    return true;
}

void DtGameLauncher::removeInputHooks() {
    if ( qzKeyboardHook ) {
        UnhookWindowsHookEx( qzKeyboardHook );
    }
}

void DtGameLauncher::setInputHooks() {
    removeInputHooks();

    HWND windowHandle = GetWindow( screen, GW_CHILD );

    if ( windowHandle ) {
        DWORD threadId = GetWindowThreadProcessId( windowHandle, 0 );

        qzKeyboardHook = SetWindowsHookEx( WH_KEYBOARD, qzKeyboardProc, 0, threadId );
    }
}

void DtGameLauncher::releaseInput() {
    char* SetCursorPosPtr = reinterpret_cast< char* >( GetProcAddress( GetModuleHandle( L"user32.dll" ),
                                                                       "SetCursorPos" ) );
    DWORD oldProtect = PAGE_EXECUTE_READWRITE;

    VirtualProtect( SetCursorPosPtr, 3, oldProtect, &oldProtect );
    memcpy( SetCursorPosPtr, "\xC2\x8\0", 3 );
    VirtualProtect( SetCursorPosPtr, 3, oldProtect, 0 );
}

void DtGameLauncher::wrapIo() {
    /* Try to detour the msvcr80 IO functions to prevent
       changing some cvars in qzconfig.cfg and repconfig.cfg.
    */

    if ( ioWrapped ) {
        return;
    }

    HMODULE crt = LoadLibrary( L"msvcr80" );
    void* fopenPtr = GetProcAddress( crt, "fopen" );
    void* fclosePtr = GetProcAddress( crt, "fclose" );
    void* fwritePtr = GetProcAddress( crt, "fwrite" );

    if ( !fopenPtr || !fclosePtr || !fwritePtr ) {
        qDebug( "GetProcAddress error" );
        return;
    }

    DetourTransactionBegin();
    DetourUpdateThread( GetCurrentThread() );

    PDETOUR_TRAMPOLINE tramp;

#define DETOUR( sym )                                                                                \
    if ( DetourAttachEx( &static_cast< PVOID& >( sym##Ptr ), sym##_d, &tramp, 0, 0 ) != NO_ERROR ) { \
        qDebug( "DetourAttachEx error" );                                                            \
        return;                                                                                      \
    }                                                                                                \
                                                                                                     \
    sym##_jmp = reinterpret_cast< sym##_t* >( tramp )

    DETOUR( fopen );
    DETOUR( fclose );
    DETOUR( fwrite );

    if ( DetourTransactionCommit() != NO_ERROR ) {
        qDebug( "DetourTransactionCommit error" );
        return;
    }

    ioWrapped = true;
}

#endif

