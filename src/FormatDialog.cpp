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
#include "FormatDialog.h"

#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QPointer>
#include <QEventLoop>

DtFormatDialog::DtFormatDialog( QString title, QString& defFormat, QString help,
                    QString& defFormat2, QString help2, QWidget* parent )
    : QDialog( parent ), defaultFormat( defFormat ), defaultFormat2( defFormat2 )
{
    setWindowTitle( title );
    setMinimumWidth( 400 );
    setWindowModality( Qt::ApplicationModal );

    QVBoxLayout* dialogLayout = new QVBoxLayout;

    if ( !help.isEmpty() ) {
        QLabel* formatHelp = new QLabel( help );
        dialogLayout->addWidget( formatHelp, 1, Qt::AlignLeft );
    }

    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    okButton = new QPushButton( "OK" );
    connect( okButton, SIGNAL( clicked() ), this, SLOT( okPressed() ) );
    okButton->setMinimumWidth( 120 );
    cancelButton = new QPushButton( tr( "Cancel" ) );
    connect( cancelButton, SIGNAL( clicked() ), this, SLOT( cancelPressed() ) );
    cancelButton->setMinimumWidth( 120 );

    formatEdit = new QLineEdit( defaultFormat );

    dialogLayout->addWidget( formatEdit );

    if ( !help2.isEmpty() ) {
        dialogLayout->addSpacing( 10 );
        QLabel* formatHelp2 = new QLabel( help2 );
        dialogLayout->addWidget( formatHelp2, 1, Qt::AlignLeft );
        formatEdit2 = new QLineEdit( defaultFormat2 );
        dialogLayout->addWidget( formatEdit2 );
    }

    buttonsLayout->addStretch( 1 );
    buttonsLayout->addWidget( okButton, 1, Qt::AlignRight );
    buttonsLayout->addWidget( cancelButton, 1, Qt::AlignRight );
    dialogLayout->addSpacing( 10 );
    dialogLayout->addLayout( buttonsLayout );
    setLayout( dialogLayout );

    ret = BTN_CANCEL;
    pEventLoop = 0;
}

void DtFormatDialog::okPressed() {
    if ( ( formatEdit->text().isEmpty() || !dtdata::isAcceptedEncoding( formatEdit->text() ) )
        && !defaultFormat.isEmpty() )
    {
        formatEdit->setText( defaultFormat );
    }
    else {
        defaultFormat = formatEdit->text().trimmed();
        defaultFormat.replace( ' ', '-' );
        rDone( BTN_OK );
    }
}

void DtFormatDialog::cancelPressed() {
    rDone( BTN_CANCEL );
}

DtFormatDialog::dialogButtons DtFormatDialog::exec() {
    if ( pEventLoop ) {
        return BTN_CANCEL;
    }

    show();

    QEventLoop eventLoop;
    pEventLoop = &eventLoop;

    QPointer< DtFormatDialog > guard = this;
    eventLoop.exec( QEventLoop::DialogExec );
    pEventLoop = 0;

    if ( guard.isNull() ) {
        return BTN_CANCEL;
    }

    return ret;
}

void DtFormatDialog::rDone( dialogButtons retc ) {
    ret = retc;
    pEventLoop->exit( 0 );
    close();
}

