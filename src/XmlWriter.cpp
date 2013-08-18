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

#include "XmlWriter.h"
#include "AbstractProtocol.h"

DtXmlWriter::DtXmlWriter( DtDemo* parent ) : d( parent ) {
    setAutoFormatting( true );
}

void DtXmlWriter::writeStartDemo() {
    writeStartDocument();
    writeStartElement( "quakedemo" );
    writeAttribute( "protocol", QString::number( d->demoProto ) );
}

void DtXmlWriter::writeEndDemo() {
    writeEndElement();
    writeEndDocument();
}

void DtXmlWriter::writeConfigString( int index, char* str ) {
    writeStartElement( "config" );
    writeAttribute( "index", QString::number( index ) );
    writeCharacters( str );
    writeEndElement();
}

void DtXmlWriter::writeCommand( int index, char* cmd ) {
    writeStartElement( "command" );
    writeAttribute( "index", QString::number( index ) );

    QStringList str = QString( cmd ).split( chatEscapeChar );

    for ( int i = 0; i < str.size(); ++i ) {
        if ( i > 0 ) {
            writeEntityReference( "EC" );
        }

        writeCharacters( str.at( i ) );
    }

    writeEndElement();
}

void DtXmlWriter::writeGamestateHeader() {
    writeStartElement( "gamestate" );
    writeAttribute( "sequence", QString::number( d->serverCommandSequence ) );
}

void DtXmlWriter::writeGamestateFooter( int clientNum, int checksumFeed ) {
    writeEndElement();
    writeStartElement( "clientNum" );
    writeAttribute( "value", QString::number( clientNum ) );
    writeEndElement();
    writeStartElement( "checksumFeed" );
    writeAttribute( "value", QString::number( checksumFeed ) );
    writeEndElement();
}

void DtXmlWriter::writeSnapshotHeader( msg_t* msg, int serverTime, int deltaNum, int snapFlags,
                                       int areamaskLen, unsigned char areamaskData[] )
{
    writeStartElement( "snapshot" );
    writeAttribute( "serverTime", QString::number( serverTime ) );
    writeAttribute( "deltaNum", QString::number( deltaNum ) );
    writeAttribute( "snapFlags", QString::number( snapFlags ) );

    QString maskValue = "0x";

    for ( int i = 0; i < areamaskLen; ++i ) {
        QString strNum = QString::number( areamaskData[ i ], 16 );

        if ( strNum.size() < 2 ) {
            strNum.prepend( "0" );
        }

        maskValue.append( QString::number( areamaskData[ i ], 16 ) );
    }

    writeAttribute( "areamaskLen", QString::number( areamaskLen ) );
    writeAttribute( "areamaskData", maskValue );
    writePlayerstate( msg );
}

void DtXmlWriter::writeSnapshotFooter() {
    writeEndElement();
}

void DtXmlWriter::writeMessageHeader( int serverMessageSequence, int reliableAcknowledge ) {
    writeStartElement( "message" );
    writeAttribute( "sequence", QString::number( serverMessageSequence ) );
    writeAttribute( "acknowledge", QString::number( reliableAcknowledge ) );
}

void DtXmlWriter::writeMessageFooter() {
    writeEndElement();
}

void DtXmlWriter::writeNop() {
    writeStartElement( "nop" );
    writeEndElement();
}

void DtXmlWriter::writeField( const char* name, int value ) {
    writeStartElement( "field" );
    writeAttribute( "name", name );
    writeAttribute( "value", QString::number( value ) );
    writeEndElement();
}

void DtXmlWriter::writeField( const char* name, float value ) {
    writeStartElement( "field" );
    writeAttribute( "name", name );
    writeAttribute( "value", QString::number( value, 'f', 7 ) );
    writeEndElement();
}

template < class T >
void DtXmlWriter::writeStatField( int index, T value ) {
    writeStartElement( "field" );
    writeAttribute( "index", QString::number( index ) );
    writeAttribute( "value", QString::number( value ) );
    writeEndElement();
}

void DtXmlWriter::writePlayerstate( msg_t* msg ) {
    writeStartElement( "player" );

    int lastPlayerField = d->proto->readByte( msg );

    if ( lastPlayerField > playerStateFieldsNum ) {
        d->error( DtDemo::FATAL, "max allowed field num is %d", playerStateFieldsNum );
    }

    netField_t* fld = playerStateFields;

    for ( int i = 0; i < lastPlayerField; ++i, ++fld ) {
        if ( d->proto->readBits( msg, 1 ) ) {
            if ( fld->bits == 0 ) {
                if ( d->proto->readBits( msg, 1 ) == 0 ) {
                    int value = d->proto->readBits( msg, FLOAT_INT_BITS ) - FLOAT_INT_BIAS;
                    writeField( fld->name, value );
                }
                else {
                    union {
                        float floatValue;
                        int intValue;
                    } value;

                    value.intValue = d->proto->readBits( msg, 32 );
                    writeField( fld->name, value.floatValue );
                }
            }
            else {
                writeField( fld->name, d->proto->readBits( msg, fld->bits ) );
            }
        }
    }

    if ( d->proto->readBits( msg, 1 ) ) {
        if ( d->proto->readBits( msg, 1 ) ) {
            writeStartElement( "stats" );

            int bVal = d->proto->readShort( msg );

            for ( int i = 0; i < 16; ++i ) {
                if ( bVal & ( 1 << i ) ) {
                    writeStatField( i, d->proto->readShort( msg ) );
                }
            }

            writeEndElement();
        }

        if ( d->proto->readBits( msg, 1 ) ) {
            writeStartElement( "persistants" );

            int bVal = d->proto->readShort( msg );

            for ( int i = 0; i < 16; ++i ) {
                if ( bVal & ( 1 << i ) ) {
                    writeStatField( i, d->proto->readShort( msg ) );
                }
            }

            writeEndElement();
        }

        if ( d->proto->readBits( msg, 1 ) ) {
            writeStartElement( "ammo" );

            int bVal = d->proto->readShort( msg );

            for ( int i = 0; i < 16; ++i ) {
                if ( bVal & ( 1 << i ) ) {
                    writeStatField( i, d->proto->readShort( msg ) );
                }
            }

            writeEndElement();
        }

        if ( d->proto->readBits( msg, 1 ) ) {
            writeStartElement( "powerups" );

            int bVal = d->proto->readShort( msg );

            for ( int i = 0; i < 16; ++i ) {
                if ( bVal & ( 1 << i ) ) {
                    writeStatField( i, d->proto->readLong( msg ) );
                }
            }

            writeEndElement();
        }
    }

    writeEndElement();
}

