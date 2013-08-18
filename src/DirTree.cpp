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

#include "DirTree.h"
#include "Data.h"
#include "MainTabWidget.h"
#include "FileDialog.h"
#include "MainWindow.h"
#include "DemoTable.h"
#include "MainTable.h"

#include <QApplication>
#include <QHeaderView>
#include <QMessageBox>
#include <QMenu>
#include <QStylePainter>
#include <QLineEdit>
#include <QContextMenuEvent>
#include <QDirIterator>
#include <QSplitter>
#include <QUrl>
#include <QTimer>

QString DtDirTree::cpLastPath;
QString DtDirTree::mvLastPath;
QDir::SortFlags DtDirTree::dirSorting = QDir::Name;
QDir::Filters DtDirTree::dirFilter = QDir::Dirs | QDir::NoDotAndDotDot;
QDir::Filters DtDirTree::fileFilter = QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks;

using namespace dtdata;

DtDirTree::DtDirTree( QWidget* parent, dirTreeMode mode ) : QTreeWidget( parent ) {
    treeMode = mode;
    header()->hide();

    setIconSize( QSize( 26, 26 ) );

    setItemDelegate( new DtDirTreeDelegate( this ) );

    qzSubDirIcon.addPixmap( QPixmap( ":/res/red_dir.png" ), QIcon::Normal, QIcon::Off );
    qzSubDirIcon.addPixmap( QPixmap( ":/res/red_dir_opened.png" ), QIcon::Normal, QIcon::On );

    qaSubDirIcon.addPixmap( QPixmap( ":/res/white_dir.png" ), QIcon::Normal, QIcon::Off );
    qaSubDirIcon.addPixmap( QPixmap( ":/res/white_dir_opened.png" ), QIcon::Normal, QIcon::On );

    newTmpDirIcon.addPixmap( QPixmap( ":/res/green_dir.png" ), QIcon::Normal, QIcon::Off );

    qzDirIcon.addFile( ":/res/qzdir.png" );
    qaDirIcon.addFile( ":/res/qadir.png" );

    connect( this, SIGNAL( itemExpanded( QTreeWidgetItem* ) ), this, SLOT( updateSize() ) );
    connect( this, SIGNAL( itemCollapsed( QTreeWidgetItem* ) ), this, SLOT( updateSize() ) );

    actNewDir = new QAction( QIcon( ":/res/folder-new.png" ), tr( "New" ), this );
    actRenameDir = new QAction( QIcon( ":/res/edit-rename.png" ), tr( "Rename" ), this );
    actDeleteDir = new QAction( QIcon( ":/res/edit-delete.png" ), tr( "Delete" ), this );
    actDirInfo = new QAction( QIcon( ":/res/info.png" ), tr( "Info" ), this );
    actPackDir = new QAction( QIcon( ":/res/zip.png" ), tr( "Pack" ), this );
    actCopyDemos = new QAction( tr( "Copy" ), this );
    actMoveDemos = new QAction( tr( "Move" ), this );

    connect( actNewDir, SIGNAL( triggered() ), this, SLOT( onNewDir() ) );
    connect( actRenameDir, SIGNAL( triggered() ), this, SLOT( onRenameDir() ) );
    connect( actDeleteDir, SIGNAL( triggered() ), this, SLOT( onDeleteDir() ) );
    connect( actDirInfo, SIGNAL( triggered() ), this, SLOT( onDirInfo() ) );
    connect( actPackDir, SIGNAL( triggered() ), this, SLOT( onPackDir() ) );
    connect( actCopyDemos, SIGNAL( triggered() ), this, SLOT( onCopyDemos() ) );
    connect( actMoveDemos, SIGNAL( triggered() ), this, SLOT( onMoveDemos() ) );

    qzDemoPath = config.getQzDemoPath();
    qaDemoPath = config.getQaDemoPath();

    if ( treeMode == DT_SELECT ) {
        lastPath = ( config.getSelectedGame() == Q_LIVE ) ? qzDemoPath : qaDemoPath;
    }
    else if ( treeMode == DT_COPY ) {
        lastPath = cpLastPath;
    }
    else if ( treeMode == DT_MOVE ) {
        lastPath = mvLastPath;
    }

    lastSelectedItem = 0;
    drawDragIndicator = false;
}

