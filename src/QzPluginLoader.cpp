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

#include "QzPluginLoader.h"
#include "NpFunctions.h"
#include "Data.h"

#include <QApplication>
#include <QTimer>
#include <QHttp>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QLibrary>
#include <QSslError>
#include <QFile>

#ifdef Q_OS_LINUX
#include <dlfcn.h>
#include <QX11Info>
#include <X11/X.h>
#endif

WId DtQzPluginLoader::screen;
QHash< QString, NPIdentifier > DtQzPluginLoader::npIds;
NPVariant DtQzPluginLoader::locationVal;
NPObject DtQzPluginLoader::npObj;
NPObject* DtQzPluginLoader::npObj_ptr;
NPClass DtQzPluginLoader::npClass;
DtQzPluginLoader* DtQzPluginLoader::loaderInstance;
QHash< QString, QString > DtQzPluginLoader::storedCvars;
QStringList DtQzPluginLoader::affectedHardwareCvars = QStringList() << "r_fullscreen" << "r_mode" <<
                                                      "timescale" << "in_nograb" << "s_volume" <<
                                                      "com_cameraMode";
QStringList DtQzPluginLoader::affectedOtherCvars = QStringList() << "r_inBrowserMode" <<
                                                   "cl_quitOnDemoCompleted" << "com_allowConsole" <<
                                                   "cg_draw2D";

using namespace dtdata;

DtQzPluginLoader::DtQzPluginLoader( WId win, QObject* parent ) : QThread( parent ),
    pluginLib( new QLibrary( this ) ),
    networkManager( new QNetworkAccessManager( this ) ),
    authId( "0" ),
    loginName( "0" ),
    eventGameExit( true ),
    eventServerInfo( true ),
    pluginInitialized( false ),
    loginDataDefined( false )
{
    loaderInstance = this;
    screen = win;
    boolResult.type = NPVariantType_Bool;

    initializeIds();
}

void DtQzPluginLoader::waitGameRunning() {
    while ( isGameRunning() ) {
        msleep( 30 );
    }
}

void DtQzPluginLoader::unload() {
    if ( isGameRunning() ) {
        sendGameCommand( "quit;" );
        waitGameRunning();
    }

    closeNP();
}


DtQzPluginLoader::~DtQzPluginLoader() {
    unload();
}

void DtQzPluginLoader::setLogin( const QString& email, const QString& pass ) {
    userName = QUrl::toPercentEncoding( email );
    password = pass;
    loginDataDefined = true;
}

void DtQzPluginLoader::cancelLogin() {
    loginCancelled = true;
}

