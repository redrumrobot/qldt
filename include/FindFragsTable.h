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

#ifndef DTFINDFRAGSTABLE_H
#define DTFINDFRAGSTABLE_H

#include "ScanTable.h"

class DtFindFragsTable : public DtScanTable {
    Q_OBJECT

public:
    DtFindFragsTable( QWidget* parent = 0 );

    enum columnTypes {
        CT_NAME,
        CT_PLAYER,
        CT_TYPE,
        CT_MAP,
        CT_TIME,
        CT_FRAGS,
        CT_MAXTIME,
        CT_SEGMENTLENGTH,
        CT_WEAPON,
        CT_PROTOCOL
    };

    void addScanRow( DtDemo* demo, int startSeqTime, int fragsInSeq, int deltaTime,
                     int segLength, int weapon );
protected:
    void getCpDemoVec( DtCpDemoVec& demoSegments, const QString& dest );
    void makeSegments( const QMap< int, int >& group, QVector< cutSegment >& cutSegments );
    QTableWidgetItem* getTimeItem( int rowNum );

public slots:
    void sortColumn( int column, Qt::SortOrder order );
    void sortColumn( int column );
    void saveColumns();
};

#endif // DTFINDFRAGSTABLE_H
