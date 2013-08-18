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
#include "FindFragsTable.h"
#include "FileDialog.h"
#include "MainWindow.h"
#include "Demo.h"
#include "MainTabWidget.h"

#include <QKeyEvent>
#include <QMenu>
#include <QHeaderView>
#include <QDir>

using namespace dtdata;

DtFindFragsTable::DtFindFragsTable( QWidget* parent ) : DtScanTable( parent ) {
    setColumnCount( defaultFindFragsTableColumnNames.count() );
    setHorizontalHeaderLabels( defaultFindFragsTableColumnNames );

    loadColumnWidths( "FindFragsColumns" );

    for ( int i = 0; i < defaultFindFragsTableColumnNames.count(); ++i ) {
        setColumnHidden( i, !config.findFragsTableVisibleColumns.at( i ) );
    }

    horizontalHeader()->setSortIndicator( config.findFragsTableSortColumn,
                                          static_cast< Qt::SortOrder >(
                                                  config.findFragsTableSortOrder ) );
}

void DtFindFragsTable::saveColumns() {
    saveColumnWidths( "FindFragsColumns" );
}

void DtFindFragsTable::sortColumn( int column ) {
    Qt::SortOrder order = Qt::AscendingOrder;

    if ( column == config.findFragsTableSortColumn ) {
        order = ( config.findFragsTableSortOrder == Qt::AscendingOrder )
                ? Qt::DescendingOrder : Qt::AscendingOrder;
    }

    config.findFragsTableSortOrder = order;
    config.findFragsTableSortColumn = column;
    config.save();
    sortByColumn( column );
}

void DtFindFragsTable::sortColumn( int column, Qt::SortOrder order ) {
    config.findFragsTableSortOrder = order;
    config.findFragsTableSortColumn = column;
    config.save();
    sortByColumn( column, order );
}

void DtFindFragsTable::addScanRow( DtDemo* demo, int startSeqTime, int fragsInSeq, int deltaTime,
                                   int segLength, int weapon ) {
    ++demo->referenceCount;

    int rowNum = rowCount();
    insertRow( rowNum );

    DtDemoTableItem* fName = new DtDemoTableItem( demo->fileInfo().baseName );
    setItem( rowNum, CT_NAME, fName );

    tableHash.insert( fName, demo );

    DtDemoTableItem* fPlayer = new DtDemoTableItem( demo->getClientName() );
    setItem( rowNum, CT_PLAYER, fPlayer );

    DtDemoTableItem* fType = new DtDemoTableItem( getGameTypeName( demo->getProto(),
                                                                   demo->getQ3Mod(),
                                                                   demo->getGameType(),
                                                                   true ) );
    setItem( rowNum, CT_TYPE, fType );

    int startTime = startSeqTime - demo->getMapRestartTime();

    DtDemoTableItem* fTime = new DtDemoTableItem( msecToString( startTime ),
                                                  DtDemoTableItem::ST_DATETIME );
    QList< QVariant > fTimeData;
    fTimeData << false << startSeqTime;
    fTime->setData( Qt::UserRole, fTimeData );
    setItem( rowNum, CT_TIME, fTime );

    DtDemoTableItem* fFrags = new DtDemoTableItem( QString::number( fragsInSeq ) );
    setItem( rowNum, CT_FRAGS, fFrags );

    DtDemoTableItem* fMaxTime = new DtDemoTableItem( msecToString( deltaTime ),
                                                     DtDemoTableItem::ST_DATETIME );
    QList< QVariant > fMaxTimeData;
    fMaxTimeData << false << deltaTime;
    fMaxTime->setData( Qt::UserRole, fMaxTimeData );
    setItem( rowNum, CT_MAXTIME, fMaxTime );

    DtDemoTableItem* fSegmentLength = new DtDemoTableItem( msecToString( segLength ),
                                                           DtDemoTableItem::ST_DATETIME );
    QList< QVariant > fSegmentLengthData;
    fSegmentLengthData << false << segLength;
    fSegmentLength->setData( Qt::UserRole, fSegmentLengthData );
    setItem( rowNum, CT_SEGMENTLENGTH, fSegmentLength );

    DtDemoTableItem* fWeapon = new DtDemoTableItem( getWeaponName( weapon, true ) );
    setItem( rowNum, CT_WEAPON, fWeapon );

    DtDemoTableItem* fMap = new DtDemoTableItem( demo->getMapName(),
                                                 DtDemoTableItem::ST_MAP );
    setItem( rowNum, CT_MAP, fMap );

    DtDemoTableItem* fProto = new DtDemoTableItem();
    setProtocolItemData( demo->getProto(), fProto );
    setItem( rowNum, CT_PROTOCOL, fProto );
}

void DtFindFragsTable::makeSegments( const QMap< int, int >& group,
                                    QVector< cutSegment >& cutSegments )
{
    QMapIterator< int, int > it( group );

    while ( it.hasNext() ) {
        it.next();

        QTableWidgetItem* lengthItem = item( it.value(), CT_SEGMENTLENGTH );
        int length = lengthItem->data( Qt::UserRole ).toList().at( 1 ).toInt();

        cutSegment part;
        part.start = it.key() - config.frags.segAddTimeBegin * 1000;
        part.end = it.key() + length + config.frags.segAddTimeEnd * 1000;

        cutSegments.append( part );
    }
}

QTableWidgetItem* DtFindFragsTable::getTimeItem( int rowNum ) {
    return item( rowNum, CT_TIME );
}