bool DtQzPluginLoader::login() {
    loginCancelled = false;

    QNetworkRequest request;
    request.setUrl( QUrl( "https://secure.quakelive.com/user/login" ) );

    const QString& uAgent = config.qzCustomUserAgent ? config.qzUserAgent : firefoxUserAgent;
    request.setRawHeader( "User-Agent", uAgent.toUtf8().data() );
    request.setRawHeader( "Accept", "application/json, text/javascript, */*" );
    request.setRawHeader( "Pragma", "no-cache" );
    request.setRawHeader( "Cache-Control", "no-cache" );
    request.setRawHeader( "Content-Type", "application/x-www-form-urlencoded" );

    waitHttpSession = true;
    QNetworkReply* reply = networkManager->post( request, QString( "email=%1&pass=%2&r=0" )
                                                 .arg( userName, password ).toUtf8().data() );
    connect( reply, SIGNAL( finished() ), this, SLOT( sessUnblock() ) );
    connect( reply, SIGNAL( sslErrors( const QList< QSslError >& ) ),
             reply, SLOT( ignoreSslErrors() ) );

    emit httpEvent( tr( "Connecting to quakelive.com" ) );

    try {
        while ( waitHttpSession ) {
            if ( loginCancelled ) {
                throw DtQzLoginError();
            }

            qApp->processEvents( QEventLoop::WaitForMoreEvents );
        }

        if ( !reply->hasRawHeader( "Set-Cookie" ) ) {
            throw DtQzLoginError( tr( "Login error: no data" ) );
        }

        QRegExp session( "(quakelive_sess=[0-9a-z]+;)" );

        if ( session.indexIn( reply->rawHeader( "Set-Cookie" ) ) == -1 ) {
            throw DtQzLoginError( tr( "Login error: can't get session" ) );
        }

        if ( session.cap( 1 ).contains( "deleted", Qt::CaseInsensitive ) ) {
            emit errorLogin( false );
            throw DtQzLoginError();
        }

        reply->deleteLater();

        QUrl url = request.url();
        url.setPath( "/user/load" );
        request.setUrl( url );
        request.setRawHeader( "X-Requested-With", "XMLHttpRequest" );
        request.setRawHeader( "Cookie", session.cap( 1 ).toUtf8().data() );

        waitHttpSession = true;
        reply = networkManager->get( request );
        connect( reply, SIGNAL( finished() ), this, SLOT( sessUnblock() ) );

        while ( waitHttpSession ) {
            if ( loginCancelled ) {
                throw DtQzLoginError();
            }

            QApplication::processEvents( QEventLoop::WaitForMoreEvents );
        }

        QString replyData = reply->readAll();
        reply->deleteLater();

        if ( replyData.isEmpty() ) {
            throw DtQzLoginError( tr( "Login error: no load data" ) );
        }

        QRegExp loginData( "USERNAME\":\"([\\w\\W]+)\",\"XAID\":\"([0-9a-z]+)" );

        if ( loginData.indexIn( replyData ) == -1 ) {
            throw DtQzLoginError( tr( "Login error: unknown response" ) );
        }

        loginName = loginData.cap( 1 );
        authId = loginData.cap( 2 );

        setGameSession();
    }
    catch ( DtQzLoginError err ) {
        if ( !err.msg.isEmpty() ) {
            emit httpEvent( err.msg );
        }

        loginName.clear();
        authId = "0";
        reply->deleteLater();
        return false;
    }

    return true;
}

void DtQzPluginLoader::setStrArg( NPVariant* arg, const char* str ) {
    arg->type = NPVariantType_String;
    arg->value.stringValue.utf8characters = const_cast< NPUTF8* >( str );
    arg->value.stringValue.utf8length = strlen( str );
}

void DtQzPluginLoader::setGameSession() {
    static const char* realmStr = "quakelive";

    NPVariant lArgs;
    setStrArg( &lArgs, realmStr );

    NPVariant lRes;
    lRes.type = NPVariantType_Int32;
    lRes.value.intValue = 0;

    pObj->_class->invoke( pObj, npIds.value( "SetDeveloperRoot" ), &lArgs, 1, &lRes );

    NPVariant sessArgs[ 4 ];
    QString usAgent = config.qzCustomUserAgent ? config.qzUserAgent : firefoxUserAgent;

    QByteArray uAgent = usAgent.toAscii();
    setStrArg( &sessArgs[ 0 ], uAgent.data() );

    QByteArray pUsername = loginName.toAscii();
    setStrArg( &sessArgs[ 1 ], pUsername.data() );

    QByteArray pPassword = authId.toAscii();
    setStrArg( &sessArgs[ 2 ], pPassword.data() );

    sessArgs[ 3 ].type = NPVariantType_Int32;
    sessArgs[ 3 ].value.intValue = 0;

    pObj->_class->invoke( pObj, npIds.value( "SetSession" ),
                          reinterpret_cast< NPVariant* >( &sessArgs ), 4, &lRes );
}

void DtQzPluginLoader::sessUnblock() {
    waitHttpSession = false;
}

void DtQzPluginLoader::sendGameCommand( const char* cmd ) {
    if ( !isGameRunning() ) {
        return;
    }

    NPVariant gcArgs, gcRes;
    gcRes.type = NPVariantType_Void;
    gcArgs.type = NPVariantType_String;
    gcArgs.value.stringValue.utf8characters = cmd;
    gcArgs.value.stringValue.utf8length = strlen( cmd );

    pObj->_class->invoke( pObj, npIds.value( "SendGameCommand" ), &gcArgs, 1, &gcRes );
}

