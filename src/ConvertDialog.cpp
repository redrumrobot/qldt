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
#include "ConvertDialog.h"
#include "Demo.h"
#include "MainWindow.h"
#include "MainTabWidget.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>

using namespace dtdata;

DtConvertDialog::DtConvertDialog( DtDemo* inDemo, QWidget* parent ) : QDialog( parent ),
    demo( inDemo )
{
    setWindowTitle( tr( "Convert" ) + " " + demo->fileInfo().fileName( false ) );
    setFixedSize( 400, 250 );
    setWindowModality( Qt::ApplicationModal );

    QVBoxLayout* dialogLayout = new QVBoxLayout;
    QString help = tr( "Old QL beta demos fix.\n\nSupported maps: qzdm14, qzdm17, qzteam7." );
    bool notSupported = false;

    if ( demo->getProto() != QZ_73 ) {
        help = tr( "Protocol isn't supported." );
        notSupported = true;
    }

    QLabel* helpLbl = new QLabel( help );
    helpLbl->setWordWrap( true );
    dialogLayout->addWidget( helpLbl, 1, Qt::AlignCenter );

    if ( notSupported ) {
        helpLbl->setWordWrap( false );
        setLayout( dialogLayout );
        return;
    }

    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    QPushButton* saveAsButton = new QPushButton( tr( "Save As" ) );
    QPushButton* saveButton = new QPushButton( tr( "Save" ) );
    connect( saveAsButton, SIGNAL( clicked() ), this, SLOT( saveAsPressed() ) );
    connect( saveButton, SIGNAL( clicked() ), this, SLOT( savePressed() ) );
    saveAsButton->setMinimumWidth( 120 );
    saveButton->setMinimumWidth( 120 );
    buttonsLayout->addStretch( 1 );
    buttonsLayout->addWidget( saveAsButton, 1, Qt::AlignRight );
    buttonsLayout->addSpacing( 5 );
    buttonsLayout->addWidget( saveButton, 1, Qt::AlignRight );
    dialogLayout->addSpacing( 10 );
    dialogLayout->addLayout( buttonsLayout );
    setLayout( dialogLayout );
}

void DtConvertDialog::saveAsPressed() {
    QString demoFilter = QString( "Quake Demo (*.%1)" ).arg( demo->fileInfo().fileExt );

    QFileDialog saveDialog( this, tr( "Save as" ), currentWorkingDir );
    saveDialog.setNameFilters( QStringList() << demoFilter );
    saveDialog.setViewMode( QFileDialog::List );
    saveDialog.setFileMode( QFileDialog::AnyFile );
    saveDialog.setAcceptMode( QFileDialog::AcceptSave );
    saveDialog.selectFile( "demo" );

    QString fName;

    if( saveDialog.exec() ) {
        fName = saveDialog.selectedFiles().first();
    }

    if ( fName.isEmpty() || !isAcceptedEncoding( QFileInfo( fName ).fileName() ) ) {
        return;
    }

    if ( !fName.endsWith( "." + demo->fileInfo().fileExt, Qt::CaseInsensitive ) ) {
        fName += "." + demo->fileInfo().fileExt;
    }

    if ( QFile::exists( fName ) ) {
        QString demoModName = getModifiedName( fName );
        DtDemo* demoFile = openedDemos.value( demoModName, 0 );

        if ( demoFile ) {
            if ( demoFile->referenceCount > 0 ) {
                QMessageBox::warning( this, tr( "File is in use" ), tr( "Can't overwrite an used demo" ) );
                return;
            }
        }

        int act = QMessageBox::question( this, tr( "File exists" ),
                                         tr( "File with the given name already exists. Overwrite it?" ),
                                         QMessageBox::Yes | QMessageBox::No );
        if ( act == QMessageBox::Yes ) {
            QFile::remove( fName );

            if ( demoFile ) {
                openedDemos.remove( demoModName );
            }
        }
        else {
            return;
        }
    }

    dtMainWindow->setCursor( Qt::WaitCursor );

    DtWriteOptions options;
    options.converter = DC_BETAMAPFIX;
    options.newFileName = fName;
    options.newFileName.remove( "." + demo->fileInfo().fileExt );

    mainTabWidget->setEnabled( false );
    demo->writeSegment( &options );
    mainTabWidget->setEnabled( true );

    dtMainWindow->setCursor( Qt::ArrowCursor );

    if ( QFileInfo( fName ).absolutePath() == currentWorkingDir ) {
        mainTabWidget->updateMainTable();
    }

    close();
}

void DtConvertDialog::savePressed() {
    bool fileInMainTable = ( demo->fileInfo().filePath == currentWorkingDir );

    if ( fileInMainTable && demo->referenceCount > 1 ) {
        QMessageBox::warning( this, tr( "File is in use" ), tr( "Can't save an used demo" ) );
        return;
    }

    int act = QMessageBox::question( this, tr( "Save file" ),
                                     tr( "Are you sure want to overwrite this file?" ),
                                     QMessageBox::Yes | QMessageBox::No );
    if ( act != QMessageBox::Yes ) {
        return;
    }

    setCursor( Qt::WaitCursor );
    mainTabWidget->setEnabled( false );

    DtWriteOptions options;
    options.converter = DC_BETAMAPFIX;
    QString tmpName = demo->writeSegment( &options );

    QString demoName = demo->fileInfo().fileName();
    QString modName = getModifiedName( demoName );

    if ( !QFile::remove( demoName ) ) {
        return;
    }

    if ( !QFile::rename( tmpName, demoName ) ) {
        return;
    }

    QFile::remove( tmpName );

    if ( fileInMainTable ) {
        openedDemos.remove( modName );
        mainTabWidget->updateMainTable();
    }

    delete demo;
    mainTabWidget->setEnabled( true );
    setCursor( Qt::ArrowCursor );
    close();
}
