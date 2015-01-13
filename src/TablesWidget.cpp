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

#include "TablesWidget.h"
#include "MainTabWidget.h"
#include "Data.h"
#include "Demo.h"
#include "MainTable.h"
#include "FindDemoTable.h"
#include "FindFragsTable.h"
#include "FindTextTable.h"

#include <QSettings>
#include <QVBoxLayout>

using namespace dtdata;

DtTablesWidget::DtTablesWidget( int tableType, QWidget* parent ) : QWidget( parent ) {
    switch ( tableType ) {
        case TT_DEMOFILES : demoTable = new DtMainTable( this ); break;
        case TT_FINDDEMO : demoTable = new DtFindDemoTable( this ); break;
        case TT_FINDFRAGS : demoTable = new DtFindFragsTable( this ); break;
        case TT_FINDTEXT : demoTable = new DtFindTextTable( this ); break;
        default : break;
    }

    playersTable = new DtPlayersTable( this );
    variablesTable = new DtVariablesTable( this );

    cSplit = new QSplitter( Qt::Horizontal, this );
    cSplit->setChildrenCollapsible( false );
    cSplit->addWidget( demoTable );

    rSplit = new QSplitter( Qt::Vertical, this );
    rSplit->setChildrenCollapsible( false );
    rSplit->addWidget( playersTable );
    rSplit->addWidget( variablesTable );

    cSplit->addWidget( rSplit );

    QVBoxLayout* chLayout = new QVBoxLayout;
    chLayout->setMargin( 3 );
    chLayout->addWidget( cSplit );

    setLayout( chLayout );
}

void DtTablesWidget::setInfoPanelVisible( bool visible ) {
    rSplit->setVisible( visible );
}

void DtTablesWidget::clearSelections() {
    demoTable->clearSelection();
    variablesTable->clearSelection();
    playersTable->clearSelection();
}

void DtTablesWidget::setSplitterSizes( QSplitter* split, QString cfgName, const QList< int >& defaultSizes ) {
    QByteArray splitterState = config.settings->value( "Main/" + cfgName ).toByteArray();

    if ( splitterState.isEmpty() ) {
        split->setSizes( defaultSizes );
    }
    else {
        split->restoreState( splitterState );
    }
}

void DtTablesWidget::saveSplitterState( QSplitter* split, QString cfgName ) {
    config.settings->setValue( "Main/" + cfgName, split->saveState() );
}

void DtTablesWidget::clearDemoInfo() {
    playersTable->setRowCount( 0 );
    variablesTable->setRowCount( 0 );
}

void DtTablesWidget::showDemoInfo( DtDemo* demo ) {
    clearDemoInfo();

    const infoMap& info = demo->getInfo();
    int infoCount = info.count();
    int proto = demo->getProto();

    for ( int i = 0; i < infoCount; ++i ) {
        if ( info.at( i ).first.startsWith( "playerName" ) ) {
            QString name = info.at( i ).second;
            int spaceidx = name.indexOf( " " );
            if ( !dtdata::config.showClanTags
              && spaceidx >= 0
              && proto == QZ_73 ) {
                playersTable->addPlayer( name.remove( 0, spaceidx + 1 ) );
            } else {
                playersTable->addPlayer( info.at( i ).second );
            }
        } else {
            bool begin = ( info.at( i ).first == "sv_skillRating" );
            variablesTable->addVariable( info.at( i ).first, info.at( i ).second, begin );
        }
    }
}


