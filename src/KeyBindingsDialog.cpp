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
#include "KeyBindingsDialog.h"

#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTabWidget>
#include <QMessageBox>
#include <QMenu>
#include <QKeyEvent>
#include <QPainter>
#include <QTableWidgetItem>

using namespace dtdata;

DtKeyBindingsDialog::DtKeyBindingsDialog( QWidget* parent ) : DtOptionsDialog( parent ) {
    setWindowTitle( tr( "Key bindings" ) );
    setMinimumSize( 700, 740 );

    tabs = new QTabWidget( this );
    QWidget* mainTab = new QWidget;
    QVBoxLayout* mainTabLayout = new QVBoxLayout;

    bindsTable = new DtBindsTable( this );
    bindsTable->setEditTriggers( QAbstractItemView::NoEditTriggers );

    QStringList actions;
    actions << tr( "Pause" ) << tr( "Slow timescale *" ) << tr( "Fast timescale *" ) <<
               tr( "Very fast timescale *" ) << tr( "Next demo" ) << tr( "Previous demo" ) <<
               tr( "Increase sound volume" ) << tr( "Decrease sound volume" ) <<
               tr( "10% sound volume" ) << tr( "20% sound volume" ) << tr( "30% sound volume" ) <<
               tr( "40% sound volume" ) << tr( "50% sound volume" ) << tr( "60% sound volume" ) <<
               tr( "70% sound volume" ) << tr( "80% sound volume" ) << tr( "90% sound volume" ) <<
               tr( "100% sound volume" ) << tr( "Mute sound" ) << tr( "Show scores *" ) <<
               tr( "Show accuracy *" ) << tr( "Show chat window" ) << tr( "Make screenshot" ) <<
               tr( "Repeat current demo" );

    bindsTable->setRowCount( actions.size() );
    bindsTable->setFixedHeight( 620 );

    QTableWidgetItem* actName;
    bindMenu = new QMenu( this );
    connect( bindMenu, SIGNAL( aboutToShow() ), this, SLOT( captureKeyPress() ) );
    connect( bindMenu, SIGNAL( aboutToHide() ), this, SLOT( captureEnd() ) );

    bindAction = new DtBindAction( this );
    bindMenu->addAction( bindAction );
    int row = 0;
    keyboardGrabbed = false;

    foreach ( const QString& actionName, actions ) {
        actName = new QTableWidgetItem( actionName );

        if ( actionName.endsWith( "*" ) ) {
            actName->setToolTip( tr( "Action ends when the key is released" ) );
        }

        bindsTable->setItem( row, DtBindsTable::TC_ACTION, actName );
        addBindButtons( bindsTable, row++, DtBindsTable::TC_KEY );
    }

    mainTabLayout->addWidget( bindsTable );

    QLabel* soundVolumeStepLbl = new QLabel( tr( "Sound volume step" ) );
    soundVolumeStepSb = new DtOptionsSpinBox( this );
    soundVolumeStepSb->setRange( 1, 100 );
    soundVolumeStepSb->setSuffix( " %" );

    QHBoxLayout* soundVolumeLayout = new QHBoxLayout;

    soundVolumeLayout->addWidget( soundVolumeStepLbl );
    soundVolumeLayout->addWidget( soundVolumeStepSb );
    soundVolumeLayout->addStretch( 1 );

    mainTabLayout->addLayout( soundVolumeLayout );
    mainTab->setLayout( mainTabLayout );

    tabs->addTab( mainTab, tr( "Standard" ) );

    QWidget* customTab = new QWidget;
    QVBoxLayout* customTabLayout = new QVBoxLayout;

    customBindsTable = new DtCustomBindsTable( this );
    customBindsTable->setEditTriggers( QAbstractItemView::NoEditTriggers );
    connect( customBindsTable, SIGNAL( newAction( int, int, const QString& ) ),
             this, SLOT( updateAction( int, int, const QString& ) ) );

    customTabLayout->addWidget( customBindsTable );

    addActionButton = new DtOptionsButton( tr( "Add action" ), this );
    connect( addActionButton, SIGNAL( clicked() ), this, SLOT( addCustomAction() ) );
    addActionButton->setFixedWidth( 170 );

    customTabLayout->addWidget( addActionButton );
    customTab->setLayout( customTabLayout );

    tabs->addTab( customTab, tr( "Custom" ) );

    mainLayout->insertWidget( 0, tabs );

    newAction = false;
}

