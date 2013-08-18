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

#include "ConfigEditorData.h"

#include <QObject>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QApplication>
#include <QTranslator>
#include <QTextCodec>
#include <QFontDatabase>
#include <QDir>

DtConfig dtdata::config;
DtGameConfigEditor* dtdata::ceMainWindow;
QString dtdata::defaultConfigFormat;
QStringList dtdata::defaultConfigFilters;
QStringList dtdata::defaultLanguageNames;
QHash< QString, DtDemoProto > dtdata::demoProtos;
static QHash< QString, QString > langLocales;
QStringList dtdata::scriptWords;

void dtdata::initializeData() {
    QFontDatabase::addApplicationFont( ":/res/fonts/LiberationSans-Regular.ttf" );
    QFontDatabase::addApplicationFont( ":/res/fonts/LiberationSans-Bold.ttf" );
    QFontDatabase::addApplicationFont( ":/res/fonts/LiberationMono-BoldItalic.ttf" );
    QFontDatabase::addApplicationFont( ":/res/fonts/LiberationMono-Bold.ttf" );
    QFontDatabase::addApplicationFont( ":/res/fonts/LiberationMono-Italic.ttf" );
    QFontDatabase::addApplicationFont( ":/res/fonts/LiberationMono-Regular.ttf" );

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

    config.loadDefaults();

#ifdef Q_OS_LINUX
    QString translatorPath = "/usr/share/qt4/translations/qt_";
#elif defined Q_OS_WIN
    QString translatorPath = QCoreApplication::applicationDirPath() + "/lang/qt/qt_";
#endif

    QTranslator* qtTranslator = new QTranslator;
    qtTranslator->load( translatorPath + langLocales.value( config.language, "en" ) );
    qApp->installTranslator( qtTranslator );

    QTranslator* translator = new QTranslator;
    translator->load( ":res/lang/ce_" + langLocales.value( config.language, "en" ) );
    qApp->installTranslator( translator );

    icons = new DtImages;

    demoProtos.insert( "dm_68", Q3_68 );
    demoProtos.insert( "dm_73", QZ_73 );

    defaultConfigFormat = "Quake Scripts (";
    defaultConfigFilters << ".cfg" << ".menu" << ".h";

    foreach ( const QString& filter, defaultConfigFilters ) {
        defaultConfigFormat.append( "*" ).append( filter ).append( " " );
    }

    defaultConfigFormat.append( ")" );
}

bool dtdata::isConfigFile( const QString& format ) {
    return ( format == "cfg"    ||
             format == "menu"   ||
             format == "h" );
}

QString dtdata::getStyle( const char* cName ) {
    QFile styleFile( ":/res/style/" + QString( cName ) + ".qss" );

    styleFile.open( QFile::ReadOnly );
    QString styleStr = QLatin1String( styleFile.readAll() );
    styleFile.close();

    return styleStr;
}

DtImages* dtdata::icons;

DtImages::DtImages() {
    rIcons[ I_QZ_SMALL ].addFile( ":/res/qlicon.png" );
    rIcons[ I_Q3_SMALL ].addFile( ":/res/q3icon.png" );
    rIcons[ I_COPY ].addFile( ":/res/editcopy.png" );
}

const QIcon& DtImages::getIcon( int index ) {
    return rIcons[ index ];
}

