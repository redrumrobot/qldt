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

#include "Data.h"
#include "MainWindow.h"
#include "Demo.h"
#include "PlayerWindow.h"
#include "GameLauncher.h"

#include <QTimer>
#include <QWidget>
#include <QFileInfo>
#include <QProcess>
#include <QMessageBox>
#include <QApplication>
#include <QGridLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QSettings>
#include <QDir>

using namespace dtdata;

bool DtGameLauncher::ioWrapped = false;

DtGameLauncher::DtGameLauncher( WId win, int mode, bool fullscreen, QWidget* parent ) :
//    qzPluginInitialized( false ),
    qaProcessInitialized( false ),
    qzProcessInitialized( false ),
    otherAppProcessInitialized( false ),
    gameStarted( false ),
    mainWindowKeyboardGrab( false )
{
//    releaseInput();
    setParent( parent );
    setMode( mode, fullscreen );
    setWindow( win );
}

DtGameLauncher::~DtGameLauncher() {
}

void DtGameLauncher::setGameStarted( bool started ) {
    gameStarted = started;
}

const QString& DtGameLauncher::getCurrentDemo() const {
    return currentDemo;
}

bool DtGameLauncher::initializeQaProcess() {
    QFileInfo qaPath( config.getQaPath() );

    if ( dtMainWindow->quakeArena ) {
        return true;
    }

    if ( qaPath.exists() && qaPath.isFile() ) {
        dtMainWindow->quakeArena = new QProcess( dtMainWindow );
        dtMainWindow->quakeArena->setWorkingDirectory( qaPath.absolutePath() );

        return true;
    }

    return false;
}

bool DtGameLauncher::initializeQzProcess() {
    QFileInfo qzPath( config.getQzPath() );

    if ( dtMainWindow->quakeLiveStandalone ) {
        return true;
    }

    if ( qzPath.exists() && qzPath.isFile() ) {
        dtMainWindow->quakeLiveStandalone = new QProcess( dtMainWindow );
        dtMainWindow->quakeLiveStandalone->setWorkingDirectory( qzPath.absolutePath() );

        return true;
    }

    return false;
}

bool DtGameLauncher::initializeOtherAppProcess() {
    QFileInfo otherAppPath( config.otherAppPath );

    if ( dtMainWindow->otherApp ) {
        return true;
    }

    if ( !dtMainWindow->otherApp && otherAppPath.exists() && otherAppPath.isFile() ) {
        dtMainWindow->otherApp = new QProcess( dtMainWindow );
        dtMainWindow->otherApp->setWorkingDirectory( otherAppPath.absolutePath() );
        return true;
    }

    return false;
}

void DtGameLauncher::runQaDemo() {
    QString cfgString;
    QString cmdFormat;
    cmdFormat += "+set r_fullscreen %1 ";
    cmdFormat += "+set r_mode %2 ";
    cmdFormat += "+set in_nograb %3 ";
    cmdFormat += "+set timescale 1 ";
    cmdFormat += "+set com_cameramode 1 ";
    cmdFormat += "+set fs_game %4";
    cmdFormat += "%5 ";
    cmdFormat += "+demo %6";

    if ( !config.qaGameConfig.isEmpty() ) {
        cfgString = " +exec " + config.qaGameConfig;
    }

    DtDemo demo( QFileInfo( config.getQaDemoPath() + "/" + currentDemo ) );

    if ( !demo.parseGamestateMsg() ) {
        return;
    }

    QString qaMode = config.qaFullscreen ? QString::number( config.qaFullscreenMode ) :
                                           QString::number( config.qaWindowedMode );
    QStringList args;

    args = cmdFormat.arg( config.qaFullscreen )
                    .arg( qaMode )
                    .arg( !config.qaFullscreen )
                    .arg( demo.getQ3ModName(),
                          cfgString,
                          currentDemo )
                    .split( ' ', QString::SkipEmptyParts );

    dtMainWindow->quakeArena->start( config.getQaPath(), args );
}

void DtGameLauncher::runQzDemo() {
    DtDemo demo( QFileInfo( config.getQzDemoPath() + "/" + currentDemo ) );

    if ( !demo.parseGamestateMsg() ) {
        return;
    }

    QStringList args;

    QString tmpBasePath = config.getQzFSBasePath();
    QString tmpHomePath = config.getQzHomePath();
#ifdef Q_OS_LINUX
    tmpBasePath.replace( QDir::homePath() + "/.wine/drive_c", "C:" );
    tmpHomePath.replace( QDir::homePath() + "/.wine/drive_c", "C:" );
#endif

    args << "+set" << "r_fullscreen" << QString::number( config.qzFullscreen );
    if ( config.qzFullscreenMode != QZ_DONTCHANGE )
        args << "+set" << "r_mode" << QString::number( config.qzFullscreenMode );
    if ( config.qzWindowedMode != QZ_DONTCHANGE )
        args << "+set" << "r_windowedmode" << QString::number( config.qzWindowedMode );
    args << "+set" << "in_nograb" << QString::number( !config.qzFullscreen );
    args << "+set" << "timescale" << "1";
    args << "+set" << "com_cameramode" << "1";
    args << "+set" << "fs_basepath" << tmpBasePath;
    args << "+set" << "fs_homepath" << tmpHomePath;
    args << "+set" << "gt_user" << "aaa";
    args << "+set" << "gt_pass" << "aaa";
    args << "+set" << "gt_realm" << "quakelive";
    args << "+set" << "web_sess" << "aaa";
    args << "+set" << "nextdemo" << "quit";
    if ( !config.qzGameConfig.isEmpty() ) {
        args << "+exec" << config.qzGameConfig;
    }
    args << "+demo" << currentDemo;

#ifdef MSG_LOG
    printf( "Running QL with args: %s\n", qPrintable( args.join( " " ) ) );
#endif

    dtMainWindow->quakeLiveStandalone->start( config.getQzPath(), args );
}

