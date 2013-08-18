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

#ifndef DTMAINTABLE_H
#define DTMAINTABLE_H

#include "DemoTable.h"

class DtMainTable : public DtDemoTable {
    Q_OBJECT

public:
    DtMainTable( QWidget* parent = 0 );

    enum columnTypes {
        CT_NAME,
        CT_PLAYER,
        CT_TYPE,
        CT_MAP,
        CT_DATE,
        CT_SERVER,
        CT_PROTOCOL
    };

    void removeDemo( DtDemo* demo );
    void clearMark();

public slots:
    void addDemo( DtDemo* );
    void deleteSelectedDemos();
    void packSelectedDemos();
    void copySelectedDemos( const QString& dest );
    void moveSelectedDemos( const QString& dest );
    void sortColumn( int column, Qt::SortOrder order );
    void sortColumn( int column );
    void autoRenameItem();
    void organizeDemos();
    void updateRow( int rowNum );
    void saveColumns();

protected:
    QAction* actRename;
    QAction* actAutoRename;
    QAction* actOrganize;

    bool demosHaveReferences( const DtDemoVec& demos, const QString& msg );
    bool renameDemo( DtDemo* demo, QString newFileName );
    void keyPressEvent( QKeyEvent* e );

protected slots:
    void renameItem();
    void commitData( QWidget* editorWidget );
};

#endif // DTMAINTABLE_H
