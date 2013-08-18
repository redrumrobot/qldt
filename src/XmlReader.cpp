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

#include "XmlReader.h"
#include "AbstractProtocol.h"

DtXmlResolver DtXmlReader::xmlResolver;

DtXmlReader::DtXmlReader( DtDemo* parent ) : d( parent ) {
    setEntityResolver( &xmlResolver );
}

void DtXmlReader::initializeFieldNums() {
    fieldNums = d->proto->entityFieldNums();
    psFieldNums = d->proto->playerFieldNums();
}

bool DtXmlReader::isXmlStart() {
    return ( isStartDocument()             &&
             documentVersion() == "1.0"    &&
             documentEncoding() == "UTF-8" );
}

bool DtXmlReader::isDemoStart() {
    return ( isStartElement() && name() == "quakedemo" );
}

void DtXmlReader::xmlReadNextStartElement() {
    QXmlStreamReader::TokenType tokenType = QXmlStreamReader::NoToken;

    while ( !atEnd() && tokenType != QXmlStreamReader::StartElement ) {
        tokenType = readNext();
    }
}

void DtXmlReader::parseXmlEntity( msg_t* outMsg ) {
    int entityNum = xmlReadAttribute( "number" );

    d->proto->writeBits( outMsg, entityNum, GENTITYNUM_BITS );

    if ( attributes().hasAttribute( "remove" ) ) {
        d->proto->writeBits( outMsg, 1, 1 );
        xmlReadNextStartElement();
        return;
    }

    xmlReadNextStartElement();

    if ( name() != "field" ) {   // entity has no fields
        d->proto->writeBits( outMsg, 0, 1);     // not removed
        d->proto->writeBits( outMsg, 0, 1);     // no delta
        return;
    }

    d->proto->writeBits( outMsg, 0, 1 ); // not removed
    d->proto->writeBits( outMsg, 1, 1 ); // delta

    QVector< QPair< QString, int > > fields;
    QSet< int > changedNums;

    netField_t* field = d->proto->entityFields();
    readXmlFields( fields, fieldNums, changedNums, field );
    int lastChangedFieldNum = fieldNums.value( fields.last().first ) + 1;
    d->proto->writeByte( outMsg, lastChangedFieldNum );

    int j = 0;
    for ( int i = 0; i < lastChangedFieldNum; ++i ) {
        if ( !changedNums.contains( i ) ) {
            d->proto->writeBits( outMsg, 0, 1 ); // not changed
            continue;
        }

        d->proto->writeBits( outMsg, 1, 1 ); // changed

        int num = fieldNums.value( fields.at( j ).first );
        netField_t* field = &d->proto->entityFields()[ num ];
        const int* toF = &fields.at( j++ ).second;

        if ( field->bits == 0 ) {
            // float
            float fullFloat = *reinterpret_cast< const float* >( toF );
            int trunc = static_cast< int >( fullFloat );

            if ( fullFloat == 0.0f ) {
                d->proto->writeBits( outMsg, 0, 1 );
            } else {
                d->proto->writeBits( outMsg, 1, 1 );
                if ( trunc == fullFloat && trunc + FLOAT_INT_BIAS >= 0 &&
                     trunc + FLOAT_INT_BIAS < ( 1 << FLOAT_INT_BITS ) ) {
                    // small integer
                    d->proto->writeBits( outMsg, 0, 1 );
                    d->proto->writeBits( outMsg, trunc + FLOAT_INT_BIAS, FLOAT_INT_BITS );
                } else {
                    // full floating point value
                    d->proto->writeBits( outMsg, 1, 1 );
                    d->proto->writeBits( outMsg, *toF, 32 );
                }
            }
        }
        else {
            if ( *toF == 0 ) {
                d->proto->writeBits( outMsg, 0, 1 );
            }
            else {
                // integer
                d->proto->writeBits( outMsg, 1, 1 );
                d->proto->writeBits( outMsg, *toF, field->bits );
            }
        }
    }
}

