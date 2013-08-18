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
#include "ScanTable.h"
#include "MainTable.h"
#include "FileDialog.h"
#include "MainWindow.h"
#include "Demo.h"
#include "MainTabWidget.h"

#include <QKeyEvent>
#include <QMenu>
#include <QHeaderView>
#include <QDir>

using namespace dtdata;

DtScanTable::DtScanTable( QWidget* parent ) : DtDemoTable( parent ) {

}

void DtScanTable::updateRow( int ) {

}

void DtScanTable::deleteSelectedDemos() {
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

void DtScanTable::packSelectedDemos() {
    DtCpDemoVec demoSegments;
    getCpDemoVec( demoSegments, QDir::tempPath() );

    dtMainWindow->files()->packSegments( demoSegments );
    clearSelection();
}

void DtScanTable::copySelectedDemos( const QString& dest ) {
    if ( contextMenu ) {
        contextMenu->close();
    }

    DtCpDemoVec demoSegments;
    getCpDemoVec( demoSegments, dest );

    dtMainWindow->files()->copySegments( demoSegments, dest );

    if ( dest == currentWorkingDir ) {
        mainDemoTable->setUpdateNeeded( true );
    }

    clearSelection();
}

void DtScanTable::moveSelectedDemos( const QString& dest ) {
    copySelectedDemos( dest );
    removeSelectedRows();
}

void DtScanTable::clearMark() {
    if ( markedName ) {
        markedName->setIcon( QIcon() );

        for ( int i = 0; i < columnCount(); ++i ) {
            item( markedName->row(), i )->setCheckState( Qt::Unchecked );
        }

        updateMarkedRow();
    }
}

void DtScanTable::getCpDemoVec( DtCpDemoVec& demoSegments, const QString& dest ) {
    QList< int > selectedRows = getSelectedRows();
    int selectedRowsCount = selectedRows.size();
    DtDemoVec sDemos;

    for ( int i = 0; i < selectedRowsCount; ++i ) {
        DtDemo* demo = demoAt( selectedRows.at( i ) );

        if ( !sDemos.contains( demo ) ) {
            sDemos.append( demo );

            DtCpDemo cpDemo;
            cpDemo.demo = demo;

            QMap< int, int > group;

            for ( int j = 0; j < selectedRowsCount; ++j ) {
                if ( demoAt( selectedRows.at( j ) ) == demo ) {
                    QTableWidgetItem* timeItem = getTimeItem( selectedRows.at( j ) );
                    group.insert( timeItem->data( Qt::UserRole ).toList().at( 1 ).toInt(),
                                  selectedRows.at( j ) );
                }
            }

            makeSegments( group, cpDemo.options.cutSegments );

            cpDemo.options.newFileName = dest + "/" + demo->fileInfo().baseName;
            cpDemo.options.singleFile = false;
            cpDemo.options.removeWarmup = true;

            demoSegments.append( cpDemo );
        }
    }
}
