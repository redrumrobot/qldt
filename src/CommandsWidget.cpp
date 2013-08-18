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

#include "CommandsWidget.h"
#include "Data.h"
#include "Demo.h"

#include <QSplitter>
#include <QMenu>
#include <QHeaderView>
#include <QContextMenuEvent>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QSettings>

using namespace dtdata;

QHash< QString, QString > DtCommandsTable::commandNamesHash;
QHash< QString, QString > DtEditCommandTable::playerInfoHash;

DtCommandsWidget::DtCommandsWidget( QWidget* parent ) : QWidget( parent ),
    commandsTable( new DtCommandsTable( this ) ),
    editTable( new DtEditCommandTable( this ) )
{
    connect( commandsTable, SIGNAL( commandSelected( const QString& ) ),
             editTable, SLOT( editCommand( const QString& ) ) );

    connect( editTable, SIGNAL( commandUpdated( const QString& ) ),
             commandsTable, SLOT( setCommand( const QString& ) ) );

    mainSplitter = new QSplitter( Qt::Horizontal, this );
    mainSplitter->setChildrenCollapsible( false );
    mainSplitter->addWidget( commandsTable );
    mainSplitter->addWidget( editTable );

    QByteArray splitterState = config.settings->value( "Edit/commandsSplitter",
                                                       QByteArray() ).toByteArray();
    if ( !splitterState.isEmpty() ) {
        mainSplitter->restoreState( splitterState );
    }
    else {
        mainSplitter->setSizes( QList< int >() << 650 << 350 );
    }

    QHBoxLayout* mainLayout = new QHBoxLayout;

    mainLayout->addWidget( mainSplitter );

    setLayout( mainLayout );
}

DtCommandsWidget::~DtCommandsWidget() {
    config.settings->setValue( "Edit/commandsSplitter", mainSplitter->saveState() );
}

void DtCommandsWidget::setCommands( const DtCmdVec& commands ) {
    commandsTable->setCommands( commands );
    editTable->removeAllRows();
}

DtCmdVec DtCommandsWidget::getCommands() const {
    return commandsTable->getCommands();
}

bool DtCommandsWidget::isChanged() {
    return commandsTable->isChanged();
}

void DtCommandsWidget::setDemoProto( int sProto, int sQ3Mod ) {
    commandsTable->setDemoProto( sProto, sQ3Mod );
}

DtCommandsTable::DtCommandsTable( QWidget* parent ) : DtTable( parent ) {
    if ( commandNamesHash.isEmpty() ) {
        commandNamesHash.insert( "scores",      tr( "Scores" ) );
        commandNamesHash.insert( "tinfo",       tr( "Team info" ) );
        commandNamesHash.insert( "print",       tr( "Print" ) );
        commandNamesHash.insert( "acc",         tr( "Accuracy" ) );
        commandNamesHash.insert( "map_restart", tr( "Map restart" ) );
        commandNamesHash.insert( "cp",          tr( "Center print" ) );
        commandNamesHash.insert( "loaddefered", tr( "Load deferred" ) );
        commandNamesHash.insert( "rcmd",        tr( "Command" ) );
        commandNamesHash.insert( "pcp",         tr( "Pause message" ) );
    }

    setSelectionMode( DtTable::ExtendedSelection );
    setColumnCount( 3 );
    setHorizontalHeaderLabels( QStringList() << tr( "Time" ) << tr( "Command" ) << tr( "Value" ) );
    setSortingEnabled( false );

    horizontalHeader()->setResizeMode( QHeaderView::Stretch );
    horizontalHeader()->setResizeMode( 0, QHeaderView::Fixed );
    horizontalHeader()->resizeSection( 0, 80 );
    horizontalHeader()->setResizeMode( 1, QHeaderView::Fixed );
    horizontalHeader()->resizeSection( 1, 220 );

    actDelete = new QAction( QIcon( ":/res/delete_table_row.png" ), tr( "Delete" ), this );
    connect( actDelete, SIGNAL( triggered() ), this, SLOT( deleteSelectedStrings() ) );

    actInsert = new QAction( QIcon( ":/res/insert_table_row.png" ), tr( "Insert" ), this );
    connect( actInsert, SIGNAL( triggered() ), this, SLOT( insertAfterSelected() ) );
}

void DtCommandsTable::deleteSelectedStrings() {
    QList< int > sRows = getSelectedRows();

    for ( int i = sRows.size() - 1; i >= 0; --i ) {
        removeRow( sRows.at( i ) );
        changed = true;
    }
}

