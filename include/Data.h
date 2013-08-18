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

#ifndef DTDATA_H
#define DTDATA_H

#include "Config.h"
#include "DemoDatabase.h"
#include "QuakeEnums.h"

#include <QIcon>
#include <QList>
#include <QPointer>

class DtDemo;
class DtMainTabWidget;
class DtDemoTable;
class DtMainTable;
class DtScanWidget;
class DtMainWindow;
class QPixmap;
class QTableWidgetItem;
class QDrag;
class QFileInfo;

const float MiB = 1048576.f;
const float GiB = 1073741824.f;
const char chatEscapeChar = '\x19';

enum DtDemoProto {
    Q_UNKNOWN = 0,
    Q3_68 = 68,
    QZ_73 = 73
};

enum iconImages {
    I_QZ_SMALL,
    I_Q3_SMALL,
    I_OTHER_SMALL,
    I_PLAYDEMO,
    I_MOVE,
    I_COPY,
    I_BROKEN
};

class DtImages {
public:
    DtImages();
    const QIcon& getIcon( int index );
private:
    QPixmap* iconsSheet;
    QIcon rIcons[ 7 ];
};

template < class FunctionPtr, class T >
FunctionPtr pointer_cast( T objectPtr ) {
    union {
        T obj;
        FunctionPtr func;
    } var;

    var.obj = objectPtr;
    return var.func;
}

namespace dtdata {

extern DtConfig config;
extern DtDemoDatabase demoDb;
extern QHash< QString, DtDemo* > openedDemos;
extern QMultiMap< int, QString > weaponNames;
extern QMultiMap< int, QString > gameTypeNames;
extern QMultiMap< int, QString > gameTypeNamesQa;
extern QMultiMap< int, QString > gameTypeNamesCpma;
extern bool fullscreenGameRunning;
extern QStringList demoNameFilters;
extern QString currentWorkingDir;
extern QHash< QString, DtDemoProto > demoProtos;
extern QString defaultTmpDirName;
extern QString defaultNewDirName;
extern QString defaultDateFormat;
extern QString defaultTimeFormat;
extern QString defaultRenameFormat;
extern QString defaultConfigFormat;
extern QStringList defaultConfigFilters;
extern QStringList defaultMainTableColumnNames;
extern QStringList defaultFindFragsTableColumnNames;
extern QStringList defaultFindTextTableColumnNames;
extern QStringList defaultFindDemoTableColumnNames;
extern QStringList defaultLanguageNames;

enum stringType {
    WEAP_NAME,
    GAME_TYPE
};

enum playDemoRequestSources {
    RS_DBLCLICK,
    RS_CONTEXTMENU,
    RS_CONTEXTMENU_OTHER,
    RS_PREVIEW
};

extern playDemoRequestSources playDemoRequestSource;

extern DtMainTable* mainDemoTable;
extern DtMainTabWidget* mainTabWidget;
extern DtScanWidget* currentScanWidget;
extern QPointer< DtMainWindow > dtMainWindow;
extern QPointer< QDrag > demoDrag;
extern DtDemoTable* currentPlayDemoTable;

bool initializeData();
const QString& getWeaponName( int key, bool shortValue = false );
const QString& getGameTypeName( int proto, int mod, int key, bool shortValue = false );
const QString& getNameString( int proto, stringType mapid, int key, bool shortValue, int mod );
QString getModifiedName( const QString& fileName );
QString getModifiedName( const QFileInfo& fileInfo );
bool acceptedFileFormat( const QString& format );
bool isConfigFile( const QString& format );
void createMainWindow( const QString& arg );
bool isAcceptedEncoding( const QString& str );
bool isArchiveFile( const QString& format );
QString getStyle( QString cName );
void cleanStringColors( QString& str );
bool openInOtherApplication();

extern DtImages* icons;

void deleteUnusedDemos();

}

//#define MSG_LOG

#endif // DTDATA_H
