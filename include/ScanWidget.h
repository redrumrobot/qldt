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

#ifndef DTSCANWIDGET_H
#define DTSCANWIDGET_H

#include "TablesWidget.h"

class DtScanTable;
class QLabel;

class DtScanWidget : public DtTablesWidget {
    Q_OBJECT
public:
    DtScanWidget( int tableType, QWidget* parent = 0 );

    DtDemoTable* scanTable;

    void setDone( bool sDone );
    bool isDone();
    void saveSplitterSizes();
    void setSearchInfo( const QString& info );

private:
    bool done;

    QLabel* searchInfo;
};

#endif // DTSCANWIDGET_H
