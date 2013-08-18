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

#include "DemoTable.h"
#include "TablesWidget.h"
#include "Data.h"
#include "DirTree.h"
#include "MainWindow.h"
#include "Demo.h"
#include "MainTabWidget.h"

#include <QKeyEvent>
#include <QMenu>
#include <QHeaderView>
#include <QMessageBox>
#include <QDateTime>
#include <QApplication>
#include <QUrl>
#include <QPainter>

using namespace dtdata;

DtDemoTable::DtDemoTable( QWidget* parent ) : DtTable( parent ) {
    setItemDelegate( new DtDemoTableDelegate( this ) );
    setSelectionMode( ExtendedSelection );
    setEditTriggers( NoEditTriggers );
    setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );

    horizontalHeader()->setClickable( true );
    horizontalHeader()->setSortIndicatorShown( true );
    horizontalHeader()->setStretchLastSection( true );
    connect( horizontalHeader(), SIGNAL( sectionClicked( int ) ), this, SLOT( sortColumn( int ) ) );

    qRegisterMetaType< DtDemo* >( "DtDemo*" );

    connect( this, SIGNAL( demoSelected( DtDemo* ) ), parent, SLOT( showDemoInfo( DtDemo* ) ) );
    connect( this, SIGNAL( emptyRowSelected() ), parent, SLOT( clearDemoInfo() ) );

    manualResize = false;
    markedName = 0;

    connect( this, SIGNAL( playDemo() ), parent, SIGNAL( actPlaydemoTriggered() ) );

    actPlaydemo = new QAction( icons->getIcon( I_PLAYDEMO ), tr( "Play" ), this );
    connect( actPlaydemo, SIGNAL( triggered() ), this, SLOT( playDemoQuake() ) );

    actPlaydemoOtherApp = new QAction( icons->getIcon( I_OTHER_SMALL ), config.otherAppTitle, this );
    connect( actPlaydemoOtherApp, SIGNAL( triggered() ), this, SLOT( playDemoOtherApp() ) );

    actSelectAll = new QAction( QIcon( ":/res/edit-select-all.png" ), tr( "Select All" ), this );
    connect( actSelectAll, SIGNAL( triggered() ), this, SLOT( selectAll() ) );

    actDelete = new QAction( QIcon( ":/res/delete_table_row.png" ), tr( "Delete" ), this );
    connect( actDelete, SIGNAL( triggered() ), this, SLOT( deleteSelectedDemos() ) );
    connect( this, SIGNAL( demoDeleted() ), parent, SLOT( clearDemoInfo() ) );

    actPack = new QAction( QIcon( ":/res/zip.png" ), tr( "Pack" ), this );
    connect( actPack, SIGNAL( triggered() ), this, SLOT( packSelectedDemos() ) );

    needsUpdate = false;
    installEventFilter( this );
    drawDragIndicator = false;

    currentPlayDemoIndex = 0;
    currentDemoIndex = -1;
    editIndex = 0;
}

void DtDemoTable::playDemoQuake() {
    playDemoRequestSource = RS_CONTEXTMENU;
    emit playDemo();
}

void DtDemoTable::playDemoOtherApp() {
    playDemoRequestSource = RS_CONTEXTMENU_OTHER;
    emit playDemo();
}

void DtDemoTable::paintEvent( QPaintEvent* e ) {
    DtTable::paintEvent( e );

    if ( drawDragIndicator ) {
        QPainter painter( viewport() );

        painter.setPen( QColor( "#3177c7" ) );
        painter.drawLine( 0, dragIndicatorTop, viewport()->width(), dragIndicatorTop );
        painter.setPen( QColor( "#8baed7" ) );
        painter.drawLine( 0, dragIndicatorTop + 1, viewport()->width(), dragIndicatorTop + 1 );
    }
}

void DtDemoTable::sortByColumn( int column ) {
    DtTable::sortByColumn( column );
}

void DtDemoTable::sortByColumn( int column, Qt::SortOrder order ) {
    DtTable::sortByColumn( column, order );
}