bool DtQzPluginLoader::isGameRunning() {
    if ( !pluginInitialized ) {
        return false;
    }

    boolResult.value.boolValue = false;
    pObj->_class->invoke( pObj, npIds.value( "IsGameRunning" ), 0, 0, &boolResult );

    return boolResult.value.boolValue;
}

void DtQzPluginLoader::closeNP() {
    if ( !pluginInitialized ) {
        return;
    }

    pluginInitialized = false;
    --pObj->referenceCount;
    NPError err = callbacks.destroy( &pluginInstance, &savedData );

    if ( err ) {
        qDebug() << "NPP_Destroy error" << err;
    }

    NP_PLUGINSHUTDOWN NP_Shutdown =
#ifdef Q_OS_LINUX
    pointer_cast
#elif defined Q_OS_WIN
    reinterpret_cast
#endif
    < NP_PLUGINSHUTDOWN >( pluginLib->resolve( "NP_Shutdown" ) );

    err = NP_Shutdown();

    if ( err ) {
        qDebug() << "NP_Shutdown error" << err;
    }

    pluginLib->unload();
}

bool DtQzPluginLoader::isLoggedIn() {
    return ( authId != "0" );
}

bool DtQzPluginLoader::haveLoginData() {
    return loginDataDefined;
}

void DtQzPluginLoader::clearSession() {
    loginName = "0";
    authId = "0";
    userName.clear();
    password.clear();
    loginDataDefined = false;
}

void DtQzPluginLoader::setLaunchCommand( const char* cmd ) {
    demoCommand = cmd;
}

const QHash< QString, QString >& DtQzPluginLoader::getStoredCvars() {
    return storedCvars;
}

bool DtQzPluginLoader::isCvarAffected( const QString& cvar ) {
    return ( affectedHardwareCvars.contains( cvar, Qt::CaseInsensitive ) ||
             affectedOtherCvars.contains( cvar, Qt::CaseInsensitive ) );
}

void DtQzPluginLoader::readStoredSettings() {
    storedCvars.clear();
    QFile qzConfig( config.getQzBasePath() + "/qzconfig.cfg" );
    static QRegExp cvarReg( "seta ([\\w]+) \"([^\"]*)" );

    if ( qzConfig.open( QFile::ReadOnly ) ) {
        while ( !qzConfig.atEnd() ) {
            QString line = qzConfig.readLine().trimmed();

            if ( cvarReg.indexIn( line ) != -1 &&
                affectedHardwareCvars.contains( cvarReg.cap( 1 ), Qt::CaseInsensitive ) )
            {
               storedCvars.insert( cvarReg.cap( 1 ), cvarReg.cap( 2 ) );
            }
        }

        qzConfig.close();
    }

    QFile repConfig( config.getQzBasePath() + "/repconfig.cfg" );

    if ( repConfig.open( QFile::ReadOnly ) ) {
        while ( !repConfig.atEnd() ) {
            QString line = repConfig.readLine().trimmed();

            if ( cvarReg.indexIn( line ) != -1 &&
                affectedOtherCvars.contains( cvarReg.cap( 1 ), Qt::CaseInsensitive ) )
            {
                storedCvars.insert( cvarReg.cap( 1 ), cvarReg.cap( 2 ) );
            }
        }

        repConfig.close();
    }
}

