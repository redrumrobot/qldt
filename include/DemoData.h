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

#ifndef DTDEMODATA_H
#define DTDEMODATA_H

#include <QVector>
#include <QMap>
#include <QString>
#include <QPair>

class DtDemo;

typedef QVector< DtDemo* > DtDemoVec;

struct cutSegment {
    cutSegment() :
            start( 0 ),
            end( 0 ) { }

    cutSegment( int segmentStart, int segmentEnd ) :
            start( segmentStart ),
            end( segmentEnd ) { }

    int start;
    int end;
};

struct DtDemoCommand {
    int time;
    int csIndex;
    QString cmd;
    bool big;
    int bcsNum;

    DtDemoCommand( int sTime, int index, const QString& sCmd, bool bcs = false, int bcsN = 0 ) :
            time( sTime ),
            csIndex( index ),
            cmd( sCmd ),
            big( bcs ),
            bcsNum( bcsN )
    { }

    DtDemoCommand() :
            time( -1 ),
            csIndex( -1 ),
            big( false ),
            bcsNum( 0 )
    { }
};

typedef QVector< DtDemoCommand > DtCmdVec;

enum demoConverters {
    DC_NONE,
    DC_BETAMAPFIX
};

struct DtWriteOptions {
    DtWriteOptions( DtDemo* parent = 0 ) :
        removeWarmup( false ),
        editChat( false ),
        writeCommands( false ),
        removePauses( false ),
        singleFile( true ),
        exportXml( false ),
        importXml( false ),
        timerInitialValue( 0 ),
        removeLags( false ),
        setGamestateString( false ),
        converter( DC_NONE ) { parentDemo = parent; }

    DtDemo* parent() const {
        return parentDemo;
    }

    DtCmdVec commands;
    QVector< cutSegment > cutSegments;
    QVector< QPair< int, QString > > chatStrings;
    QString newFileName; /* Used as a basename for naming new files when copying demo parts into them,
                            or as a target path for importing new demo.
                         */
    bool removeWarmup;
    bool editChat; /* Write strings from "chatStrings" vector. */
    bool writeCommands; /* Writes commands and ignores any cutting or time changes*/
    bool removePauses;
    bool singleFile; /* Put all parts into the one file. If false, creates a new file for each part. */
    bool exportXml;  /* Ignores all other settings and writes demo contents to the file in xml format. */
    bool importXml;
    int timerInitialValue; /* Not used if singleFile = false.
                              Initial time of each part will be equal to its start time.
                           */
    bool removeLags;
    bool setGamestateString; /* Sets a gamestate var from the "gamestateString" and simple copies
                                all the rest snapshots */
    demoConverters converter;
    QPair< int, QString > gamestateString;

private:
    DtDemo* parentDemo;
};

struct DtCpDemo {
    DtWriteOptions options;
    DtDemo* demo;
};

struct DtDemoDbData {
    DtDemoDbData() { }
    DtDemoDbData( QString fileName, int modified ) :
        name( fileName ),
        lastModified( modified ) { }

    int id;
    QString name;
    int lastModified;
    QString player;
    int type;
    int mod;
    QString map;
    int date;
    QString server;
    int protocol;
    bool broken;
};

typedef QVector< DtCpDemo > DtCpDemoVec;

#endif // DTDEMODATA_H
