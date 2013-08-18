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

#ifndef DTENCRYPT_H
#define DTENCRYPT_H

#include <QByteArray>
#include <QString>

#define NUM_SUBKEYS  18
#define NUM_S_BOXES  4
#define NUM_ENTRIES  256
#define MAX_STRING   256
#define MAX_PASSWD   56

struct WordByte {
    quint32 three:8;
    quint32 two:8;
    quint32 one:8;
    quint32 zero:8;
};

union Word {
    quint32 word;
    WordByte byte;
};

struct DWord {
    Word word0;
    Word word1;
};

class DtCrypt {
public:
    DtCrypt();
    ~DtCrypt();

    QByteArray encodePassword( QByteArray decoded );
    QString decodePassword( QByteArray encoded );

private:
    quint32 PA[ NUM_SUBKEYS ];
    quint32 SB[ NUM_S_BOXES ][ NUM_ENTRIES ];

    inline void blowfishEncode( Word*, Word* );
    inline void blowfishDecode( Word*, Word* );
    void reset( bool destruct = false );
    void encrypt( void*, quint32 );
    void decrypt( void*, quint32 );
};


#endif // DTENCRYPT_H
