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

#include "ProgressDialog.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QApplication>
#include <QProgressBar>
#include <QLabel>

DtProgressDialog::DtProgressDialog( const QString& title, QWidget* parent ) : QDialog( parent ) {
    setModal( true );
    setWindowTitle( title );
    setFixedSize( 400, 120 );
    setWindowFlags( Qt::Dialog | Qt::WindowMaximizeButtonHint );

    QVBoxLayout* vBox = new QVBoxLayout;
    pb = new QProgressBar;
    infoLabel = new QLabel();
    mButton = new QPushButton( this );
    mButton->setFixedWidth( 120 );
    connect( mButton, SIGNAL( clicked() ), this, SLOT( mButtonClicked() ) );

    vBox->addWidget( infoLabel, 1, Qt::AlignCenter );
    vBox->addWidget( pb );
    vBox->addSpacing( 15 );

    QHBoxLayout* hBox = new QHBoxLayout();
    hBox->addWidget( mButton, Qt::AlignCenter );
    vBox->addLayout( hBox );

    setLayout( vBox );

    okBtnText = tr( "OK" );
    cancelBtnText = tr( "Cancel" );

    start();
}

void DtProgressDialog::start() {
    done = false;
    mButton->setText( cancelBtnText );
}

void DtProgressDialog::mButtonClicked() {
    if ( !done ) {
        emit buttonClicked();
        done = true;
    }

    close();
}

void DtProgressDialog::setPos( int p ) {
    pb->setValue( p );
    QApplication::processEvents();
}

void DtProgressDialog::setData( const QString& lbl, int p, dialogButtons btn ) {
    if ( !lbl.isEmpty() ) {
        if ( !infoLabel->isVisible() ) {
            infoLabel->setVisible( true );
        }

        infoLabel->setText( lbl );
    }
    else {
        infoLabel->setVisible( false );
    }

    mButton->setVisible( ( btn != NoButton ) );

    if ( btn == OkButton ) {
        mButton->setText( okBtnText );
    }
    else if ( btn == CancelButton ) {
        mButton->setText( cancelBtnText );
    }

    pb->setValue( p );
}
