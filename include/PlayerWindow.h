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

#ifndef DPPLAYERWINDOW_H
#define DPPLAYERWINDOW_H

#include "PlayerData.h"
#include "DemoTable.h"

#include <QDialog>
#include <QToolButton>

class DtPlayerScreenWidget;
class DtPlayerButton;
class DtQZLoginDialog;
class DtGameLauncher;
class QEvent;
class QKeyEvent;
class QSlider;
class QCheckBox;
class QEventLoop;
class QLabel;
class QLineEdit;

enum loginDialogButtons {
    BTN_CANCEL,
    BTN_OK
};

class DtPlayerWindow : public QDialog {
    Q_OBJECT
public:

    DtPlayerWindow( QWidget* parent = 0 );
    ~DtPlayerWindow();
    void playDemo( const QString& demoFileName, const QString& windowTitle = QString() );
    void playDemo( int pos );
    bool isQzRunning();
    bool isQzLoggedIn();
    void setShowQzWarining( bool s );

public slots:
    void qzConnectEvent( const QString& );
    void qzExitEvent();
    void qzDisconnectEvent();
    void qzServerInfoEvent();
    void showQzLoginDialog( bool );
    void cmdStopPlay();
    void exit();
    void updateStyle();

private:
    QString defaultWindowTitle;
    QString consoleCmdString;
    QWidget* controlWidget;

    DtGameLauncher* gameLauncher;
    DtPlayerScreenWidget* screenArea;
    DtQZLoginDialog* loginDialog;

    DtPlayerButton* prevButton;
    DtPlayerButton* stopButton;
    DtPlayerButton* playButton;
    DtPlayerButton* pauseButton;
    DtPlayerButton* fastButton;
    DtPlayerButton* veryFastButton;
    DtPlayerButton* nextButton;
    DtPlayerButton* screenshotButton;
    DtPlayerButton* fullscreenButton;
    DtPlayerButton* muteButton;

    QSlider* soundVolumeSlider;
    QTimer* panelTimer;
    QStringList playList;
    QPoint lastPos;
    QTimer* panelVisibleTimer;

    void gCmd( const char* cmd );

    int soundVolume;
    bool muted;
    bool slowPressed;
    bool pausePressed;
    bool fullscreen;
    int modeNum;
    bool autoClosed;
    bool chatVisible;
    bool timescaleSoundMuted;
    bool accVisible;
    bool scoresVisible;
    bool goingToDisconnect;
    bool showQzWarning;
    bool hideOnLogin;
    bool consoleOpened;

protected:
    void closeEvent( QCloseEvent* e );
    void keyPressEvent( QKeyEvent* e );
    void keyReleaseEvent( QKeyEvent* e );
    void setVolume( int volume );
    void keyAction( int action );
    void stopAction( int action );
    bool eventFilter( QObject* obj, QEvent* e );
    void mouseMove();
#ifdef Q_OS_LINUX
    void sendKeyEvent( QKeyEvent* e );
#endif

private slots:
    void cmdPrevDemo();
    void cmdNextDemo();
    void cmdTogglePlay();
    void cmdSlow();
    void cmdPause();
    void cmdFast();
    void cmdVeryFast();
    void cmdNormalSpeed();
    void cmdSpeed( float timeScale );
    void cmdMute();
    void cmdVolumeChanged( int volume );
    void cmdScreenshot();
    void cmdToggleFullscreen();
    void panelTimerEvent();
    void closePanel();

signals:
    void sPlayDemo( tableDemoPos sDemo );
    void clearLoginData();
};

class DtPlayerScreenWidget : public QWidget {
    Q_OBJECT
public:
    DtPlayerScreenWidget( QSize mode, QWidget* parent = 0 );
    ~DtPlayerScreenWidget();

    QString connectionMsg;

private:
    QPixmap* bgPic;

protected:
    void paintEvent( QPaintEvent* e );

public slots:
    void clearMsg();

};

class DtClearLineEdit;

class DtPlayerButton: public QToolButton {
    Q_OBJECT
public:
    DtPlayerButton( QWidget* parent = 0 );
};

class DtQzLoginDialog : public QDialog {
    Q_OBJECT
public:
    DtQzLoginDialog( QWidget* parent = 0 );

    int exec( const QString& msg, QString& email, QString& pass );

protected:
    QLabel* text;
    QPushButton* okButton;
    QPushButton* cancelButton;
    QPushButton* registerButton;
    QLineEdit* emailEdit;
    DtClearLineEdit* passwordEdit;
    QCheckBox* savePasswordCb;
    QEventLoop* pEventLoop;

    int ret;
    QString retEmail;
    QString retPass;

    void rDone( int retc = BTN_CANCEL );
    void closeEvent( QCloseEvent* e );

private slots:
    void onSavePasswordCb();
    void okPressed();
    void cancelPressed();
    void registerPressed();
};

#endif // DPPLAYERWINDOW_H