void DtXmlReader::parseXmlGamestate( msg_t* outMsg ) {
    d->serverCommandSequence = xmlReadAttribute( "sequence" );
    d->proto->writeByte( outMsg, svc_gamestate );
    d->proto->writeLong( outMsg, d->serverCommandSequence );
    xmlReadNextStartElement();

    forever {
        if ( name() == "nop" ) {
            d->proto->writeByte( outMsg, svc_nop );
            xmlReadNextStartElement();
        }
        else if ( name() == "config" ) {
            d->proto->writeByte( outMsg, svc_configstring );

            int cfgIndex = attributes().value( "index" ).toString().toInt();
            QString cfgStr = readElementText();

            d->proto->writeShort( outMsg, cfgIndex );
            d->proto->writeBigString( outMsg, cfgStr.toUtf8().data() );
            xmlReadNextStartElement();
        }
        else if ( name() == "baseline" ) {
            d->proto->writeByte( outMsg, svc_baseline );
            parseXmlEntity( outMsg );
        }
        else {
            d->proto->writeByte( outMsg, svc_EOF );
            break;
        }

        if ( tokenType() == QXmlStreamReader::Invalid ) {
            d->error( DtDemo::FATAL, "Invalid token on line %d", lineNumber() );
        }
    }

    if ( name() == "clientNum" ) {
        d->proto->writeLong( outMsg, attributes().value( "value" ).toString().toInt() );
    }
    else {
        d->error( DtDemo::FATAL, "Invalid token on line %d", lineNumber() );
    }

    xmlReadNextStartElement();

    if ( name() == "checksumFeed" ) {
        d->proto->writeLong( outMsg, attributes().value( "value" ).toString().toInt() );
    }
    else {
        d->error( DtDemo::FATAL, "Invalid token on line %d", lineNumber() );
    }

    readNext();
}

int DtXmlReader::xmlReadAttribute( const QString& name ) {
    return attributes().value( name ).toString().toInt();
}

void DtXmlReader::parseXmlCommand( msg_t* outMsg ) {
    d->proto->writeByte( outMsg, svc_serverCommand );
    d->proto->writeLong( outMsg, xmlReadAttribute( "index" ) );

    QString str = readElementText();
    str.replace( "&EC;", QChar( chatEscapeChar ) );
    d->proto->writeString( outMsg, str.toUtf8().data() );
    xmlReadNextStartElement();
}

void DtXmlReader::readXmlFields( QVector< QPair< QString, int > >& fields,
                                 const QHash< QString, int >& nums,
                                 QSet< int >& changedNums,
                                 netField_t* netFields )
{
    forever {
        QString fieldName = attributes().value( "name" ).toString();
        int fieldNum = nums.value( fieldName );
        int val;
        float floatVal;
        netField_t* field = &netFields[ fieldNum ];

        if ( field->bits == 0 ) {
            floatVal = attributes().value( "value" ).toString().toFloat();
            memcpy( &val, &floatVal, 4 );
        }
        else {
            val = xmlReadAttribute( "value" );
        }

        fields.append( QPair< QString, int >( fieldName, val ) );
        changedNums.insert( fieldNum );
        xmlReadNextStartElement();

        if ( name() != "field" ) {
            break;
        }
    }
}

void DtXmlReader::parseXmlPlayerstate( msg_t* outMsg ) {
    xmlReadNextStartElement();

    QVector< QPair< QString, int > > fields;
    QSet< int > changedNums;
    int lastChangedFieldNum = 0;

    if ( name() == "field" ) {
        readXmlFields( fields, psFieldNums, changedNums, &playerStateFields[ 0 ] );
        lastChangedFieldNum = psFieldNums.value( fields.last().first ) + 1;
    }

    d->proto->writeByte( outMsg, lastChangedFieldNum );

    int j = 0;

    for ( int i = 0; i < lastChangedFieldNum; ++i ) {
        if ( !changedNums.contains( i ) ) {
            d->proto->writeBits( outMsg, 0, 1 ); // not changed
            continue;
        }

        d->proto->writeBits( outMsg, 1, 1 ); // changed

        netField_t* field = &playerStateFields[ psFieldNums.value( fields.at( j ).first ) ];
        const int* toF = &fields.at( j++ ).second;

        if ( field->bits == 0 ) {
            // float
            float fullFloat = *reinterpret_cast< const float* >( toF );
            int trunc = static_cast< int >( fullFloat );

            if ( trunc == fullFloat && trunc + FLOAT_INT_BIAS >= 0 &&
                 trunc + FLOAT_INT_BIAS < ( 1 << FLOAT_INT_BITS ) ) {
                // small integer
                d->proto->writeBits( outMsg, 0, 1 );
                d->proto->writeBits( outMsg, trunc + FLOAT_INT_BIAS, FLOAT_INT_BITS );
            } else {
                // full floating point value
                d->proto->writeBits( outMsg, 1, 1 );
                d->proto->writeBits( outMsg, *toF, 32 );
            }
        }
        else {
            // integer
            d->proto->writeBits( outMsg, *toF, field->bits );
        }
    }

    if ( name() != "stats"       &&
         name() != "persistants" &&
         name() != "ammo"        &&
         name() != "powerups" )
    {
        d->proto->writeBits( outMsg, 0, 1 );
        return;
    }

    d->proto->writeBits( outMsg, 1, 1 );

    #define WRITE_PS_ARRAY( arrayName, writeLength )                        \
        if ( name() == #arrayName ) {                                       \
            d->proto->writeBits( outMsg, 1, 1 );                            \
            xmlReadNextStartElement();                                      \
                                                                            \
            short bits = 0;                                                 \
            int values[ 16 ];                                               \
                                                                            \
            while ( name() == "field" ) {                                   \
                int i = xmlReadAttribute( "index" );                        \
                                                                            \
                bits |= 1 << i;                                             \
                values[ i ] = xmlReadAttribute( "value" );                  \
                xmlReadNextStartElement();                                  \
            }                                                               \
                                                                            \
            d->proto->writeShort( outMsg, bits );                           \
                                                                            \
            for ( int i = 0; i < 16; ++i ) {                                \
                if ( bits & ( 1 << i ) ) {                                  \
                    d->proto->write##writeLength( outMsg, values[ i ] );    \
                }                                                           \
            }                                                               \
        }                                                                   \
        else {                                                              \
            d->proto->writeBits( outMsg, 0, 1 );                            \
        }

    WRITE_PS_ARRAY( stats, Short );
    WRITE_PS_ARRAY( persistants, Short );
    WRITE_PS_ARRAY( ammo, Short );
    WRITE_PS_ARRAY( powerups, Long );
}

