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
#include "FindDemoTable.h"
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

DtFindDemoTable::DtFindDemoTable( QWidget* parent ) : DtDemoTable( parent ) {
    setColumnCount( defaultFindDemoTableColumnNames.count() );
    setHorizontalHeaderLabels( defaultFindDemoTableColumnNames );

    loadColumnWidths( "FindDemoColumns" );

    for ( int i = 0; i < defaultFindDemoTableColumnNames.count(); ++i ) {
        setColumnHidden( i, !config.findDemoTableVisibleColumns.at( i ) );
    }

    horizontalHeader()->setSortIndicator( config.findDemoTableSortColumn,
                                          static_cast< Qt::SortOrder >(
                                                  config.findDemoTableSortOrder ) );
}

void DtFindDemoTable::saveColumns() {
    saveColumnWidths( "FindDemoColumns" );
}

void DtFindDemoTable::sortColumn( int column ) {
    Qt::SortOrder order = Qt::AscendingOrder;

    if ( column == config.findDemoTableSortColumn ) {
        order = ( config.findDemoTableSortOrder == Qt::AscendingOrder )
                ? Qt::DescendingOrder : Qt::AscendingOrder;
    }

    config.findDemoTableSortOrder = order;
    config.findDemoTableSortColumn = column;
    config.save();
    sortByColumn( column );
}

void DtFindDemoTable::sortColumn( int column, Qt::SortOrder order ) {
    config.findDemoTableSortOrder = order;
    config.findDemoTableSortColumn = column;
    config.save();
    sortByColumn( column, order );
}

void DtFindDemoTable::clearMark() {
    if ( markedName ) {
        markedName->setIcon( QIcon() );

        for ( int i = 0; i < columnCount(); ++i ) {
            item( markedName->row(), i )->setCheckState( Qt::Unchecked );

            if ( demoAt( markedName->row() )->isBroken() ) {
                markedName->setIcon( icons->getIcon( I_BROKEN ) );
            }
        }

        updateMarkedRow();
    }
}

void DtFindDemoTable::addDemo( DtDemo* demo ) {
    ++demo->referenceCount;

    int rowNum = rowCount();
    insertRow( rowNum );

    DtDemoTableItem* fName = new DtDemoTableItem( demo->fileInfo().baseName );
    setItem( rowNum, CT_NAME, fName );

    if ( demo->isBroken() ) {
        fName->setIcon( icons->getIcon( I_BROKEN ) );
    }

    tableHash.insert( fName, demo );

    int basePathSize = ( config.getSelectedGame() == Q_LIVE ) ? config.getQzBasePath().size() :
                                                                config.getQaBasePath().size();
    QString dirName = demo->fileInfo().filePath;
    dirName.remove( 0, basePathSize + 1 );

    DtDemoTableItem* fDir = new DtDemoTableItem( dirName );
    setItem( rowNum, CT_DIR, fDir );

    DtDemoTableItem* fPlayer = new DtDemoTableItem( demo->getClientName() );
    setItem( rowNum, CT_PLAYER, fPlayer );

    QString gtName = demo->isInfoAvailable() ? getGameTypeName( demo->getProto(),
                                                                demo->getQ3Mod(),
                                                                demo->getGameType(),
                                                                true ) : "";
    DtDemoTableItem* fType = new DtDemoTableItem( gtName );
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

    DtDemoTableItem* fServer = new DtDemoTableItem( demo->getHostName() );
    setItem( rowNum, CT_SERVER, fServer );

    DtDemoTableItem* fProto = new DtDemoTableItem;
    setProtocolItemData( demo->getProto(), fProto );
    setItem( rowNum, CT_PROTOCOL, fProto );
}

void DtFindDemoTable::deleteSelectedDemos() {
    if ( getSelectedRows().size() && confirmDeletion( tr( "Delete selected rows?" ) ) ) {
        int row = currentIndex().row();

        removeSelectedRows();

        if ( rowCount() ) {
            if ( row > rowCount() - 1 ) {
                row = rowCount() - 1;
            }

            selectRow( row );
        }

        emit demoDeleted();
    }
}

void DtFindDemoTable::packSelectedDemos() {
    dtMainWindow->files()->pack( getSelectedDemos() );
    clearSelection();
}

void DtFindDemoTable::copySelectedDemos( const QString& dest ) {
    if ( !dtMainWindow->files()->checkDestDir( dest ) ) {
        return;
    }

    if ( contextMenu ) {
        contextMenu->close();
    }

    dtMainWindow->files()->copy( getSelectedDemos(), dest );
    clearSelection();
}

void DtFindDemoTable::moveSelectedDemos( const QString& dest ) {
    copySelectedDemos( dest );
    removeSelectedRows();
}

void DtFindDemoTable::updateRow( int rowNum ) {
    DtDemo* demo = demoAt( rowNum );

    item( rowNum, CT_PLAYER )->setText( demo->getClientName() );
    item( rowNum, CT_TYPE )->setText( getGameTypeName( demo->getProto(), demo->getQ3Mod(),
                                                       demo->getGameType(), true ) );
    item( rowNum, CT_MAP )->setText( demo->getMapName() );

    QDateTime demoTime;
    demoTime.setTime_t( demo->getLevelStartTime() );

    item( rowNum, CT_DATE )->setText( demoTime.toString( defaultDateFormat ) );
    item( rowNum, CT_DATE )->data( Qt::UserRole ).toList()[ 1 ] = demo->getLevelStartTime();
    item( rowNum, CT_SERVER )->setText( demo->getHostName() );
    setProtocolItemData( demo->getProto(), item( rowNum, CT_PROTOCOL ) );
}

