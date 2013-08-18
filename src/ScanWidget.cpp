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

#include "ScanWidget.h"
#include "Data.h"
#include "ScanTable.h"

#include <QLabel>
#include <QVBoxLayout>

using namespace dtdata;

DtScanWidget::DtScanWidget( int tableType, QWidget* parent ) : DtTablesWidget( tableType, parent ) {
    done = false;

    searchInfo = new QLabel;
    searchInfo->setMaximumHeight( 26 );

    QVBoxLayout* cLayout = qobject_cast< QVBoxLayout* >( layout() );
    cLayout->insertWidget( 0, searchInfo );

    setSplitterSizes( cSplit, "ScanListCSplit", QList< int >() << 705 << 292 );
    setSplitterSizes( rSplit, "ScanListRSplit", QList< int >() << 120 << 239 );

    scanTable = qobject_cast< DtDemoTable* >( demoTable );
}

void DtScanWidget::setSearchInfo( const QString& info ) {
    searchInfo->setText( info );
}

void DtScanWidget::setDone( bool sDone ) {
    done = sDone;
}

bool DtScanWidget::isDone() {
    return done;
}

void DtScanWidget::saveSplitterSizes() {
    saveSplitterState( cSplit, "ScanListCSplit" );
    saveSplitterState( rSplit, "ScanListRSplit" );

    demoTable->saveColumns();
}