void DtCommandsTable::insertAfterSelected() {
    int row = currentIndex().row() + 1;
    clearSelection();

    insertRow( row );

    QTableWidgetItem* nItem = new QTableWidgetItem( item( row - 1, 0 )->text() );
    setItem( row, 0, nItem );

    nItem = new QTableWidgetItem;
    nItem->setData( Qt::UserRole, "_dt_new_cmd" );
    setItem( row, 1, nItem );
    editItem( nItem );

    nItem = new QTableWidgetItem;
    setItem( row, 2, nItem );
}

void DtCommandsTable::setDemoProto( int sProto, int sQ3Mod ) {
    proto = sProto;
    q3Mod = sQ3Mod;
}

QString DtCommandsTable::getConfigStringName( int index ) {
    if ( proto == Q3_68 ) {
        switch ( index ) {
            case 0  : return tr( "Server info" );
            case 1  : return tr( "System info" );
            case 2  : return tr( "Music" );
            case 3  : return tr( "Map message" );
            case 4  : return tr( "Message of the day" );
            case 5  : return tr( "Warmup end time" );
            case 6  : return tr( "Scores" ) +" 1";
            case 7  : return tr( "Scores" ) + " 2";
            case 8  : return tr( "Vote time" );
            case 9  : return tr( "Vote string" );
            case 10 : return tr( "Vote Yes" );
            case 11 : return tr( "Vote No" );
            case 20 : return tr( "Game version" );
            case 21 : return tr( "Match start time" );
            case 22 : return tr( "Intermission" );
            case 23 : return tr( "Flag status" );
            case 27 : return tr( "Items" );
        }

        if ( q3Mod == MOD_CPMA ) {
            if ( index == 672 ) return tr( "Game info" );
            if ( index == 710 ) return tr( "Round info" );
        }
    }
    else {
        switch ( index ) {
            case 0   : return tr( "Server info" );
            case 1   : return tr( "System info" );
            case 2   : return tr( "Music" );
            case 3   : return tr( "Map message" );
            case 5   : return tr( "Warmup end time" );
            case 6   : return tr( "Scores" ) +" 1";
            case 7   : return tr( "Scores" ) + " 2";
            case 8   : return tr( "Vote time" );
            case 9   : return tr( "Vote string" );
            case 10  : return tr( "Vote Yes" );
            case 11  : return tr( "Vote No" );
            case 12  : return tr( "Game version" );
            case 13  : return tr( "Match start time" );
            case 14  : return tr( "Intermission" );
            case 15  : return tr( "Items" );
            case 658 : return tr( "Flag status" );
            case 659 : return tr( "First place" );
            case 660 : return tr( "Second place" );
            case 661 : return tr( "Round countdown" );
            case 662 : return tr( "Round start" );
            case 663 : return tr( "Red team survivors" );
            case 664 : return tr( "Blue team survivors" );
            case 669 : return tr( "Pause begin" );
            case 670 : return tr( "Pause end" );
            case 671 : return tr( "Red team timeouts left" );
            case 672 : return tr( "Blue team timeouts left" );
            case 679 :
            case 680 : return tr( "Level design by" );
            case 681 : return tr( "Waiting on advertisement" );
            case 682 : return tr( "Move settings" );
            case 683 : return tr( "Weapon settings" );
            case 684 : return tr( "Player models" );
            case 690 : return tr( "Most damage dealt" );
            case 691 : return tr( "Most accurate" );
            case 692 : return tr( "Best item control" );
        }
    }

    int csSounds;
    int csPlayers;
    int csLocations;

    if ( proto == Q3_68 ) {
        csSounds = 289;
        csPlayers = 544;
        csLocations = 608;
    }
    else {
        csSounds = 274;
        csPlayers = 529;
        csLocations = 593;
    }

    if ( index >= csSounds && index < csPlayers ) {
        return tr( "Sound %1" ).arg( index - csSounds + 1 );
    }
    else if ( index >= csPlayers && index < csPlayers + MAX_CLIENTS ) {
        return tr( "Player %1" ).arg( index - csPlayers + 1 );
    }
    else if ( index >= csLocations && index < csLocations + MAX_LOCATIONS ) {
        return tr( "Location %1" ).arg( index - csLocations + 1 );
    }

    return QString::number( index );
}

QString DtCommandsTable::getCommandName( const QString& command ) {
    return commandNamesHash.value( command, command );
}

QString DtCommandsTable::getCommandLine( bool cfgString, QString command, const QString& cmd,
                                         bool& quotes ) {
    if ( !cfgString ) {
        command.remove( cmd );

        if ( command.startsWith( ' ' ) ) {
            command.remove( 0, 1 );
        }
    }

    if ( command.endsWith( "\n" ) ) {
        command.chop( 1 );
    }

    if ( command.startsWith( '"' ) ) {
        quotes = true;
        command.remove( 0, 1 );
        command.chop( 1 );
    }

    if ( command.endsWith( "\n" ) ) {
        command.chop( 1 );
    }

    return command;
}