void DtDirTree::paintEvent( QPaintEvent* e ) {
    QTreeWidget::paintEvent( e );

    if ( drawDragIndicator ) {
        QPainter painter( viewport() );

        painter.setPen( QColor( "#3177c7" ) );
        painter.drawRect( dragIndicatorRect );

        QRect innerRect = dragIndicatorRect;
        innerRect.setTop( innerRect.top() + 1 );
        innerRect.setLeft( innerRect.left() + 1 );
        innerRect.setWidth( innerRect.width() - 1 );
        innerRect.setHeight( innerRect.height() - 1 );

        painter.setPen( QColor( "#8baed7" ) );
        painter.drawRect( innerRect );
    }
}

void DtDirTree::demoDragMoved( QDragMoveEvent* e ) {
    QTreeWidgetItem* item = itemAt( mapFrom( dtMainWindow, e->pos() ) );

    if( item ) {
        const QRect& itemRect = visualItemRect( item );
        showDragIndicator( itemRect );
    }
    else {
        hideDragIndicator();
    }
}

void DtDirTree::showDragIndicator( const QRect& rect ) {
    dragIndicatorRect = rect;
    dragIndicatorRect.setRight( dragIndicatorRect.right() - 1 );
    drawDragIndicator = true;

    QRect updateRect = rect;
    updateRect.setTop( updateRect.top() - updateRect.top() );
    updateRect.setBottom( updateRect.bottom() + updateRect.bottom() );
    updateRect.setLeft( 0 );
    updateRect.setWidth( viewport()->width() );
    setDirtyRegion( updateRect );
}

void DtDirTree::hideDragIndicator() {
    drawDragIndicator = false;
    setDirtyRegion( viewport()->rect() );
}

void DtDirTree::demoDragDropped( QDropEvent* e ) {
    hideDragIndicator();

    QTreeWidgetItem* item = itemAt( mapFrom( dtMainWindow, e->pos() ) );

    if( !item ) {
        return;
    }

    lastDropEvent = e;
    dropTargetItem = item;

    QMenu menu( this );
    menu.addAction( actCopyDemos );
    menu.addAction( actMoveDemos );
    menu.addAction( tr( "Cancel" ) );
    menu.exec( QCursor::pos() );

    lastDropEvent = 0;
    dropTargetItem = 0;
}

void DtDirTree::onCopyDemos() {
    if ( lastDropEvent ) {
        DtDemoTable* senderTable = qobject_cast< DtDemoTable* >( lastDropEvent->source() );

        if ( senderTable && dropTargetItem ) {
            senderTable->copySelectedDemos( dropTargetItem->data( 0, Qt::UserRole ).toString() );
        }
    }
}

void DtDirTree::onMoveDemos() {
    if ( lastDropEvent ) {
        DtDemoTable* senderTable = qobject_cast< DtDemoTable* >( lastDropEvent->source() );

        if ( senderTable && dropTargetItem ) {
            senderTable->moveSelectedDemos( dropTargetItem->data( 0, Qt::UserRole ).toString() );
        }
    }
}

