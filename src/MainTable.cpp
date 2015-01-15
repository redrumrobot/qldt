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
#include "MainTable.h"
#include "FileDialog.h"
#include "MainWindow.h"
#include "MainTabWidget.h"
#include "MainWidget.h"
#include "DirTree.h"
#include "Demo.h"
#include "FormatDialog.h"

#include <QKeyEvent>
#include <QMenu>
#include <QHeaderView>
#include <QMessageBox>
#include <QDateTime>
#include <QSplitter>
#include <QFileInfo>
#include <QLineEdit>

using namespace dtdata;

DtMainTable::DtMainTable( QWidget* parent ) : DtDemoTable( parent ) {
    setColumnCount( defaultMainTableColumnNames.count() );
    setHorizontalHeaderLabels( defaultMainTableColumnNames );
    loadColumnWidths( "DemoListColumns" );

    for ( int i = 0; i < defaultMainTableColumnNames.count(); ++i ) {
        setColumnHidden( i, !config.mainTableVisibleColumns.at( i ) );
    }

    mainDemoTable = this;
    horizontalHeader()->setSortIndicator( config.mainTableSortColumn,
                                          static_cast< Qt::SortOrder >( config.mainTableSortOrder ) );

    actRename = new QAction( QIcon( ":/res/edit-rename.png" ), tr( "Rename" ), this );
    connect( actRename, SIGNAL( triggered() ), this, SLOT( renameItem() ) );

    actAutoRename = new QAction( tr( "Auto Rename" ), this );
    connect( actAutoRename, SIGNAL( triggered() ), this, SLOT( autoRenameItem() ) );

    actOrganize = new QAction( tr( "Organize" ), this );
    connect( actOrganize, SIGNAL( triggered() ), this, SLOT( organizeDemos() ) );

    menuActions.append( actRename );
    menuActions.append( actAutoRename );
    menuActions.append( actOrganize );
}

void DtMainTable::saveColumns() {
    saveColumnWidths( "DemoListColumns" );
}

void DtMainTable::renameItem() {
    if ( !config.mainTableVisibleColumns.at( 0 ) ) {
        dtMainWindow->toggleMainTableNameColumn();
    }

    editIndex = item( currentIndex().row(), CT_NAME );
    editItem( editIndex );
}

void DtMainTable::autoRenameItem() {
    QString help = tr( "&lt;P&gt; - Recorded by player<br />"
                       "&lt;T&gt; - Game type<br />"
                       "&lt;M&gt; - Map name<br />"
                       "&lt;D&gt; - Record date and time<br />"
                       "&lt;N&gt; - Number<br /><br />"
                       "<b>Enter format for team modes and FFA here:</b>" );
    QString duelhelp = tr( "For duel, additional fields are available:<br />"
                       "&lt;FC&gt; - First player's clan<br />"
                       "&lt;FP&gt; - First player's name<br />"
                       "&lt;SC&gt; - Second player's clan<br />"
                       "&lt;SP&gt; - Second player's name<br /><br />"
                       "<b>Enter format for duel demos here:</b>" );

    DtFormatDialog renameDialog( tr( "Automatically rename demos" ),
    config.lastRenameFormat, help,
    config.lastDuelRenameFormat, duelhelp, this );
    DtFormatDialog::dialogButtons button = renameDialog.exec();

    if ( button == DtFormatDialog::BTN_OK ) {
        QList< int > selectedRows = getSelectedRows();

        foreach ( int rowNum, selectedRows ) {
            DtDemo* demo = demoAt( rowNum );

            if ( demo && demo->referenceCount > 1 ) {
                QMessageBox::warning( this, tr( "File is in use" ), tr( "Unable to rename an used demo" ) );
                return;
            }
        }

        foreach ( int rowNum, selectedRows ) {
            DtDemo* demo = demoAt( rowNum );
            QString newFileName = config.lastRenameFormat;
            if ( demo->getProto() == QZ_73 && demo->getGameType() == GT_DUEL ) {
                newFileName = config.lastDuelRenameFormat;
            }
            newFileName.replace( "<P>", demo->getClientName() );
            newFileName.replace( "<T>", getGameTypeName( demo->getProto(),
                                                         demo->getQ3Mod(),
                                                         demo->getGameType(),
                                                         true ) );

            newFileName.replace( "<M>", demo->getMapName() );

            QDateTime demoTime;
            demoTime.setTime_t( demo->getLevelStartTime() );
            QString dateTime = demoTime.toString( "yyyy_MM_dd-HH_mm_ss" );
            newFileName.replace( "<D>", dateTime );
            newFileName.replace( "<N>", QString::number( rowNum ) );

            if ( demo->getProto() == QZ_73 && demo->getGameType() == GT_DUEL )
            {
                if ( !demo->isGamesateParsed() ) demo->parseGamestateMsg();

                QString pp, pc;
                QString player = demo->getFirstPlace();
                cleanStringColors( player );
                pp = player.section( ' ', -1, -1 );
                pc = player.section( ' ', -2, -2 );
                newFileName.replace( "<FC>", pc );
                newFileName.replace( "<FP>", pp );

                player = demo->getSecondPlace();
                cleanStringColors( player );
                pp = player.section( ' ', -1, -1 );
                pc = player.section( ' ', -2, -2 );
                newFileName.replace( "<SC>", pc );
                newFileName.replace( "<SP>", pp );
            }

            newFileName.replace( ' ', '-' );
            // People have all kinds of crazy shit in their clantags, sanitize this a bit:
            newFileName.remove( QRegExp( "[?:*<>$\"/\\|]" ) );
            // "[,^@={}[]~!?:*|#%<>$\"'();`/\\]"

            QString newName;
            QString newNamePath;
            QString addNumString;
            int addNum = 0;

            do {
                newName = newFileName + addNumString;
                newNamePath = QString( "%1/%2.%3" ).arg( demo->fileInfo().filePath, newName,
                                                         demo->fileInfo().fileExt );
                addNumString = QString( "_%1" ).arg( ++addNum );
            } while ( QFile::exists( newNamePath ) );

            if ( renameDemo( demo, newName ) ) {
                item( rowNum, CT_NAME )->setText( newName );
            }
        }
    }
}