void DtCommandsTable::setCommands( const DtCmdVec& commands ) {
    removeAllRows();
    int commandsCount = commands.count();
    setRowCount( commandsCount );
    int row = 0;

    for ( int i = 0; i < commandsCount; ++i ) {
        float time = commands.at( i ).time / 1000.f;

        QString timeStr = ( time < 0 ) ? "" : QString::number( time, 'f', 3 );

        QTableWidgetItem* item = new QTableWidgetItem( timeStr );
        setItem( row, 0, item );

        bool cfgString = ( commands.at( i ).csIndex >= 0 );

        QString cmd;

        if ( !cfgString ) {
            cmd = commands.at( i ).cmd.split( QRegExp( "\\s+" ) ).at( 0 );
        }

        QString cmdName = cfgString ?
                          getConfigStringName( commands.at( i ).csIndex ) :
                          getCommandName( cmd );

        item = new QTableWidgetItem( cmdName );
        item->setFlags( item->flags() & ~Qt::ItemIsEditable );

        if ( cfgString ) {
            QString big = ( commands.at( i ).big ) ? "b" : "";
            QString bigNum = ( commands.at( i ).big ) ? QString::number( commands.at( i ).bcsNum ) : "";

            item->setData( Qt::UserRole, QString( "%1cs%2 %3" )
                           .arg( big, bigNum )
                           .arg( commands.at( i ).csIndex ) );
        }
        else {
            item->setData( Qt::UserRole, cmd );
        }

        setItem( row, 1, item );

        bool quotes = false;
        QString cmdLine = getCommandLine( cfgString, commands.at( i ).cmd, cmd, quotes );

        item = new QTableWidgetItem( cmdLine );

        if ( cmdLine.isEmpty() ) {
            item->setFlags( item->flags() & ~Qt::ItemIsEditable );
        }

        setItem( row, 2, item );

        ++row;
    }

    changed = false;
}

DtCmdVec DtCommandsTable::getCommands() const {
    int commandsCount = rowCount();
    DtCmdVec commands;

    for ( int i = 0; i < commandsCount; ++i ) {
        QString cmd;
        int bcsNum = 0;
        int csIndex = -1;

        int time = item( i, 0 )->text().remove( '.' ).toInt();

        if ( item( i, 0 )->text().isEmpty() || item( i, 0 )->text().toInt() < 0 ) {
            time = -1;
        }

        bool header = ( time == -1 );
        QByteArray cmdArray = item( i, 1 )->data( Qt::UserRole ).toByteArray();

        if ( cmdArray == "_dt_new_cmd" ) {
            cmdArray = item( i, 1 )->text().toAscii();
        }

        if ( cmdArray.isEmpty() ) {
            continue;
        }

        const char* cmdStr = cmdArray.data();

        bool big = ( cmdStr == strstr( cmdStr, "bcs" ) );

        if ( big ) {
            bcsNum = atoi( cmdStr + 3 );
            csIndex = atoi( cmdStr + 5 );
        }
        else if ( cmdStr == strstr( cmdStr, "cs" ) ) {
            csIndex = atoi( cmdStr + 3 );
        }

        if ( !header ) {
            cmd += cmdArray;

            if ( !item( i, 2 )->text().isEmpty() ) {
                cmd += " ";
            }
        }

        if ( !header ) {
            cmd += '"';
        }

        cmd += item( i, 2 )->text();

        if ( cmdArray == "print" ) {
            cmd += "\n";
        }

        if ( !header ) {
            cmd += "\"\n";
        }

        commands.append( DtDemoCommand( time, csIndex, cmd, big, bcsNum ) );
    }

    return commands;
}

void DtCommandsTable::contextMenuEvent( QContextMenuEvent* e ) {
    QMenu contextMenu( this );
    contextMenu.setMinimumWidth( 100 );
    contextMenu.addAction( actInsert );
    contextMenu.addAction( actDelete );
    contextMenu.exec( e->globalPos() );
}

void DtCommandsTable::commitData( QWidget* editorWidget ) {
    DtTable::commitData( editorWidget );

    emit commandSelected( item( currentIndex().row(), 2 )->text() );
    changed = true;
}

bool DtCommandsTable::isChanged() {
    return changed;
}

void DtCommandsTable::selectionChanged( const QItemSelection& o, const QItemSelection& n ) {
    emit commandSelected( item( currentIndex().row(), 2 )->text() );
    DtTable::selectionChanged( o, n );
}