void DtKeyBindingsDialog::addCustomAction() {
    int row = customBindsTable->rowCount();

    customBindsTable->setRowCount( row + 1 );

    addBindButtons( customBindsTable, row, DtCustomBindsTable::TC_KEY );
    customBindsTable->button( row, DtCustomBindsTable::TC_KEY )->setKey( -1 );
    customBindsTable->button( row, DtCustomBindsTable::TC_ALTKEY )->setKey( -1 );

    QTableWidgetItem* actKeyPress = new QTableWidgetItem;
    customBindsTable->setItem( row, DtCustomBindsTable::TC_KEYPRESS_ACTION, actKeyPress );
    customBindsTable->editItem( actKeyPress );
    customBindsTable->editIndex = actKeyPress;

    QTableWidgetItem* actKeyRelease = new QTableWidgetItem;
    customBindsTable->setItem( row, DtCustomBindsTable::TC_KEYRELEASE_ACTION, actKeyRelease );

    customBindsTable->clearHighlight();

    newAction = true;
}

void DtKeyBindingsDialog::removeAction( int row ) {
    if ( !newAction ) {
        int action = customBindsTable->button( row, DtCustomBindsTable::TC_KEY )->getAction();
        int key1 = customBindsTable->button( row, DtCustomBindsTable::TC_KEY )->getKey();
        int key2 = customBindsTable->button( row, DtCustomBindsTable::TC_ALTKEY )->getKey();

        customKeyPressActions.remove( action );
        customKeyReleaseActions.remove( action );
        playerKeys.remove( key1 );
        playerKeys.remove( key2 );
        playerAlternateKeys.remove( key1 );
        playerAlternateKeys.remove( key2 );
    }

    newAction = false;
    customBindsTable->removeRow( row );
    DtOptionsDialog::setOptionsChanged();
}

void DtKeyBindingsDialog::deleteRow() {
    removeAction( customBindsTable->currentRow() );
}

int DtKeyBindingsDialog::maxMapKey( const QMap< int, QString >& map ) {
    QMapIterator< int, QString > it( map );
    it.toBack();
    it.previous();
    return it.key();
}

void DtKeyBindingsDialog::updateAction( int row, int column, const QString& cmd ) {
    int actionNum;

    if ( !newAction ) {
        actionNum = customBindsTable->button( row, DtCustomBindsTable::TC_KEY )->getAction();
    }
    else {
        actionNum = 101;

        if ( !customKeyPressActions.isEmpty() ) {
            actionNum = maxMapKey( customKeyPressActions ) + 1;
        }

        customBindsTable->button( row, DtCustomBindsTable::TC_KEY )->setAction( actionNum );
        customBindsTable->button( row, DtCustomBindsTable::TC_ALTKEY )->setAction( actionNum );
    }

    if ( column == DtCustomBindsTable::TC_KEYPRESS_ACTION ) {
        customKeyPressActions.insert( actionNum, cmd );

        if ( !customKeyReleaseActions.contains( actionNum ) ) {
            customKeyReleaseActions.insert( actionNum, "" );
        }
    }
    else {
        customKeyReleaseActions.insert( actionNum, cmd );

        if ( !customKeyPressActions.contains( actionNum ) ) {
            customKeyPressActions.insert( actionNum, "" );
        }
    }

    newAction = false;

    DtOptionsDialog::setOptionsChanged();
}

void DtKeyBindingsDialog::addBindButtons( QTableWidget* table, int row, int from ) {
    DtOptionsBindButton* button;
    DtBindButtonWidget* buttonWidget;
    QHBoxLayout* buttonLayout;

    for ( int i = from; i <= from + 1; ++i ) {
        button = new DtOptionsBindButton( row, i, row, this );
        button->setMenu( bindMenu );
        button->installEventFilter( this );
        connect( button, SIGNAL( mouseEntered( int ) ), bindsTable, SLOT( onRowEntered( int ) ) );
        buttonLayout = new QHBoxLayout;
        buttonLayout->setMargin( 0 );
        buttonLayout->addWidget( button, 0, Qt::AlignCenter );
        buttonWidget = new DtBindButtonWidget( row, button, this );
        connect( buttonWidget, SIGNAL( mouseEntered( int ) ),
                 table, SLOT( onRowEntered( int ) ) );
        buttonWidget->setLayout( buttonLayout );
        table->setCellWidget( row, i, buttonWidget );
    }
}

