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

#ifndef DTGAMELAUNCHER_H
#define DTGAMELAUNCHER_H

#include <qwindowdefs.h>
#include <QObject>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class QTimer;
class QFileInfo;
class QProcess;

class DtGameLauncher : public QObject {
    Q_OBJECT
public:

    DtGameLauncher( WId win, int mode, bool fullscreen, QWidget* parent = 0 );
    ~DtGameLauncher();

    bool qzProcessInitialized;
    bool qaProcessInitialized;
    bool otherAppProcessInitialized;

    bool setDemo( QString demoName );
    void setWindow( WId win );
    void setMode( int mode, bool fullscreen );
    bool playDemo();
    const QString& getCurrentDemo() const;
    void setGameStarted( bool started );

public slots:
    void toggleAutoexecWarning( bool checked );

#ifdef Q_OS_WIN
public:
    static HWND playerWindow;
    static bool sendKey;

private:
    HHOOK qzKeyboardHook;
#endif

private:
    QString currentDemo;
    WId screen;
    int rMode;
    bool gameStarted;
    bool rFullscreen;
    bool mainWindowKeyboardGrab;
    static bool ioWrapped;

    bool initializeQaProcess();
    bool initializeQzProcess();
    bool initializeOtherAppProcess();
    bool execOtherApp();
    void releaseInput();
    void wrapIo();

private slots:
    void runQaDemo();
    void runQzDemo();
    void runOtherAppDemo();

signals:
    void gameRunning();
    void gameStopped();
    void noLoginData( bool );
    void clearLoginData();
};

#endif // DTGAMELAUNCHER_H
