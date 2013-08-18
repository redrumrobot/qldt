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

#include "DemoDatabase.h"
#include "Config.h"
#include "Data.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlResult>
#include <QDebug>
#include <QDir>
#include <QMutex>

using namespace dtdata;

DtDemoDatabase::DtDemoDatabase() :
    dbOpened( false ),
    currentDirId( -1 ),
    currentGameId( 0 ),
    dbMutex( new QMutex )
{
}

DtDemoDatabase::~DtDemoDatabase() {
    delete dbMutex;
}

bool DtDemoDatabase::open() {
    if ( opened() ) {
        return true;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE" );

    QString dbDirName =
#ifdef Q_OS_LINUX
    ".qldt/db";
#elif defined Q_OS_WIN
    "QLDT/db";
#endif

    QString appDataPath = config.getAppDataPath();
    QDir dbDir( appDataPath );

    if ( !dbDir.exists( dbDirName ) ) {
        dbDir.mkpath( dbDirName );
    }

    db.setDatabaseName( appDataPath + "/" + dbDirName + "/demos.db" );

    if ( !db.open() ) {
        qDebug( "Error: couldn't open database" );
        return false;
    }

    QStringList tableNames = db.tables();

    if ( !tableNames.contains( "directories" ) ) {
        QSqlQuery query;
        query.exec( "CREATE TABLE directories ( id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                                "game INT,"
                                                "path TEXT )" );
    }

    if ( !tableNames.contains( "demos" ) ) {
        QSqlQuery query;
        query.exec( "CREATE TABLE demos ( id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                          "directory INTEGER,"
                                          "name TEXT,"
                                          "last_modified INTEGER,"
                                          "player TEXT,"
                                          "type INTEGER,"
                                          "mod INTEGER,"
                                          "map TEXT,"
                                          "date INTEGER,"
                                          "server TEXT,"
                                          "protocol INTEGER,"
                                          "broken INTEGER )" );
    }

    dbOpened = true;
    return true;
}

bool DtDemoDatabase::opened() {
    return dbOpened;
}

void DtDemoDatabase::close() {
    QSqlDatabase::removeDatabase( "QSQLITE" );
    dbOpened = false;
}

QStringList DtDemoDatabase::getList( const QString& queryString ) {
    QSqlQuery query;
    query.exec( queryString );

    QStringList results;

    while ( query.next() ) {
        results.append( query.value( 0 ).toString() );
    }

    return results;
}

void DtDemoDatabase::setDir( const QString& dir ) {
    QDir demoDir( dir );
    dbIds.clear();
    realIds.clear();

    if ( demoDir.dirName() == defaultNewDirName ) {
        currentDirId = -1;
        return;
    }

    QStringList dirId = getList( "SELECT id FROM directories WHERE path='" + dir + "'" );

    if ( dirId.size() ) {
        currentDirId = dirId.first().toInt();

        QSqlQuery query;
        query.exec( "SELECT * FROM demos WHERE directory=" + QString::number( currentDirId ) );

        dirData.clear();

        while ( query.next() ) {
            DtDemoDbData demo;

            demo.id = query.value( 0 ).toInt();
            demo.name = query.value( 2 ).toString();
            demo.lastModified = query.value( 3 ).toInt();
            demo.player = query.value( 4 ).toString();
            demo.type = query.value( 5 ).toInt();
            demo.mod = query.value( 6 ).toInt();
            demo.map = query.value( 7 ).toString();
            demo.date = query.value( 8 ).toInt();
            demo.server = query.value( 9 ).toString();
            demo.protocol = query.value( 10 ).toInt();
            demo.broken = query.value( 11 ).toBool();

            dirData.append( demo );
            dbIds.insert( demo.id );
        }
    }
    else {
        int game = dir.startsWith( config.getQzHomePath() ) ? Q_LIVE : Q_ARENA;

        QSqlQuery query;
        query.exec( "INSERT INTO directories ( game, path )"
                    "VALUES ( " + QString::number( game ) + ", '" + dir + "' )");
        setDir( dir );
    }
}

bool DtDemoDatabase::contains( DtDemoDbData& demo ) {
    if ( currentDirId == -1 ) {
        return false;
    }

    int dirDataSize = dirData.size();

    for ( int i = 0; i < dirDataSize; ++i ) {
        if ( dirData.at( i ).name == demo.name &&
             dirData.at( i ).lastModified == demo.lastModified )
        {
            demo = dirData.at( i );
            dbMutex->lock();
            realIds.insert( demo.id );
            dbMutex->unlock();
            return true;
        }
    }

    return false;
}

void DtDemoDatabase::insertRecord( const DtDemoDbData& demo ) {
    QSqlQuery query;
    query.exec( QString( "INSERT INTO demos ( directory, name, last_modified, player, type, mod,"
                                              "map, date, server, protocol, broken )"
                         "VALUES ( %1, '%2', %3, '%4', %5, %6, '%7', %8, '%9', %10, %11 )" )
                .arg( QString::number( currentDirId ),
                      demo.name,
                      QString::number( demo.lastModified ),
                      demo.player,
                      QString::number( demo.type ),
                      QString::number( demo.mod ),
                      demo.map,
                      QString::number( demo.date ),
                      demo.server )
                .arg( demo.protocol )
                .arg( demo.broken ) );
}

void DtDemoDatabase::removeUnusedDirs( const QStringList& openedDirs ) {
    QSqlQuery query;
    QSqlQuery delQuery;
    query.exec( "SELECT id, path FROM directories" );

    while ( query.next() ) {
        if ( !openedDirs.contains( query.value( 1 ).toString() ) ) {
            QString dirId = query.value( 0 ).toString();
            delQuery.exec( "DELETE FROM demos WHERE directory=" + dirId );
            delQuery.exec( "DELETE FROM directories WHERE id=" + dirId );
        }
    }
}

void DtDemoDatabase::cleanObsoleteRecords() {
    QSet< int > obsolete = dbIds - realIds;

    if ( !obsolete.size() ) {
        return;
    }

    QString queryString = "DELETE FROM demos WHERE id IN (";
    QSqlQuery query;

    foreach ( int id, obsolete ) {
        queryString += QString( "%1," ).arg( id );
    }

    queryString.chop( 1 );
    queryString += ")";
    query.exec( queryString );
}

void DtDemoDatabase::getDemosForDir( QString path, QString id, QVector< DtDemoDbData >& demos ) {
    QSqlQuery query;
    query.exec( "SELECT name, player, type, mod, map, date, server "
                "FROM demos WHERE directory=" + id );

    while ( query.next() ) {
        DtDemoDbData demo;

        demo.name = QString( "%1/%2" ).arg( path, query.value( 0 ).toString() );
        demo.player = query.value( 1 ).toString();
        demo.type = query.value( 2 ).toInt();
        demo.mod = query.value( 3 ).toInt();
        demo.map = query.value( 4 ).toString();
        demo.date = query.value( 5 ).toInt();
        demo.server = query.value( 6 ).toString();

        demos.append( demo );
    }
}

void DtDemoDatabase::selectDemosForGame( int game, QVector< DtDemoDbData >& demos ) {
    QSqlQuery query;
    query.exec( "SELECT id, path FROM directories WHERE game=" + QString::number( game ) );

    while ( query.next() ) {
        getDemosForDir( query.value( 1 ).toString(), query.value( 0 ).toString(), demos );
    }
}

void DtDemoDatabase::selectDemosForDir( QString dir, QVector< DtDemoDbData >& demos ) {
    QStringList dirId = getList( "SELECT id FROM directories WHERE path='" + dir + "'" );

    if ( !dirId.size() ) {
        return;
    }

    getDemosForDir( dir, dirId.first(), demos );
}