bool DtKeyBindingsDialog::eventFilter( QObject* obj, QEvent* e ) {
    currentBindButton = qobject_cast< DtOptionsBindButton* >( obj );

    return QObject::eventFilter( obj, e );
}

void DtKeyBindingsDialog::captureKeyPress() {
    grabKeyboard();
    keyboardGrabbed = true;
    lastKeyCode = -1;
}

QString DtKeyBindingsDialog::getKeyName( int key ) {
    switch ( key ) {
        case -1 :               return tr( "None" );
        case Qt::Key_Shift :    return "Shift";
        case Qt::Key_Alt :      return "Alt";
        case Qt::Key_Control :  return "Ctrl";
        case Qt::Key_Meta :     return "Meta";
        default : return QKeySequence( key ).toString( QKeySequence::NativeText );
    }
}

void DtKeyBindingsDialog::captureEnd() {
    releaseKeyboard();
    keyboardGrabbed = false;

    if ( currentBindButton ) {
        if ( lastKeyCode != -1 &&
             ( playerKeys.contains( lastKeyCode )          ||
             playerAlternateKeys.contains( lastKeyCode ) ) )
        {
            if ( currentBindButton->getKey() != lastKeyCode ) {
                QMessageBox::warning( this, tr( "Error" ),
                                      tr( "Key \"%1\" is already defined." )
                                      .arg( getKeyName( lastKeyCode ) ) );
            }

            return;
        }

        if ( lastKeyCode == Qt::Key_Escape || lastKeyCode == Qt::Key_QuoteLeft ) {
            return;
        }

        int keyColumn = ( tabs->currentIndex() == 0 ) ?
                        DtBindsTable::TC_KEY : static_cast< int >( DtCustomBindsTable::TC_KEY );

        if ( currentBindButton->getColumn() == keyColumn ) {
            if ( currentBindButton->getKey() != -1 ) {
                playerKeys.remove( currentBindButton->getKey() );
            }

            if ( lastKeyCode != -1 ) {
                playerKeys.insert( lastKeyCode, currentBindButton->getAction() );
            }
        }
        else {
            if ( currentBindButton->getKey() != -1 ) {
                playerAlternateKeys.remove( currentBindButton->getKey() );
            }

            if ( lastKeyCode != -1 ) {
                playerAlternateKeys.insert( lastKeyCode, currentBindButton->getAction() );
            }
        }

        currentBindButton->setKey( lastKeyCode );
        DtOptionsDialog::setOptionsChanged();
    }
}

bool DtKeyBindingsDialog::event( QEvent* e ) {
    if ( e->type() == QEvent::KeyPress && keyboardGrabbed ) {
        QKeyEvent* keyEvent = static_cast< QKeyEvent* >( e );
        lastKeyCode = keyEvent->key();
        bindMenu->close();

        return true;
    }

    return DtOptionsDialog::event( e );
}

void DtKeyBindingsDialog::readKeyNames( int action, int& row ) {
    int key = config.playerKeys.key( action, -1 );

    DtOptionsBindButton* button = bindsTable->button( row, DtBindsTable::TC_KEY );
    button->setKey( key );

    key = config.playerAlternateKeys.key( action, -1 );

    button = bindsTable->button( row++, DtBindsTable::TC_ALTKEY );
    button->setKey( key );
}

