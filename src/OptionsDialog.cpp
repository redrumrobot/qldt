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

#include "OptionsDialog.h"

#include <QFileDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QKeyEvent>
#include <QApplication>

DtOptionsDialog::DtOptionsDialog( QWidget* parent ) :
        QDialog( parent ),
        btnApply( 0 ),
        optionsChanged( true )
{
    setWindowModality( Qt::ApplicationModal );

    mainLayout = new QVBoxLayout;
    mainLayout->setMargin( 3 );

    QHBoxLayout* buttonsLayout = new QHBoxLayout;

    btnDefaults = new DtOptionsButton( tr( "Defaults" ), this );
    connect( btnDefaults, SIGNAL( clicked() ), this, SLOT( defaults() ) );

    btnOK = new DtOptionsButton( "OK", this );
    connect( btnOK, SIGNAL( clicked() ), this, SLOT( ok() ) );

    btnApply = new DtOptionsButton( tr( "Apply" ), this, QIcon( ":/res/dialog-ok-apply.png" ) );
    connect( btnApply, SIGNAL( clicked() ), this, SLOT( apply() ) );
    btnApply->setEnabled( false );

    btnClose = new DtOptionsButton( tr( "Close" ), this );
    connect( btnClose, SIGNAL( clicked() ), this, SLOT( close() ) );

    buttonsLayout->addWidget( btnDefaults );
    buttonsLayout->addStretch( 1 );
    buttonsLayout->addWidget( btnOK );
    buttonsLayout->addWidget( btnApply );
    buttonsLayout->addWidget( btnClose );

    mainLayout->addLayout( buttonsLayout );

    setLayout( mainLayout );
}


int DtOptionsDialog::exec() {
    readConfig();
    return QDialog::exec();
}

void DtOptionsDialog::apply() {
    writeConfig();
    optionsChanged = false;
    btnApply->setEnabled( false );
}

void DtOptionsDialog::ok() {
    if ( optionsChanged ) {
        apply();
    }

    close();
}

void DtOptionsDialog::setOptionsChanged() {
    if ( optionsChanged ) {
        return;
    }

    optionsChanged = true;

    if ( btnApply ) {
        btnApply->setEnabled( true );
    }
}

DtPathEdit::DtPathEdit( QString nTitle, QString nText, bool fileDlg, QWidget* parent, int minWidth ) :
        QWidget( parent )
{
    QHBoxLayout* mainLayout = new QHBoxLayout;
    mainLayout->setMargin( 0 );

    if ( !nTitle.isEmpty() ) {
        title = new QLabel( nTitle, this );
        title->setMinimumWidth( minWidth );
    }

    textField = new QLineEdit( nText, this );
    textField->setMinimumWidth( 400 );
    connect( textField, SIGNAL( textChanged( const QString& ) ), parent, SLOT( setOptionsChanged() ) );
    textField->installEventFilter( this );
    selectDirBtn = new QToolButton( this );
    selectDirBtn->setIcon( QIcon( ":/res/document-open.png" ) );
    connect( selectDirBtn, SIGNAL( clicked() ), this, SLOT( showSelectPathDialog() ) );

    if ( !nTitle.isEmpty() ) {
        mainLayout->addWidget( title );
    }

    mainLayout->addWidget( textField );
    mainLayout->addWidget( selectDirBtn );

    setLayout( mainLayout );

    fileDialog = fileDlg;
}

void DtPathEdit::setFocus() {
    textField->setFocus();
}

void DtPathEdit::setTextFieldMinWidth( int width ) {
    textField->setMinimumWidth( width );
}

bool DtPathEdit::eventFilter( QObject* obj, QEvent* e ) {
    if ( obj == textField && e->type() == QEvent::KeyPress ) {
        QKeyEvent* keyEvent = static_cast< QKeyEvent* >( e );

        if ( keyEvent->key() == Qt::Key_Backslash && keyEvent->modifiers() == Qt::NoModifier ) {
            QKeyEvent event( QEvent::KeyPress, Qt::Key_Slash, Qt::NoModifier, "/" );
            qApp->sendEvent( textField, &event );
            return true;
        }
    }

    return false;
}

void DtPathEdit::showSelectPathDialog() {
    QFileDialog selectDialog( this );
    QString dir = ( fileDialog ) ? QFileInfo( textField->text() ).absolutePath() :
                                   textField->text();
    selectDialog.setDirectory( dir );
    selectDialog.setFileMode( ( fileDialog ) ? QFileDialog::ExistingFile : QFileDialog::Directory );

    if ( selectDialog.exec() ) {
        textField->setText( selectDialog.selectedFiles().first() );
        setPath( textField->text().replace( "\\", "/" ) );
    }
}

void DtPathEdit::setPath( QString nText ) {
    textField->setText( nText );
}

QString DtPathEdit::getPath() {
    QString path = textField->text();

    if ( path.size() > 1 && path.endsWith( '/' ) ) {
        path.chop( 1 );
    }

    return path;
}

DtOptionsButton::DtOptionsButton( const QString& text, QWidget* parent, const QIcon& icon ) : QPushButton( icon, text, parent ) {
    setFixedSize( 140, 30 );
}

DtOptionsButtonGroup::DtOptionsButtonGroup( QWidget* parent ) : QButtonGroup( parent ) {
    connect( this, SIGNAL( buttonClicked( int ) ), parent, SLOT( setOptionsChanged() ) );
}

DtOptionsComboBox::DtOptionsComboBox( QWidget* parent ) : QComboBox( parent ) {
    connect( this, SIGNAL( currentIndexChanged( int ) ), parent, SLOT( setOptionsChanged() ) );
}

DtOptionsCheckBox::DtOptionsCheckBox( QWidget* parent ) : QCheckBox( parent ) {
    connect( this, SIGNAL( stateChanged( int ) ), parent, SLOT( setOptionsChanged() ) );
}

DtOptionsLineEdit::DtOptionsLineEdit( const QString& text, QWidget* parent ) : QLineEdit( text, parent ) {
    connect( this, SIGNAL( textChanged( const QString& ) ), parent, SLOT( setOptionsChanged() ) );
}

DtOptionsClearLineEdit::DtOptionsClearLineEdit( const QString&, QWidget* parent ) : DtClearLineEdit( parent ) {
    connect( this, SIGNAL( textChanged( const QString& ) ), parent, SLOT( setOptionsChanged() ) );
}

DtOptionsSlider::DtOptionsSlider( Qt::Orientation o, QWidget* parent ) : QSlider( o, parent ) {
    connect( this, SIGNAL( valueChanged( int ) ), parent, SLOT( setOptionsChanged() ) );
}

DtOptionsSpinBox::DtOptionsSpinBox( QWidget* parent ) : QSpinBox( parent ) {
    connect( this, SIGNAL( valueChanged( int ) ), parent, SLOT( setOptionsChanged() ) );
}