void DtDemoTable::mouseMoveEvent( QMouseEvent* e ) {
    if( !( e->buttons() & Qt::LeftButton ) ) {
        return;
    }

    if( ( e->pos() - dragStartPosition ).manhattanLength() < QApplication::startDragDistance() ) {
        return;
    }

    QMimeData* mimeData = new QMimeData;

    QList< int > selectedRows = getSelectedRows();
    QList< QUrl > demoUrls;

    foreach ( int rowNum, selectedRows ) {
        demoUrls << QUrl::fromLocalFile( demoAt( rowNum )->fileInfo().fileName() );
    }

    mimeData->setUrls( demoUrls );

    demoDrag = new QDrag( this );
    demoDrag->setMimeData( mimeData );
    demoDrag->exec( Qt::CopyAction );
}

QTableWidgetItem* DtDemoTable::dragPointItem( QPoint point ) {
    point = mapFrom( dtMainWindow, point );
    point.setY( point.y() - horizontalHeader()->height() );

    return itemAt( point );
}

void DtDemoTable::showDragIndicator( int itemTop, int itemHeight ) {
    dragIndicatorTop = itemTop;
    drawDragIndicator = true;
    setDirtyRegion( QRect( 0, itemTop - itemHeight, viewport()->width(), itemHeight * 2 ) );
}

void DtDemoTable::hideDragIndicator() {
    drawDragIndicator = false;
    setDirtyRegion( viewport()->rect() );
}

void DtDemoTable::demoDragMoved( QDragMoveEvent* e ) {
    QTableWidgetItem* item = dragPointItem( e->pos() );

    if( item ) {
        const QRect& itemRect = visualItemRect( item );
        showDragIndicator( itemRect.top() - 1, itemRect.height() + 3 );
    }
    else {
        hideDragIndicator();
    }
}

void DtDemoTable::demoDragDropped( QDropEvent* e ) {
    hideDragIndicator();

    QTableWidgetItem* topItem = dragPointItem( e->pos() );

    if( !topItem ) {
        return;
    }

    int insertRowNum = topItem->row();
    QList< int > selectedRows = getSelectedRows();

    if ( selectedRows.contains( insertRowNum ) ) {
        return;
    }

    clearSelection();

    QList< QList< QTableWidgetItem* > > rows;
    int selectedRowsCount = selectedRows.count();
    int tableColumnCount = columnCount();

    for ( int i = 0; i < selectedRowsCount; ++i ) {
        QList< QTableWidgetItem* > selectedRow;

        for ( int j = 0; j < tableColumnCount; ++j ) {
            selectedRow.append( takeItem( selectedRows.at( i ), j ) );
        }

        rows.append( selectedRow );
    }

    for ( int i = selectedRowsCount - 1; i >= 0; --i ) {
        removeRow( selectedRows.at( i ) );
    }

    insertRowNum = topItem->row();

    for ( int i = 0; i < selectedRowsCount; ++i ) {
        insertRow( insertRowNum );

        for ( int j = 0; j < tableColumnCount; ++j ) {
            setItem( insertRowNum, j, rows.at( i ).at( j ) );
        }

        ++insertRowNum;
    }
}

void DtDemoTable::mousePressEvent( QMouseEvent* e ) {
    if ( e->button() == Qt::LeftButton ) {
        dragStartPosition = e->pos();
    }

    DtTable::mousePressEvent( e );
}

void DtDemoTable::setUpdateNeeded( bool s ) {
    needsUpdate = s;
}

bool DtDemoTable::isUpdateNeeded() {
    return needsUpdate;
}

void DtDemoTable::selectNextRow() {
    if ( !selectionModel()->hasSelection() ) {
        return;
    }

    int rowToSelect = getLastSelectedRow() + 1;

    if ( rowToSelect > rowCount() - 1 ) {
        return;
    }

    selectionModel()->select( selectionModel()->model()->index( rowToSelect, 0 ),
                              QItemSelectionModel::Select | QItemSelectionModel::Rows );
}