void DtKeyBindingsDialog::readConfig() {
    optionsChanged = true;

    int row = 0;

    readKeyNames( DtConfig::AC_PAUSE, row );
    readKeyNames( DtConfig::AC_SLOW, row );
    readKeyNames( DtConfig::AC_FAST, row );
    readKeyNames( DtConfig::AC_VERYFAST, row );
    readKeyNames( DtConfig::AC_NEXT, row );
    readKeyNames( DtConfig::AC_PREV, row );
    readKeyNames( DtConfig::AC_SOUNDUP, row );
    readKeyNames( DtConfig::AC_SOUNDDOWN, row );
    readKeyNames( DtConfig::AC_SOUND10, row );
    readKeyNames( DtConfig::AC_SOUND20, row );
    readKeyNames( DtConfig::AC_SOUND30, row );
    readKeyNames( DtConfig::AC_SOUND40, row );
    readKeyNames( DtConfig::AC_SOUND50, row );
    readKeyNames( DtConfig::AC_SOUND60, row );
    readKeyNames( DtConfig::AC_SOUND70, row );
    readKeyNames( DtConfig::AC_SOUND80, row );
    readKeyNames( DtConfig::AC_SOUND90, row );
    readKeyNames( DtConfig::AC_SOUND100, row );
    readKeyNames( DtConfig::AC_MUTE, row );
    readKeyNames( DtConfig::AC_SCORES, row );
    readKeyNames( DtConfig::AC_ACC, row );
    readKeyNames( DtConfig::AC_CHAT, row );
    readKeyNames( DtConfig::AC_SCREENSHOT, row );
    readKeyNames( DtConfig::AC_REPEATDEMO, row );

    playerKeys = config.playerKeys;
    playerAlternateKeys = config.playerAlternateKeys;
    customKeyPressActions = config.customKeyPressActions;
    customKeyReleaseActions = config.customKeyReleaseActions;

    soundVolumeStepSb->setValue( config.qzSoundVolumeStep );

    customBindsTable->setRowCount( 0 );

    QMapIterator< int, QString > it( customKeyPressActions );
    QTableWidgetItem* colName;
    row = 0;

    while ( it.hasNext() ) {
        it.next();
        customBindsTable->setRowCount( row + 1 );

        colName = new QTableWidgetItem( it.value() );
        customBindsTable->setItem( row, DtCustomBindsTable::TC_KEYPRESS_ACTION, colName );

        colName = new QTableWidgetItem( customKeyReleaseActions.value( it.key() ) );
        customBindsTable->setItem( row, DtCustomBindsTable::TC_KEYRELEASE_ACTION, colName );

        addBindButtons( customBindsTable, row, DtCustomBindsTable::TC_KEY );

        int key = playerKeys.key( it.key(), -1 );

        DtOptionsBindButton* button = customBindsTable->button( row, DtCustomBindsTable::TC_KEY );
        button->setKey( key );
        button->setAction( it.key() );

        key = playerAlternateKeys.key( it.key(), -1 );

        button = customBindsTable->button( row, DtCustomBindsTable::TC_ALTKEY );
        button->setKey( key );
        button->setAction( it.key() );

        ++row;
    }

    optionsChanged = false;
    btnApply->setEnabled( false );
}

void DtKeyBindingsDialog::writeConfig() {
    config.playerKeys = playerKeys;
    config.playerAlternateKeys = playerAlternateKeys;
    config.customKeyPressActions = customKeyPressActions;
    config.customKeyReleaseActions = customKeyReleaseActions;
    config.qzSoundVolumeStep = soundVolumeStepSb->value();

    config.save();
}

void DtKeyBindingsDialog::defaults() {
    int act = QMessageBox::question( this,
                                     tr( "Defaults" ),
                                     tr( "Restore Defaults?" ),
                                     QMessageBox::Yes | QMessageBox::No );

    if ( act == QMessageBox::Yes ) {
        config.playerBindingsDefaults();
        readConfig();
    }
}

void DtKeyBindingsDialog::closeEvent( QCloseEvent* event ) {
    if ( optionsChanged ) {
        int act = QMessageBox::question( this,
                                         tr( "Options changed" ),
                                         tr( "Save changes?" ),
                                         QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel );
        if ( act == QMessageBox::Yes ) {
            apply();
        }
        else if ( act == QMessageBox::Cancel ) {
            event->ignore();
            return;
        }
    }

    optionsChanged = false;
    event->accept();
}