void DtDirTree::onNewDir() {
    QTreeWidgetItem* item = new QTreeWidgetItem( currentItem() );
    item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );
    item->setCheckState( 0, Qt::Unchecked );

    QString parentPath = currentItem()->data( 0, Qt::UserRole ).toString();
    QString defDirNameBase = "New-folder";
    QString defDirName = parentPath + "/" + defDirNameBase;

    QDir dir( defDirName );

    int num = 0;

    while ( dir.exists() ) {
        defDirName = QString( "%1/%2%3" ).arg( parentPath, defDirNameBase ).arg( ++num );
        dir.setPath( defDirName );
    }

    QString pref = num ? QString::number( num ) : "";

    item->setText( 0, defDirNameBase + pref );
    item->setData( 0, Qt::UserRole, defDirName );
    item->setIcon( 0, parentPath.startsWith( qzDemoPath ) ? qzSubDirIcon : qaSubDirIcon );

    QFont itemFont( item->font( 0 ) );
    itemFont.setPixelSize( itemFont.pointSize() + 5 );
    item->setFont( 0, itemFont );

    if ( !currentItem()->isExpanded() ) {
        currentItem()->setExpanded( true );
    }

    updateSize();

    dirCreated = false;
    createNewDir = true;

    item->setFlags( item->flags() | Qt::ItemIsEditable );
    editIndex = item;
    editItem( item, 0 );
    item->setFlags( item->flags() & ~Qt::ItemIsEditable );
}

bool DtDirTree::isCurrentDirUsed() {
    return ( lastPath.startsWith( currentItem()->data( 0, Qt::UserRole ).toString() )
             && ( currentPlayDemoTable == mainDemoTable && mainDemoTable->getCurrentPlayDemoIndex() ) );
}

void DtDirTree::onRenameDir() {
    QTreeWidgetItem* current = currentItem();

    if ( !current ) {
        return;
    }

    if ( isCurrentDirUsed() ) {
        QMessageBox::warning( this, tr( "Directory is in use" ), tr( "Can't rename directory which is in use" ) );
        return;
    }

    createNewDir = false;

    current->setFlags( current->flags() | Qt::ItemIsEditable );
    editIndex = current;
    editItem( current, 0 );
    current->setFlags( current->flags() & ~Qt::ItemIsEditable );
}

void DtDirTree::renameItem( QTreeWidgetItem* renamedItem, const QString& oldPath,
                            const QString& newPath, bool rootItem = false ) {
    if ( !rootItem ) {
        QString dirPath = renamedItem->data( 0, Qt::UserRole ).toString();
        dirPath.replace( oldPath, newPath );
        renamedItem->setData( 0, Qt::UserRole, dirPath );
    }

    for ( int i = 0; i < renamedItem->childCount(); ++i ) {
        renameItem( renamedItem->child( i ), oldPath, newPath );
    }
}

void DtDirTree::renameItemTree( QTreeWidgetItem* rootItem, const QString& oldPath,
                                const QString& newPath ) {
    renameItem( rootItem, oldPath, newPath, true );
}

void DtDirTree::commitData( QWidget* editor ) {
    QLineEdit* ed = qobject_cast< QLineEdit* >( editor );

    if ( !ed || !editIndex || ed->text().isEmpty() ) {
        return;
    }

    QString newName = ed->text().trimmed();
    newName.replace( ' ', '-' );

    if ( !isAcceptedEncoding( newName ) ) {
        updateTree();

        if ( !config.dirTreeAlwaysOpened ) {
            updateSize();
        }

        return;
    }

    QString dirName = editIndex->data( 0, Qt::UserRole ).toString();
    QString newDirName = dirName.left( dirName.lastIndexOf( '/' ) ) + "/" + newName;

    QDir dir( dirName );
    bool selectParentDir = false;

    if ( dir.exists() ) { /* rename dir */
        if ( !dir.rename( dir.absolutePath(), newDirName ) ) {
            return;
        }

        if ( dirName == lastPath ) {
            selectParentDir = true;
        }

        if ( editIndex->childCount() > 0 ) {
            renameItemTree( editIndex, dirName, newDirName );
        }
    }
    else { /* create new */
        if ( QDir( newDirName ).exists() ) {
            updateTree();

            if ( !config.dirTreeAlwaysOpened ) {
                updateSize();
            }

            return;
        }

        dir.mkpath( newDirName );
        dirCreated = true;
    }

    editIndex->setData( 0, Qt::UserRole, newDirName );
    editIndex->setText( 0, newName );

    updateSize();

    if ( selectParentDir ) {
        lastPath = newDirName;
        QTimer::singleShot( 0, this, SLOT( setLastDir() ) );
    }
}

