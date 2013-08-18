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

#ifndef DTSCANTABLE_H
#define DTSCANTABLE_H

#include "DemoTable.h"

class DtScanTable : public DtDemoTable {
    Q_OBJECT

public:
    DtScanTable( QWidget* parent = 0 );

    void clearMark();

protected:
    void getCpDemoVec( DtCpDemoVec& demoSegments, const QString& dest );
    virtual void makeSegments( const QMap< int, int >& group, QVector< cutSegment >& cutSegments ) = 0;
    virtual QTableWidgetItem* getTimeItem( int rowNum ) = 0;

public slots:
    void deleteSelectedDemos();
    void packSelectedDemos();
    void copySelectedDemos( const QString& dest );
    void moveSelectedDemos( const QString& dest );
    void sortColumn( int column, Qt::SortOrder order ) = 0;
    void sortColumn( int column ) = 0;
    void updateRow( int rowNum );
};

#endif // DTSCANTABLE_H