bool DtQzPluginLoader::launchDemo( QString dName ) {
    demoName = dName;

    if ( !pluginInitialized && !initializePlugin() ) {
        return false;
    }

    if ( authId == "0" ) {
        if ( !login() ) {
            clearSession();
            return false;
        }
    }
    else {
        setGameSession();
    }

    if ( isGameRunning() ) {
        QString cmd = "demo " + demoName + ";";
        sendGameCommand( cmd.toAscii().data() );
        return true;
    }

    QString execConfig;

    if ( !config.qzGameConfig.isEmpty() ) {
        execConfig = " +exec " + config.qzGameConfig;
    }

    QString cmdString;
    cmdString = demoCommand +
                " +set gt_user " +                 loginName +
                " +set gt_pass " +                 authId +
                " +set gt_realm quakelive" +
                " +set r_fullscreen " +            QString::number( config.qzFullscreen ) +
                " +set r_mode " +                  QString::number( config.qzFullscreenMode ) +
                " +set r_inBrowserMode " +         QString::number( config.qzWindowedMode ) +
                " +set in_nograb " +               QString::number( !config.qzFullscreen ) +
                " +set cl_quitOnDemoCompleted 1" +
                " +set com_cameramode 1" +
                " +set com_allowConsole 1" +
                " +set timescale 1" +
                execConfig +
                " +demo " + demoName;

    demoCommand.clear();

    if ( config.qzSoundMute ) {
        cmdString += " +set s_volume 0";
    }
    else {
        cmdString += " +set s_volume " + QString::number( config.qzSoundVolume / 50.f );
    }

    readStoredSettings();

    QByteArray lastCommand = cmdString.toAscii();
    NPVariant lArgs;
    setStrArg( &lArgs, lastCommand.data() );

    NPVariant lRes;
    lRes.type = NPVariantType_Int32;
    lRes.value.intValue = 0;

    fullscreenGameRunning = config.qzFullscreen;
    pObj->_class->invoke( pObj, npIds.value( "LaunchGameWithCmdBuffer" ), &lArgs, 1, &lRes );

    if ( lRes.value.intValue ) {
        fullscreenGameRunning = false;
        return false;
    }

    eventGameExit = false;
    eventServerInfo = false;

    return true;
}

bool DtQzPluginLoader::initializePlugin() {
    QString pluginPath = config.getQzPath();

    if ( pluginPath.isEmpty() ) {
        qDebug( "Plugin path is empty" );
        return false;
    }

    initializeNsCallbacks();

    const NPUTF8* locHref = "http://www.quakelive.com/";
    locationVal.type = NPVariantType_String;
    locationVal.value.stringValue.utf8characters = locHref;
    locationVal.value.stringValue.utf8length = 25;

    npClass.structVersion = 3;
    npClass.allocate = NPAllocate;
    npClass.deallocate = NPDeallocate;
    npClass.invalidate = NPInvalidate;
    npClass.hasMethod = NPHasMethod;
    npClass.invoke = NPInvoke;
    npClass.invokeDefault = NPInvokeDefault;
    npClass.hasProperty = NPHasProperty;
    npClass.getProperty = NPGetProperty;
    npClass.setProperty = NPSetProperty;
    npClass.removeProperty = NPRemoveProperty;
    npClass.enumerate = NPEnumeration;
    npClass.construct = NPConstruct;

    npObj._class = &npClass;
    npObj.referenceCount = 1;

    pluginLib->setFileName( pluginPath );
    pluginLib->load();

    if ( !pluginLib->isLoaded() ) {
        qDebug( "Couldn't load plugin" );
        return false;
    }

    memset( reinterpret_cast< void* >( &callbacks ), 0, sizeof( callbacks ) );
    callbacks.size = sizeof( callbacks );

    NPError err;

#ifdef Q_OS_LINUX
    NP_PLUGINUNIXINIT NP_Initialize = pointer_cast< NP_PLUGINUNIXINIT >(
                                      pluginLib->resolve( "NP_Initialize" ) );

    if ( !NP_Initialize ) {
        qDebug( "Couldn't find NP_Initialize" );
        return false;
    }

    err = NP_Initialize( &nsCallbacks, &callbacks );
#elif defined Q_OS_WIN
    NP_GETENTRYPOINTS NP_GetEntryPoints = reinterpret_cast< NP_GETENTRYPOINTS >(
                                          pluginLib->resolve( "NP_GetEntryPoints" ) );

    if ( !NP_GetEntryPoints ) {
        qDebug( "Couldn't find NP_GetEntryPoints" );
        return false;
    }

    NP_PLUGININIT NP_Initialize = reinterpret_cast< NP_PLUGININIT >(
                                  pluginLib->resolve( "NP_Initialize" ) );

    if ( !NP_GetEntryPoints ) {
        qDebug( "Couldn't find NP_Initialize" );
        return false;
    }

    err = NP_GetEntryPoints( &callbacks );

    if ( err ) {
        qDebug() << "NP_GetEntryPoints error" << err;
        return false;
    }

    err = NP_Initialize( &nsCallbacks );
#endif

    if ( err ) {
        qDebug() << "NP_Initialize error" << err;
        return false;
    }

    pluginInstance.pdata = 0;
    pluginInstance.ndata = this;
    savedData = 0;

    int16 argc = 5;
    const char* argn[] = { "id", "class", "width", "height", "type" };
    const char* argv[] = { "qz_instance", "game_viewport", "100%", "100%",
                           "application/x-id-quakelive" };
    const char* mime = "application/x-id-quakelive";

    err = callbacks.newp( const_cast< char* >( mime ),
                          &pluginInstance,
                          NP_EMBED,
                          argc,
                          const_cast< char** >( argn ),
                          const_cast< char** >( argv ),
                          savedData );

    if ( err ) {
        qDebug() << "NPP_New error" << err;
        return false;
    }

    err = callbacks.getvalue( &pluginInstance, NPPVpluginScriptableNPObject, &pObj );

    if ( err ) {
        qDebug() << "NPP_GetValue error" << err;
        return false;
    }

    if ( !checkMethods() ) {
        return false;
    }

    pluginInitialized = true;

    return setMode();
}

