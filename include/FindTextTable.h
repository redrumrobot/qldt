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

#ifndef DTFINDTEXTTABLE_H
#define DTFINDTEXTTABLE_H

#include "ScanTable.h"

class DtFindTextTable : public DtScanTable {
    Q_OBJECT

public:
    DtFindTextTable( QWidget* parent = 0 );

    enum columnTypes {
        CT_NAME,
        CT_PLAYER,
        CT_TYPE,
        CT_MAP,
        CT_DATE,
        CT_TIME,
        CT_COMMAND,
        CT_MSG,
        CT_DEMOLENGTH,
        CT_PROTOCOL
    };

    void addScanRow( DtDemo* demo, int time, const QString& player, const QString& cmd,
                     const QString& msg );

protected:
    void makeSegments( const QMap< int, int >& group, QVector< cutSegment >& cutSegments );
    QTableWidgetItem* getTimeItem( int rowNum );

public slots:
    void sortColumn( int column, Qt::SortOrder order );
    void sortColumn( int column );
    void saveColumns();
};

#endif // DTFINDTEXTTABLE_H
