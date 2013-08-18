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

#ifndef DTXMLWRITER_H
#define DTXMLWRITER_H

#include "Demo.h"
#include <QXmlStreamWriter>

class DtXmlWriter : public QXmlStreamWriter {
public:
    DtXmlWriter( DtDemo* parent );

    void writeStartDemo();
    void writeEndDemo();
    void writeConfigString( int index, char* str );
    void writeCommand( int index, char* cmd );
    void writeGamestateHeader();
    void writeGamestateFooter( int clientNum, int checksumFeed );
    void writeSnapshotHeader( msg_t* msg, int serverTime, int deltaNum, int snapFlags, int areamaskLen,
                              unsigned char areamaskData[] );
    void writeSnapshotFooter();
    void writeMessageHeader( int serverMessageSequence, int reliableAcknowledge );
    void writeMessageFooter();
    void writeNop();
    template < class T >
    void writeEntity( msg_t* msg, T* from, T* to, int number,
                      const QString& elementName = QString( "entity" ) );
private:
    DtDemo* d;

    void writeField( const char* name, int value );
    void writeField( const char* name, float value );
    template < class T >
    void writeStatField( int index, T value );
    void writePlayerstate( msg_t* msg );
};

#endif