void DtCommandsTable::setCommand( const QString& command ) {
    if ( proto == Q3_68 && q3Mod == MOD_CPMA ) {
        QString cmd = command;
        QByteArray cmdArray = item( currentIndex().row(), 1 )->data( Qt::UserRole ).toByteArray();
        const char* cmdStr = cmdArray.data();

        if ( cmdStr == strstr( cmdStr, "cs" ) ) {
            int csIndex = atoi( cmdStr + 3 );

            if ( csIndex == 710 && cmd.endsWith( confDelimeter ) ) {
                cmd.chop( 1 );
            }
            else if ( csIndex >= 728 && csIndex <= 731 ) {
                cmd += confDelimeter;
            }
        }

        item( currentIndex().row(), 2 )->setText( cmd );
    }
    else {
        item( currentIndex().row(), 2 )->setText( command );
    }

    changed = true;
}

DtEditCommandTable::DtEditCommandTable( QWidget* parent ) : DtTable( parent ) {
    if ( playerInfoHash.isEmpty() ) {
        playerInfoHash.insert( "n",         tr( "Name" ) );
        playerInfoHash.insert( "t",         tr( "Team" ) );
        playerInfoHash.insert( "model",     tr( "Model" ) );
        playerInfoHash.insert( "hmodel",    tr( "Head model" ) );
        playerInfoHash.insert( "c1",        tr( "Color" ) + " 1" );
        playerInfoHash.insert( "c2",        tr( "Color" ) + " 2" );
        playerInfoHash.insert( "hc",        tr( "Handicap" ) );
        playerInfoHash.insert( "w",         tr( "Wins" ) );
        playerInfoHash.insert( "l",         tr( "Losses" ) );
        playerInfoHash.insert( "tt",        tr( "Team task" ) );
        playerInfoHash.insert( "tl",        tr( "Team leader" ) );
        playerInfoHash.insert( "skill",     tr( "Skill" ) );
        playerInfoHash.insert( "cn",        tr( "Clan tag" ) );
        playerInfoHash.insert( "rp",        tr( "Ready" ) );
        playerInfoHash.insert( "pq",        tr( "Play queue" ) );
        playerInfoHash.insert( "su",        tr( "Subscribed" ) );
        playerInfoHash.insert( "xcn",       tr( "Clan" ) );
        playerInfoHash.insert( "c",         tr( "Country" ) );
    }

    setSelectionMode( DtTable::SingleSelection );
    setColumnCount( 2 );
    setHorizontalHeaderLabels( QStringList() << tr( "Variable" ) << tr( "Value" ) );

    horizontalHeader()->setResizeMode( QHeaderView::Stretch );
    horizontalHeader()->setResizeMode( 0, QHeaderView::Fixed );
    horizontalHeader()->resizeSection( 0, 160 );
}

QString DtEditCommandTable::getPlayerInfoString( const QString& name ) {
    return playerInfoHash.value( name, name );
}

void DtEditCommandTable::editCommand( const QString& cmd ) {
    removeAllRows();

    if ( cmd.isEmpty() ) {
        return;
    }

    bool playerInfo = cmd.startsWith( "n\\" );
    bool sb = cmd.startsWith( "sb\\" );

    if ( !cmd.startsWith( confDelimeter )   &&
         !playerInfo                        &&
         !sb )
    {
        return;
    }

    QStringList vars = cmd.split( confDelimeter );

    if ( ( sb || playerInfo ) && cmd.endsWith( confDelimeter ) ) {
        vars.removeLast();
    }

    setRowCount( vars.count() / 2 );
    int row = 0;
    int i = vars.at( 0 ).isEmpty() ? 1 : 0;

    for ( ; i < vars.count(); i += 2 ) {
        if ( row == rowCount() ) {
            setRowCount( rowCount() + 1 );
        }

        QString itemText = playerInfo ? getPlayerInfoString( vars.at( i ) ) : vars.at( i );

        QTableWidgetItem* item = new QTableWidgetItem( itemText );
        item->setData( Qt::UserRole, vars.at( i ) );
        item->setFlags( item->flags() & ~Qt::ItemIsEditable );

        setItem( row, 0, item );

        itemText = ( vars.count() != i + 1 ) ? vars.at( i + 1 ) : "";
        item = new QTableWidgetItem( itemText );

        setItem( row++, 1, item );
    }
}

void DtEditCommandTable::commitData( QWidget* editorWidget ) {
    DtTable::commitData( editorWidget );

    QString cmd;
    bool sb = item( 0, 0 )->data( Qt::UserRole ).toString() == "sb";
    bool delimeters = ( !sb && item( 0, 0 )->data( Qt::UserRole ).toString() != "n" );

    if ( delimeters ) {
        cmd += confDelimeter;
    }

    for ( int i = 0; i < rowCount(); ++i ) {
        if ( i != 0 ) {
            cmd += confDelimeter;
        }

        cmd += QString( "%1\\%2" ).arg( item( i, 0 )->data( Qt::UserRole ).toString(),
                                        item( i, 1 )->text() );
    }

    if ( sb ) {
        cmd += confDelimeter;
    }

    emit commandUpdated( cmd );
}
