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

#ifndef DTDIRTREE_H
#define DTDIRTREE_H

#include <QTreeWidget>
#include <QDir>
#include <QWidgetAction>
#include <QToolButton>
#include <QStyledItemDelegate>

class DtMainTabWidget;
class DtDirTreeDelegate;

enum dirTreeMode {
    DT_SELECT,
    DT_COPY,
    DT_MOVE
};

class DtDirTree : public QTreeWidget {
    Q_OBJECT
public:
    DtDirTree( QWidget* parent, dirTreeMode mode );

    void demoDragMoved( QDragMoveEvent* e );
    void demoDragDropped( QDropEvent* e );
    static bool rmDir( QDir& dir );
    void showTmpNewDir();
    void updateTmpNewDir();

public slots:
    void editFinished();

protected:
    QIcon qzSubDirIcon;
    QIcon qaSubDirIcon;
    QIcon qzDirIcon;
    QIcon qaDirIcon;
    QIcon newTmpDirIcon;

    QString qzDemoPath;
    QString qaDemoPath;
    QString lastPath;
    bool pathExists;

    QAction* actNewDir;
    QAction* actDeleteDir;
    QAction* actRenameDir;
    QAction* actDirInfo;
    QAction* actPackDir;
    QAction* actCopyDemos;
    QAction* actMoveDemos;
    QDropEvent* lastDropEvent;
    QTreeWidgetItem* dropTargetItem;

    QRect dragIndicatorRect;
    bool drawDragIndicator;
    bool createNewDir;
    bool dirCreated;

    void showEvent( QShowEvent* );
    void contextMenuEvent( QContextMenuEvent* e );
    void commitData( QWidget* editor );
    void mouseDoubleClickEvent( QMouseEvent* e );
    void mousePressEvent( QMouseEvent* e );
    void paintEvent( QPaintEvent* e );

    QTreeWidgetItem* editIndex;
    QTreeWidgetItem* lastSelectedItem;
    QList< QTreeWidgetItem* > items;
    QList< QTreeWidgetItem* > rootItems;

    static QDir::SortFlags dirSorting;
    static QDir::Filters dirFilter;
    static QDir::Filters fileFilter;

    static QString cpLastPath;
    static QString mvLastPath;
    QStringList openedDirs;

    dirTreeMode treeMode;

    void updateTree();
    void addDir( QDir& dir, QTreeWidgetItem* parentItem = 0, QString name = QString() );
    int filesCount( const QDir& dir, bool& unknownFormat );
    int filesCount( const QString& dirPath );
    int getDirInfo( const QDir& dir, int& dirCount, quint64& totalSize );
    void cExpanded( QTreeWidgetItem* item, int& eCnt );
    void vecAddDirDemos( QDir& dir, QStringList& demos );
    void showDragIndicator( const QRect& rect );
    void hideDragIndicator();
    static bool removeDir( QDir& dir );
    bool isCurrentDirUsed();
    void renameItem( QTreeWidgetItem* renamedItem, const QString& oldPath,
                     const QString& newPath, bool rootItem );
    void renameItemTree( QTreeWidgetItem* rootItem, const QString& oldPath, const QString& newPath );

protected slots:
    void onNewDir();
    void onRenameDir();
    void onDeleteDir();
    void onDirInfo();
    void onPackDir();
    void updateSize();
    void onCopyDemos();
    void onMoveDemos();
    void setLastDir();

signals:
    void dirSelected( const QString& path );

};

class DtDirAction : public QWidgetAction {
    Q_OBJECT
public:
    DtDirAction( QObject* parent, dirTreeMode mode );

private:
    DtDirTree* dirTree;
    dirTreeMode treeMode;

protected:
    QWidget* createWidget( QWidget* parent );
};

class DtDirTreeButton : public QToolButton {
    Q_OBJECT
public:
    DtDirTreeButton( QWidget* parent, QMenu* pMenu );

    void updateIcon();

protected:
    void paintEvent( QPaintEvent* );
};

class DtDirTreeDelegate : public QStyledItemDelegate {
public:
    DtDirTreeDelegate( QWidget* parent = 0 );

    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    void updateEditorGeometry( QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& ) const;
};

#endif // DTDIRTREE_H
