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

#include "TableDelegate.h"
#include "Data.h"
#include "DemoTable.h"

#include <QPainter>

using namespace dtdata;

DtTableDelegate::DtTableDelegate( QWidget* parent ) : QStyledItemDelegate( parent ) {

}

void DtTableDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const {
    painter->save();

    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, index );

    bool hasIcon = !opt.icon.isNull();
    bool itemSelected = opt.state & QStyle::State_Selected;
    bool itemMarked = opt.checkState;

    if ( itemSelected || itemMarked ) {
        QRect rec = opt.rect;

        QBrush pBrush = painter->brush();
        pBrush.setStyle( Qt::SolidPattern );
        pBrush.setColor( itemMarked ? QColor( "#76a9db" ) : config.tableSelectionColor );

        painter->setPen( Qt::NoPen );
        painter->setBrush( pBrush );
        painter->drawRect( rec );

        if ( itemSelected ) {
            painter->setPen( config.tableSelectionBorderColor );

            if ( index.row() > 0 ) {
                DtTable* table = qobject_cast< DtTable* >( parent() );

                if ( table &&
                     !table->selectionModel()->isSelected( table->model()->index( index.row() - 1, 0 ) ) )
                {
                    painter->drawLine( rec.topLeft(), rec.topRight() );
                }
            }
            else {
                painter->drawLine( rec.topLeft(), rec.topRight() );
            }

            painter->drawLine( rec.bottomLeft(), rec.bottomRight() );
        }

        painter->setPen( itemMarked ? QColor( "white" ) : config.tableTextColor );
    }

    QSize iSize;

    if ( hasIcon ) {
        QRect iconDrawRect = opt.rect;
        iconDrawRect.setLeft( opt.rect.left() + 4 );
        iconDrawRect.setTop( opt.rect.top() + 4 );
        iSize = opt.icon.availableSizes().at( 0 );

        painter->drawPixmap( iconDrawRect.x(), iconDrawRect.y(), iSize.width(), iSize.height(),
                             opt.icon.pixmap( iSize ) );
    }

    QRect drawRect = opt.rect;
    drawRect.setLeft( drawRect.left() + iSize.width() + 7 );

    painter->drawText( drawRect, Qt::AlignVCenter | Qt::TextSingleLine | Qt::AlignLeft, opt.text );
    painter->restore();
}
