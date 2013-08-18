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
#include "FindTextTable.h"
#include "FileDialog.h"
#include "MainWindow.h"
#include "Demo.h"
#include "MainTabWidget.h"

#include <QKeyEvent>
#include <QMenu>
#include <QHeaderView>
#include <QDir>
#include <QDateTime>

using namespace dtdata;

DtFindTextTable::DtFindTextTable( QWidget* parent ) : DtScanTable( parent ) {
    setColumnCount( defaultFindTextTableColumnNames.count() );
    setHorizontalHeaderLabels( defaultFindTextTableColumnNames );

    loadColumnWidths( "FindTextColumns" );

    for ( int i = 0; i < defaultFindTextTableColumnNames.count(); ++i ) {
        setColumnHidden( i, !config.findTextTableVisibleColumns.at( i ) );
    }

    horizontalHeader()->setSortIndicator( config.findTextTableSortColumn,
                                          static_cast< Qt::SortOrder >(
                                                  config.findTextTableSortOrder ) );
}

void DtFindTextTable::saveColumns() {
    saveColumnWidths( "FindTextColumns" );
}

void DtFindTextTable::sortColumn( int column ) {
    Qt::SortOrder order = Qt::AscendingOrder;

    if ( column == config.findTextTableSortColumn ) {
        order = ( config.findTextTableSortOrder == Qt::AscendingOrder )
                ? Qt::DescendingOrder : Qt::AscendingOrder;
    }

    config.findTextTableSortOrder = order;
    config.findTextTableSortColumn = column;
    config.save();
    sortByColumn( column );
}

void DtFindTextTable::sortColumn( int column, Qt::SortOrder order ) {
    config.findTextTableSortOrder = order;
    config.findTextTableSortColumn = column;
    config.save();
    sortByColumn( column, order );
}

void DtFindTextTable::addScanRow( DtDemo* demo, int time, const QString& player, const QString& cmd,
                                  const QString& msg )
{
    ++demo->referenceCount;

    int rowNum = rowCount();
    insertRow( rowNum );

    DtDemoTableItem* fName = new DtDemoTableItem( demo->fileInfo().baseName );
    setItem( rowNum, CT_NAME, fName );

    tableHash.insert( fName, demo );

    DtDemoTableItem* fPlayer = new DtDemoTableItem( player );
    setItem( rowNum, CT_PLAYER, fPlayer );

    DtDemoTableItem* fType = new DtDemoTableItem( getGameTypeName( demo->getProto(),
                                                                   demo->getQ3Mod(),
                                                                   demo->getGameType(),
                                                                   true ) );
    setItem( rowNum, CT_TYPE, fType );

    DtDemoTableItem* fMap = new DtDemoTableItem( demo->getMapName(),
                                                 DtDemoTableItem::ST_MAP );
    setItem( rowNum, CT_MAP, fMap );

    QDateTime demoTime;
    demoTime.setTime_t( demo->getLevelStartTime() );

    QString demoDate = demo->isInfoAvailable() ? demoTime.toString( defaultDateFormat ) : "";
    DtDemoTableItem* fDate = new DtDemoTableItem( demoDate, DtDemoTableItem::ST_DATETIME );
    QList< QVariant > itemData;
    itemData << false << demo->getLevelStartTime();
    fDate->setData( Qt::UserRole, itemData );
    setItem( rowNum, CT_DATE, fDate );

    int startTime = time - demo->getMapRestartTime();
    DtDemoTableItem* fTime = new DtDemoTableItem( msecToString( startTime ),
                                                  DtDemoTableItem::ST_DATETIME );
    QList< QVariant > fTimeData;
    fTimeData << false << time;
    fTime->setData( Qt::UserRole, fTimeData );
    setItem( rowNum, CT_TIME, fTime );

    DtDemoTableItem* fCommand = new DtDemoTableItem( cmd );
    setItem( rowNum, CT_COMMAND, fCommand );

    DtDemoTableItem* fMessage = new DtDemoTableItem( msg );
    setItem( rowNum, CT_MSG, fMessage );

    DtDemoTableItem* fLength = new DtDemoTableItem( msecToString( demo->getLength() ),
                                                    DtDemoTableItem::ST_DATETIME );
    setItem( rowNum, CT_DEMOLENGTH, fLength );

    DtDemoTableItem* fProto = new DtDemoTableItem;
    setProtocolItemData( demo->getProto(), fProto );
    setItem( rowNum, CT_PROTOCOL, fProto );
}

void DtFindTextTable::makeSegments( const QMap< int, int >& group,
                                    QVector< cutSegment >& cutSegments )
{
    QMapIterator< int, int > it( group );

    while ( it.hasNext() ) {
        it.next();

        cutSegment part;
        part.start = it.key() - config.textSegAddTimeBegin * 1000;
        part.end = it.key() + config.textSegAddTimeEnd * 1000;
        cutSegments.append( part );
    }
}

QTableWidgetItem* DtFindTextTable::getTimeItem( int rowNum ) {
    return item( rowNum, CT_TIME );
}

