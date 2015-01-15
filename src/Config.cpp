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
    settings( new QSettings( QSettings::NativeFormat, QSettings::UserScope, "QLDT", "settings" ) )
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
    QString qzExePath = getQzDefaultExePath();
    QString qzDefaultFSBasePath = getQzDefaultFSBasePath();

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
    VAR( String,        qzFSBasePath,                 qzDefaultFSBasePath )
    VAR( String,        qzHomePath,                   qzDefaultFSBasePath + "/home" )
    VAR( String,        qaHomePath,                   "" )
    VAR( String,        qzPath,                       qzExePath )

    VAR( String,        qaPath,                       "" )
    VAR( Bool,          dirTreeAlwaysOpened,          true )
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
    VAR( String,        lastDuelRenameFormat,         "<FC><FP>-vs-<SC><SP>-<M>-<D>" )
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
    VAR( Bool,          qzFullscreen,                 false )
    VAR( Bool,          qaFullscreen,                 false )
    VAR( Int,           qzWindowedMode,               QZ_1024x640 )
    VAR( Int,           qaWindowedMode,               QA_800x600 )
    VAR( Int,           qzFullscreenMode,             QZ_DESKTOP )
    VAR( Int,           qaFullscreenMode,             QA_800x600 )
    VAR( String,        qzGameConfig,                 "" )
    VAR( String,        qaGameConfig,                 "" )
    VAR( String,        otherAppTitle,                QObject::tr( "Other" ) )
    VAR( Bool,          otherAppDm68,                 false )
    VAR( Bool,          otherAppDm73,                 true )
    VAR( Bool,          otherAppDoubleClick,          false )
    VAR( Bool,          otherAppMenu,                 false )
    VAR( Bool,          otherAppPreview,              false )
    VAR( String,        otherAppPath,                 "" )
    VAR( String,        otherAppCmdLine,              "+demo \"%demoName\" +set nextdemo quit" )
    VAR( Bool,          otherAppFromDemos,            true )

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
    SET( String,        qzFSBasePath )
    SET( String,        qzHomePath )
    SET( String,        qaHomePath )
    SET( String,	qzPath )
    SET( String,	qaPath )

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
    SET( String,        lastDuelRenameFormat )
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

    GROUP( Player )
    SET( Bool,          qzFullscreen )
    SET( Bool,          qaFullscreen )
    SET( Int,           qzWindowedMode )
    SET( Int,           qaWindowedMode )
    SET( Int,           qzFullscreenMode )
    SET( Int,           qaFullscreenMode )
    SET( String,        qzGameConfig )
    SET( String,        qaGameConfig )
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

void DtConfig::updatePaths() {
    qzHomePath.clear();
    qzBasePath.clear();
    qaBasePath.clear();
    qzDemoPath.clear();
    qaDemoPath.clear();

    if( !qzFSBasePath.isEmpty() ) {
        qzHomePath = qzFSBasePath + "/home";
        if ( qzFSBasePath.contains( "Steam/SteamApps" ) ) {
            QDir steamdir = QDir( qzFSBasePath );
            foreach( QString homePath, steamdir.entryList() ) {
                homePath = qzFSBasePath + "/" + homePath;
                if ( QDir( homePath + "/baseq3/demos" ).exists() ) {
                    qzHomePath = homePath;
                    break;
                }
            }
        }
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

void DtConfig::setQzFSBasePath( const QString& path ) {
    qzFSBasePath = path;
    updatePaths();
}

const QString& DtConfig::getQzFSBasePath() const {
    return qzFSBasePath;
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

const QString& DtConfig::getQzHomePath() const {
    return qzHomePath;
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

QString DtConfig::getQzDefaultFSBasePath() {
#ifdef Q_OS_LINUX
    // Hackish way but should work on default installs without actually running WINE
    // Windows 7 emulation
    QString basePath = QDir::homePath() + "/.wine/drive_c/users/" + qgetenv( "USER" ) + "/Local Settings/LocalLow/id Software/quakelive";
    if ( !QDir( basePath ).exists() ) {
        // Windows XP emulation
        basePath = QDir::homePath() + "/.wine/drive_c/users/" + qgetenv( "USER" ) + "/Application Data/id Software/quakelive";
        if ( !QDir( basePath ).exists() ) {
            return "";
        }
    }
    return basePath;
#elif defined Q_OS_WIN
    QStringList knownlocs, steamlocs;
    QString homePath;

    steamlocs << qgetenv( "PROGRAMFILES(X86)" ).replace( "\\", "/" ) + "/Steam/SteamApps/common/Quake Live"
              << "C:/Steam/SteamApps/common/Quake Live";
    foreach ( homePath, steamlocs ) {
        QDir onedir = QDir( homePath );
        if ( QDir( homePath ).exists() ) {
            return homePath;
        }
    }

    knownlocs << qgetenv( "USERPROFILE" ).replace( "\\", "/" ) + "/AppData/LocalLow/id Software/quakelive"
              << qgetenv( "USERPROFILE" ).replace( "\\", "/" ) + "/Local Settings/LocalLow/id Software/quakelive"
              << qgetenv( "APPDATA" ).replace( "\\", "/" ) + "/id Software/quakelive";

    foreach ( homePath, knownlocs ) {
        homePath.replace( "/Roaming", "" );

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

        if ( QFile( homePath ).exists() ) {
            return homePath;
        }
    }

    return "";
#endif
}

QString DtConfig::getQzDefaultExePath() {
#ifdef Q_OS_LINUX
    QString exePath = QDir::homePath() + "/.wine/drive_c/users/" + qgetenv( "USER" ) + "/Local Settings/Application Data/id Software/quakelive/quakelive.exe";
    if ( !QFile( exePath ).exists() ) {
        exePath = QDir::homePath() + "/.wine/drive_c/users/" + qgetenv( "USER" ) + "/Application Data/id Software/quakelive/quakelive.exe";
        if ( !QFile( exePath ).exists() ) {
            return "";
        }
    }
    return exePath;
#elif defined Q_OS_WIN
    QStringList knownlocs;
    QString exePath;

    knownlocs << qgetenv( "PROGRAMFILES(X86)" ).replace( "\\", "/" ) + "/Steam/SteamApps/common/Quake Live/quakelive_steam.exe"
              << "C:/Steam/SteamApps/common/Quake Live/quakelive_steam.exe"
              << qgetenv( "USERPROFILE" ).replace( "\\", "/" ) + "/AppData/Local/id Software/quakelive/quakelive.exe"
              << qgetenv( "USERPROFILE" ).replace( "\\", "/" ) + "/Local Settings/Application Data/id Software/quakelive/quakelive.exe"
              << qgetenv( "USERPROFILE" ).replace( "\\", "/" ) + "/Application Data/id Software/quakelive/quakelive.exe"
              << qgetenv( "APPDATA" ).replace( "\\", "/" ) + "/Application Data/id Software/quakelive/quakelive.exe";

    foreach ( exePath, knownlocs ) {
        exePath.replace( "/Roaming", "" );

        if ( !QFile( exePath ).exists() ) {
            QString defaultCharset = QString( "CP-%1" ).arg( GetACP() );
            QTextCodec* pathCodec = QTextCodec::codecForName( defaultCharset.toAscii() );
            QTextDecoder* pathDecoder = pathCodec->makeDecoder();

            QString reencodedPath = pathDecoder->toUnicode( exePath.toAscii() );

            delete pathDecoder;

            if ( QFile( reencodedPath ).exists() ) {
                exePath = reencodedPath;
            }
        }

        if ( QFile( exePath ).exists() ) {
            return exePath;
        }
    }

    return "";
#endif
}
