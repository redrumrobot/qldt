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

#ifndef DTXMLREADER_H
#define DTXMLREADER_H

#include "Demo.h"

#include <QXmlStreamReader>

class DtXmlResolver : public QXmlStreamEntityResolver {
public:
    QString resolveUndeclaredEntity( const QString & name );
};

class DtXmlReader : public QXmlStreamReader {
public:
    DtXmlReader( DtDemo* parent );

    bool isXmlStart();
    bool isDemoStart();
    int xmlReadAttribute( const QString& name );
    void parseXmlMessage( msg_t* outMsg );
    void initializeFieldNums();

private:
    static DtXmlResolver xmlResolver;
    DtDemo* d;
    QHash< QString, int > fieldNums;
    QHash< QString, int > psFieldNums;

    void xmlReadNextStartElement();
    void parseXmlGamestate( msg_t* outMsg );
    void parseXmlCommand( msg_t* outMsg );
    void parseXmlSnapshot( msg_t* outMsg );
    void parseXmlEntity( msg_t* outMsg );
    void parseXmlPlayerstate( msg_t* outMsg );
    void readXmlFields( QVector< QPair< QString, int > >& fields,
                        const QHash< QString, int >& nums,
                        QSet< int >& changedNums,
                        netField_t* netFields );
};

#endif
