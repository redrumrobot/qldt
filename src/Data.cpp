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
#include "Demo.h"
#include "MainWindow.h"
#include "AbstractProtocol.h"

#include <QObject>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QApplication>
#include <QTranslator>
#include <QTextCodec>
#include <QFontDatabase>
#include <QDir>
#include <QMessageBox>

DtConfig dtdata::config;
DtDemoDatabase dtdata::demoDb;
QHash< QString, DtDemo* > dtdata::openedDemos;
QMultiMap< int, QString > dtdata::weaponNames;
QMultiMap< int, QString > dtdata::gameTypeNames;
QMultiMap< int, QString > dtdata::gameTypeNamesQa;
QMultiMap< int, QString > dtdata::gameTypeNamesCpma;
QHash< QString, DtDemoProto > dtdata::demoProtos;
static QHash< QString, QString > langLocales;

DtMainTable* dtdata::mainDemoTable;
QPointer< DtMainWindow > dtdata::dtMainWindow;
DtMainTabWidget* dtdata::mainTabWidget;
DtScanWidget* dtdata::currentScanWidget;
QPointer< QDrag > dtdata::demoDrag;
DtDemoTable* dtdata::currentPlayDemoTable;

QStringList dtdata::demoNameFilters;
QString dtdata::currentWorkingDir;
QString dtdata::defaultTmpDirName;
QString dtdata::defaultNewDirName;
QString dtdata::defaultDateFormat;
QString dtdata::defaultTimeFormat;
QString dtdata::defaultRenameFormat;
QString dtdata::defaultConfigFormat;
QStringList dtdata::defaultConfigFilters;
QStringList dtdata::defaultMainTableColumnNames;
QStringList dtdata::defaultFindFragsTableColumnNames;
QStringList dtdata::defaultFindTextTableColumnNames;
QStringList dtdata::defaultFindDemoTableColumnNames;
QStringList dtdata::defaultLanguageNames;
dtdata::playDemoRequestSources dtdata::playDemoRequestSource;

bool dtdata::fullscreenGameRunning;

const QString& dtdata::getNameString( int proto, stringType mapid, int key, bool shortValue, int mod ) {
    QMultiMap< int, QString >::const_iterator it;

    if ( mapid == WEAP_NAME ) {
        it = weaponNames.find( key );
    }
    else {
        if ( proto == QZ_73 ) {
            it = gameTypeNames.find( key );
        }
        else {
            if ( mod == MOD_CPMA ) {
                it = gameTypeNamesCpma.find( key );
            }
            else {
                it = gameTypeNamesQa.find( key );
            }
        }
    }

    if ( key < 0 ) {
        return it.value();
    }

    return shortValue ? it.value() : ( ++it ).value();
}

const QString& dtdata::getWeaponName( int key, bool shortValue ) {
    return getNameString( 0, WEAP_NAME, key, shortValue, 0 );
}

const QString& dtdata::getGameTypeName( int proto, int mod, int key, bool shortValue ) {
    return getNameString( proto, GAME_TYPE, key, shortValue, mod );
}

bool dtdata::initializeData() {
    DtAbstractProtocol::initializeHuffman();

    QFontDatabase::addApplicationFont( ":/res/fonts/LiberationSans-Regular.ttf" );
    QFontDatabase::addApplicationFont( ":/res/fonts/LiberationSans-Bold.ttf" );

    QTextCodec* codec = QTextCodec::codecForName( "UTF-8" );
    QTextCodec::setCodecForCStrings( codec );
    QTextCodec::setCodecForTr( codec );

    defaultLanguageNames << "Arabic" << "Catalan" << "Chinese Simplified" << "Chinese Traditional" <<
            "Czech" << "Danish" << "English" << "French"<< "German" << "Hebrew" << "Japanese" <<
            "Polish" << "Portuguese" << "Russian" << "Slovak" << "Slovenian" << "Spanish" <<
            "Swedish" << "Ukrainian";

    langLocales.insert( "Arabic", "ar" );
    langLocales.insert( "Catalan", "ca" );
    langLocales.insert( "Chinese Simplified", "zh_CN" );
    langLocales.insert( "Chinese Traditional", "zh_TW" );
    langLocales.insert( "Czech", "cs" );
    langLocales.insert( "Danish", "da" );
    langLocales.insert( "German", "de" );
    langLocales.insert( "Spanish", "es" );
    langLocales.insert( "French", "fr" );
    langLocales.insert( "Hebrew", "he" );
    langLocales.insert( "Japanese", "ja_JP" );
    langLocales.insert( "Polish", "pl" );
    langLocales.insert( "Portuguese", "pt" );
    langLocales.insert( "Russian", "ru" );
    langLocales.insert( "Slovak", "sk" );
    langLocales.insert( "Slovenian", "sl" );
    langLocales.insert( "Swedish", "sv" );
    langLocales.insert( "Ukrainian", "uk" );

    config.loadLanguage();

    if ( !demoDb.open() ) {
        QMessageBox::critical( 0, "Error", "Couldn't open database" );
        return false;
    }

#ifdef Q_OS_LINUX
    QString translatorPath = "/usr/share/qt4/translations/qt_";
#elif defined Q_OS_WIN
    QString translatorPath = QCoreApplication::applicationDirPath() + "/lang/qt/qt_";
#endif

    QTranslator* qtTranslator = new QTranslator;
    qtTranslator->load( translatorPath + langLocales.value( config.language, "en" ) );
    qApp->installTranslator( qtTranslator );

    QTranslator* translator = new QTranslator;
    translator->load( ":res/lang/" + langLocales.value( config.language, "en" ) );
    qApp->installTranslator( translator );

    config.loadDefaults();

    icons = new DtImages;

    demoProtos.insert( "dm_68", Q3_68 );
    demoProtos.insert( "dm_73", QZ_73 );
    demoProtos.insert( "dm_90", QZ_73 );

    QHashIterator< QString, DtDemoProto > it( demoProtos );

    while ( it.hasNext() ) {
        it.next();
        demoNameFilters << QString( "*.%1" ).arg( it.key() );
    }

    defaultTmpDirName = "_demotools_tmp";
    defaultNewDirName = "_demotools_new";
    defaultDateFormat = "dd.MM.yy hh:mm";
    defaultTimeFormat = "h:m:ss";
    defaultRenameFormat = "<T>-<P>-<M>-<D>";

    defaultConfigFormat = "Quake Scripts (";
    defaultConfigFilters << ".cfg" << ".menu" << ".h";

    foreach ( const QString& filter, defaultConfigFilters ) {
        defaultConfigFormat.append( "*" ).append( filter ).append( " " );
    }

    defaultConfigFormat.append( ")" );

    fullscreenGameRunning = false;
    currentPlayDemoTable = 0;

    return true;
}

