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

#ifndef DTDEMODATABASE_H
#define DTDEMODATABASE_H

#include "DemoData.h"

#include <QSet>

class QMutex;

class DtDemoDatabase {
public:
    DtDemoDatabase();
    ~DtDemoDatabase();

    bool opened();
    bool open();
    void close();
    void setDir( const QString& dir );
    bool contains( DtDemoDbData& demo );
    void insertRecord( const DtDemoDbData& demo );
    void removeUnusedDirs( const QStringList& openedDirs );
    void cleanObsoleteRecords();
    void selectDemosForGame( int game, QVector< DtDemoDbData >& demos );
    void selectDemosForDir( QString dir, QVector< DtDemoDbData >& demos );

private:
    bool dbOpened;
    int currentDirId;
    int currentGameId;
    QVector< DtDemoDbData > dirData;
    QSet< int > dbIds;
    QSet< int > realIds;
    QMutex* dbMutex;

    QStringList getList( const QString& queryString );
    void getDemosForDir( QString path, QString id, QVector< DtDemoDbData >& demos );
};

#endif // DTDEMODATABASE_H