bool DtQzPluginLoader::setMode() {
    npWindow.window = reinterpret_cast< void* >( screen );
    npWindow.x = 0;
    npWindow.y = 0;
    npWindow.width = qzModes.at( config.qzWindowedMode ).width();
    npWindow.height = qzModes.at( config.qzWindowedMode ).height();
    npWindow.type = NPWindowTypeWindow;

    NPError err = callbacks.setwindow( &pluginInstance, &npWindow );

    if ( err ) {
        qDebug() << "NPP_SetWindow error" << err;
        return false;
    }

    return true;
}

void DtQzPluginLoader::onGameExit() {
    if ( !eventGameExit ) {
        eventGameExit = true;

        waitGameRunning();
        emit gameExitEvent();
    }
}

void DtQzPluginLoader::onGameDisconnect() {
    emit gameDisconnectEvent();
}

void DtQzPluginLoader::onGameServerInfo() {
    if ( !eventServerInfo ) {
        eventServerInfo = true;
        emit gameServerInfoEvent();
    }
}

void DtQzPluginLoader::onKicked() {
    emit kicked();
}

NPIdentifier DtQzPluginLoader::_getstringidentifier( const NPUTF8* name ) {
    return npIds.value( name, 0 );
}

NPUTF8* DtQzPluginLoader::_utf8fromidentifier( NPIdentifier id ) {
    QString key = npIds.key( id, "0"  );

    return ( key == "0" ) ? 0 : reinterpret_cast< NPUTF8* >( npIds.key( id ).toUtf8().data() );
}

QString jsonGetVal( const QString& json, const QString& varName ) {
    QRegExp reg( varName + "\" : \"([^\"]+)" );

    if ( reg.indexIn( json ) != -1 ) {
        return reg.cap( 1 );
    }

    return "";
}

bool DtQzPluginLoader::_invoke( NPP, NPObject*, NPIdentifier method,
                                const NPVariant* args, uint32_t, NPVariant* ) {
    QString key = npIds.key( method, "0" );

    if ( key == "OnGameExit" ) {
        loaderInstance->onGameExit();
    }
    else if ( key == "OnCommNotice" ) {
        if ( ( *args ).value.intValue == 0 ) {
            QString jsonNotice = ( *( ++args ) ).value.stringValue.utf8characters;

            if ( jsonGetVal( jsonNotice, "MSG_TYPE" ) == "error" &&
                 jsonGetVal( jsonNotice, "TEXT" ) == "Disconnected from server" )
            {
                loaderInstance->onGameDisconnect();

            }
            else if ( jsonGetVal( jsonNotice, "MSG_TYPE" ) == "serverinfo" ) {
                loaderInstance->onGameServerInfo();
            }
        }
    } else if ( key == "IM_OnKicked" ) {
        loaderInstance->onKicked();
        loaderInstance->clearSession();
    }

    if ( key != "0" ) {
        //qDebug() << "invoke" << key;
        return true;
    }

    return false;
}