void DtDemoTable::keyPressEvent( QKeyEvent* e ) {
    switch ( e->key() ) {
        case Qt::Key_Insert :
            selectNextRow();
        break;

        case Qt::Key_Home :
            selectRow( 0 );
        break;

        case Qt::Key_End :
            selectRow( rowCount() - 1 );
        break;

        case Qt::Key_Delete :
            deleteSelectedDemos();
        break;

        case Qt::Key_Enter :
        case Qt::Key_Return :
            if ( editIndex == 0 ) {
                dtMainWindow->playSelectedDemo();
            }
        break;
    }

    DtTable::keyPressEvent( e );
}

bool DtDemoTable::eventFilter( QObject* obj, QEvent* event ) {
    if ( event->type() == QEvent::WindowActivate ) {
        manualResize = true;
    }

    return QObject::eventFilter( obj, event );
}

void DtDemoTable::showEvent( QShowEvent* ) {
    setFocus( Qt::PopupFocusReason );
}

void DtDemoTable::removeSelectedRows() {
    QList< int > sRows = getSelectedRows();

    for ( int i = sRows.size() - 1; i >= 0; --i ) {
        --demoAt( sRows.at( i ) )->referenceCount;

        if ( item( sRows.at( i ), 0 ) == markedName ) {
            deleteMarkedName();
        }

        removeRow( sRows.at( i ) );
    }

    mainTabWidget->clearScanWidgetSelections( this );
    deleteUnusedDemos();
}

bool DtDemoTable::confirmDeletion( const QString& msg ) {
    if ( contextMenu ) {
        contextMenu->close();
    }

    if ( config.confirmOnDelete ) {
        int confirm = QMessageBox::question( this, tr( "Delete" ), msg,
                                             QMessageBox::Yes | QMessageBox::No );
        if ( confirm != QMessageBox::Yes ) {
            return false;
        }
    }

    return true;
}

DtDemoVec DtDemoTable::getSelectedDemos() {
    DtDemoVec sDemos;
    QList< QTableWidgetSelectionRange > ranges = selectedRanges();
    int rangesSize = ranges.size();

    for ( int i = 0; i < rangesSize; ++i ) {
        int rangeRowCount = ranges.at( i ).rowCount();

        for ( int j = 0; j < rangeRowCount; ++j ) {
            sDemos.append( demoAt( ranges.at( i ).topRow() + j ) );
        }
    }

    return sDemos;
}

int DtDemoTable::getSelectedRow() const {
    QList< int > sRows = getSelectedRows();
    return sRows.isEmpty() ? -1 : sRows.first();
}

int DtDemoTable::getLastSelectedRow() const {
    QList< int > sRows = getSelectedRows();
    return sRows.isEmpty() ? -1 : sRows.last();
}


void DtDemoTable::contextMenuEvent( QContextMenuEvent* e ) {
    if ( contextMenu ) {
        return;
    }

    contextMenu = new QMenu( this );
    contextMenu->setMinimumWidth( 100 );

    contextMenu->addAction( actPlaydemo );

    if ( config.otherAppMenu &&
         ( ( config.getSelectedGame() == Q_ARENA && config.otherAppDm68 ) ||
           ( config.getSelectedGame() == Q_LIVE && config.otherAppDm73 ) ) )
    {
        actPlaydemoOtherApp->setText( config.otherAppTitle );
        contextMenu->addAction( actPlaydemoOtherApp );
    }

    contextMenu->addSeparator();

    if ( !menuActions.isEmpty() ) {
        foreach ( QAction* action, menuActions ) {
            contextMenu->addAction( action );
        }

        contextMenu->addSeparator();
    }

    contextMenu->addAction( actSelectAll );
    contextMenu->addAction( actDelete );
    contextMenu->addSeparator();

    cpMenu = new QMenu( tr( "Copy to" ), this );
    cpMenu->setIcon( icons->getIcon( I_COPY ) );

    DtDirAction* cpAction = new DtDirAction( this, DT_COPY );
    cpMenu->addAction( cpAction );

    contextMenu->addMenu( cpMenu );

    mvMenu = new QMenu( tr( "Move to" ), this );

    mvMenu->setIcon( icons->getIcon( I_MOVE ) );
    DtDirAction* mvAction = new DtDirAction( this, DT_MOVE );
    mvMenu->addAction( mvAction );

    contextMenu->addMenu( mvMenu );

    contextMenu->addSeparator();
    contextMenu->addAction( actPack );

    contextMenu->exec( e->globalPos() );

    delete contextMenu;
}

