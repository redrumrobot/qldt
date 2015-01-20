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

#include "PlayerWindow.h"
#include "Data.h"
#include "DemoTable.h"
#include "GameLauncher.h"
#include "ClearLineEdit.h"

#include <QEvent>
#include <QKeyEvent>
#include <QTimer>
#include <QPainter>
#include <QPointer>
#include <QSettings>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QCheckBox>
#include <QEventLoop>
#include <QLabel>
#include <QLineEdit>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QTime>

#ifdef Q_OS_LINUX
#include <QX11Info>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#undef KeyPress
#undef KeyRelease
#define KeyPressX11 2
#define KeyReleaseX11 3
#elif defined Q_OS_WIN
#include <windows.h>
#endif

const int MAX_CONSOLE_COMMAND_SIZE = 10;

using namespace dtdata;

DtPlayerWindow::DtPlayerWindow( QWidget* parent ) : QDialog( parent ) {
    fullscreen = config.qzFullscreen;
    modeNum = config.qzWindowedMode;

    defaultWindowTitle = "Player";
    setWindowTitle( defaultWindowTitle );
    updateStyle();

    QSize rMode = qzModes.at( modeNum >= 0 ? modeNum : 0 );
    QWidget* cWidget = this;

/* OLD    if ( !config.controlPanelAlwaysVisible ) {
        cWidget = new QWidget( this );
        cWidget->setFixedSize( rMode.width(), rMode.height() );
        cWidget->setAttribute( Qt::WA_TransparentForMouseEvents );

        panelTimer = new QTimer( this );
        panelTimer->setInterval( 50 );
        connect( panelTimer, SIGNAL( timeout() ), this, SLOT( panelTimerEvent() ) );
        panelTimer->start();

        panelVisibleTimer = new QTimer( this );
        connect( panelVisibleTimer, SIGNAL( timeout() ), this, SLOT( closePanel() ) );
        panelVisibleTimer->setInterval( 4000 );
    } */

    screenArea = new DtPlayerScreenWidget( rMode, cWidget );
    gameLauncher = 0;

    int layoutMargin = 0;
    QVBoxLayout* mLayout = new QVBoxLayout();
    mLayout->setSpacing( 0 );
    mLayout->setMargin( layoutMargin );
    mLayout->addWidget( screenArea );

    controlWidget = new QWidget( cWidget );
    controlWidget->setObjectName( "controlWidget" );
    controlWidget->setFixedSize( rMode.width(), 34 );
// OLD    controlWidget->setVisible( config.controlPanelAlwaysVisible );

    QHBoxLayout* cLayout = new QHBoxLayout;
    cLayout->setMargin( 3 );

    #define pButton( buttonName, bSlot, tip )                               \
        buttonName = new DtPlayerButton( this );                            \
        buttonName->setFocusPolicy( Qt::NoFocus );                          \
        buttonName->setObjectName( #buttonName );                           \
        cLayout->addWidget( buttonName );                                   \
        buttonName->setToolTip( tip );                                      \
        buttonName->setMouseTracking( true );                               \
        connect( buttonName, SIGNAL( clicked() ), this, SLOT( bSlot() ) );

    pButton( prevButton, cmdPrevDemo, tr( "Previous" ) );
    pButton( stopButton, cmdStopPlay, tr( "Stop" ) );
    pButton( playButton, cmdTogglePlay, tr( "Play" ) );

    slowPressed = false;
    pausePressed = false;
    pButton( pauseButton, cmdPause, tr( "Pause" ) );
    pauseButton->setCheckable( true );

    fastButton = new DtPlayerButton( this );
    fastButton->setToolTip( tr( "Fast" ) );
    fastButton->setObjectName( "fastButton" );
    fastButton->setMouseTracking( true );
    fastButton->setFocusPolicy( Qt::NoFocus );
    cLayout->addWidget( fastButton );
    connect( fastButton, SIGNAL( pressed() ), this, SLOT( cmdFast() ) );
    connect( fastButton, SIGNAL( released() ), this, SLOT( cmdNormalSpeed() ) );

    veryFastButton = new DtPlayerButton( this );
    veryFastButton->setToolTip( tr( "Maximum Speed" ) );
    veryFastButton->setObjectName( "veryFastButton" );
    veryFastButton->setMouseTracking( true );
    veryFastButton->setFocusPolicy( Qt::NoFocus );
    cLayout->addWidget( veryFastButton );
    connect( veryFastButton, SIGNAL( pressed() ), this, SLOT( cmdVeryFast() ) );
    connect( veryFastButton, SIGNAL( released() ), this, SLOT( cmdNormalSpeed() ) );

    pButton( nextButton, cmdNextDemo, tr( "Next" ) );
    cLayout->addSpacing( 20 );

    soundVolumeSlider = new QSlider( Qt::Horizontal, this );
    soundVolumeSlider->setMouseTracking( true );
    soundVolumeSlider->setObjectName( "soundVolumeSlider" );
    soundVolumeSlider->setToolTip( tr( "Sound Volume" ) );
    soundVolumeSlider->setFocusPolicy( Qt::NoFocus );
    connect( soundVolumeSlider, SIGNAL( valueChanged( int ) ),
             this, SLOT( cmdVolumeChanged( int ) ) );
    soundVolume = 0; // OLD config.qzSoundVolume;
    soundVolumeSlider->setValue( soundVolume );
    soundVolumeSlider->setMinimum( 0 );
    soundVolumeSlider->setMaximum( 100 );
    cLayout->addWidget( soundVolumeSlider );

    pButton( muteButton, cmdMute, tr( "Mute Sound" ) );
    muted = false; // OLD config.qzSoundMute;
    muteButton->setCheckable( true );
    muteButton->setChecked( muted );

    cLayout->addStretch( 1 );
    pButton( screenshotButton, cmdScreenshot, tr( "Screenshot" ) );
    pButton( fullscreenButton, cmdToggleFullscreen, tr( "Full Screen mode" ) );

    controlWidget->setLayout( cLayout );

    mLayout->addWidget( controlWidget );

    setLayout( mLayout );
    setFocus();

    int nWidth = rMode.width() + layoutMargin * 2;
    int nHeight = rMode.height() + layoutMargin * 2;

/* OLD    if ( config.controlPanelAlwaysVisible ) {
        nHeight += controlWidget->height();
    } */

    setFixedSize( nWidth, nHeight );

    gameLauncher = new DtGameLauncher( screenArea->winId(), modeNum, fullscreen, this );
    connect( gameLauncher, SIGNAL( gameRunning() ), screenArea, SLOT( clearMsg() ) );
    connect( this, SIGNAL( clearLoginData() ), gameLauncher,
             SIGNAL( clearLoginData() ), Qt::DirectConnection );

    connect( this, SIGNAL( sPlayDemo( tableDemoPos ) ), parent, SLOT( playDemo( tableDemoPos ) ) );

    autoClosed = false;
}

DtPlayerWindow::~DtPlayerWindow() {
    delete gameLauncher;
}

void DtPlayerWindow::updateStyle() {
// OLD    setStyleSheet( getStyle( config.controlPanelStyle + "_player" ) );
}

void DtPlayerWindow::closePanel() {
    QPoint pos = mapFromGlobal( QCursor::pos() );
    QRect panelRect( 0, height() - controlWidget->height() - 10,
                     controlWidget->width(), controlWidget->height() + 10 );

    if ( !panelRect.contains( pos ) ) {
        controlWidget->setVisible( false );
        controlWidget->update();
        panelVisibleTimer->stop();
    }
}

void DtPlayerWindow::panelTimerEvent() {
    if ( isHidden() ) {
        return;
    }

    if ( QCursor::pos() != lastPos ) {
        lastPos = QCursor::pos();
        mouseMove();
    }
}

void DtPlayerWindow::mouseMove() {
    QPoint pos = mapFromGlobal( QCursor::pos() );

    if ( rect().contains( pos ) && !controlWidget->isVisible() ) {
        controlWidget->setVisible( true );
        controlWidget->update();
        panelVisibleTimer->start();
    }
    else if ( !rect().contains( pos ) && controlWidget->isVisible() ) {
        closePanel();
    }
}

void DtPlayerWindow::cmdVolumeChanged( int volume ) {
    soundVolume = volume;
// OLD    config.qzSoundVolume = volume;
    gCmd( QString( "s_volume %1;" ).arg( soundVolume / 50.f ).toAscii().data() );
}

void DtPlayerWindow::cmdMute() {
    if ( !muted ) {
        gCmd( "s_volume 0;" );
    }
    else {
        cmdVolumeChanged( soundVolume );
    }

    muted = !muted;
// OLD    config.qzSoundMute = muted;
}


void DtPlayerWindow::playDemo( const QString& demoFileName, const QString& windowTitle ) {
    QString demoName = demoFileName;

    if ( demoName.isEmpty() ) {
        return;
    }

    QString dir = ( config.getSelectedGame() == Q_LIVE )
                  ? config.getQzDemoPath() : config.getQaDemoPath();

    if ( demoName.startsWith( dir ) ) {
        demoName.remove( 0, dir.size() + 1 );
    }

    if ( !gameLauncher->setDemo( demoName.toUtf8().data() ) ) {
        return;
    }

//    if ( QFileInfo( demoName ).suffix() == "dm_68" || openInOtherApplication() ) {
        gameLauncher->playDemo();
        return;
//    }
}

void DtPlayerWindow::cmdPrevDemo() {
    emit sPlayDemo( TD_PREV );
}

void DtPlayerWindow::cmdNextDemo() {
    emit sPlayDemo( TD_NEXT );
}

void DtPlayerWindow::cmdTogglePlay() {
    emit sPlayDemo( TD_CUR );
}

void DtPlayerWindow::cmdStopPlay() {
    cmdNormalSpeed();
    pauseButton->setChecked( false );
    slowPressed = false;
    pausePressed = false;
}

void DtPlayerWindow::cmdSpeed( float timeScale ) {
    QString val = "timescale " + QString::number( timeScale ) + ";";
    gCmd( val.toUtf8().data() );
}

void DtPlayerWindow::cmdSlow() {
/* OLD    if ( !slowPressed ) {
        if ( !config.draw2dOnSlow ) {
            gCmd( "cg_draw2d 0;" );
        }

        cmdSpeed( config.slowTimescale );
    }
    else {
        if ( !config.draw2dOnSlow ) {
            gCmd( "cg_draw2d 1;" );
        }

        cmdNormalSpeed();
    } */

    slowPressed = !slowPressed;
}

void DtPlayerWindow::cmdPause() {
/* OLD    if ( !pausePressed ) {
        if ( !config.draw2dOnPause ) {
            gCmd( "cg_draw2d 0;" );
        }

        if ( !config.qzSoundMute && config.qzPauseMuteSound && !muted ) {
            timescaleSoundMuted = true;
            gCmd( "s_volume 0;" );
        }

        cmdSpeed( 0 );
        pauseButton->setChecked( true );
    }
    else {
        if ( !config.draw2dOnPause ) {
            gCmd( "cg_draw2d 1;" );
        }

        cmdNormalSpeed();
        pauseButton->setChecked( false );
    } */

    pausePressed = !pausePressed;
}

void DtPlayerWindow::cmdFast() {
/* OLD    if ( !config.qzSoundMute && config.qzForwardMuteSound && !muted ) {
        timescaleSoundMuted = true;
        gCmd( "s_volume 0;" );
    } */

    slowPressed = false;
    pauseButton->setChecked( false );
    pausePressed = false;
// OLD    cmdSpeed( config.fastTimescale );
}

void DtPlayerWindow::cmdVeryFast() {
/* OLD    if ( !config.qzSoundMute && config.qzForwardMuteSound && !muted ) {
        timescaleSoundMuted = true;
        gCmd( "s_volume 0;" );
    } */

    slowPressed = false;
    pauseButton->setChecked( false );
    pausePressed = false;
// OLD    cmdSpeed( config.fastestTimescale );
}

void DtPlayerWindow::cmdNormalSpeed() {
    if ( timescaleSoundMuted ) {
        timescaleSoundMuted = false;
        cmdVolumeChanged( soundVolume );
    }

    cmdSpeed( 1 );
}

void DtPlayerWindow::gCmd( const char* cmd ) {
    if ( !gameLauncher ) {
        return;
    }

/*    if ( !gameLauncher->qzPluginInitialized ) {
        return;
    }

    gameLauncher->qzLoader->sendGameCommand( cmd ); */
}

void DtPlayerWindow::cmdScreenshot() {
    gCmd( "screenshot;" );
}

void DtPlayerWindow::cmdToggleFullscreen() {
}

void DtPlayerWindow::qzConnectEvent( const QString& msg ) {
    screenArea->connectionMsg = msg;
    screenArea->update();
}

void DtPlayerWindow::qzExitEvent() {
    screenArea->clearMsg();

    cmdNormalSpeed();
    pauseButton->setChecked( false );
    slowPressed = false;
    pausePressed = false;

    if ( !currentPlayDemoTable ) {
        close();
        return;
    }

/* OLD    if ( autoClosed && config.autoPlayNext ) {
//        gameLauncher->checkPlugin();

        if ( config.repeatPlaylist ) {
            cmdNextDemo();
        }
        else {
            emit sPlayDemo( TD_NEXT_NOLOOP );
        }
    }
    else if ( !config.autoPlayNext ) {
        currentPlayDemoTable->clearMark();
        currentPlayDemoTable->stopDemoPlay();
        close();
    } */
}

void DtPlayerWindow::qzDisconnectEvent() {
    if ( goingToDisconnect ) {
        goingToDisconnect = false;
        return;
    }

    cmdStopPlay();
    close();
}

void DtPlayerWindow::qzServerInfoEvent() {
    if ( fullscreen && isVisible() ) {
        hide();
        gameLauncher->setGameStarted( false );
//        gameLauncher->checkPlugin();
    }
}

void DtPlayerWindow::exit() {
}

void DtPlayerWindow::closeEvent( QCloseEvent* ) {
    if ( currentPlayDemoTable ) {
        currentPlayDemoTable->clearMark();
        currentPlayDemoTable->stopDemoPlay();
    }

    hide();
}

void DtPlayerWindow::setVolume( int volume ) {
    soundVolumeSlider->setValue( volume );

    if ( volume > 0 && muted ) {
        muteButton->click();
    }
}

bool DtPlayerWindow::eventFilter( QObject*, QEvent* e ) {
    if ( e->type() == QEvent::KeyPress          ||
         e->type() == QEvent::KeyRelease        ||
         e->type() == QEvent::ShortcutOverride )
    {
        event( e );
        return true;
    }

    return false;
}

#ifdef Q_OS_LINUX
void sendX11KeyEvent( Window& win, Window& winRoot, bool press,
                      int keyCode, int modifiers, Time tm )
{
    XKeyEvent event;
    event.display     = QX11Info::display();
    event.window      = win;
    event.root        = winRoot;
    event.subwindow   = None;
    event.time        = tm;
    event.same_screen = True;
    event.state       = modifiers;
    event.keycode     = keyCode;
    event.type        = press ? KeyPressX11 : KeyReleaseX11;
    event.x = event.y = event.x_root = event.y_root = 1;

    XSendEvent( QX11Info::display(), win, True, KeyPressMask,
                reinterpret_cast< XEvent* >( &event ) );
}

void DtPlayerWindow::sendKeyEvent( QKeyEvent* e ) {
    Window winRoot = XDefaultRootWindow( QX11Info::display() );
    Window rootRet;
    Window parentRet;
    Window* child;
    unsigned int childCount;

    Window qzWindow = 0;
    Window from = fullscreen ? winRoot : screenArea->effectiveWinId();

    if ( XQueryTree( QX11Info::display(), from, &rootRet, &parentRet, &child, &childCount ) ) {
        for ( unsigned i = 0; i < childCount; ++i ) {
            char* windowName = 0;

            if( XFetchName( QX11Info::display(), child[ i ], &windowName ) && windowName ) {
                if( !strcmp( windowName, "QuakeLive" ) ) {
                    qzWindow = child[ i ];
                }

                XFree( windowName );
            }
        }
    }

    if ( qzWindow ) {
        bool keyDown = ( e->type() == QEvent::KeyPress );
        Time eventTime = QX11Info::appUserTime();

        if ( keyDown && e->isAutoRepeat() ) {
            sendX11KeyEvent( qzWindow, winRoot, false, e->nativeScanCode(),
                             e->nativeModifiers(), eventTime );
        }

        sendX11KeyEvent( qzWindow, winRoot, keyDown, e->nativeScanCode(),
                         e->nativeModifiers(), eventTime );
    }
}
#endif

void DtPlayerWindow::keyPressEvent( QKeyEvent* e ) {
#ifdef Q_OS_LINUX
    const quint32 leftQuoteCode = 49;
#elif defined Q_OS_WIN
    const quint32 leftQuoteCode = 41;
#endif

    if ( e->nativeScanCode() == leftQuoteCode || e->key() == Qt::Key_QuoteLeft ) {
        if ( !e->isAutoRepeat() ) {
            gCmd( "in_nograb 1;toggleconsole;" );
            consoleOpened = !consoleOpened;
            consoleCmdString.clear();
        }

#ifdef Q_OS_WIN
//        gameLauncher->sendKey = false;
#endif
        return;
    }

    switch ( e->key() ) {
        case Qt::Key_Escape :
            if ( !e->isAutoRepeat() ) {
                if ( !chatVisible ) {
                    cmdStopPlay();
                    close();
                }
                else {
                    gCmd( "-chat;" );
                    chatVisible = false;
                }
            }
            break;

        default :
            if ( !e->isAutoRepeat() ) {
                if ( e->key() == Qt::Key_Return && e->modifiers() & Qt::AltModifier ) {
                    cmdToggleFullscreen();
                    break;
                }
            }

            bool sendKey = false;

            if ( consoleOpened ) {
                sendKey = true;

                if ( !e->isAutoRepeat() &&
                     e->key() == Qt::Key_Return &&
                     consoleCmdString.size() )
                {
                    if ( consoleCmdString.trimmed() == "quit" ) {
                        close();
                        return;
                    }

                    consoleCmdString.clear();
                }
                else if ( e->key() == Qt::Key_Backspace && consoleCmdString.size() ) {
                    consoleCmdString = consoleCmdString.left( consoleCmdString.size() - 1 );
                }
                else if ( consoleCmdString.size() < MAX_CONSOLE_COMMAND_SIZE ) {
                    consoleCmdString += e->text().toLower();
                }
            }
/* OLD            else {
                if ( config.playerKeys.contains( e->key() ) ) {
                    if ( !e->isAutoRepeat() ) {
                        keyAction( config.playerKeys.value( e->key(), -1 ) );
                    }
                }
                else if ( config.playerAlternateKeys.contains( e->key() ) ) {
                    if ( !e->isAutoRepeat() ) {
                        keyAction( config.playerAlternateKeys.value( e->key(), -1 ) );
                    }
                }
                else {
                    sendKey = true;
                }
            } */
#ifdef Q_OS_LINUX
/* OLD            if ( sendKey && config.qzKeyboardFilter ) {
                sendKeyEvent( e );
            } */
#elif defined Q_OS_WIN
            if ( e->modifiers()
                && ( e->key() == Qt::Key_Shift  ||
                     e->key() == Qt::Key_Alt    ||
                     e->key() == Qt::Key_Control ) )
            {
                sendKey = true;
            }

//            gameLauncher->sendKey = sendKey;
            return;
#endif
            break;
    }

#ifdef Q_OS_WIN
//    gameLauncher->sendKey = false;
#endif
}

void DtPlayerWindow::keyReleaseEvent( QKeyEvent* e ) {
/* OLD    if ( config.playerKeys.contains( e->key() ) ) {
        if ( !e->isAutoRepeat() ) {
            stopAction( config.playerKeys.value( e->key(), -1 ) );
        }
    }
    else if ( config.playerAlternateKeys.contains( e->key() ) ) {
        if ( !e->isAutoRepeat() ) {
            stopAction( config.playerAlternateKeys.value( e->key(), -1 ) );
        }
    }
    else {
#ifdef Q_OS_LINUX
        if ( config.qzKeyboardFilter ) {
            if ( !e->isAutoRepeat() ) {
                sendKeyEvent( e );
            }
        }
#endif
    } */
}

void DtPlayerWindow::keyAction( int action ) {
    if ( action < 0 ) {
        return;
    }
/* OLD
    if ( action <= 100 ) {
        switch ( action ) {
            case DtConfig::AC_PAUSE :     cmdPause();     break;
            case DtConfig::AC_SLOW :      cmdSlow();      break;
            case DtConfig::AC_FAST :      cmdFast();      break;
            case DtConfig::AC_VERYFAST :  cmdVeryFast();  break;
            case DtConfig::AC_NEXT :      cmdNextDemo();  break;
            case DtConfig::AC_PREV :      cmdPrevDemo();  break;
            case DtConfig::AC_SOUNDUP : {
                    int vol = soundVolume;

                    if ( vol < 100 ) {
                        vol += config.qzSoundVolumeStep;
                    }

                    if ( vol > 100 ) {
                        vol = 100;
                    }

                    setVolume( vol );
                }
                break;

            case DtConfig::AC_SOUNDDOWN: {
                    int vol = soundVolume;

                    if ( vol > 0 ) {
                        vol -= config.qzSoundVolumeStep;
                    }

                    if ( vol < 0 ) {
                        vol = 0;
                    }

                    setVolume( vol );
                }
                break;

            case DtConfig::AC_SOUND10 :       setVolume( 10 );      break;
            case DtConfig::AC_SOUND20 :       setVolume( 20 );      break;
            case DtConfig::AC_SOUND30 :       setVolume( 30 );      break;
            case DtConfig::AC_SOUND40 :       setVolume( 40 );      break;
            case DtConfig::AC_SOUND50 :       setVolume( 50 );      break;
            case DtConfig::AC_SOUND60 :       setVolume( 60 );      break;
            case DtConfig::AC_SOUND70 :       setVolume( 70 );      break;
            case DtConfig::AC_SOUND80 :       setVolume( 80 );      break;
            case DtConfig::AC_SOUND90 :       setVolume( 90 );      break;
            case DtConfig::AC_SOUND100 :      setVolume( 100 );     break;
            case DtConfig::AC_MUTE :          muteButton->click();  break;
            case DtConfig::AC_SCREENSHOT :    cmdScreenshot();      break;
            case DtConfig::AC_SCORES :
                if ( !scoresVisible ) {
                    scoresVisible = true;
                    gCmd( "+scores;" );
                }
                break;

            case DtConfig::AC_ACC :
                if ( !accVisible ) {
                    accVisible = true;
                    gCmd( "+acc;" );
                }
                break;

            case DtConfig::AC_CHAT :
                if ( chatVisible ) {
                    gCmd( "-chat;" );
                    chatVisible = false;
                }
                else {
                    gCmd( "+chat;" );
                    chatVisible = true;
                }
                break;

            case DtConfig::AC_REPEATDEMO :
                gameLauncher->playDemo();
                break;
        }

    }
    else {
        QString cmd = config.customKeyPressActions.value( action, "" );

        if ( !cmd.isEmpty() ) {
            if ( !cmd.trimmed().endsWith( ";" ) ) {
                cmd = cmd.trimmed() + ";";
            }

            QByteArray cmdBuf = cmd.toAscii();
            gCmd( cmdBuf.data() );
        }
    } */
}

void DtPlayerWindow::stopAction( int action ) {
    if ( action < 0 ) {
        return;
    }

/* OLD    if ( action <= 100 ) {
        switch ( action ) {
            case DtConfig::AC_SLOW :      cmdSlow();          break;
            case DtConfig::AC_FAST :
            case DtConfig::AC_VERYFAST :  cmdNormalSpeed();   break;
            case DtConfig::AC_SCORES :
                gCmd( "-scores;" );
                scoresVisible = false;
                break;

            case DtConfig::AC_ACC :
                gCmd( "-acc;" );
                accVisible = false;
                break;

            default : break;
        }
    }
    else {
        QString cmd = config.customKeyReleaseActions.value( action, "" );

        if ( !cmd.isEmpty() ) {
            if ( !cmd.trimmed().endsWith( ";" ) ) {
                cmd = cmd.trimmed() + ";";
            }

            QByteArray cmdBuf = cmd.toAscii();
            gCmd( cmdBuf.data() );
        }
    } */
}

DtPlayerScreenWidget::DtPlayerScreenWidget( QSize mode, QWidget* ) {
    setFixedSize( mode.width(), mode.height() );
    connectionMsg = "";
    bgPic = new QPixmap( ":/res/ql_button.png" );
}

DtPlayerScreenWidget::~DtPlayerScreenWidget() {
    delete bgPic;
}

void DtPlayerScreenWidget::paintEvent( QPaintEvent* ) {
    QPainter painter( this );
    painter.fillRect( 0, 0, width(), height(), QBrush( QColor( 0, 0, 0 ) ) );

    int bgTop = static_cast< int >( ( height() - bgPic->height() ) / 2 );
    int bgLeft = static_cast< int >( ( width() - bgPic->width() ) / 2 );
    painter.drawPixmap( bgLeft, bgTop, *bgPic );

    if ( !connectionMsg.isEmpty() ) {
        QRect rect( 0, bgTop + bgPic->height() + 25, width(), 40 );
        painter.setPen( QColor( 255, 255, 255 ) );
        painter.setFont( QFont( "Liberation Sans", 17 ) );
        painter.drawText( rect, Qt::AlignCenter, connectionMsg );
    }
}

void DtPlayerScreenWidget::clearMsg() {
    connectionMsg = "";
    update();
}

DtPlayerButton::DtPlayerButton( QWidget* parent ) : QToolButton( parent ) {

}

DtQzLoginDialog::DtQzLoginDialog( QWidget* parent ) : QDialog( parent ) {
    setWindowTitle( tr( "Login" ) );
    setMinimumSize( 360, 150 );

    QVBoxLayout* mLayout = new QVBoxLayout;

    text = new QLabel( this );

    okButton = new QPushButton( tr( "Ok" ), this );
    okButton->setFixedSize( 100, 28 );
    connect( okButton, SIGNAL( clicked() ), this, SLOT( okPressed() ) );

    cancelButton = new QPushButton( tr( "Cancel" ), this );
    cancelButton->setFixedSize( 100, 28 );
    connect( cancelButton, SIGNAL( clicked() ), this, SLOT( cancelPressed() ) );

    registerButton = new QPushButton( tr( "Register" ) );
    registerButton->setFixedSize( 100, 28 );
    connect( registerButton, SIGNAL( clicked() ), this, SLOT( registerPressed() ) );

    QLabel* emailLbl = new QLabel( "Email", this );
    emailEdit = new QLineEdit( this );

    QLabel* passLbl = new QLabel( tr( "Password" ), this );
    passwordEdit = new DtClearLineEdit( this );
    passwordEdit->setEchoMode( QLineEdit::Password );

    QLabel* savePasswordLbl = new QLabel( tr( "Save password" ), this );
    savePasswordCb = new QCheckBox( this );
    savePasswordCb->setChecked( config.settings->value( "Player/qzSavePassword", false ).toBool() );
    connect( savePasswordCb, SIGNAL( stateChanged( int ) ), this, SLOT( onSavePasswordCb() ) );

    mLayout->addSpacing( 10 );
    mLayout->addWidget( text, 1, Qt::AlignCenter );
    mLayout->addSpacing( 15 );

    QGridLayout* editLayout = new QGridLayout;

    editLayout->addWidget( emailLbl, 0, 0 );
    editLayout->addWidget( emailEdit, 0, 1 );
    editLayout->addWidget( passLbl, 1, 0 );
    editLayout->addWidget( passwordEdit, 1, 1 );

    QHBoxLayout* savePwdLayout = new QHBoxLayout;

    savePwdLayout->addWidget( savePasswordCb );
    savePwdLayout->addWidget( savePasswordLbl );
    savePwdLayout->addStretch( 1 );

    editLayout->addLayout( savePwdLayout, 2, 1, 1, 1, Qt::AlignLeft );

    mLayout->addLayout( editLayout );

    QHBoxLayout* buttonsLayout = new QHBoxLayout;

    buttonsLayout->addWidget( registerButton );
    buttonsLayout->addStretch( 1 );
    buttonsLayout->addWidget( okButton );
    buttonsLayout->addWidget( cancelButton );

    mLayout->addSpacing( 15 );
    mLayout->addLayout( buttonsLayout );

    setLayout( mLayout );

    pEventLoop = 0;
    retEmail.clear();
    retPass.clear();
}

void DtQzLoginDialog::okPressed() {
    rDone( BTN_OK );
}

void DtQzLoginDialog::cancelPressed() {
    rDone( BTN_CANCEL );
}

void DtQzLoginDialog::registerPressed() {
    QDesktopServices::openUrl( QUrl( "http://www.quakelive.com/#register" ) );
}

void DtQzLoginDialog::onSavePasswordCb() {
// OLD    config.qzSavePassword = savePasswordCb->isChecked();
}

void DtQzLoginDialog::closeEvent( QCloseEvent* e ) {
    pEventLoop->exit( 0 );
    e->accept();
}

int DtQzLoginDialog::exec( const QString& msg, QString& email, QString& pass ) {
    text->setText( msg );

    if ( pEventLoop ) {
        return BTN_CANCEL;
    }

    if ( !email.isEmpty() ) {
        emailEdit->setText( email );
    }

    if ( !pass.isEmpty() ) {
        passwordEdit->setText( pass );
    }

    show();

    ret = BTN_CANCEL;
    QEventLoop eventLoop;
    pEventLoop = &eventLoop;

    QPointer< DtQzLoginDialog > guard = this;

    eventLoop.exec( QEventLoop::DialogExec );

    pEventLoop = 0;

    if ( guard.isNull() ) {
        return BTN_CANCEL;
    }

    email = emailEdit->text();
    pass = passwordEdit->text();

    return ret;
}

void DtQzLoginDialog::rDone( int retc ) {
    ret = retc;
    close();
}