void DtMainTable::organizeDemos() {
    QString help = tr( "&lt;M&gt; - Month<br />"
                       "&lt;Y&gt; - Year<br />"
                       "&lt;T&gt; - Game type<br />"
                       "&lt;MN&gt; - Map name<br />" );
    QString placeholder = "";

    DtFormatDialog organizeDialog( tr( "Organize in subdirectories" ), config.lastOrganizeFormat, help, placeholder, "", this );
    organizeDialog.setFixedHeight( 220 );
    DtFormatDialog::dialogButtons button = organizeDialog.exec();

    if ( button == DtFormatDialog::BTN_OK ) {
        QList< int > selectedRows = getSelectedRows();

        if ( selectedRows.isEmpty() ) {
            for ( int i = 0; i < rowCount(); ++i ) {
                selectedRows.append( i );
            }
        }

        foreach ( int rowNum, selectedRows ) {
            DtDemo* demo = demoAt( rowNum );

            if ( demo && demo->referenceCount > 1 ) {
                QMessageBox::warning( this, tr( "File is in use" ), tr( "Unable to move an used demo" ) );
                return;
            }
        }

        QMultiMap< QString, DtDemo* > destPaths;
        QStringList subDirs;

        foreach ( int rowNum, selectedRows ) {
            DtDemo* demo = demoAt( rowNum );
            QString newFilePath = config.lastOrganizeFormat;
            newFilePath.replace( "<T>", getGameTypeName( demo->getProto(),
                                                         demo->getQ3Mod(),
                                                         demo->getGameType(),
                                                         true ) );
            QDateTime demoTime;
            demoTime.setTime_t( demo->getLevelStartTime() );
            newFilePath.replace( "<M>", demoTime.toString( "MM" ) );
            newFilePath.replace( "<Y>", demoTime.toString( "yyyy" ) );
            newFilePath.replace( "<MN>", demo->getMapName() );
            newFilePath.replace( ' ', '-' );
            newFilePath.replace( '\\', '/' );

            if ( !subDirs.contains( newFilePath ) ) {
                subDirs.append( newFilePath );
            }

            if ( demo ) {
                destPaths.insert( newFilePath, demo );
            }
        }

        foreach ( const QString& subDir, subDirs ) {
            QList< DtDemo* > demos = destPaths.values( subDir );
            DtDemoVec demoVec = demos.toVector();
            QString path = currentWorkingDir + "/" + subDir;

            QDir destDir( currentWorkingDir );

            if ( !destDir.exists( subDir ) ) {
                destDir.mkpath( subDir );
            }

            QStringList notRemoved = dtMainWindow->files()->move( demoVec, path );

            for ( int i = 0; i < demoVec.size(); ++i ) {
                DtDemo* demo = demoVec.at( i );

                if ( notRemoved.contains( demo->fileInfo().fileName() ) ) {
                    continue;
                }

                removeDemo( demo );
            }

            emit demoDeleted();
        }
    }
}

void DtMainTable::keyPressEvent( QKeyEvent* e ) {
    switch ( e->key() ) {
        case Qt::Key_F9 :
            renameItem();
        break;
    }

    DtDemoTable::keyPressEvent( e );
}