const DtTableHash& DtDemoTable::getTableHash() {
    return tableHash;
}

void DtDemoTable::removeAllRows() {
    DtTable::removeAllRows();

    DtTableHashIterator it( tableHash );

    while ( it.hasNext() ) {
        it.next();
        --it.value()->referenceCount;
    }

    tableHash.clear();
}

QString DtDemoTable::msecToString( int mseconds ) {
    int minutes = static_cast< int >( mseconds / 60000.f );
    int seconds = static_cast< int >( ( mseconds / 1000.f ) - minutes * 60 );

    if ( minutes <= 60 ) {
        return QString( "%1:%2" ).arg( minutes ).arg( seconds, 2, 10, QChar( '0' ) );
    }

    int hours = static_cast< int >( minutes / 60.f );
    minutes -= hours * 60;

    return QString( "%1:%2:%3" ).arg( hours ).arg( minutes ).arg( seconds, 2, 10, QChar( '0' ) );
}

int DtDemoTable::stringToMsec( const QString& str ) {
    QStringList time = str.split( ":" );

    if ( time.size() <= 2 ) {
        return  time.at( 0 ).toInt() * 60000
                + time.at( 1 ).toInt() * 1000;
    }

    return  time.at( 0 ).toInt() * 3600000
            + time.at( 1 ).toInt() * 60000
            + time.at( 2 ).toInt() * 1000;
}

void DtDemoTable::updateMarkedRow() {
    QRect itemRect = visualItemRect( markedName );
    setDirtyRegion( QRect( 0, itemRect.top(), viewport()->width(), itemRect.height() ) );
}

void DtDemoTable::markDemoPlaying() {
    if ( !currentPlayDemoIndex ) {
        return;
    }

    markedName = currentPlayDemoIndex;
    markedName->setIcon( icons->getIcon( I_PLAYDEMO ) );

    for ( int i = 0; i < columnCount(); ++i ) {
        item( markedName->row(), i )->setCheckState( Qt::Checked );
    }

    updateMarkedRow();
}

void DtDemoTable::deleteMarkedName() {
    markedName = 0;
}

void DtDemoTable::setProtocolItemData( int proto, QTableWidgetItem* it ) {
    switch ( proto ) {
        case Q3_68 :
            it->setText( "Arena" );
            it->setIcon( icons->getIcon( I_Q3_SMALL ) );
            break;
        case QZ_73 :
            it->setText( "Live" );
            it->setIcon( icons->getIcon( I_QZ_SMALL ) );
            break;
    }
}

DtDemo* DtDemoTable::demoAt( int rowNum ) {
    return tableHash.value( item( rowNum, 0 ) );
}

void DtDemoTable::selectionChanged( const QItemSelection& o , const QItemSelection& n ) {
    if ( currentIndex().row() < 0 ) {
        return;
    }

    if ( isUpdateNeeded() ) {
        return;
    }

    if ( n.contains( currentIndex() ) ) {
        setCurrentIndex( QModelIndex() );
        setDirtyRegion( rect() );
        emit emptyRowSelected();
        return;
    }

    DtDemo* demo = demoAt( currentIndex().row() );

    if ( !demo->isGamesateParsed() && demo->parseGamestateMsg() ) {
        updateRow( currentIndex().row() );
    }

    emit demoSelected( demo );
    DtTable::selectionChanged( o, n );
    currentDemoIndex = currentIndex().row();
}