void DtDirTree::setLastDir(){
    emit dirSelected( lastPath );
}

int DtDirTree::filesCount( const QString& dirPath ) {
    QDir dir( dirPath );

    int count = 0;
    QDirIterator it( dir, QDirIterator::Subdirectories );

    while ( it.hasNext() ) {
        it.next();

        if ( it.fileInfo().isFile() ) {
            ++count;
        }
    }

    return count;
}

int DtDirTree::filesCount( const QDir& dir, bool& unknownFormat ) {
    int count = 0;
    QDirIterator it( dir, QDirIterator::Subdirectories );

    while ( it.hasNext() ) {
        it.next();

        if ( it.fileInfo().isFile() ) {
            ++count;

            if ( !unknownFormat && !demoProtos.contains( it.fileInfo().suffix() ) ) {
                unknownFormat = true;
            }
        }
    }

    return count;
}

int DtDirTree::getDirInfo( const QDir& dir, int& dirCount, quint64& totalSize ) {
    int cnt = 0;
    QDirIterator it( dir, QDirIterator::Subdirectories );

    while ( it.hasNext() ) {
        it.next();

        if ( it.fileInfo().isDir()
            && it.fileInfo().fileName().compare( "." )
            && it.fileInfo().fileName().compare( ".." ) )
        {
            ++dirCount;
        }

        if ( it.fileInfo().isFile() ) {
            ++cnt;
            totalSize += it.fileInfo().size();
        }
    }

    return cnt;
}

void DtDirTree::onDirInfo() {
    QString dirName = currentItem()->data( 0, Qt::UserRole ).toString();
    QDir dir( dirName );

    int dirCount = 1;
    quint64 totalSize = 0;

    int cnt = getDirInfo( dir, dirCount, totalSize );

    QString sSize = QString::number( totalSize / MiB, 'f', 1 );
    QString format = tr( "%1\n\n%2 files in %3 %4\nTotal size: %5 MB" );
    QString directoryText = dirCount > 1 ? tr( "directories" ) : tr( "directory" );
    QString infoText = format.arg( dirName ).arg( cnt ).arg( dirCount ).arg( directoryText ).arg( sSize );

    QMessageBox::information( this, tr( "Information" ), infoText, QMessageBox::Ok );
}