void DtMainTable::commitData( QWidget* editorWidget ) {
    QLineEdit* editor = qobject_cast< QLineEdit* >( editorWidget );
    QString fileName = editor->text().trimmed();

    if ( editIndex && editor && !fileName.isEmpty() ) {
        if ( !isAcceptedEncoding( fileName ) ) {
            return;
        }

        DtDemo* demo = tableHash.value( editIndex );

        if ( demo && demo->referenceCount > 1 ) {
            QMessageBox::warning( this, tr( "File is in use" ), tr( "Unable to rename an used demo" ) );
        }
        else if ( renameDemo( demo, fileName ) ) {
            DtDemoTable::commitData( editorWidget );
        }
    }

    editIndex = 0;
}

bool DtMainTable::renameDemo( DtDemo* demo, QString newFileName ) {
    if ( !demo ) {
        return false;
    }

    newFileName.replace( ' ', '-' );
    QString oldName = demo->fileInfo().fullFilePath;
    QString newName = QString( "%1/%2.%3" ).arg( demo->fileInfo().filePath,
                                                 newFileName,
                                                 demo->fileInfo().fileExt );
    QString oldModifiedName = getModifiedName( QFileInfo( oldName ) );

    if ( QFile::rename( oldName, newName ) ) {
        QFileInfo info( newName );

        demo->setFileInfo( info );
        openedDemos.remove( oldModifiedName );
        openedDemos.insert( getModifiedName( info ), demo );
        return true;
    }

    return false;
}

void DtMainTable::sortColumn( int column ) {
    Qt::SortOrder order = Qt::AscendingOrder;

    if ( column == config.mainTableSortColumn ) {
        order = ( config.mainTableSortOrder == Qt::AscendingOrder )
                ? Qt::DescendingOrder : Qt::AscendingOrder;
    }

    config.mainTableSortOrder = order;
    config.mainTableSortColumn = column;
    config.save();
    sortByColumn( column );
}

void DtMainTable::sortColumn( int column, Qt::SortOrder order ) {
    config.mainTableSortOrder = order;
    config.mainTableSortColumn = column;
    config.save();
    sortByColumn( column, order );
}

void DtMainTable::packSelectedDemos() {
    dtMainWindow->files()->pack( getSelectedDemos() );
    clearSelection();
}

void DtMainTable::copySelectedDemos( const QString& dest ) {
    if ( !dtMainWindow->files()->checkDestDir( dest ) ) {
        return;
    }

    if ( contextMenu ) {
        contextMenu->close();
    }

    dtMainWindow->files()->copy( getSelectedDemos(), dest );
    clearSelection();
}

bool DtMainTable::demosHaveReferences( const DtDemoVec& demos, const QString& msg ) {
    DtDemo* playedDemo = 0;

    if ( currentPlayDemoTable == this && currentPlayDemoIndex ) {
        playedDemo = tableHash.value( currentPlayDemoIndex, 0 );

        if ( playedDemo && playedDemo->getProto() == Q3_68 ) {
            playedDemo = 0;
        }
    }

    for ( int i = 0; i < demos.size(); ++i ) {
        DtDemo* demo = demos.at( i );

        if ( demo->referenceCount > 1 || ( playedDemo && playedDemo == demo ) ) {
            QMessageBox::warning( this, tr( "File is in use" ), msg );
            return true;
        }
    }

    return false;
}

void DtMainTable::moveSelectedDemos( const QString& dest ) {
    if ( !dtMainWindow->files()->checkDestDir( dest ) ) {
        return;
    }

    if ( contextMenu ) {
        contextMenu->close();
    }

    DtDemoVec selectedDemoVec = getSelectedDemos();

    if( demosHaveReferences( selectedDemoVec, tr( "Unable to move an used demos" ) ) ) {
        return;
    }

    QStringList notRemoved = dtMainWindow->files()->move( selectedDemoVec, dest );

    for ( int i = 0; i < selectedDemoVec.size(); ++i ) {
        DtDemo* demo = selectedDemoVec.at( i );

        if ( notRemoved.contains( demo->fileInfo().fileName() ) ) {
            continue;
        }

        removeDemo( demo );
    }

    emit demoDeleted();
}

