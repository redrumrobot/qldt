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

#ifndef DTTABLESWIDGET_H
#define DTTABLESWIDGET_H

#include <QSplitter>
#include <QWidget>
#include <QList>

class DtDemo;
class DtDemoTable;
class DtPlayersTable;
class DtVariablesTable;
class DtDirTree;

class DtTablesWidget : public QWidget {
    Q_OBJECT

public:
    DtTablesWidget( int tableType, QWidget* parent = 0 );

    enum demoTableType {
        TT_DEMOFILES,
        TT_FINDDEMO,
        TT_FINDFRAGS,
        TT_FINDTEXT
    };

    DtDemoTable* demoTable;

    void setInfoPanelVisible( bool visible );

public slots:
    void clearSelections();
    void clearDemoInfo();

protected:
    QSplitter* cSplit, *rSplit;
    DtPlayersTable* playersTable;
    DtVariablesTable* variablesTable;

    void setSplitterSizes( QSplitter* split, QString cfgName, const QList< int >& defaultSizes );
    void saveSplitterState( QSplitter* split, QString cfgName );

public slots:
    void showDemoInfo( DtDemo* );

signals:
    void actPlaydemoTriggered();

};

#endif // DTTABLESWIDGET_H
