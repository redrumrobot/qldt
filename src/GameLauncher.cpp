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

using namespace dtdata;

bool DtGameLauncher::ioWrapped = false;

DtGameLauncher::DtGameLauncher( WId win, int mode, bool fullscreen, QWidget* parent ) :
//    qzPluginInitialized( false ),
    qaProcessInitialized( false ),
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

    if ( cmdLine.startsWith( "%demoName" ) ) {
        cmdLine.replace( "%demoName", demoFileName );
    }
    else {
        cmdLine.replace( " %demoName", " " + demoFileName );
    }

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
        return execOtherApp();
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
/*
    else if ( ext == "dm_73" ) {
        if ( !qzPluginInitialized ) {
            if ( initializeQzPlugin() ) {
                qzPluginInitialized = true;
            }
            else {
                QMessageBox::critical( dtMainWindow, tr( "Error" ),
                                       tr( "Unable to load Quake Live plugin" ) );
                return false;
            }
        }

        if ( !qzLoader->isLoggedIn() && !qzLoader->haveLoginData() ) {
            if ( config.getQzEmail().isEmpty() || config.getQzPass().isEmpty() ) {
                emit noLoginData( true );
                return false;
            }
            else {
                setQzLoginData( config.getQzEmail(), config.getQzPass() );
            }
        }
    }
    else {
        qDebug( "Unknown format" );
        return false;
    }
*/
    return true;
}

void DtGameLauncher::setWindow( WId win ) {
    screen = win;
}

void DtGameLauncher::setMode( int mode, bool fullscreen ) {
    rMode = mode;
    rFullscreen = fullscreen;
}