void DtXmlReader::parseXmlSnapshot( msg_t* outMsg ) {
    d->proto->writeByte( outMsg, svc_snapshot );
    d->proto->writeLong( outMsg, xmlReadAttribute( "serverTime" ) );
    d->proto->writeByte( outMsg, xmlReadAttribute( "deltaNum" ) );
    d->proto->writeByte( outMsg, xmlReadAttribute( "snapFlags" ) );

    int areamaskLen = xmlReadAttribute( "areamaskLen" );
    unsigned char areamaskData[ MAX_MAP_AREA_BYTES ];

    QString areamask = attributes().value( "areamaskData" ).toString();

    for ( int i = 1; i < areamaskLen + 1; ++i ) {
        bool ok;
        QString str( "%1%2" );
        unsigned char mask = str.arg( areamask.at( i * 2 ) )
                             .arg( areamask.at( i * 2 + 1 ) )
                             .toInt( &ok, 16 );
        areamaskData[ i - 1 ] = mask;
    }

    d->proto->writeByte( outMsg, areamaskLen );
    d->proto->writeData( outMsg, &areamaskData, areamaskLen );

    xmlReadNextStartElement();

    if ( name() == "player" ) {
        parseXmlPlayerstate( outMsg );

        while ( name() == "entity" ) {
            parseXmlEntity( outMsg );
        }

        d->proto->writeBits( outMsg, ( MAX_GENTITIES - 1 ), GENTITYNUM_BITS );
    }
    else {
        d->error( DtDemo::FATAL, "No playerstate found on line %d", lineNumber() );
    }
}

void DtXmlReader::parseXmlMessage( msg_t* outMsg ) {
    if ( name() != "message" ) {
        xmlReadNextStartElement();
    }

    if ( isStartElement() ) {
        if ( name() == "message" ) {
            d->msgSeq = xmlReadAttribute( "sequence" );
            int acknowledge = xmlReadAttribute( "acknowledge" );

            d->proto->writeLong( outMsg, acknowledge );
            xmlReadNextStartElement();

            forever {
                if ( name() == "nop" ) {
                    d->proto->writeByte( outMsg, svc_nop );
                }
                else if ( name() == "gamestate" ) {
                    parseXmlGamestate( outMsg );
                    break;
                }
                else if ( name() == "command" ) {
                    parseXmlCommand( outMsg );
                }
                else if ( name() == "snapshot" ) {
                    parseXmlSnapshot( outMsg );
                }
                else {
                    break;
                }
            }
        }
        else {
            d->error( DtDemo::FATAL, "Message expected but %d found on line %d",
                         name().toString().toUtf8().data(), lineNumber() );
        }
    }

    if ( isEndElement()      ||
         name() == "message" ||
         isEndDocument() )
    {
        d->proto->writeByte( outMsg, svc_EOF );

        if ( isEndDocument() ) {
            return;
        }
    }
    else {
        d->error( DtDemo::FATAL, "End of message expected on line %d", lineNumber() );
    }
}

QString DtXmlResolver::resolveUndeclaredEntity( const QString & name ) {
    if ( name == "EC" ) {
        return "&amp;EC;";
    }

    return name;
}
