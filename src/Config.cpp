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

#include "Config.h"
#include "PlayerData.h"
#include "Crypt.h"
#include "Data.h"

#include <QTextCodec>
#include <QDir>
#include <QSettings>
#include <QBuffer>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

DtConfig::DtConfig() :
    settings( new QSettings( QSettings::NativeFormat, QSettings::UserScope, "QLDT", "settings" ) ),
    crypt( new DtCrypt )
{
}

#define GROUP( name ) settings->beginGroup( #name );
#define END settings->endGroup();

#define VAR( type, val, def )                       \
    defaultVars.insert( #val, def );                \
    val = settings->value( #val, def ).to##type();

#define FVAR( val, def )                                                    \
    defaultVars.insert( #val, def );                                        \
    val = byteArrayToFormat( settings->value( #val, def ).toByteArray() );

#define COLOR_VAR( val, def )                             \
    defaultVars.insert( #val, def );                      \
    val = settings->value( #val, def ).value< QColor >();

void DtConfig::loadLanguage() {
    QString uiDefaultLanguage = getDefaultLanguage();

    GROUP( Main )
    VAR( String, language, uiDefaultLanguage )
    END
}

void DtConfig::loadDefaults() {
    QString qzPluginPath = getQzPluginPath();
    QString qzDefaultHomePath = getQzDefaultHomePath();

    QBitArray mainColumnsDefault( 7, true );
    mainColumnsDefault.setBit( 6, false );

    QBitArray findFragsColumnsDefault( 10, true );
    findFragsColumnsDefault.setBit( 9, false );

    QBitArray findTextColumnsDefault( 10, true );
    findTextColumnsDefault.setBit( 2, false );
    findTextColumnsDefault.setBit( 3, false );
    findTextColumnsDefault.setBit( 4, false );
    findTextColumnsDefault.setBit( 8, false );
    findTextColumnsDefault.setBit( 9, false );

    QBitArray findDemoColumnsDefault( 8, true );
    findDemoColumnsDefault.setBit( 1, false );
    findDemoColumnsDefault.setBit( 7, false );

    loadLanguage();

    GROUP( Main )
    VAR( Int,           tableAlternateColorFactor,    8 )
    VAR( Int,           tableSelectionColorFactor,    17 )
    VAR( Int,           lastRecentMenuIndex,          0 )
    VAR( BitArray,      mainTableVisibleColumns,      mainColumnsDefault )
    VAR( BitArray,      findFragsTableVisibleColumns, findFragsColumnsDefault )
    VAR( BitArray,      findTextTableVisibleColumns,  findTextColumnsDefault )

    if ( findTextTableVisibleColumns.size() < 10 ) { // support of the versions prior to 1.1.9
        findTextTableVisibleColumns = findTextColumnsDefault;
    }

    VAR( BitArray,      findDemoTableVisibleColumns,  findDemoColumnsDefault )
    VAR( Bool,          headerInfoVisible,            true )
    VAR( Bool,          showClanTags,                 false )
    VAR( StringList,    recentOpenedFiles,            QStringList() )
    VAR( StringList,    recentOpenedUrls,             QStringList() )
    VAR( StringList,    recentOpenedConfigs,          QStringList() )
    VAR( Int,           selectedGame,                 Q_LIVE )
    VAR( Bool,          confirmOnDelete,              true )
    VAR( Int,           zlibCompressionLevel,         6 )
    VAR( Bool,          archiveRemovePaths,           true )
    VAR( Int,           lastArchiverFormat,           F_ZIP )
    VAR( String,        lastPackPath,                 QDir::homePath() )
    VAR( String,        lastExportPath,               QDir::homePath() )
    VAR( String,        lastImportPath,               QDir::homePath() )
    VAR( String,        qzHomePath,                   qzDefaultHomePath )
    VAR( String,        qaHomePath,                   "" )
    VAR( String,        qzPath,                       qzPluginPath )

    VAR( String,        qaPath,                       "" )
    VAR( Bool,          dirTreeAlwaysOpened,          false )
    VAR( Int,           mainTableSortColumn,          0 )
    VAR( Int,           mainTableSortOrder,           Qt::AscendingOrder )
    VAR( Int,           findFragsTableSortColumn,     0 )
    VAR( Int,           findFragsTableSortOrder,      Qt::AscendingOrder )
    VAR( Int,           findTextTableSortColumn,      0 )
    VAR( Int,           findTextTableSortOrder,       Qt::AscendingOrder )
    VAR( Int,           findDemoTableSortColumn,      0 )
    VAR( Int,           findDemoTableSortOrder,       Qt::AscendingOrder )
    VAR( Bool,          dropDemosToNewDir,            true )
    VAR( String,        lastRenameFormat,             "<T>-<P>-<M>-<D>" )
    VAR( String,        lastOrganizeFormat,           "<Y>" )
    VAR( String,        lastFindTextString,           "" )
    VAR( Bool,          findTextIgnoreColors,         true )
    VAR( Bool,          findTextMatchCase,            true )
    END

    GROUP( Edit )
    VAR( Bool,          editorShowGameTime,           true )
    VAR( Int,           partsSelectorMode,            0 )
    VAR( String,        timeDisplayFormat,            "m:ss.zzz" )
    VAR( Bool,          showLags,                     true )
    END

    GROUP( FindFrags )
    VAR( Bool,          frags.tabOpened,              false )
    VAR( Int,           frags.segAddTimeBegin,        1 )
    VAR( Int,           frags.segAddTimeEnd,          1 )
    VAR( Int,           frags.selectorIndex,          0 )
    VAR( Int,           frags.cbWeapon,               0 )
    VAR( Int,           frags.cbGameType,             0 )
    VAR( Int,           frags.cbMap,                  0 )
    VAR( String,        frags.leMap,                  "" )
    VAR( String,        frags.leMaxTime,              "20" )
    VAR( String,        frags.leMinFrags,             "1" )
    VAR( String,        frags.playerName,             "" )
    VAR( String,        frags.playerNames,            "" )
    VAR( Bool,          frags.directHitsOnly,         false )
    VAR( Bool,          frags.countTeamKills,         false )
    END

    GROUP( FindText )
    VAR( Int,           textSegAddTimeBegin,          2 )
    VAR( Int,           textSegAddTimeEnd,            2 )
    END

    GROUP( Player )
    VAR( Bool,          repeatPlaylist,               false )
    VAR( Bool,          autoPlayNext,                 true )
    VAR( Bool,          controlPanelAlwaysVisible,    false )
    VAR( String,        controlPanelStyle,            "carbon" )
    VAR( Bool,          draw2dOnSlow,                 true )
    VAR( Bool,          draw2dOnPause,                false )
    VAR( Double,        slowTimescale,                0.0001f )
    VAR( Double,        fastTimescale,                25.f )
    VAR( Double,        fastestTimescale,             128.f )
    VAR( Bool,          qzPauseMuteSound,             true )
    VAR( Bool,          qzForwardMuteSound,           true )
    VAR( Bool,          qzFullscreen,                 false )
    VAR( Bool,          qaFullscreen,                 false )
    VAR( Int,           qzWindowedMode,               QZ_800x600 )
    VAR( Int,           qaWindowedMode,               QA_800x600 )
    VAR( Int,           qzFullscreenMode,             QZ_800x600 )
    VAR( Int,           qaFullscreenMode,             QA_800x600 )
    VAR( Bool,          qzSoundMute,                  false )
    VAR( Int,           qzSoundVolume,                50 )
    VAR( Int,           qzSoundVolumeStep,            10 )
    VAR( String,        qzGameConfig,                 "" )
    VAR( String,        qaGameConfig,                 "" )
    VAR( Bool,          qzCustomUserAgent,            false )
    VAR( String,        qzUserAgent,                  firefoxUserAgent )
    VAR( String,        qzEmail,                      "" )
    VAR( Bool,          qzSavePassword,               false )
    VAR( ByteArray,     qzEncodedPassword,            "" )
    VAR( Bool,          qzKeyboardFilter,             true )
    VAR( Bool,          qzRemoveAdvertDelay,          false )
    VAR( Bool,          qzPreventSettingsCaching,     true )
    VAR( String,        otherAppTitle,                QObject::tr( "Other" ) )
    VAR( Bool,          otherAppDm68,                 false )
    VAR( Bool,          otherAppDm73,                 true )
    VAR( Bool,          otherAppDoubleClick,          true )
    VAR( Bool,          otherAppMenu,                 true )
    VAR( Bool,          otherAppPreview,              true )
    VAR( String,        otherAppPath,                 "" )
    VAR( String,        otherAppCmdLine,              "+demo %demoName +set nextdemo quit" )
    VAR( Bool,          otherAppFromDemos,            true )

    if ( !qzCustomUserAgent ) {
        qzUserAgent = defaultVars.value( "qzUserAgent", firefoxUserAgent ).toString();
    }

    qzPassword = qzEncodedPassword.size() ? crypt->decodePassword( qzEncodedPassword ) : "";

    defaultPlayerKeys.insert( Qt::Key_Space,        AC_PAUSE );
    defaultPlayerKeys.insert( Qt::Key_Delete,       AC_SLOW );
    defaultPlayerKeys.insert( Qt::Key_End,          AC_FAST );
    defaultPlayerKeys.insert( Qt::Key_PageDown,     AC_VERYFAST );
    defaultPlayerKeys.insert( Qt::Key_Right,        AC_NEXT );
    defaultPlayerKeys.insert( Qt::Key_Left,         AC_PREV );
    defaultPlayerKeys.insert( Qt::Key_Equal,        AC_SOUNDUP );
    defaultPlayerKeys.insert( Qt::Key_Minus,        AC_SOUNDDOWN );
    defaultPlayerKeys.insert( Qt::Key_1,            AC_SOUND10 );
    defaultPlayerKeys.insert( Qt::Key_2,            AC_SOUND20 );
    defaultPlayerKeys.insert( Qt::Key_3,            AC_SOUND30 );
    defaultPlayerKeys.insert( Qt::Key_4,            AC_SOUND40 );
    defaultPlayerKeys.insert( Qt::Key_5,            AC_SOUND50 );
    defaultPlayerKeys.insert( Qt::Key_6,            AC_SOUND60 );
    defaultPlayerKeys.insert( Qt::Key_7,            AC_SOUND70 );
    defaultPlayerKeys.insert( Qt::Key_8,            AC_SOUND80 );
    defaultPlayerKeys.insert( Qt::Key_9,            AC_SOUND90 );
    defaultPlayerKeys.insert( Qt::Key_0,            AC_SOUND100 );
    defaultPlayerKeys.insert( Qt::Key_Backspace,    AC_MUTE );
    defaultPlayerKeys.insert( Qt::Key_Tab,          AC_SCORES );
    defaultPlayerKeys.insert( Qt::Key_A,            AC_ACC );
    defaultPlayerKeys.insert( Qt::Key_T,            AC_CHAT );
    defaultPlayerKeys.insert( Qt::Key_F11,          AC_SCREENSHOT );
    defaultPlayerKeys.insert( Qt::Key_Return,       AC_REPEATDEMO );

    QByteArray settingsKeyMap = settings->value( "playerKeyBindings" ).toByteArray();

    if ( settingsKeyMap.isEmpty() ) {
        playerKeys = defaultPlayerKeys;
    }
    else {
        QBuffer buf( &settingsKeyMap );
        buf.open( QBuffer::ReadOnly );
        QDataStream in( &buf );

        in >> playerKeys;

        QMapIterator< int, int > it( defaultPlayerKeys );

        while ( it.hasNext() ) {
            it.next();

            if ( !playerKeys.contains( it.key() ) ) {
                playerKeys.insert( it.key(), it.value() );
            }
        }
    }

    QByteArray settingsAlternateKeyMap = settings->value( "playerAlternateKeyBindings" )
                                         .toByteArray();

    if ( !settingsAlternateKeyMap.isEmpty() ) {
        QBuffer buf( &settingsAlternateKeyMap );
        buf.open( QBuffer::ReadOnly );
        QDataStream in( &buf );

        in >> playerAlternateKeys;
    }

    QByteArray customKeyPressActionsMap = settings->value( "playerCustomActions" ).toByteArray();

    if ( !customKeyPressActionsMap.isEmpty() ) {
        QBuffer buf( &customKeyPressActionsMap );
        buf.open( QBuffer::ReadOnly );
        QDataStream in( &buf );

        in >> customKeyPressActions;
    }

    QByteArray customKeyReleaseActionsMap = settings->value( "playerCustomKeyReleaseActions" )
                                            .toByteArray();

    if ( !customKeyReleaseActionsMap.isEmpty() ) {
        QBuffer buf( &customKeyReleaseActionsMap );
        buf.open( QBuffer::ReadOnly );
        QDataStream in( &buf );

        in >> customKeyReleaseActions;
    }

    END

    QString customDirName = getAppDataPath() + "/" +
#ifdef Q_OS_LINUX
    ".qldt/idscript";
#elif defined Q_OS_WIN
    "QLDT/idscript";
#endif

    GROUP( TextEditor )
    VAR( String,        textEditor.fontFamily,          "Liberation Mono" )
    VAR( Int,           textEditor.fontSize,            11 )
    VAR( Int,           textEditor.tabSize,             5 )
    COLOR_VAR(          textEditor.backgroundColor,     QColor( "#ffffff" ) )
    COLOR_VAR(          textEditor.lineNumbersBgColor,  QColor( "#eeeeee" ) )
    COLOR_VAR(          textEditor.lineNumbersColor,    QColor( "#bcbcbc" ) )
    COLOR_VAR(          textEditor.lineHighlightColor,  QColor( "#006af8" ) )
    FVAR(               textEditor.normalText,          formatToByteArray() )
    FVAR(               textEditor.number,              formatToByteArray( 0, 0, 255 ) )
    FVAR(               textEditor.string,              formatToByteArray( 128 ) )
    FVAR(               textEditor.key,                 formatToByteArray( 165, 118 ) )
    FVAR(               textEditor.comment,             formatToByteArray( 128, 128, 128 ) )
    FVAR(               textEditor.action,              formatToByteArray( 128, 128 ) )
    FVAR(               textEditor.cvar,                formatToByteArray( 0, 50, 128, true ) )
    FVAR(               textEditor.command,             formatToByteArray( 0, 30, 80 ) )
    FVAR(               textEditor.shortCommand,        formatToByteArray( 0, 0, 0, true ) )
    FVAR(               textEditor.say,                 formatToByteArray( 255 ) )
    FVAR(               textEditor.semicolon,           formatToByteArray( 0, 128 ) )
    FVAR(               textEditor.preprocessor,        formatToByteArray( 0, 110, 40 ) )
    VAR( Bool,          textEditor.searchMatchCase,     false )
    VAR( Bool,          textEditor.useCustomFiles,      false )
    VAR( String,        textEditor.customFilesPath,     customDirName )
    END

    updatePaths();
}

#define SET( type, var )                            \
    if ( saveDefaults ) {                           \
        var = defaultVars.value( #var ).to##type(); \
    }                                               \
                                                    \
    settings->setValue( #var, var );

void DtConfig::save( bool saveDefaults ) {
    GROUP( Main )
    SET( String,        language )
    SET( Int,           tableAlternateColorFactor )
    SET( Int,           tableSelectionColorFactor )

    SET( Int,           lastRecentMenuIndex )
    SET( BitArray,      mainTableVisibleColumns )
    SET( BitArray,      findFragsTableVisibleColumns )
    SET( BitArray,      findTextTableVisibleColumns )
    SET( BitArray,      findDemoTableVisibleColumns )
    SET( Bool,          headerInfoVisible )
    SET( Bool,          showClanTags )
    SET( StringList,    recentOpenedFiles )
    SET( StringList,    recentOpenedUrls )
    SET( StringList,    recentOpenedConfigs )
    SET( Int,           selectedGame )
    SET( String,        qzHomePath )
    SET( String,        qaHomePath )

    if ( !qzPath.isEmpty() ) {
        SET( String, qzPath )
    }

    if ( !qaPath.isEmpty() ) {
        SET( String, qaPath )
    }

    SET( Bool,          confirmOnDelete )
    SET( Int,           zlibCompressionLevel )
    SET( Bool,          archiveRemovePaths )
    SET( Int,           lastArchiverFormat )
    SET( String,        lastPackPath )
    SET( String,        lastExportPath )
    SET( String,        lastImportPath )
    SET( Bool,          dirTreeAlwaysOpened )
    SET( Int,           mainTableSortColumn )
    SET( Int,           mainTableSortOrder )
    SET( Int,           findFragsTableSortColumn )
    SET( Int,           findFragsTableSortOrder )
    SET( Int,           findTextTableSortColumn )
    SET( Int,           findTextTableSortOrder )
    SET( Int,           findDemoTableSortColumn )
    SET( Int,           findDemoTableSortOrder )
    SET( Bool,          dropDemosToNewDir )
    SET( String,        lastRenameFormat )
    SET( String,        lastOrganizeFormat )
    SET( String,        lastFindTextString )
    SET( Bool,          findTextIgnoreColors )
    SET( Bool,          findTextMatchCase )
    END

    GROUP( Edit )
    SET( Bool,          editorShowGameTime )
    SET( String,        timeDisplayFormat )
    SET( Int,           partsSelectorMode )
    SET( Bool,          showLags )
    END

    GROUP( FindFrags )
    SET( Bool,          frags.tabOpened )
    SET( Int,           frags.segAddTimeBegin )
    SET( Int,           frags.segAddTimeEnd )
    SET( Int,           frags.selectorIndex )
    SET( Int,           frags.cbWeapon )
    SET( Int,           frags.cbGameType )
    SET( Int,           frags.cbMap )
    SET( String,        frags.leMap )
    SET( String,        frags.leMaxTime )
    SET( String,        frags.leMinFrags )
    SET( String,        frags.playerName )
    SET( String,        frags.playerNames )
    SET( Bool,          frags.directHitsOnly )
    SET( Bool,          frags.countTeamKills )
    END

    GROUP( FindText )
    SET( Int,           textSegAddTimeBegin )
    SET( Int,           textSegAddTimeEnd )
    END

    if ( qzSavePassword ) {
        qzEncodedPassword = crypt->encodePassword( qzPassword.toUtf8() );
    }

    GROUP( Player )
    SET( Bool,          repeatPlaylist )
    SET( Bool,          autoPlayNext )
    SET( Bool,          controlPanelAlwaysVisible )
    SET( String,        controlPanelStyle )
    SET( Bool,          draw2dOnSlow )
    SET( Bool,          draw2dOnPause )
    SET( Double,        slowTimescale )
    SET( Double,        fastTimescale )
    SET( Double,        fastestTimescale )
    SET( Bool,          qzForwardMuteSound )
    SET( Bool,          qzPauseMuteSound )
    SET( Bool,          qzFullscreen )
    SET( Bool,          qaFullscreen )
    SET( Int,           qzWindowedMode )
    SET( Int,           qaWindowedMode )
    SET( Int,           qzFullscreenMode )
    SET( Int,           qaFullscreenMode )
    SET( Bool,          qzSoundMute )
    SET( Int,           qzSoundVolume )
    SET( String,        qzGameConfig )
    SET( String,        qaGameConfig )
    SET( Bool,          qzCustomUserAgent )
    SET( String,        qzUserAgent )
    SET( String,        qzEmail )
    SET( ByteArray,     qzEncodedPassword )
    SET( Bool,          qzSavePassword )
    SET( Bool,          qzKeyboardFilter )
    SET( Bool,          qzRemoveAdvertDelay )
    SET( Bool,          qzPreventSettingsCaching )
    SET( String,        otherAppTitle )
    SET( Bool,          otherAppDm68 )
    SET( Bool,          otherAppDm73 )
    SET( Bool,          otherAppDoubleClick )
    SET( Bool,          otherAppMenu )
    SET( Bool,          otherAppPreview )
    SET( String,        otherAppPath )
    SET( String,        otherAppCmdLine )
    SET( Bool,          otherAppFromDemos )
    END

    saveTextEditorSettings( saveDefaults );
    saveKeyBindingsSettings( saveDefaults );
    updatePaths();
}

#define FSET( var )                                                         \
    if ( saveDefaults ) {                                                   \
        var = byteArrayToFormat( defaultVars.value( #var ).toByteArray() ); \
    }                                                                       \
                                                                            \
    settings->setValue( #var, textCharFormatToByteArray( var ) );

#define COLOR_SET( var )                                                    \
    if ( saveDefaults ) {                                                   \
        var = defaultVars.value( #var ).value< QColor >();                  \
    }                                                                       \
                                                                            \
    settings->setValue( #var, var );

void DtConfig::saveTextEditorSettings( bool saveDefaults ) {
    GROUP( TextEditor )
    SET( String,    textEditor.fontFamily )
    SET( Int,       textEditor.fontSize )
    SET( Int,       textEditor.tabSize )
    COLOR_SET(      textEditor.backgroundColor )
    COLOR_SET(      textEditor.lineNumbersBgColor )
    COLOR_SET(      textEditor.lineNumbersColor )
    COLOR_SET(      textEditor.lineHighlightColor )
    FSET(           textEditor.normalText )
    FSET(           textEditor.number )
    FSET(           textEditor.string )
    FSET(           textEditor.key )
    FSET(           textEditor.comment )
    FSET(           textEditor.action )
    FSET(           textEditor.cvar )
    FSET(           textEditor.command )
    FSET(           textEditor.shortCommand )
    FSET(           textEditor.say )
    FSET(           textEditor.semicolon )
    FSET(           textEditor.preprocessor )
    SET( Bool,      textEditor.searchMatchCase )
    SET( Bool,      textEditor.useCustomFiles )
    SET( String,    textEditor.customFilesPath )
    END
}

void DtConfig::saveKeyBindingsSettings( bool saveDefaults ) {
    if ( saveDefaults ) {
        playerKeys = defaultPlayerKeys;
        playerAlternateKeys.clear();
        customKeyPressActions.clear();
    }

    GROUP( Player )

    QByteArray settingsKeyMap;
    QBuffer buf( &settingsKeyMap );
    buf.open( QBuffer::WriteOnly );
    QDataStream out( &buf );

    out << playerKeys;
    settings->setValue( "playerKeyBindings", settingsKeyMap );

    QByteArray settingsAlternateKeyMap;
    buf.close();
    buf.setBuffer( &settingsAlternateKeyMap );
    buf.open( QBuffer::WriteOnly );

    out << playerAlternateKeys;
    settings->setValue( "playerAlternateKeyBindings", settingsAlternateKeyMap );

    QByteArray customKeyPressActionsMap;
    buf.close();
    buf.setBuffer( &customKeyPressActionsMap );
    buf.open( QBuffer::WriteOnly );

    out << customKeyPressActions;
    settings->setValue( "playerCustomActions",  customKeyPressActionsMap );

    QByteArray customKeyReleaseActionsMap;
    buf.close();
    buf.setBuffer( &customKeyReleaseActionsMap );
    buf.open( QBuffer::WriteOnly );

    out << customKeyReleaseActions;
    settings->setValue( "playerCustomKeyReleaseActions",  customKeyReleaseActionsMap );

    SET( Int, qzSoundVolumeStep )
    END
}

QString DtConfig::getDefaultLanguage() {
    QString systemLanguage = QLocale::languageToString( QLocale::system().language() );

    if ( systemLanguage == "Chinese" ) {
        QString systemCountry = QLocale::countryToString( QLocale::system().country() );

        if ( systemCountry == "China" ) {
            return "Chinese Simplified";
        }
        else {
            return "Chinese Traditional";
        }
    }

    foreach ( const QString& languageName, dtdata::defaultLanguageNames ) {
        if ( languageName.startsWith( systemLanguage ) ) {
            return languageName;
        }
    }

    return "English";
}

QByteArray DtConfig::formatToByteArray( unsigned char red, unsigned char green, unsigned char blue,
                                        bool bold, bool italic, bool underline ) {
    charFormat format;

    format.red = red;
    format.green = green;
    format.blue = blue;
    format.bold = bold;
    format.italic = italic;
    format.underline = underline;

    QByteArray array( sizeof( format ), 0 );
    memcpy( array.data(), &format, sizeof( format ) );

    return array;
}

QByteArray DtConfig::textCharFormatToByteArray( const QTextCharFormat& textFormat ) {
    QColor color = textFormat.foreground().color();
    unsigned char red = static_cast< unsigned char >( color.red() );
    unsigned char green = static_cast< unsigned char >( color.green() );
    unsigned char blue = static_cast< unsigned char >( color.blue() );
    bool bold = ( textFormat.fontWeight() == QFont::Bold );
    bool italic = textFormat.fontItalic();
    bool underline = textFormat.fontUnderline();

    return formatToByteArray( red, green, blue, bold, italic, underline );
}

QTextCharFormat DtConfig::byteArrayToFormat( const QByteArray& array ) {
    charFormat format;
    memcpy( &format, array.data(), sizeof( format ) );

    QTextCharFormat textCharFormat;
    textCharFormat.setForeground( QBrush( QColor( format.red, format.green, format.blue, 255 ) ) );
    textCharFormat.setFontWeight( format.bold ? QFont::Bold : QFont::Normal );
    textCharFormat.setFontItalic( format.italic );
    textCharFormat.setFontUnderline( format.underline );

    return textCharFormat;
}

void DtConfig::defaults() {
    save( true );
}

void DtConfig::textEditorDefaults() {
    saveTextEditorSettings( true );
}

void DtConfig::playerBindingsDefaults() {
    saveKeyBindingsSettings( true );
}

void DtConfig::updatePaths() {
    qzBasePath.clear();
    qaBasePath.clear();
    qzDemoPath.clear();
    qaDemoPath.clear();

    if( !qzHomePath.isEmpty() ) {
        qzBasePath = qzHomePath + "/" + baseSubDir;
        qzDemoPath = qzBasePath + "/" + demoSubDir;
    }

    if( !qaHomePath.isEmpty() ) {
        qaBasePath = qaHomePath + "/" + baseSubDir;
        qaDemoPath = qaBasePath + "/" + demoSubDir;
    }
}

void DtConfig::setSelectedGame( int game ) {
    selectedGame = game;
}

int DtConfig::getSelectedGame() const {
    return selectedGame;
}

void DtConfig::setQzPath( const QString& path ) {
    qzPath = path;
}

const QString& DtConfig::getQzPath() const {
    return qzPath;
}

void DtConfig::setQaPath( const QString& path ) {
    qaPath = path;
}

const QString& DtConfig::getQaPath() const {
    return qaPath;
}

void DtConfig::setQzHomePath( const QString& path ) {
    qzHomePath = path;
    updatePaths();
}

const QString& DtConfig::getQzHomePath() const {
    return qzHomePath;
}

void DtConfig::setQaHomePath( const QString& path ) {
    qaHomePath = path;
    updatePaths();
}

const QString& DtConfig::getQaHomePath() const {
    return qaHomePath;
}

const QString& DtConfig::getQzBasePath() const {
    return qzBasePath;
}

const QString& DtConfig::getQaBasePath() const {
    return qaBasePath;
}

const QString& DtConfig::getQzDemoPath() const {
    return qzDemoPath;
}

const QString& DtConfig::getQaDemoPath() const {
    return qaDemoPath;
}

void DtConfig::setQzUserAgent( const QString& ua ) {
    qzUserAgent = ua;
}

QString DtConfig::getQzUserAgent() const {
    if ( !qzCustomUserAgent ) {
        return firefoxUserAgent;
    }
    else {
        return qzUserAgent;
    }
}

void DtConfig::setQzLoginData( const QString& email, const QString& pass ) {
    qzEmail = email;
    qzPassword = pass;
}

const QString& DtConfig::getQzEmail() const {
    return qzEmail;
}

const QString& DtConfig::getQzPass() const {
    return qzPassword;
}

QString DtConfig::getQzPluginPath() {
    QString pluginPath = "";

#ifdef Q_OS_LINUX
    /* try to get the plugin path from the firefox extension settings */

    QString profilesPath = QDir::homePath() + firefox3ProfilesPath;
    QSettings profiles( profilesPath + "/profiles1.ini", QSettings::IniFormat );
    QStringList profileGroups = profiles.childGroups();

    bool defUserNum = false;
    int i = 0;

    for ( ; i < profileGroups.size(); ++i ) {
        QString groupName = profileGroups.at( i );

        if ( groupName == QString( "Profile%1" ).arg( i ) ) {
            if ( profiles.value( groupName + "/Default", false ).toBool() ) {
                defUserNum = true;
                break;
            }
        }
    }

    if ( !defUserNum ) {
        i = 0;
    }

    QString userPath = profiles.value( QString( "Profile%1/Path" ).arg( i ), "" ).toString();

    if ( userPath.isEmpty() ) {
        return "";
    }

    QSettings extensions( QString( "%1/%2/extensions.ini" ).arg( profilesPath, userPath ),
                          QSettings::IniFormat );
    extensions.beginGroup( "ExtensionDirs" );

    QStringList extensionDirs = extensions.childKeys();
    const QString qlPluginExtName = "quakeliveplugin@idsoftware.com";

    for ( i = 0; i < extensionDirs.size(); ++i ) {
        QString extDir = extensions.value( extensionDirs.at( i ), "" ).toString();

        if ( extDir.contains( qlPluginExtName ) ) {
            pluginPath = extDir + "/plugins/" + pluginFileName;
        }
    }

    extensions.endGroup();

#elif defined Q_OS_WIN
    QSettings mozillaPlugins( QSettings::SystemScope, "MozillaPlugins", "@idsoftware.com/QuakeLive" );

    pluginPath = mozillaPlugins.value( "Path", "" ).toString();

    if ( pluginPath.isEmpty() || !QFile::exists( pluginPath ) ) {
        pluginPath = QDir::rootPath() + "Documents and Settings/All Users.WINDOWS/"
                                        "Application Data/id Software/QuakeLive/npquakezero.dll";

        if ( !QFile::exists( pluginPath ) ) {
            pluginPath.clear();
        }
    }
    else {
        pluginPath.replace( "\\", "/" );
    }
#endif

    return pluginPath;
}

QString DtConfig::getAppDataPath() {
#ifdef Q_OS_LINUX
    return QDir::homePath();
#elif defined Q_OS_WIN
    QSettings shellFolders( "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\"
                            "Explorer\\Shell Folders", QSettings::NativeFormat );

    QString homePath = shellFolders.value( "AppData", "" ).toString();
    homePath.replace( "\\", "/" );

    return homePath;
#endif
}

QString DtConfig::getQzDefaultHomePath() {
#ifdef Q_OS_LINUX
    return QDir::homePath() + "/.quakelive/quakelive/home";
#elif defined Q_OS_WIN
    QString homePath = getAppDataPath();

    homePath.replace( "/Roaming", "" );

    if ( QSysInfo::windowsVersion() == QSysInfo::WV_VISTA ||
         QSysInfo::windowsVersion() == QSysInfo::WV_WINDOWS7 )
    {
        homePath.append( "/LocalLow" );
    }

    homePath.append( "/id Software/quakelive/home" );

    if ( !QDir( homePath ).exists() ) {
        QString defaultCharset = QString( "CP-%1" ).arg( GetACP() );
        QTextCodec* pathCodec = QTextCodec::codecForName( defaultCharset.toAscii() );
        QTextDecoder* pathDecoder = pathCodec->makeDecoder();

        QString reencodedPath = pathDecoder->toUnicode( homePath.toAscii() );

        delete pathDecoder;

        if ( QDir( reencodedPath ).exists() ) {
            homePath = reencodedPath;
        }
    }

    return homePath;
#endif
}
