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

#ifndef DTFINDDEMOTABLE_H
#define DTFINDDEMOTABLE_H

#include "DemoTable.h"

class DtFindDemoTable : public DtDemoTable {
    Q_OBJECT

public:
    DtFindDemoTable( QWidget* parent = 0 );

    enum columnTypes {
        CT_NAME,
        CT_DIR,
        CT_PLAYER,
        CT_TYPE,
        CT_MAP,
        CT_DATE,
        CT_SERVER,
        CT_PROTOCOL
    };

    void addDemo( DtDemo* demo );
    void clearMark();

public slots:
    void sortColumn( int column, Qt::SortOrder order );
    void sortColumn( int column );
    void saveColumns();
    void deleteSelectedDemos();
    void packSelectedDemos();
    void copySelectedDemos( const QString& dest );
    void moveSelectedDemos( const QString& dest );
    void updateRow( int rowNum );
};

#endif // DTFINDDEMOTABLE_H