void DtDirTree::onDeleteDir() {
    if ( !currentItem() ) {
        return;
    }

    if ( isCurrentDirUsed() ) {
        QMessageBox::warning( this, tr( "Directory is in use" ), tr( "Can't delete directory which is in use" ) );
        return;
    }

    QString dirName = currentItem()->data( 0, Qt::UserRole ).toString();
    QDir dir( dirName );

    bool unknownFormat = false;
    bool removeDir = true;

    int cnt = filesCount( dir, unknownFormat );

    if ( cnt > 0 ) {
        QString warnText;

        if ( unknownFormat ) {
            warnText = tr( "This directory contains files in unknown format." ) + "\n";
        }
        else {
            warnText = tr( "This directory contains demo files." ) + "\n";
        }

        warnText += tr( "Are you sure want to delete it?" ) + "\n\n";
        warnText += tr( "Path" ) + ": " + dirName + "\n";
        warnText += tr( "Files" ) + ": " + QString::number( cnt );

        int act = QMessageBox::No;

        if ( unknownFormat ) {
            act = QMessageBox::warning( this, tr( "Delete directory" ), warnText,
                                        QMessageBox::Yes | QMessageBox::No );
        }
        else {
            act = QMessageBox::question( this, tr( "Delete directory" ), warnText,
                                         QMessageBox::Yes | QMessageBox::No );
        }

        removeDir = ( act == QMessageBox::Yes );
    }

    bool dirRemoved = false;
    bool dirChanged = false;

    if ( removeDir ) {
        dir.setFilter( dirFilter );
        dirRemoved = rmDir( dir );
    }

    if ( !dirRemoved ) {
        lastPath = dirName;
    }
    else {
        if ( lastPath.startsWith( currentItem()->data( 0, Qt::UserRole ).toString() ) ) {
            QTreeWidgetItem* item = currentItem()->parent();
            lastPath = item->data( 0, Qt::UserRole ).toString();

            item->setCheckState( 0, Qt::Checked );

            if ( lastSelectedItem &&
                 lastSelectedItem->data( 0, Qt::UserRole ).toString() != lastPath )
            {
                lastSelectedItem->setCheckState( 0, Qt::Unchecked );
            }

            lastSelectedItem = item;
            dirChanged = true;

            emit dirSelected( lastPath );
        }
        else {
            setUpdatesEnabled( false );
            delete currentItem();
            qApp->processEvents();
            clearSelection();
            updateSize();
            setUpdatesEnabled( true );
        }
    }

    if ( qobject_cast< QSplitter* >( parentWidget() ) ) { /* dir tree always opened */
        if ( dirRemoved && dirChanged ) {
            delete currentItem();
            qApp->processEvents();
            clearSelection();
        }
    }
    else {
        if ( dirChanged ) {
            updateTree();
        }
    }
}

bool DtDirTree::removeDir( QDir& dir ) {
    QStringList files = dir.entryList( fileFilter );

    foreach ( const QString& file, files ) {
        if ( !dir.remove( file ) ) {
            return false;
        }
    }

    QStringList dirs = dir.entryList( dirFilter );

    foreach ( const QString& dirName, dirs ) {
        QString subDirPath = dir.absolutePath() + "/" + dirName;
        QDir subDir( subDirPath, QString(), dirSorting, dirFilter );

        if ( !removeDir( subDir ) ) {
            return false;
        }

        dir.rmdir( subDirPath );
    }

    return true;
}

bool DtDirTree::rmDir( QDir& dir ) {
    if ( removeDir( dir ) ) {
        return dir.rmdir( dir.absolutePath() );
    }

    return false;
}

void DtDirTree::showEvent( QShowEvent* ) {
    updateTree();
    updateSize();

    setFocus( Qt::PopupFocusReason );
}

void DtDirTree::updateTree() {
    setUpdatesEnabled( false );

    reset();
    clear();
    items.clear();
    rootItems.clear();
    openedDirs.clear();

    QDir demoDir( qzDemoPath, QString(), dirSorting, dirFilter );

    pathExists = false;

    if ( !qzDemoPath.isEmpty() && demoDir.exists() ) {
        openedDirs.append( qzDemoPath );
        addDir( demoDir, 0, "Live" );
    }

    demoDir.setPath( qaDemoPath );

    if ( !qaDemoPath.isEmpty() && !qaDemoPath.startsWith( qzDemoPath ) && demoDir.exists() ) {
        openedDirs.append( qaDemoPath );
        addDir( demoDir, 0, "Arena" );
    }

    if ( !pathExists ) {
        lastPath.clear();
    }

    demoDb.removeUnusedDirs( openedDirs );
    setUpdatesEnabled( true );
}

void DtDirTree::showTmpNewDir() {
    const QString& baseDir = config.getSelectedGame() == Q_LIVE ?
                             config.getQzDemoPath() : config.getQaDemoPath();
    QTreeWidgetItem* rootItem = 0;

    foreach ( QTreeWidgetItem* baseItem, rootItems ) {
        if ( baseItem->data( 0, Qt::UserRole ).toString() == baseDir ) {
            rootItem = baseItem;
            break;
        }
    }

    if ( rootItem ) {
        lastPath = QString( "%1/%2" ).arg( baseDir, defaultNewDirName );
        updateTree();
        emit dirSelected( lastPath );
    }
}

