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

#include "GameLauncher.h"
#include "Data.h"
#include "QzPluginLoader.h"
#include "MainWindow.h"
#include "Demo.h"
#include "PlayerWindow.h"

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

#ifdef Q_OS_LINUX
#include "GameLauncherX11.h"
#elif defined Q_OS_WIN
#include "GameLauncherWin32.h"
#endif

bool DtGameLauncher::ioWrapped = false;

DtGameLauncher::DtGameLauncher( WId win, int mode, bool fullscreen, QWidget* parent ) :
    qzLoader( 0 ),
    qzPluginInitialized( false ),
    qaProcessInitialized( false ),
    otherAppProcessInitialized( false ),
    gameStarted( false ),
    mainWindowKeyboardGrab( false )
{
#ifdef Q_OS_WIN
    qzKeyboardHook = 0;
    playerWindow = parent->winId();
#endif
    releaseInput();
    setParent( parent );
    setMode( mode, fullscreen );
    setWindow( win );
}

DtGameLauncher::~DtGameLauncher() {
    onDestroy();
}

void DtGameLauncher::setGameStarted( bool started ) {
    gameStarted = started;
}

const QString& DtGameLauncher::getCurrentDemo() const {
    return currentDemo;
}

void DtGameLauncher::checkPlugin() {
    if ( !qzPluginInitialized ) {
        return;
    }

    if ( qzLoader->isGameRunning() ) {
        if ( !gameStarted ) {
            gameStarted = true;
            emit gameRunning();

            fullscreenGameRunning = rFullscreen;
#ifdef Q_OS_LINUX
            if ( config.qzKeyboardFilter ) {
                QWidget* player = qobject_cast< QWidget* >( parent() );

                if ( player->isVisible() ) {
                    player->grabKeyboard();
                    mainWindowKeyboardGrab = false;
                }
                else {
                    dtMainWindow->installEventFilter( parent() );
                    dtMainWindow->grabKeyboard();
                    mainWindowKeyboardGrab = true;
                }
            }
#endif
        }
    }
    else if ( gameStarted ) {
        gameStarted = false;
        emit gameStopped();

        fullscreenGameRunning = false;

#ifdef Q_OS_LINUX
        if ( config.qzKeyboardFilter ) {
            QWidget* player = qobject_cast< QWidget* >( parent() );

            if ( mainWindowKeyboardGrab ) {
                dtMainWindow->releaseKeyboard();
                dtMainWindow->removeEventFilter( parent() );
            }
            else {
                player->releaseKeyboard();
            }
        }
#endif
    }

#ifdef Q_OS_WIN
    if ( !rFullscreen ) {
        char windowClassName[ 255 ];
        GetClassName( GetCapture(), reinterpret_cast< LPWSTR >( windowClassName ), 255 );

        if ( strncmp( windowClassName, "QuakeLive", 9 ) ) {
            ClipCursor( 0 );
            ReleaseCapture();

            while ( ShowCursor( true ) < 0 ) {

            }
        }
    }
#endif
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

bool DtGameLauncher::initializeQzPlugin() {
    qzLoader = new DtQzPluginLoader( screen, this );
    connect( qzLoader, SIGNAL( httpEvent( const QString& ) ),
             parent(), SLOT( qzConnectEvent( const QString& ) ) );
    connect( qzLoader, SIGNAL( gameExitEvent() ), parent(), SLOT( qzExitEvent() ) );
    connect( qzLoader, SIGNAL( gameDisconnectEvent() ), parent(), SLOT( qzDisconnectEvent() ) );
    connect( qzLoader, SIGNAL( gameServerInfoEvent() ), parent(), SLOT( qzServerInfoEvent() ) );
    connect( this, SIGNAL( gameStopped() ), qzLoader, SLOT( onGameExit() ) );
    connect( qzLoader, SIGNAL( errorLogin( bool ) ), parent(), SLOT( showQzLoginDialog( bool ) ) );
    connect( qzLoader, SIGNAL( kicked() ), parent(), SLOT( cmdStopPlay() ) );
    connect( this, SIGNAL( noLoginData( bool ) ), parent(), SLOT( showQzLoginDialog( bool ) ) );
    connect( this, SIGNAL( clearLoginData() ), qzLoader, SLOT( clearSession() ), Qt::DirectConnection );

    pluginLoadTimer = new QTimer( this );
    connect( pluginLoadTimer, SIGNAL( timeout() ), this, SLOT( checkPlugin() ) );
    pluginLoadTimer->start( 100 );

    if ( config.qzPreventSettingsCaching ) {
        wrapIo();
    }

    return qzLoader->initializePlugin();
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
        if ( openInOtherApp && config.otherAppDm73 ) {
            return execOtherApp();
        }

        if ( !qzPluginInitialized ) {
            QMessageBox::critical( dtMainWindow, tr( "Error" ),
                                   tr( "Plugin not initialized, unable to play demo" ) );
            return false;
        }

        bool showWarning = config.settings->value( "GameLauncher/showAutoexecWarning", true ).toBool();

        if ( !ioWrapped && showWarning && !QFile::exists( config.getQzBasePath() + "/autoexec.cfg" ) ) {
            DtPlayerWindow* player = qobject_cast< DtPlayerWindow* >( parent() );
            QMessageBox msg( QMessageBox::Warning, tr( "Warning" ),
                             tr( "<b>autoexec.cfg</b> not found in baseq3 directory" ),
                             QMessageBox::NoButton, player );

            msg.setDetailedText( tr( "Continue - play demo without creating"
                                     " autoexec.cfg\nCreate - create autoexec.cfg"
                                     " before demo starts\nCancel - stop demo playback\n\n"
                                     "The game can cache its settings changes after the demo playback."
                                     " To always play with the same settings after demo watching,"
                                     " create autoexec.cfg file in baseq3 directory with "
                                     "\"exec yourconfig.cfg\" command inside or with your settings."
                                     " Now you can create one automatically, by making the snapshot"
                                     " of your current settings.\n\n"
                                     "The game settings affected by QLDT:\n\n"
                                     "r_fullscreen, r_mode, com_cameramode, r_inBrowserMode,"
                                     " cl_quitOnDemoCompleted, timescale, in_nograb, s_volume,"
                                     " com_allowConsole, cg_draw2D.\n\n"
                                     "You must have in_nograb 0 in your game config to be able"
                                     " to play fullscreen." ) );

            msg.addButton( tr( "Continue" ), QMessageBox::ActionRole );
            QPushButton* createButton = msg.addButton( tr( "Create" ),
                                                       QMessageBox::ActionRole );
            QPushButton* cancelButton = msg.addButton( QMessageBox::Cancel );
            QGridLayout* msgGrid = static_cast< QGridLayout* >( msg.layout() );
            QHBoxLayout* defaultLayout = new QHBoxLayout;
            QCheckBox* defaultCb = new QCheckBox;
            connect( defaultCb, SIGNAL( toggled( bool ) ),
                     this, SLOT( toggleAutoexecWarning( bool ) ) );
            QLabel* defaultLbl = new QLabel( tr( "Don't show that message again" ) );
            defaultLayout->addWidget( defaultCb );
            defaultLayout->addWidget( defaultLbl );
            msgGrid->addLayout( defaultLayout, 4, 0, 1, 3, Qt::AlignLeft );

            msg.exec();

            if ( msg.clickedButton() == cancelButton ) {
                player->hide();
                player->setShowQzWarining( false );
                return false;
            }
            else if ( msg.clickedButton() == createButton ) {
                qzLoader->setLaunchCommand( "+writeconfig autoexec" );
            }
        }

#ifdef Q_OS_LINUX
        return qzLoader->launchDemo( currentDemo );
#elif defined Q_OS_WIN

        if ( qzLoader->launchDemo( currentDemo ) ) {
            if ( config.qzKeyboardFilter ) {
                QTimer::singleShot( 30, this, SLOT( setInputHooks() ) );
            }

            return true;
        }
        else {
            return false;
        }
#endif
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

    return true;
}

void DtGameLauncher::setQzLoginData( const QString& email, const QString& pass ) {
    if ( qzLoader ) {
        qzLoader->setLogin( email, pass );
    }
}

void DtGameLauncher::setWindow( WId win ) {
    screen = win;
}

void DtGameLauncher::setMode( int mode, bool fullscreen ) {
    rMode = mode;
    rFullscreen = fullscreen;
}