DtBindButtonWidget::DtBindButtonWidget( int rowNum, DtOptionsBindButton* cButton,
                                        QWidget* parent ) : QWidget( parent ) {
    row = rowNum;
    setMouseTracking( true );
    bindButton = cButton;
}

DtOptionsBindButton* DtBindButtonWidget::button() {
    return bindButton;
}

void DtBindButtonWidget::enterEvent( QEvent* ) {
    emit mouseEntered( row );
}

DtOptionsBindButton::DtOptionsBindButton( int rowNum, int colNum, int actCode, QWidget* parent )
                                          : DtOptionsButton( "", parent ) {
    row = rowNum;
    column = colNum;
    action = actCode;
    setMinimumHeight( 24 );
    setMouseTracking( true );
}

void DtOptionsBindButton::setAction( int nAct ) {
    action = nAct;
}

int DtOptionsBindButton::getRow() const {
    return row;
}

int DtOptionsBindButton::getColumn() const {
    return column;
}

int DtOptionsBindButton::getAction() const {
    return action;
}

void DtOptionsBindButton::setKey( int nKey ) {
    key = nKey;
    setText( DtKeyBindingsDialog::getKeyName( key ) );
}

int DtOptionsBindButton::getKey() const {
    return key;
}

void DtOptionsBindButton::enterEvent( QEvent* ) {
    emit mouseEntered( row );
}

DtBindsTable::DtBindsTable( QWidget* parent ) : DtTable( parent ) {
    setItemDelegate( new DtColorTableDelegate( this ) );
    setColumnCount( 3 );
    setShowGrid( false );

    setSelectionBehavior( QAbstractItemView::SelectRows );
    setSelectionMode( QAbstractItemView::NoSelection );

    QStringList columnNames;
    columnNames << tr( "Action" ) << tr( "Key" ) << tr( "Alternate" );
    setHorizontalHeaderLabels( columnNames );

    horizontalHeader()->setResizeMode( TC_ACTION, QHeaderView::Stretch );
    horizontalHeader()->resizeSection( TC_ACTION, 300 );

    horizontalHeader()->setResizeMode( TC_KEY, QHeaderView::Fixed );
    horizontalHeader()->resizeSection( TC_KEY, 200 );

    horizontalHeader()->setResizeMode( TC_ALTKEY, QHeaderView::Fixed );
    horizontalHeader()->resizeSection( TC_KEY, 200 );

    horizontalHeader()->setStretchLastSection( true );

    connect( this, SIGNAL( cellEntered( int, int ) ),
             this, SLOT( onCellEntered( int, int ) ) );

    setMouseTracking( true );
}

DtOptionsBindButton* DtBindsTable::button( int row, int column ) {
    DtBindButtonWidget* buttonWidget = qobject_cast< DtBindButtonWidget* >(
                                       cellWidget( row, column ) );
    return buttonWidget->button();
}

void DtBindsTable::onRowEntered( int row ) {
    onCellEntered( row, 0 );
}

void DtBindsTable::onCellEntered( int row, int ) {
    DtColorTableDelegate* delegate = qobject_cast< DtColorTableDelegate* >( itemDelegate() );
    delegate->highlightRow( row );
    setDirtyRegion( viewport()->rect() );
    update();
}

void DtBindsTable::clearHighlight() {
    DtColorTableDelegate* delegate = qobject_cast< DtColorTableDelegate* >( itemDelegate() );
    delegate->highlightRow( -1 );
    setDirtyRegion( viewport()->rect() );
    update();
}

void DtBindsTable::leaveEvent( QEvent* ) {
    clearHighlight();
}

DtBindsTableHeader::DtBindsTableHeader( Qt::Orientation orientation, QWidget* parent ) :
    QHeaderView( orientation, parent )
{
    setFixedHeight( 30 );

    keyDown = new QPixmap( ":/res/keypress.png" );
    keyUp = new QPixmap( ":/res/keyrelease.png" );
}

DtBindsTableHeader::~DtBindsTableHeader() {
    delete keyDown;
    delete keyUp;
}