void DtDirTree::updateTmpNewDir() {
    updateTree();
}

void DtDirTree::addDir( QDir& dir, QTreeWidgetItem* parentItem, QString name ) {
    QTreeWidgetItem* item;

    if ( parentItem ) {
        item = new QTreeWidgetItem( parentItem );
    }
    else {
        item = new QTreeWidgetItem( this );
        rootItems.append( item );
    }

    item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );
    item->setCheckState( 0, Qt::Unchecked );
    items.append( item );

    QFont itemFont( item->font( 0 ) );
    itemFont.setPixelSize( itemFont.pointSize() + 5 );
    item->setFont( 0, itemFont );

    if ( name.isEmpty() ) {
        name = dir.dirName();
        item->setIcon( 0, dir.absolutePath().startsWith( qzDemoPath )
                       ? qzSubDirIcon : qaSubDirIcon );

        if ( name == defaultNewDirName ) {
            name = tr( "New (%1)" ).arg( filesCount( dir.absolutePath() ) );
            item->setIcon( 0, newTmpDirIcon );
        }
        else {
            openedDirs.append( dir.absolutePath() );
        }
    }
    else {
        item->setIcon( 0, name.endsWith( "Live" ) ? qzDirIcon : qaDirIcon );
    }

    item->setText( 0, name );
    item->setData( 0, Qt::UserRole, dir.absolutePath() );

    if ( !pathExists && currentWorkingDir == dir.absolutePath() ) {
        item->setSelected( true );

        if ( !parentItem ) {
            item->setExpanded( true );
        }

        if ( treeMode == DT_SELECT ) {
            item->setCheckState( 0, Qt::Checked );
            lastSelectedItem = item;
        }

        pathExists = true;

        QTreeWidgetItem* iParent = parentItem;

        while ( iParent ) {
            iParent->setExpanded( true );
            iParent = iParent->parent();
        }
    }

    QStringList dirs = dir.entryList();

    if ( !parentItem ) {
        int newDirIndex;
        if ( ( newDirIndex = dirs.indexOf( defaultNewDirName ) ) != -1 ) {
            QString newDirPath = dirs.at( newDirIndex );
            dirs.removeAt( newDirIndex );
            dirs.insert( 0, newDirPath );
        }
    }

    foreach ( const QString& nDir, dirs ) {
        if ( parentItem || nDir != defaultTmpDirName ) {
            QString subDirPath = QString( "%1/%2" ).arg( dir.absolutePath(), nDir );
            QDir subDir( subDirPath, QString(), dirSorting, dirFilter );

            addDir( subDir, item );
        }
    }
}

void DtDirTree::cExpanded( QTreeWidgetItem* item, int& eCnt ) {
    for ( int i = 0; i < item->childCount(); ++i, ++eCnt ) {
        if ( item->child( i )->isExpanded() ) {
            cExpanded( item->child( i ), eCnt );
        }
    }
}

void DtDirTree::updateSize() {
    if ( qobject_cast< QSplitter* >( parentWidget() ) ) {
        return;
    }

    int expCount = rootItems.size();

    foreach ( QTreeWidgetItem* item, rootItems ) {
        if ( item->isExpanded() ) {
            cExpanded( item, expCount );
        }
    }

    QWidget* menu = qobject_cast< QWidget* >( parent() );

    int sbSize = style()->pixelMetric( QStyle::PM_ScrollBarExtent );
    int borderWidth = style()->pixelMetric( QStyle::PM_MenuPanelWidth );
    int bordersHWidth = ( borderWidth + style()->pixelMetric( QStyle::PM_MenuHMargin ) ) * 2;
    int bordersVWidth = ( borderWidth + style()->pixelMetric( QStyle::PM_MenuVMargin ) ) * 2;

    const int branchDecorWidth = 18;

    int pWidth = sizeHintForColumn( 0 ) + sbSize + branchDecorWidth;
    int pHeight = sizeHintForRow( 0 ) * expCount + sbSize;

    menu->resize( pWidth, pHeight );
    resize( pWidth - bordersHWidth, pHeight - bordersVWidth );
}

