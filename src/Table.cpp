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

#include "Table.h"
#include "Data.h"
#include "TableDelegate.h"

#include <QHeaderView>
#include <QSettings>
#include <QScrollBar>

using namespace dtdata;

bool DtTable::colorsUpdated = false;

DtTable::DtTable( QWidget* parent ) : QTableWidget( 0, 0, parent ) {
    verticalHeader()->hide();
    verticalHeader()->setDefaultSectionSize( 24 );

    horizontalHeader()->setHighlightSections( false );
    horizontalHeader()->setResizeMode( QHeaderView::Interactive );
    horizontalHeader()->setStretchLastSection( true );
    horizontalHeader()->setDefaultAlignment( Qt::AlignCenter | Qt::AlignVCenter );

    setSelectionBehavior( SelectRows );
    setSelectionMode( SingleSelection );
    setShowGrid( false );
    setAlternatingRowColors( true );
    setItemDelegate( new DtTableDelegate( this ) );

    QPalette pal = palette();

    if ( !colorsUpdated ) {
        QColor baseColor = pal.base().color();
        int alternateFactor = config.tableAlternateColorFactor + 100;
        int selectorFactor = config.tableSelectionColorFactor + 100;
        const int selectorBorderFactor = 142;

        if ( baseColor.value() >= 127 ) {
            config.tableAlternateColor = baseColor.darker( alternateFactor );
            config.tableSelectionColor = baseColor.darker( selectorFactor );
            config.tableSelectionBorderColor = config.tableSelectionColor
                                               .darker( selectorBorderFactor );
        }
        else {
            config.tableAlternateColor = baseColor.lighter( alternateFactor );
            config.tableSelectionColor = baseColor.lighter( selectorFactor );
            config.tableSelectionBorderColor = config.tableSelectionColor
                                               .lighter( selectorBorderFactor );
        }

        config.tableTextColor = pal.text().color();
        colorsUpdated = true;
    }

    pal.setColor( QPalette::AlternateBase, config.tableAlternateColor );
    setPalette( pal );
}

void DtTable::selectionChanged( const QItemSelection& o, const QItemSelection& n ) {
    if ( selectionMode() != SingleSelection ) {
        setDirtyRegion( rect() );
    }

    QTableWidget::selectionChanged( o, n );
}

void DtTable::removeAllRows() {
    if ( rowCount() ) {
        clearSelection();
        setRowCount( 0 );
    }
}

QList< int > DtTable::getSelectedRows() const {
    QList< int > sRows;
    QList< QTableWidgetSelectionRange > ranges = selectedRanges();
    int rangesSize = ranges.size();

    for ( int i = 0; i < rangesSize; ++i ) {
        int rangeRowCount = ranges.at( i ).rowCount();

        for ( int j = 0; j < rangeRowCount; ++j ) {
            sRows.append( ranges.at( i ).topRow() + j );
        }
    }

    return sRows;
}

void DtTable::saveColumnWidths( const QString& cfgTableName ) {
    config.settings->beginWriteArray( cfgTableName );

    for ( int i = 0; i < columnCount(); ++i ) {
        config.settings->setArrayIndex( i );
        config.settings->setValue( "width", columnWidth( i ) );
    }

    config.settings->endArray();
}

void DtTable::loadColumnWidths( const QString& cfgTableName ) {
    QList< int > columnWidths;
    QList< int > defaultColumnWidths;

    if ( cfgTableName == "DemoListColumns" ) {
        defaultColumnWidths << 198 << 125 << 52 << 108 << 102 << 99 << 50;
    }
    else if ( cfgTableName == "FindDemoColumns" ) {
        defaultColumnWidths << 198 << 125 << 125 << 52 << 108 << 102 << 99 << 50;
    }
    else if ( cfgTableName == "FindFragsColumns" ) {
        defaultColumnWidths << 130 << 90 << 45 << 94 << 50 << 46 << 86 << 65 << 76 << 76;
    }
    else if ( cfgTableName == "FindTextColumns" ) {
        defaultColumnWidths << 147 << 116 << 45 << 86 << 53 << 53 << 77 << 289 << 60 << 62;
    }
    else {
        defaultColumnWidths << 120 << 119;
    }

    int sSize = config.settings->beginReadArray( cfgTableName );

    for ( int i = 0; i < sSize; ++i ) {
        config.settings->setArrayIndex( i );
        int nsize = config.settings->value( "width" ).toInt();

        if ( !nsize ) {
            columnWidths.append( defaultColumnWidths.at( i ) );
        } else {
            columnWidths.append( nsize );
        }
    }

    config.settings->endArray();

    if ( !sSize || sSize < defaultColumnWidths.size() ) {
        columnWidths = defaultColumnWidths;
    }

    for ( int i = 0, j = 0; i < columnCount(); ++i ) {
        if ( !isColumnHidden( i ) ) {
            setColumnWidth( i, columnWidths.at( j++ ) );
        }
    }
}