void DtMainTable::deleteSelectedDemos() {
    DtDemoVec selectedDemoVec = getSelectedDemos();

    if ( selectedDemoVec.size() && confirmDeletion( tr( "Delete selected demos?" ) ) ) {
        if ( demosHaveReferences( selectedDemoVec, tr( "Unable to remove an used demos" ) ) ) {
            return;
        }

        QStringList notRemoved = dtMainWindow->files()->remove( selectedDemoVec );
        int row = currentIndex().row();

        for ( int i = 0; i < selectedDemoVec.size(); ++i ) {
            DtDemo* demo = selectedDemoVec.at( i );

            if ( notRemoved.contains( demo->fileInfo().fileName() ) ) {
                continue;
            }

            removeDemo( demo );
        }

        emit demoDeleted();

        if ( notRemoved.count() == 0 && rowCount() ) {
            if ( row > rowCount() - 1 ) {
                row = rowCount() - 1;
            }

            selectRow( row );
        }
    }
}

void DtMainTable::removeDemo( DtDemo* demo ) {
    QTableWidgetItem* key = tableHash.key( demo, 0 );

    if ( key ) {
        if ( key == markedName ) {
            deleteMarkedName();
        }

        removeRow( key->row() );
        tableHash.remove( key );
    }

    openedDemos.remove( getModifiedName( demo->fileInfo().fileName() ) );
    delete demo;

    if ( config.dropDemosToNewDir && config.dirTreeAlwaysOpened ) {
        if ( mainTabWidget->mainWidget && mainTabWidget->mainWidget->dirTree ) {
            mainTabWidget->mainWidget->dirTree->updateTmpNewDir();
        }
    }
}

void DtMainTable::clearMark() {
    if ( markedName ) {
        markedName->setIcon( QIcon() );

        for ( int i = 0; i < columnCount(); ++i ) {
            item( markedName->row(), i )->setCheckState( Qt::Unchecked );

            if ( demoAt( markedName->row() )->isBroken() ) {
                markedName->setIcon( icons->getIcon( I_BROKEN ) );
            }
        }

        updateMarkedRow();
    }
}

void DtMainTable::addDemo( DtDemo* demo ) {
    ++demo->referenceCount;

    int rowNum = rowCount();
    insertRow( rowNum );

    DtDemoTableItem* fName = new DtDemoTableItem( demo->fileInfo().baseName );
    setItem( rowNum, CT_NAME, fName );

    if ( demo->isBroken() ) {
        fName->setIcon( icons->getIcon( I_BROKEN ) );
    }

    tableHash.insert( fName, demo );

    DtDemoTableItem* fPlayer = new DtDemoTableItem( demo->getClientName() );
    setItem( rowNum, CT_PLAYER, fPlayer );

    QString gtName = demo->isInfoAvailable() ? getGameTypeName( demo->getProto(),
                                                                demo->getQ3Mod(),
                                                                demo->getGameType(),
                                                                true ) : "";
    DtDemoTableItem* fType = new DtDemoTableItem( gtName );
    setItem( rowNum, CT_TYPE, fType );

    DtDemoTableItem* fMap = new DtDemoTableItem( demo->getMapName(),
                                                 DtDemoTableItem::ST_MAP );
    setItem( rowNum, CT_MAP, fMap );

    QDateTime demoTime;
    demoTime.setTime_t( demo->getLevelStartTime() );

    QString demoDate = demo->isInfoAvailable() ? demoTime.toString( defaultDateFormat ) : "";
    DtDemoTableItem* fDate = new DtDemoTableItem( demoDate, DtDemoTableItem::ST_DATETIME );
    QList< QVariant > itemData;
    itemData << false << demo->getLevelStartTime();
    fDate->setData( Qt::UserRole, itemData );
    setItem( rowNum, CT_DATE, fDate );

    DtDemoTableItem* fServer = new DtDemoTableItem( demo->getHostName() );
    setItem( rowNum, CT_SERVER, fServer );

    DtDemoTableItem* fProto = new DtDemoTableItem;
    setProtocolItemData( demo->getProto(), fProto );
    setItem( rowNum, CT_PROTOCOL, fProto );
}

void DtMainTable::updateRow( int rowNum ) {
    DtDemo* demo = demoAt( rowNum );

    item( rowNum, CT_PLAYER )->setText( demo->getClientName() );
    item( rowNum, CT_TYPE )->setText( getGameTypeName( demo->getProto(), demo->getQ3Mod(),
                                                       demo->getGameType(), true ) );
    item( rowNum, CT_MAP )->setText( demo->getMapName() );

    QDateTime demoTime;
    demoTime.setTime_t( demo->getLevelStartTime() );

    item( rowNum, CT_DATE )->setText( demoTime.toString( defaultDateFormat ) );
    item( rowNum, CT_DATE )->data( Qt::UserRole ).toList()[ 1 ] = demo->getLevelStartTime();
    item( rowNum, CT_SERVER )->setText( demo->getHostName() );
    setProtocolItemData( demo->getProto(), item( rowNum, CT_PROTOCOL ) );
}