void DtDirTree::contextMenuEvent( QContextMenuEvent* e ) {
    QTreeWidgetItem* current = currentItem();

    if ( !current ) {
        return;
    }

    QString dirPath = current->data( 0, Qt::UserRole ).toString();
    QMenu menu( this );

    bool tmpDir = dirPath.endsWith( defaultNewDirName );

    if( !tmpDir ) {
        menu.addAction( actNewDir );
    }

    if ( dirPath != qzDemoPath && dirPath != qaDemoPath ) {
        if( !tmpDir ) {
            menu.addAction( actRenameDir );
        }

        menu.addAction( actDeleteDir );
    }

    if ( treeMode == DT_SELECT ) {
        menu.addSeparator();
        menu.addAction( actPackDir );
    }

    menu.addSeparator();
    menu.addAction( actDirInfo );
    menu.setMinimumWidth( 100 );
    menu.exec( e->globalPos() );
}

void DtDirTree::mouseDoubleClickEvent( QMouseEvent* e ) {
    QTreeWidgetItem* item = itemAt( e->pos() );

    if ( item ) {
        lastPath = item->data( 0, Qt::UserRole ).toString();

        emit dirSelected( lastPath );

        if ( treeMode == DT_SELECT ) {
            item->setCheckState( 0, Qt::Checked );

            if ( lastSelectedItem ) {
                lastSelectedItem->setCheckState( 0, Qt::Unchecked );
            }

            lastSelectedItem = item;
        }
        else if ( treeMode == DT_COPY ) {
            cpLastPath = lastPath;
        }
        else if ( treeMode == DT_MOVE ) {
            mvLastPath = lastPath;
        }
    }

    e->ignore();
}

void DtDirTree::vecAddDirDemos( QDir& dir, QStringList& demos ) {
    QStringList dirs = dir.entryList();
    QStringList files = dir.entryList( demoNameFilters,  fileFilter );

    foreach ( const QString& file, files ) {
        demos << QString( "%1/%2" ).arg( dir.absolutePath(), file );
    }

    foreach ( const QString& demoDir, dirs ) {
        QDir subDir( QString( "%1/%2" ).arg( dir.absolutePath(), demoDir ), QString(),
                     dirSorting, dirFilter );
        vecAddDirDemos( subDir, demos );
    }
}

void DtDirTree::onPackDir() {
    QStringList demos;
    QString path = currentItem()->data( 0, Qt::UserRole ).toString();
    QDir dir( path, QString(), dirSorting, dirFilter );

    vecAddDirDemos( dir, demos );

    dtMainWindow->files()->packDir( path, demos );
}

void DtDirTree::mousePressEvent( QMouseEvent* e ) {
    if ( e->button() == Qt::MidButton ) {
        QTreeWidgetItem* item = itemAt( e->pos() );
        item->setExpanded( !item->isExpanded() );
    }

    QTreeWidget::mousePressEvent( e );
}

void DtDirTree::editFinished() {
    if ( createNewDir && !dirCreated && editIndex ) {
        delete editIndex;
        editIndex = 0;
    }
}

DtDirAction::DtDirAction( QObject* parent, dirTreeMode mode ) : QWidgetAction( parent ),
    treeMode( mode ) {
}

QWidget* DtDirAction::createWidget( QWidget* parent ) {
    dirTree = new DtDirTree( parent, treeMode );

    if ( treeMode == DT_SELECT ) {
        connect( dirTree, SIGNAL( dirSelected( const QString& ) ),
                 parentWidget(), SLOT( onDirSelected( const QString& ) ) );
    }
    else if ( treeMode == DT_COPY ) {
        connect( dirTree, SIGNAL( dirSelected( const QString& ) ),
                 parentWidget(), SLOT( copySelectedDemos( const QString& ) ) );
    }
    else if ( treeMode == DT_MOVE ) {
        connect( dirTree, SIGNAL( dirSelected( const QString& ) ),
                 parentWidget(), SLOT( moveSelectedDemos( const QString& ) ) );
    }

    return dirTree;
}

