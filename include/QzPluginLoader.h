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

#ifndef DTQZPLUGINLOADER_H
#define DTQZPLUGINLOADER_H

#include "PlayerData.h"
#include "firefox.h"

#include <QThread>
#include <QHash>
#include <qwindowdefs.h>
#include <QNetworkReply>

class QNetworkAccessManager;
class QLibrary;

class DtQzPluginLoader : public QThread {
    Q_OBJECT
public:

    DtQzPluginLoader( WId win, QObject* parent = 0 );
    ~DtQzPluginLoader();

    bool initializePlugin();
    bool setMode();
    bool launchDemo( QString dName );
    void sendGameCommand( const char* cmd );
    bool isGameRunning();
    void setLogin( const QString& email, const QString& pass );
    bool isLoggedIn();
    bool haveLoginData();
    void setLaunchCommand( const char* cmd );
    static const QHash< QString, QString >& getStoredCvars();
    static bool isCvarAffected( const QString& cvar );

public slots:
    void closeNP();
    void onGameExit();
    void onGameDisconnect();
    void onGameServerInfo();
    void cancelLogin();
    void unload();
    void clearSession();

private:
    QString demoName;
    QString demoCommand;
    NPNetscapeFuncs nsCallbacks;
    NPVariant boolResult;
    NPWindow npWindow;
    NPPluginFuncs callbacks;
    NPP_t pluginInstance;
    NPSavedData* savedData;
    NPObject* pObj;

    struct DtQzLoginError {
        QString msg;

        DtQzLoginError( const QString& errorMsg = "" ) {
            msg = errorMsg;
        }
    };

    static WId screen;
    static QHash< QString, NPIdentifier > npIds;
    static NPVariant locationVal;
    static NPObject npObj;
    static NPObject* npObj_ptr;
    static NPClass npClass;

    QLibrary* pluginLib;
    QNetworkAccessManager* networkManager;
    bool waitHttpSession;
    QString userName;
    QString password;
    QString authId;
    QString loginName;
    static QHash< QString, QString > storedCvars;
    static QStringList affectedHardwareCvars;
    static QStringList affectedOtherCvars;

    bool eventGameExit;
    bool eventServerInfo;
    bool pluginInitialized;
    bool loginDataDefined;
    bool loginCancelled;

    void initializeIds();
    void initializeNsCallbacks();
    void newId( const NPUTF8* idStr );
    void setGameSession();
    void readStoredSettings();

    static NPIdentifier _getstringidentifier( const NPUTF8* name );
    static NPUTF8* _utf8fromidentifier( NPIdentifier id );
    static bool _invoke( NPP npp, NPObject* npobj, NPIdentifier method, const NPVariant* args,
                         uint32_t argCount, NPVariant* result );
    static bool _getproperty( NPP npp, NPObject* npobj, NPIdentifier property, NPVariant* result );
    static NPError _getvalue( NPP npp, NPNVariable variable, void* r_value );
    static void _releaseobject( NPObject* npobj );
    static DtQzPluginLoader* loaderInstance;
    inline void setStrArg( NPVariant* arg, const char* str );
    void onKicked();
    bool login();
    bool checkMethods();
    void waitGameRunning();

private slots:
    void sessUnblock();

signals:
    void httpEvent( const QString& );
    void gameExitEvent();
    void gameDisconnectEvent();
    void gameServerInfoEvent();
    void errorLogin( bool );
    void kicked();

};


#endif // DTQZPLUGINLOADER_H