void DtGameLauncher::runOtherAppDemo() {
    QStringList args;
    QString cmdLine = config.otherAppCmdLine;
    QString demoFileName;

    if ( config.otherAppFromDemos ) {
        demoFileName = currentDemo;
    }
    else {
        QString path = ( config.getSelectedGame() == Q_LIVE ) ? config.getQzDemoPath() : config.getQaDemoPath();
        demoFileName = path + "/" + currentDemo;
    }

    cmdLine.replace( "%demoName", demoFileName );

    args = cmdLine.split( ' ', QString::SkipEmptyParts );
    dtMainWindow->otherApp->start( config.otherAppPath, args );
}

void DtGameLauncher::toggleAutoexecWarning( bool checked ) {
    config.settings->setValue( "GameLauncher/showAutoexecWarning", !checked );
}

bool DtGameLauncher::execOtherApp() {
    if ( !otherAppProcessInitialized ) {
        if ( initializeOtherAppProcess() ) {
            otherAppProcessInitialized = true;
        }
        else {
            QMessageBox::critical( dtMainWindow, tr( "Error" ),
                                   tr( "Unable to initialize process" ) + " " +
                                   config.otherAppTitle );
            return false;
        }
    }

    if ( dtMainWindow->otherApp->state() == QProcess::NotRunning ) {
        runOtherAppDemo();
    }
    else {
        dtMainWindow->otherApp->terminate();

        if ( dtMainWindow->otherApp->waitForFinished( 1000 ) ) {
            QTimer::singleShot( 0, this, SLOT( runOtherAppDemo() ) );
        }
    }

    return true;
}

bool DtGameLauncher::playDemo() {
    QString demoExt = QFileInfo( currentDemo ).suffix();
    bool openInOtherApp = openInOtherApplication();

    if ( demoExt == "dm_68" ) {
        if ( openInOtherApp && config.otherAppDm68 ) {
            return execOtherApp();
        }

        if ( !qaProcessInitialized ) {
            QMessageBox::critical( dtMainWindow, tr( "Error" ),
                                   tr( "Process not initialized, unable to play demo" ) );
            return false;
        }

        if ( dtMainWindow->quakeArena->state() == QProcess::NotRunning ) {
            runQaDemo();
        }
        else {
            dtMainWindow->quakeArena->terminate();

            if ( dtMainWindow->quakeArena->waitForFinished( 1000 ) ) {
                QTimer::singleShot( 0, this, SLOT( runQaDemo() ) );
            }
        }

        return true;
    }
    else if ( demoExt == "dm_73" ) {
//        if ( openInOtherApp && config.otherAppDm73 ) {
            return execOtherApp();
//        }
    }
    else if ( demoExt == "dm_90" ) {
        if ( openInOtherApp && config.otherAppDm73 ) {
            return execOtherApp();
        }

        if ( !qzProcessInitialized ) {
            QMessageBox::critical( dtMainWindow, tr( "Error" ),
                                   tr( "Process not initialized, unable to play demo" ) );
            return false;
        }

        if ( dtMainWindow->quakeLiveStandalone->state() == QProcess::NotRunning ) {
            runQzDemo();
        }
        else {
            dtMainWindow->quakeLiveStandalone->terminate();

            if ( dtMainWindow->quakeLiveStandalone->waitForFinished( 1000 ) ) {
                QTimer::singleShot( 0, this, SLOT( runQzDemo() ) );
            }
        }

        return true;
    }

    return false;
}

bool DtGameLauncher::setDemo( QString demoName ) {
    currentDemo = demoName;
    QString ext = QFileInfo( currentDemo ).suffix();

    if ( ext == "dm_68" ) {
        if ( !qaProcessInitialized ) {
            if ( initializeQaProcess() ) {
                qaProcessInitialized = true;
            }
            else {
                QMessageBox::critical( dtMainWindow, tr( "Error" ),
                                       tr( "Unable to initialize Quake Arena process" ) );
                return false;
            }
        }
    }
    else if ( ext == "dm_73" || ext == "dm_90" ) {
        if ( !qzProcessInitialized ) {
            if ( initializeQzProcess() ) {
                qzProcessInitialized = true;
            }
            else {
                QMessageBox::critical( dtMainWindow, tr( "Error" ),
                                       tr( "Unable to initialize Quake Live process" ) );
                return false;
            }
        }
    }
    else {
        qDebug( "Unknown format" );
        return false;
    }

    return true;
}

void DtGameLauncher::setWindow( WId win ) {
    screen = win;
}

void DtGameLauncher::setMode( int mode, bool fullscreen ) {
    rMode = mode;
    rFullscreen = fullscreen;
}