DtDirTreeButton::DtDirTreeButton( QWidget* parent, QMenu* pMenu ) : QToolButton( parent ) {
    setPopupMode( QToolButton::InstantPopup );
    setMenu( pMenu );
    setProperty( "winStyle", qApp->style()->inherits( "QWindowsStyle" ) ? "true" : "false" );
    updateIcon();
}

void DtDirTreeButton::updateIcon() {
    setProperty( "qzIcon", ( config.getSelectedGame() == Q_LIVE ) ? "true" : "false" );
    //setStyle( QApplication::style() );
}

void DtDirTreeButton::paintEvent( QPaintEvent* ) {
    QStyleOptionToolButton opt;
    initStyleOption( &opt );
    opt.features &= ~QStyleOptionToolButton::HasMenu;

    QStylePainter( this ).drawComplexControl( QStyle::CC_ToolButton, opt );
}

DtDirTreeDelegate::DtDirTreeDelegate( QWidget* parent ) {
    connect( this, SIGNAL( closeEditor( QWidget*, QAbstractItemDelegate::EndEditHint ) ),
             parent, SLOT( editFinished() ) );
}

void DtDirTreeDelegate::updateEditorGeometry( QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& ) const {
    QRect optRect = option.rect;
    optRect.setLeft( optRect.left() + 29 );

    editor->setGeometry( optRect );
}

void DtDirTreeDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const {
    painter->save();

    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, index );

    bool selected = opt.state & QStyle::State_Selected;
    bool checked = ( opt.checkState );

    if ( selected || checked ) {
        QColor brushColor( "#e5e5e5" );
        QColor penColor( "#dddddd" );

        if ( checked ) {
            brushColor = selected ? "#d7dee5" : "#c6d5e5";
            penColor = "#cbd2d8";
        }

        QRect rec = opt.rect;

        QBrush pBrush = painter->brush();
        pBrush.setStyle( Qt::SolidPattern );
        pBrush.setColor( brushColor );

        painter->setPen( Qt::NoPen );
        painter->setBrush( pBrush );
        painter->drawRect( rec );

        painter->setPen( Qt::SolidLine );
        painter->setPen( penColor );

        painter->drawLine( rec.topLeft(), rec.topRight() );
        painter->drawLine( rec.bottomLeft(), rec.bottomRight() );
        painter->drawLine( rec.topLeft(), rec.bottomLeft() );
        painter->drawLine( rec.topRight(), rec.bottomRight() );

        painter->setPen( QColor( "black" ) );
    }

    QRect drawRect = opt.rect;

    QRect iconDrawRect = opt.rect;
    iconDrawRect.setLeft( opt.rect.left() + 3 );

    QVariant pvalue = index.data( Qt::DecorationRole );
    QSize iSize;

    if ( pvalue.isValid() ) {
        if ( pvalue.type() == QVariant::Icon ) {
            QIcon picon = qvariant_cast< QIcon >( pvalue );
            iSize = picon.availableSizes().at( 0 );
            QIcon::State state = opt.state & QStyle::State_Open ? QIcon::On : QIcon::Off;

            painter->drawPixmap( iconDrawRect.x(), iconDrawRect.y(), iSize.width(), iSize.height(),
                                 picon.pixmap( iSize, QIcon::Normal, state ) );
        }
    }

    drawRect.setLeft( opt.rect.left() + iSize.width() + 7 );

    painter->setFont( opt.font );
    painter->drawText( drawRect, Qt::AlignVCenter | Qt::TextSingleLine | Qt::AlignLeft,
                       opt.text );
    painter->restore();
}