void DtBindsTableHeader::paintSection( QPainter* painter, const QRect& rect, int logicalIndex ) const {
    painter->save();
    QHeaderView::paintSection( painter, rect, logicalIndex );
    painter->restore();

    if ( logicalIndex == 0 || logicalIndex == 1 ) {
        painter->drawPixmap( rect.left() + rect.width() / 2 - 10, 2,
                             ( logicalIndex ) ? *keyUp : *keyDown );
    }
}

DtCustomBindsTable::DtCustomBindsTable( QWidget* parent ) : DtBindsTable( parent ) {
    setColumnCount( 4 );

    setHorizontalHeader( new DtBindsTableHeader( Qt::Horizontal, this ) );

    QStringList columnNames;
    columnNames << "" << "" << tr( "Key" ) << tr( "Alternate" );
    setHorizontalHeaderLabels( columnNames );

    horizontalHeader()->setResizeMode( TC_KEYPRESS_ACTION, QHeaderView::Stretch );
    horizontalHeader()->resizeSection( TC_KEYPRESS_ACTION, 150 );
    horizontalHeaderItem( TC_KEYPRESS_ACTION )->setToolTip( tr( "Key press action" ) );

    horizontalHeader()->setResizeMode( TC_KEYRELEASE_ACTION, QHeaderView::Stretch );
    horizontalHeader()->resizeSection( TC_KEYRELEASE_ACTION, 150 );
    horizontalHeaderItem( TC_KEYRELEASE_ACTION )->setToolTip( tr( "Key release action" ) );

    horizontalHeader()->setResizeMode( TC_KEY, QHeaderView::Fixed );
    horizontalHeader()->resizeSection( TC_KEY, 200 );

    horizontalHeader()->setResizeMode( TC_ALTKEY, QHeaderView::Fixed );
    horizontalHeader()->resizeSection( TC_KEY, 200 );

    horizontalHeader()->setStretchLastSection( true );

    actNew = new QAction( QIcon( ":/res/insert_table_row.png" ), tr( "Add" ), this );
    connect( actNew, SIGNAL( triggered() ), parent, SLOT( addCustomAction() ) );
    actDelete = new QAction( QIcon( ":/res/delete_table_row.png" ), tr( "Delete" ), this );
    connect( actDelete, SIGNAL( triggered() ), parent, SLOT( deleteRow() ) );
}

void DtCustomBindsTable::mouseDoubleClickEvent( QMouseEvent* e ) {
    if( e->button() == Qt::LeftButton ) {
        QTableWidgetItem* clickedItem = itemAt( e->x(), e->y() );

        if( clickedItem && ( clickedItem->column() == TC_KEYPRESS_ACTION ||
                             clickedItem->column() == TC_KEYRELEASE_ACTION ) )
        {
            clickedItem->setFlags( clickedItem->flags() | Qt::ItemIsEditable );
            editItem( clickedItem );
            clickedItem->setFlags( clickedItem->flags() & ~Qt::ItemIsEditable );
            editIndex = clickedItem;
            clearHighlight();
        }
    }
}

void DtCustomBindsTable::contextMenuEvent( QContextMenuEvent* e ) {
    QMenu menu( this );

    menu.addAction( actNew );
    menu.addAction( actDelete );
    menu.exec( e->globalPos() );
}

void DtCustomBindsTable::commitData( QWidget* editor ) {
    QLineEdit* ed = qobject_cast< QLineEdit* >( editor );

    if ( !ed ) {
        return;
    }

    ed->setText( ed->text().trimmed() );

    emit newAction( editIndex->row(), editIndex->column(), ed->text() );

    DtTable::commitData( editor );
}

DtBindWidget::DtBindWidget( QWidget* parent ) : QWidget( parent ) {
    QHBoxLayout* labelLayout = new QHBoxLayout;
    QLabel* lbl = new QLabel( "<b>" + tr( "Press a key" ) + "</b>", this );
    labelLayout->addWidget( lbl, 1, Qt::AlignCenter );

    setLayout( labelLayout );
    setFixedWidth( 150 );
}

DtBindAction::DtBindAction( QWidget* parent ) : QWidgetAction( parent ) {

}

QWidget* DtBindAction::createWidget( QWidget* parent ) {
    bindWidget = new DtBindWidget( parent );

    return bindWidget;
}

