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

#include "ClearLineEdit.h"

#include <QToolButton>
#include <QHBoxLayout>
#include <QApplication>
#include <QStyle>

DtClearLineEdit::DtClearLineEdit( QWidget* parent ) : QLineEdit( parent ),
    clearButton( new QToolButton( this ) )
{
    connect( clearButton, SIGNAL( clicked() ), this, SLOT( clear() ) );
    clearButton->setIcon( QIcon( ":/res/edit-clear-locationbar-rtl.png" ) );
    clearButton->setAutoRaise( true );
    clearButton->setCursor( Qt::ArrowCursor );
    clearButton->setToolTip( tr( "Clear" ) );
    clearButton->setStyleSheet( QString( "QToolButton { border: none; padding-right: 5px; }" ) );
    clearButton->setVisible( false );

    connect( this, SIGNAL( textChanged ( const QString & ) ), this, SLOT( changed( const QString& ) ) );

    QHBoxLayout* clearLayout = new QHBoxLayout;
    clearLayout->addStretch( 1 );
    clearLayout->addWidget( clearButton );
    clearLayout->setMargin( 0 );

    hintHeight = QLineEdit::sizeHint().rheight();

    setStyleSheet( QString( "QLineEdit { padding-right: 15px; }" ) );
    setLayout( clearLayout );
}

void DtClearLineEdit::changed( const QString& text ) {
    clearButton->setVisible( !text.isEmpty() );
}

QSize DtClearLineEdit::sizeHint() const {
    QSize sz = QLineEdit::sizeHint();
    sz.rheight() = hintHeight;

    return sz;
}

