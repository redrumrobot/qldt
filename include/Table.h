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

#ifndef DTTABLE_H
#define DTTABLE_H

#include <QTableWidget>

class DtTable : public QTableWidget {
    Q_OBJECT
public:
    DtTable( QWidget* parent = 0 );

    static bool colorsUpdated;

    void saveColumnWidths( const QString& cfgTableName );
    void loadColumnWidths( const QString& cfgTableName );
    void removeAllRows();
    QList< int > getSelectedRows() const;

protected:
    void selectionChanged( const QItemSelection& o, const QItemSelection& n );
};

#endif // DTTABLE_H
