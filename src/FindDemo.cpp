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

#include "FindDemo.h"
#include "Data.h"
#include "QuakeEnums.h"
#include "MainTabWidget.h"
#include "ScanWidget.h"
#include "FindDemoTable.h"
#include "DemoTable.h"
#include "MainTable.h"
#include "Demo.h"
#include "ProgressDialog.h"

#include <QApplication>
#include <QMutex>
#include <QFileInfo>
#include <QDateTime>

using namespace dtdata;

DtFindDemo::DtFindDemo(  QWidget* parent ) :
    scanMutex( new QMutex ),
    progressDialog( new DtProgressDialog( tr( "Searching" ), parent ) )
{
    connect( progressDialog, SIGNAL( buttonClicked() ), this, SLOT( stop() ) );
    connect( this, SIGNAL( newScanStarted() ), this, SLOT( updateProgress() ) );
}

DtFindDemo::~DtFindDemo() {
    delete scanMutex;
}

static bool checkString( QRegExp& reg, const QString& pattern, const QString& str ) {
    if ( !pattern.isEmpty() ) {
        reg.setPattern( pattern );

        if ( !reg.exactMatch( str ) ) {
            return false;
        }
    }

    return true;
}

void DtFindDemo::find( const DtFindDemoOptions& opt ) {
    currentScanWidget = mainTabWidget->addScanWidget( DtTablesWidget::TT_FINDDEMO );

    progressDialog->start();
    progressDialog->show();
    QApplication::processEvents();

    QString searchInfo;

    if ( !opt.player.isEmpty() ) {
        searchInfo.append( tr( "player:" ) + " " + opt.player + "    " );
    }

    if ( !opt.map.isEmpty() ) {
        searchInfo.append( tr( "map:" ) + " " + opt.map + "    " );
    }

    if ( !opt.server.isEmpty() ) {
        searchInfo.append( tr( "server:" ) + " " + opt.server + "    " );
    }

    if ( !opt.name.isEmpty() ) {
        searchInfo.append( tr( "name:" ) + " " + opt.name + "    " );
    }

    if ( config.getSelectedGame() == Q_LIVE && opt.gameType != GT_ANY ) {
        QString gameType = getGameTypeName( 73, opt.mod, opt.gameType, true );
        searchInfo.append( tr( "type:" ) + " " + gameType + "    " );
    }
    else if ( config.getSelectedGame() == Q_ARENA && opt.mod != MOD_ANY ) {
        QString modName;

        switch ( opt.mod ) {
            case MOD_BASEQ3 : modName = "baseq3"; break;
            case MOD_OSP : modName = "osp"; break;
            case MOD_CPMA : modName = "cpma"; break;
            case MOD_DEFRAG : modName = "defrag"; break;
            default : break;
        }

        searchInfo.append( tr( "mod:" ) + " " + modName + "    " );
    }

    QDateTime date;
    QString dateFrom;
    QString dateTo;

    date.setTime_t( opt.dateFrom );
    dateFrom = date.toString( "dd-MM-yy" );

    date.setTime_t( opt.dateTo );
    dateTo = date.toString( "dd-MM-yy" );

    searchInfo.append( tr( "date:" ) + " " + dateFrom + " - " + dateTo );

    currentScanWidget->setSearchInfo( searchInfo );

    QVector< DtDemoDbData > selectedDemos;

    if ( opt.allDirs ) {
        demoDb.selectDemosForGame( config.getSelectedGame(), selectedDemos );
    }
    else {
        demoDb.selectDemosForDir( currentWorkingDir, selectedDemos );
    }

    haveResults = false;
    stopSearch = false;
    demosCount = selectedDemos.count();
    demosDone = 0;

    progressDialog->setData( "0 " + tr( "of" ) + " " + QString::number( demosCount ), 0,
                             DtProgressDialog::CancelButton );

    Qt::CaseSensitivity senitivity = ( opt.matchCase ) ? Qt::CaseSensitive : Qt::CaseInsensitive;
    QRegExp reg( "", senitivity, QRegExp::WildcardUnix );

    for ( int i = 0; i < demosCount; ++i ) {
        if ( stopSearch ) {
            break;
        }

        const DtDemoDbData& demo = selectedDemos.at( i );

        emit newScanStarted();
        ++demosDone;

        if ( demo.date < opt.dateFrom || demo.date > opt.dateTo ) {
            continue;
        }

        if ( config.getSelectedGame() == Q_LIVE ) {
            if ( opt.gameType != GT_ANY && demo.type != opt.gameType ) {
                continue;
            }
        }
        else {
            if ( opt.mod != MOD_ANY && demo.mod != opt.mod ) {
                continue;
            }
        }

        if ( !checkString( reg, opt.player, demo.player ) ) {
            continue;
        }

        if ( !checkString( reg, opt.map, demo.map ) ) {
            continue;
        }

        if ( !checkString( reg, opt.server, demo.server ) ) {
            continue;
        }

        QFileInfo fileInfo( demo.name );
        QString name = fileInfo.fileName();

        if ( !checkString( reg, opt.name, name ) ) {
            continue;
        }

        scanMutex->lock();
        DtFindDemoTable* table = qobject_cast< DtFindDemoTable* >( currentScanWidget->scanTable );
        QString modifiedName = getModifiedName( demo.name );
        table->addDemo( openedDemos.value( modifiedName, 0 ) );
        scanMutex->unlock();
        QApplication::processEvents();
        haveResults = true;
    }

    qApp->processEvents();

    if ( haveResults ) {
        progressDialog->close();
    } else {
        progressDialog->setData( tr( "Nothing found" ), 100, DtProgressDialog::OkButton );
    }

    emit scanFinished();
    currentScanWidget->scanTable->sortColumn( config.findDemoTableSortColumn,
                                              static_cast< Qt::SortOrder >(
                                                      config.findDemoTableSortOrder ) );
}

void DtFindDemo::stop() {
    stopSearch = true;
}

void DtFindDemo::updateProgress() {
    QString lbl = QString::number( demosDone ) + " " + tr( "of" ) + " "
                  + QString::number( demosCount );
    int prc = static_cast< int >( demosDone / demosCount * 100 );
    progressDialog->setData( lbl, prc, DtProgressDialog::CancelButton );
}
