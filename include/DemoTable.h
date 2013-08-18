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

#ifndef DTDEMOTABLE_H
#define DTDEMOTABLE_H

#include "Table.h"
#include "TableDelegate.h"
#include "DemoData.h"

#include <QMutex>
#include <QPointer>

class DtPlayersTable;
class DtVariablesTable;
class DtDemoTableDelegate;
class DtDemo;

typedef QHash< QTableWidgetItem*, DtDemo* > DtTableHash;
typedef QHashIterator< QTableWidgetItem*, DtDemo* > DtTableHashIterator;

enum tableDemoPos {
    TD_PREV,
    TD_CUR,
    TD_NEXT,
    TD_NEXT_NOLOOP
};

typedef struct {
    DtDemo* demo;
    bool part;
    int startTime;
    int endTime;
    int timerValue;
} selectedDemo_t;

class DtDemoTable : public DtTable {
    Q_OBJECT

public:
    DtDemoTable( QWidget* parent = 0 );

    void removeAllRows();
    const DtTableHash& getTableHash();
    DtDemo* demoAt( int rowNum );
    void markDemoPlaying();
    virtual void clearMark() = 0;
    void deleteMarkedName();
    DtDemoVec getSelectedDemos();
    int getSelectedRow() const;
    int getLastSelectedRow() const;
    void setUpdateNeeded( bool s );
    bool isUpdateNeeded();
    bool eventFilter( QObject* obj, QEvent* event );
    void demoDragMoved( QDragMoveEvent* e );
    void demoDragDropped( QDropEvent* e );
    QTableWidgetItem* getCurrentPlayDemoIndex() const;
    void setCurrentDemoPlaying();
    void setPrevDemoPlaying();
    void setNextDemoPlaying();
    void stopDemoPlay();
    static QString msecToString( int msec );
    static int stringToMsec( const QString& str );

public slots:
    virtual void sortColumn( int column, Qt::SortOrder order ) = 0;
    virtual void sortColumn( int column ) = 0;
    virtual void deleteSelectedDemos() = 0;
    virtual void packSelectedDemos() = 0;
    virtual void copySelectedDemos( const QString& dest ) = 0;
    virtual void moveSelectedDemos( const QString& dest ) = 0;
    virtual void updateRow( int rowNum ) = 0;
    virtual void saveColumns() = 0;

protected:
    QTableWidgetItem* markedName;
    QTableWidgetItem* currentPlayDemoIndex;
    int currentDemoIndex;
    QPointer< QMenu > contextMenu;
    QList< QAction* > menuActions;
    QMenu* cpMenu;
    QMenu* mvMenu;
    QAction* actPlaydemo;
    QAction* actPlaydemoOtherApp;
    QAction* actSelectAll;
    QAction* actDelete;
    QAction* actPack;
    QMutex loadMutex;
    bool manualResize;
    bool needsUpdate;
    DtTableHash tableHash;
    bool drawDragIndicator;
    int dragIndicatorTop;
    QTableWidgetItem* editIndex;

    void showDragIndicator( int top, int itemHeight );
    void hideDragIndicator();
    QTableWidgetItem* dragPointItem( QPoint point );
    void setProtocolItemData( int proto, QTableWidgetItem* it );
    void showEvent( QShowEvent* );
    void mouseDoubleClickEvent( QMouseEvent* e );
    void selectionChanged( const QItemSelection& o, const QItemSelection& n );

    QPoint dragStartPosition;
    void mouseMoveEvent( QMouseEvent* e );
    void mousePressEvent( QMouseEvent* e );
    void paintEvent( QPaintEvent* e );
    void keyPressEvent( QKeyEvent* e );
    void contextMenuEvent( QContextMenuEvent* e );
    void removeSelectedRows();
    bool confirmDeletion( const QString& msg );
    void sortByColumn( int column, Qt::SortOrder order );
    void sortByColumn( int column );
    void updateMarkedRow();
    void selectNextRow();

protected slots:
    void playDemoQuake();
    void playDemoOtherApp();

signals:
    void rowSelected( int index, bool selected );
    void demoSelected( DtDemo* );
    void emptyRowSelected();
    void demoDeleted();
    void playDemo();
};

class DtDemoTableDelegate : public DtTableDelegate {
public:
    DtDemoTableDelegate( QWidget* parent = 0 ) : DtTableDelegate( parent ) {}

    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
};

class DtPlayersTable : public DtTable {
    Q_OBJECT
public:
    DtPlayersTable( QWidget* parent = 0 );

    void addPlayer( QString name );

protected slots:
    void commitData( QWidget* );
};

class DtVariablesTable : public DtTable {
    Q_OBJECT
public:
    DtVariablesTable( QWidget* parent = 0 );

    void addVariable( QString key, QString val, bool toBegin = false );

protected slots:
    void commitData( QWidget* );
};

class DtDemoTableItem: public QTableWidgetItem {
public:
    enum sortTypes {
        ST_STRING,
        ST_DATETIME,
        ST_MAP
    };

    DtDemoTableItem( const QString& text = "", sortTypes sortType = ST_STRING );

    bool operator<( const QTableWidgetItem& other ) const;

protected:
    sortTypes sort;
};

#endif // DTDEMOTABLE_H