bool DtQzPluginLoader::_getproperty( NPP, NPObject* npobj, NPIdentifier property,
                                     NPVariant* result ) {
    if ( npobj                                      &&
         ( property == npIds.value( "location" )    ||
           property == npIds.value( "href" )        ||
           property == npIds.value( "document" ) ) )
    {
        if ( property == npIds.value( "href" ) ) {
            *result = locationVal;
        }
        else {
            result->value.objectValue = &npObj;
        }

        return true;
    }

    return false;
}

NPError DtQzPluginLoader::_getvalue( NPP, NPNVariable variable, void* r_value ) {
    if ( variable == NPNVWindowNPObject ) {
        npObj_ptr = &npObj;
        *reinterpret_cast< NPObject** >( r_value ) = npObj_ptr;

        return 0;
    }
    else if ( variable == NPNVnetscapeWindow ) {
        *reinterpret_cast< WId** >( r_value ) = &screen;
        return 0;
    }

    return 1;
}

void DtQzPluginLoader::_releaseobject( NPObject* ) {
    npObj_ptr = 0;
}

bool DtQzPluginLoader::checkMethods() {
    #define CHECK_METHOD( x )                                           \
        if ( !pObj->_class->hasMethod( pObj, npIds.value( #x ) ) ) {    \
            qDebug( "Function %s not found", #x );                      \
            return false;                                               \
        }

    CHECK_METHOD( LaunchGameWithCmdBuffer );
    CHECK_METHOD( IsGameRunning );
    CHECK_METHOD( SendGameCommand );
    CHECK_METHOD( SetDeveloperRoot );
    CHECK_METHOD( SetSession );

    return true;
}

void DtQzPluginLoader::newId( const NPUTF8* idStr ) {
    npIds[ idStr ] = reinterpret_cast< NPIdentifier >( STRING_TO_JSVAL( idStr ) );
}

void DtQzPluginLoader::initializeIds() {
    newId( "IsGameRunning" );
    newId( "LaunchGameWithCmdBuffer" );
    newId( "SendGameCommand" );
    newId( "SetDeveloperRoot" );
    newId( "SetSession" );
    newId( "OnCvarChanged" );
    newId( "OnGameExit" );
    newId( "location" );
    newId( "href" );
    newId( "document" );
    newId( "OnPluginReady" );
    newId( "OnVidRestart" );
    newId( "OnCommNotice" );
    newId( "OnBindChanged" );

    newId( "IM_Subscribe" );
    newId( "IM_SetPrivacyList" );
    newId( "IM_SendMessage" );
    newId( "IM_RemovePrivacyList" );
    newId( "IM_OnSubscribeRequest" );
    newId( "IM_OnSelfPresence" );
    newId( "IM_OnRosterFilled" );
    newId( "IM_OnPrivacyResult" );
    newId( "IM_OnPrivacyNames" );
    newId( "IM_OnPrivacyList" );
    newId( "IM_OnPrivacyChanged" );
    newId( "IM_OnPresence" );
    newId( "IM_OnMessage" );
    newId( "IM_OnKicked" );
    newId( "IM_OnItemUpdated" );
    newId( "IM_OnItemUnsubscribed" );
    newId( "IM_OnItemSubscribed" );
    newId( "IM_OnItemRemoved" );
    newId( "IM_OnItem" );
    newId( "IM_OnDisconnected" );
    newId( "IM_OnConnected" );
    newId( "IM_OnConnectFail" );
    newId( "IM_GetRoster" );
    newId( "IM_GetPrivacyListNames" );
    newId( "IM_GetPrivacyList" );
    newId( "IM_AnswerSubscribeRequest" );
    newId( "IM_ActivatePrivacyList" );
}

void DtQzPluginLoader::initializeNsCallbacks() {
    nsCallbacks.size = sizeof( nsCallbacks );
    nsCallbacks.version = ( NP_VERSION_MAJOR << 8 ) + NP_VERSION_MINOR;

    nsCallbacks.geturl = NewNPN_GetURLProc( _geturl );
    nsCallbacks.posturl = NewNPN_PostURLProc( _posturl );
    nsCallbacks.requestread = NewNPN_RequestReadProc( _requestread );
    nsCallbacks.newstream = NewNPN_NewStreamProc( _newstream );
    nsCallbacks.write = NewNPN_WriteProc( _write );
    nsCallbacks.destroystream = NewNPN_DestroyStreamProc( _destroystream );
    nsCallbacks.status = NewNPN_StatusProc( _status );
    nsCallbacks.uagent = NewNPN_UserAgentProc( _useragent );
    nsCallbacks.memalloc = NewNPN_MemAllocProc( _memalloc );
    nsCallbacks.memfree = NewNPN_MemFreeProc( _memfree );
    nsCallbacks.memflush = NewNPN_MemFlushProc( _memflush );
    nsCallbacks.reloadplugins = NewNPN_ReloadPluginsProc( _reloadplugins );
    nsCallbacks.geturlnotify = NewNPN_GetURLNotifyProc( _geturlnotify );
    nsCallbacks.posturlnotify = NewNPN_PostURLNotifyProc( _posturlnotify );
    nsCallbacks.getvalue = NewNPN_GetValueProc( _getvalue );
    nsCallbacks.setvalue = NewNPN_SetValueProc( _setvalue );
    nsCallbacks.invalidaterect = NewNPN_InvalidateRectProc( _invalidaterect );
    nsCallbacks.invalidateregion = NewNPN_InvalidateRegionProc( _invalidateregion );
    nsCallbacks.forceredraw = NewNPN_ForceRedrawProc( _forceredraw );
    nsCallbacks.getstringidentifier = NewNPN_GetStringIdentifierProc( _getstringidentifier );
    nsCallbacks.getstringidentifiers = NewNPN_GetStringIdentifiersProc( _getstringidentifiers );
    nsCallbacks.getintidentifier = NewNPN_GetIntIdentifierProc( _getintidentifier );
    nsCallbacks.identifierisstring = NewNPN_IdentifierIsStringProc( _identifierisstring );
    nsCallbacks.utf8fromidentifier = NewNPN_UTF8FromIdentifierProc( _utf8fromidentifier );
    nsCallbacks.intfromidentifier = NewNPN_IntFromIdentifierProc( _intfromidentifier );
    nsCallbacks.createobject = NewNPN_CreateObjectProc( _createobject );
    nsCallbacks.retainobject = NewNPN_RetainObjectProc( _retainobject );
    nsCallbacks.releaseobject = NewNPN_ReleaseObjectProc( _releaseobject );
    nsCallbacks.invoke = NewNPN_InvokeProc( _invoke );
    nsCallbacks.invokeDefault = NewNPN_InvokeDefaultProc( _invokeDefault );
    nsCallbacks.evaluate = NewNPN_EvaluateProc( _evaluate );
    nsCallbacks.getproperty = NewNPN_GetPropertyProc( _getproperty );
    nsCallbacks.setproperty = NewNPN_SetPropertyProc( _setproperty );
    nsCallbacks.removeproperty = NewNPN_RemovePropertyProc( _removeproperty );
    nsCallbacks.hasproperty = NewNPN_HasPropertyProc( _hasproperty );
    nsCallbacks.hasmethod = NewNPN_HasMethodProc( _hasmethod );
    nsCallbacks.enumerate = NewNPN_EnumerateProc( _enumerate );
    nsCallbacks.construct = NewNPN_ConstructProc( _construct );
    nsCallbacks.releasevariantvalue = NewNPN_ReleaseVariantValueProc( _releasevariantvalue );
    nsCallbacks.setexception = NewNPN_SetExceptionProc( _setexception);
    nsCallbacks.pushpopupsenabledstate = NewNPN_PushPopupsEnabledStateProc( _pushpopupsenabledstate );
    nsCallbacks.poppopupsenabledstate = NewNPN_PopPopupsEnabledStateProc( _poppopupsenabledstate );
    nsCallbacks.pluginthreadasynccall = NewNPN_PluginThreadAsyncCallProc( _pluginthreadasynccall );
}