void DtDemoTable::mouseDoubleClickEvent( QMouseEvent* e ) {
    e->ignore();

    if ( e->button() == Qt::RightButton || currentIndex().row() < 0 ) {
        return;
    }

    if ( currentPlayDemoTable && currentPlayDemoTable != this ) {
        currentPlayDemoTable->clearMark();
    }

    playDemoRequestSource = RS_DBLCLICK;
    dtMainWindow->playSelectedDemo();
}

QTableWidgetItem* DtDemoTable::getCurrentPlayDemoIndex() const {
    return currentPlayDemoIndex;
}

void DtDemoTable::setCurrentDemoPlaying() {
    currentPlayDemoIndex = ( currentDemoIndex == -1 ) ? 0 : item( currentDemoIndex, 0 );
}

void DtDemoTable::setNextDemoPlaying() {
    int tableIndex = currentPlayDemoIndex->row() + 1;

    if ( tableIndex > rowCount() - 1 ) {
        tableIndex = 0;
    }

    currentPlayDemoIndex = item( tableIndex, 0 );
}

void DtDemoTable::stopDemoPlay() {
    currentPlayDemoIndex = 0;
}

void DtDemoTable::setPrevDemoPlaying() {
    int tableIndex = currentPlayDemoIndex->row() - 1;

    if ( tableIndex < 0 ) {
        tableIndex = rowCount() - 1;
    }

    currentPlayDemoIndex = item( tableIndex, 0 );
}

void DtDemoTableDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const {
    DtTableDelegate::paint( painter, option, index );
}

DtPlayersTable::DtPlayersTable( QWidget* parent ) : DtTable( parent ) {
    setColumnCount( 1 );
    setHorizontalHeaderLabels( QStringList() << tr( "Players" ) );
    horizontalHeader()->setResizeMode( QHeaderView::Fixed );
}

void DtPlayersTable::addPlayer( QString name ) {
    int rowNum = rowCount();
    insertRow( rowNum );
    setItem( rowNum, 0, new QTableWidgetItem( name ) );
}

void DtPlayersTable::commitData( QWidget* ) {

}

DtVariablesTable::DtVariablesTable( QWidget* parent ) : DtTable( parent ) {
    setColumnCount( 2 );
    setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
    setHorizontalHeaderLabels( QStringList() << tr( "Variable" ) << tr( "Value" ) );
    loadColumnWidths( "CommandsListColumns" );
}

void DtVariablesTable::commitData( QWidget* ) {

}

void DtVariablesTable::addVariable( QString key, QString val, bool toBegin ) {
    int rowNum = ( toBegin ) ? 0 : rowCount();
    insertRow( rowNum );
    setItem( rowNum, 0, new QTableWidgetItem( key ) );
    setItem( rowNum, 1, new QTableWidgetItem( val ) );
}

DtDemoTableItem::DtDemoTableItem( const QString& text, sortTypes sortType ) : QTableWidgetItem( text ) {
    sort = sortType;
    setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );
}

bool DtDemoTableItem::operator<( const QTableWidgetItem& other ) const {
    switch ( sort ) {
        case ST_DATETIME :
            return data( Qt::UserRole ).toList().at( 1 ).toInt()
                   < other.data( Qt::UserRole ).toList().at( 1 ).toInt();

        case ST_MAP : {
            const QString& map1 = data( Qt::DisplayRole ).toString();
            const QString& map2 = other.data( Qt::DisplayRole ).toString();
            QString mapName1;
            QString mapName2;
            int mapNum1 = 0;
            int mapNum2 = 0;

            QRegExp mapReg( "^([a-z]+)(\\d*)$" );

            if ( mapReg.indexIn( map1 ) != -1 ) {
                mapName1 = mapReg.cap( 1 );
                mapNum1 = mapReg.cap( 2 ).toInt();
            }

            if ( mapReg.indexIn( map2 ) != -1 ) {
                mapName2 = mapReg.cap( 1 );
                mapNum2 = mapReg.cap( 2 ).toInt();
            }

            return ( mapName1 == mapName2 ) ? mapNum1 < mapNum2 : mapName1 < mapName2;
        }
        default : break;
    }

    return data( Qt::DisplayRole ).toString() < other.data( Qt::DisplayRole ).toString();
}


