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

#include "FindTextDialog.h"
#include "Data.h"

#include <QEventLoop>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>

using namespace dtdata;

DtFindTextDialog::DtFindTextDialog( QWidget* parent ) : QDialog( parent ),
    ret( false )
{
    setWindowTitle( tr( "Find chat messages" ) );
    setFixedSize( 420, 130 );
    setWindowModality( Qt::ApplicationModal );

    QVBoxLayout* dialogLayout = new QVBoxLayout;

    searchStringEdit = new QLineEdit( this );
    dialogLayout->addWidget( searchStringEdit, 1 );

    QLabel* matchCaseLbl = new QLabel( tr( "Match case" ), this );
    matchCaseCb = new QCheckBox( this );

    QLabel* ignoreColorsLbl = new QLabel( tr( "Ignore text colors" ), this );
    ignoreColorsCb = new QCheckBox( this );

    QHBoxLayout* optLayout = new QHBoxLayout;

    optLayout->addWidget( matchCaseLbl, 1, Qt::AlignLeft );
    optLayout->addWidget( matchCaseCb, 1, Qt::AlignLeft );
    optLayout->addWidget( ignoreColorsLbl, 1, Qt::AlignLeft );
    optLayout->addWidget( ignoreColorsCb, 1, Qt::AlignLeft );

    dialogLayout->addLayout( optLayout );

    QPushButton* okButton = new QPushButton( tr( "Find" ), this );
    okButton->setMinimumWidth( 120 );
    connect( okButton, SIGNAL( clicked() ), this, SLOT( startSearch() ) );

    QPushButton* cancelButton = new QPushButton( tr( "Cancel" ), this );
    cancelButton->setMinimumWidth( 120 );
    connect( cancelButton, SIGNAL( clicked() ), this, SLOT( close() ) );

    QHBoxLayout* buttonsLayout = new QHBoxLayout;

    dialogLayout->addSpacing( 10 );
    buttonsLayout->addStretch( 1 );
    buttonsLayout->addWidget( okButton );
    buttonsLayout->addWidget( cancelButton );
    dialogLayout->addLayout( buttonsLayout );

    setLayout( dialogLayout );
}

void DtFindTextDialog::closeEvent( QCloseEvent* ) {
    pEventLoop->exit( 0 );
}

void DtFindTextDialog::startSearch() {
    ret = true;
    config.lastFindTextString = searchStringEdit->text();
    config.findTextIgnoreColors = ignoreColorsCb->isChecked();
    config.findTextMatchCase = matchCaseCb->isChecked();
    close();
}

void DtFindTextDialog::showEvent( QShowEvent* ) {
    ret = false;
    searchStringEdit->setText( config.lastFindTextString );
    ignoreColorsCb->setChecked( config.findTextIgnoreColors );
    matchCaseCb->setChecked( config.findTextMatchCase );
}

bool DtFindTextDialog::exec() {
    if ( pEventLoop ) {
        return ret;
    }

    show();

    QEventLoop eventLoop;
    pEventLoop = &eventLoop;

    QPointer< DtFindTextDialog > guard = this;
    eventLoop.exec( QEventLoop::DialogExec );
    pEventLoop = 0;

    if ( guard.isNull() ) {
        return false;
    }

    return ret;
}
