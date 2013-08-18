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

#ifndef DTCOMMANDSWIDGET_H
#define DTCOMMANDSWIDGET_H

#include "Table.h"
#include "DemoData.h"

#include <QWidget>

class DtCommandsTable;
class DtEditCommandTable;
class QSplitter;

class DtCommandsWidget : public QWidget {
    Q_OBJECT
public:
    DtCommandsWidget( QWidget* parent = 0 );
    ~DtCommandsWidget();

    void setCommands( const DtCmdVec& commands );
    void setDemoProto( int sProto, int sQ3Mod );
    DtCmdVec getCommands() const;
    bool isChanged();

protected:
    DtCommandsTable* commandsTable;
    DtEditCommandTable* editTable;
    QSplitter* mainSplitter;
};

class DtCommandsTable : public DtTable {
    Q_OBJECT
public:
    DtCommandsTable( QWidget* parent = 0 );

    void setCommands( const DtCmdVec& commands );
    void setDemoProto( int sProto, int sQ3Mod );
    DtCmdVec getCommands() const;
    bool isChanged();

protected:
    QAction* actDelete;
    QAction* actInsert;
    bool changed;
    int proto;
    int q3Mod;

    static QHash< QString, QString > commandNamesHash;

    void contextMenuEvent( QContextMenuEvent* e );
    QString getConfigStringName( int index );
    QString getCommandName( const QString& command );
    QString getCommandLine( bool cfgString, QString command, const QString& cmd, bool& quotes );
    void selectionChanged( const QItemSelection& selected, const QItemSelection& );

public slots:
    void setCommand( const QString& command );

protected slots:
    void deleteSelectedStrings();
    void insertAfterSelected();
    void commitData( QWidget* editorWidget );

signals:
    void commandSelected( const QString& );
};

class DtEditCommandTable : public DtTable {
    Q_OBJECT
public:
    DtEditCommandTable( QWidget* parent = 0 );

protected:
    static QHash< QString, QString > playerInfoHash;

public slots:
    void editCommand( const QString& );

protected slots:
    void commitData( QWidget* editorWidget );

protected:
    QString getPlayerInfoString( const QString& name );

signals:
    void commandUpdated( const QString& );
};

#endif // DTCOMMANDSWIDGET_H