void dtdata::createMainWindow( const QString& arg ) {
    if ( dtMainWindow ) {
        return;
    }

    dtMainWindow = new DtMainWindow;

    if ( arg.isEmpty() ) {
        dtMainWindow->updateMainTable();
    }

    QObject::connect( qApp, SIGNAL( messageReceived( const QString& ) ),
                      dtMainWindow, SLOT( applicationMessage( const QString& ) ) );

    dtMainWindow->show();
    dtMainWindow->applicationMessage( arg );
}

bool dtdata::isArchiveFile( const QString& format ) {
    return format == "zip" || format == "rar" || format == "7z";
}

bool dtdata::acceptedFileFormat( const QString& format ) {
   return ( demoProtos.contains( format )   ||
            isArchiveFile( format )         ||
            isConfigFile( format ) );
}

bool dtdata::isConfigFile( const QString& format ) {
    return ( format == "cfg"    ||
             format == "menu"   ||
             format == "h" );
}

QString dtdata::getModifiedName( const QString& fileName ) {
    QFileInfo fInfo( fileName );
    return getModifiedName( fInfo );
}

QString dtdata::getModifiedName( const QFileInfo& fileInfo ) {
    return QString( "%1::%2" ).arg( QString::number( fileInfo.lastModified().toTime_t() ),
                                    fileInfo.absoluteFilePath() );
}

QString dtdata::getStyle( QString cName ) {
    QFile styleFile( ":/res/style/" + cName + ".qss" );

    styleFile.open( QFile::ReadOnly );
    QString styleStr = QLatin1String( styleFile.readAll() );
    styleFile.close();

    return styleStr;
}

const int cachedDemosLimit = 30000;

void dtdata::deleteUnusedDemos() {
    if ( openedDemos.size() <= cachedDemosLimit ) {
        return;
    }

    QHashIterator< QString, DtDemo* > it( openedDemos );

    while ( it.hasNext() ) {
        it.next();
        DtDemo* demo = it.value();

        if ( demo->referenceCount == 0 ) {
            openedDemos.remove( it.key() );
            delete demo;

            if ( openedDemos.size() <= cachedDemosLimit ) {
                return;
            }
        }
    }
}

bool dtdata::isAcceptedEncoding( const QString& str ) {
    QRegExp enc( "^[\\041-\\0175]+$", Qt::CaseInsensitive );

    return enc.exactMatch( str );
}

void dtdata::cleanStringColors( QString& str ) {
    QString tmp;

    for ( int i = 0; i < str.size(); ++i ) {
        if ( str.at( i ) != '^' ) {
            tmp += str.at( i );
        }
        else {
            ++i;
        }
    }

    str = tmp;
}

bool dtdata::openInOtherApplication() {
    return ( ( playDemoRequestSource == RS_DBLCLICK && config.otherAppDoubleClick )   ||
             ( playDemoRequestSource == RS_CONTEXTMENU_OTHER && config.otherAppMenu ) ||
             ( playDemoRequestSource == RS_PREVIEW && config.otherAppPreview ) );
}

DtImages* dtdata::icons;

DtImages::DtImages() {
    rIcons[ I_QZ_SMALL ].addFile( ":/res/qlicon.png" );
    rIcons[ I_Q3_SMALL ].addFile( ":/res/q3icon.png" );
    rIcons[ I_OTHER_SMALL ].addFile( ":/res/other_app.png" );
    rIcons[ I_PLAYDEMO ].addFile( ":/res/playdemo.png" );
    rIcons[ I_MOVE ].addFile( ":/res/editmove.png" );
    rIcons[ I_COPY ].addFile( ":/res/editcopy.png" );
    rIcons[ I_BROKEN ].addFile( ":/res/broken-demo.png" );
}

const QIcon& DtImages::getIcon( int index ) {
    return rIcons[ index ];
}

