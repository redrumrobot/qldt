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

#ifndef DTABSTRACTPROTOCOL_H
#define DTABSTRACTPROTOCOL_H

#include <QString>
#include <QHash>
#include "Huffman.h"
#include "XmlWriter.h"
#include <math.h>

class DtAbstractProtocol {
public:
    DtAbstractProtocol( DtDemo* parent );

    virtual void parseConfigString( msg_t* msg, int& dataCount, msg_t* outMsg ) = 0;
    virtual void parseCommandString( msg_t* msg, msg_t* outMsg ) = 0;
    virtual void parsePacketEnities( msg_t* msg, clSnapshot_t* oldframe, clSnapshot_t* newframe ) = 0;
    virtual void emitPacketEntities( clSnapshot_t* from, clSnapshot_t* to, msg_t* msg ) = 0;
    virtual void updateGamestateInfo( int time ) = 0;
    virtual void parseBaseline( msg_t* msg, msg_t* outMsg ) = 0;
    virtual void setGameTime( int sTime ) = 0;
    virtual void afterParse() = 0;
    virtual QHash< QString, int > entityFieldNums() = 0;
    virtual QHash< QString, int > playerFieldNums() = 0;
    virtual netField_t* entityFields() = 0;
    virtual int entityFieldsCount() = 0;

    void initialize( msg_t* buf, byte* data, int length );
    void writeData( msg_t* buf, const void* data, int length );
    static void initializeHuffman();
    void writeBits( msg_t* msg, int value, int bits );
    void writeByte( msg_t* msg, int c);
    void writeShort( msg_t* msg, int c);
    void writeLong( msg_t* msg, int c);
    void writeString( msg_t* msg, const char* s );
    void writeBigString( msg_t* msg, const char* s );
    int readBits( msg_t* msg, int bits );
    int readByte( msg_t* msg );
    int	readShort( msg_t* msg );
    int	readLong( msg_t* msg );
    char* readString( msg_t* msg );
    char* readBigString( msg_t* msg );
    void readData( msg_t* msg, void* data, int len );
    void writeDeltaPlayerstate( msg_t* msg, struct playerState_s* from, struct playerState_s* to );
    void readDeltaPlayerstate( msg_t* msg, struct playerState_s* from, struct playerState_s* to );
    void skipPlayerState( msg_t* msg );

protected:
    int	overflows;
    int oldsize;
    DtHuffman huffman;
    char stringBuf[ MAX_STRING_CHARS ];
    char bigStringBuf[ BIG_INFO_STRING ];
    DtDemo* d;

    void Com_Printf( const char *fmt, ... );
    void Com_Error( int code, const char *fmt, ... );
    void getStrN( char* to, char*& from, int count );
    void removeQuotes( QString& str );
    void updateScores( int cmdIndex, int num );
    void readPlayerName( char*& str, char* pName );
    virtual void obutuaryEventCheck( int eType, int value, DtDemo::DtObituaryEvent& event ) = 0;

    template < class U, class T >
    void readDeltaEntity( msg_t* msg, T* from, T* to, int number );

    template < class U, class T >
    void writeDeltaEntity( msg_t* msg, T* from, T* to, bool force );

    template < class U, class T >
    void deltaEntity( msg_t* msg, clSnapshot_t* frame, int newnum, T* old, bool unchanged );

    template < class U, class T >
    void readEntity( msg_t* msg, T* from, T* to, int number );

    template < class U, class T >
    inline void parsePacketEnitiesImpl( msg_t* msg, clSnapshot_t* oldframe, clSnapshot_t* newframe );

    template < class U, class T >
    inline void emitPacketEntitiesImpl( clSnapshot_t* from, clSnapshot_t* to, msg_t* msg );

    template < class T >
    inline void fixTrTime( T* baseline );

    template < class U, class T >
    inline void parseBaselineImpl( msg_t* msg, msg_t* outMsg );
};

template < class U, class T >
void DtAbstractProtocol::readDeltaEntity( msg_t* msg, T* from, T* to,
                                          int number ) {
    if ( number < 0 || number >= MAX_GENTITIES ) {
        Com_Error( ERR_DROP, "Bad delta entity number: %i", number );
    }

    /* check for a remove */
    if ( readBits( msg, 1 ) == 1 ) {
        memset( to, 0, sizeof( *to ) );
        to->number = MAX_GENTITIES - 1;
        return;
    }

    /* check for no delta */
    if ( readBits( msg, 1 ) == 0 ) {
        *to = *from;
        to->number = number;
        return;
    }

    int* fromF;
    int* toF;
    int trunc;
    int numFields = U::entityStateFieldsNum;
    int lc = readByte( msg );

    to->number = number;
    netField_t* field = U::entityStateFields;

    for ( int i = 0; i < lc ; i++, field++ ) {
        fromF = (int *)( (byte *)from + field->offset );
        toF = (int *)( (byte *)to + field->offset );

        if ( !readBits( msg, 1 ) ) {
            /* no change */
            *toF = *fromF;
        } else {
            if ( field->bits == 0 ) {
                /* float */
                if ( readBits( msg, 1 ) == 0 ) {
                        *(float *)toF = 0.0f;
                } else {
                    if ( readBits( msg, 1 ) == 0 ) {
                        /* integral float */
                        trunc = readBits( msg, FLOAT_INT_BITS );
                        /* bias to allow equal parts positive and negative */
                        trunc -= FLOAT_INT_BIAS;
                        *(float *)toF = trunc;
                    } else {
                        /* full floating point value */
                        *toF = readBits( msg, 32 );
                    }
                }
            } else {
                if ( readBits( msg, 1 ) == 0 ) {
                    *toF = 0;
                } else {
                    /* integer */
                    *toF = readBits( msg, field->bits );
                }
            }
        }
    }

    field = &U::entityStateFields[ lc ];

    for ( int i = lc; i < numFields ; i++, field++ ) {
        fromF = (int *)( (byte *)from + field->offset );
        toF = (int *)( (byte *)to + field->offset );
        /* no change */
        *toF = *fromF;
    }
}

template < class U, class T >
void DtAbstractProtocol::writeDeltaEntity( msg_t* msg, T* from, T* to, bool force ) {
    /* a NULL to is a delta remove message */
    if ( to == NULL ) {
        if ( from == NULL ) {
            return;
        }
        writeBits( msg, from->number, GENTITYNUM_BITS );
        writeBits( msg, 1, 1 );
        return;
    }

    if ( to->number < 0 || to->number >= MAX_GENTITIES ) {
        Com_Error( ERR_FATAL, "MSG_WriteDeltaEntity: Bad entity number: %i", to->number );
    }

    int lc = 0;
    /* build the change vector as bytes so it is endien independent */

    netField_t* field = U::entityStateFields;
    int numFields = U::entityStateFieldsNum;
    int* fromF;
    int* toF;

    for ( int i = 0; i < numFields; i++, field++ ) {
        fromF = (int *)( (byte *)from + field->offset );
        toF = (int *)( (byte *)to + field->offset );
        if ( *fromF != *toF ) {
            lc = i + 1;
        }
    }

    if ( lc == 0 ) {
        /* nothing at all changed */
        if ( !force ) {
            return;        /* nothing at all */
        }
        /* write two bits for no change */
        writeBits( msg, to->number, GENTITYNUM_BITS );
        writeBits( msg, 0, 1 );        /* not removed */
        writeBits( msg, 0, 1 );        /* no delta */
        return;
    }

    writeBits( msg, to->number, GENTITYNUM_BITS );
    writeBits( msg, 0, 1 );  /* not removed */
    writeBits( msg, 1, 1 );  /* we have a delta */
    writeByte( msg, lc );    /* # of changes */

    oldsize += numFields;
    field = U::entityStateFields;

    int trunc;
    float fullFloat;

    for ( int i = 0; i < lc ; i++, field++ ) {
        fromF = (int *)( (byte *)from + field->offset );
        toF = (int *)( (byte *)to + field->offset );

        if ( *fromF == *toF ) {
            writeBits( msg, 0, 1 );    /* no change */
            continue;
        }

        writeBits( msg, 1, 1 );    /* changed */

        if ( field->bits == 0 ) {
            /* float */
            fullFloat = *(float *)toF;
            trunc = (int)fullFloat;

            if ( fullFloat == 0.0f ) {
                    writeBits( msg, 0, 1 );
                    oldsize += FLOAT_INT_BITS;
            } else {
                writeBits( msg, 1, 1 );
                if ( trunc == fullFloat && trunc + FLOAT_INT_BIAS >= 0 &&
                    trunc + FLOAT_INT_BIAS < ( 1 << FLOAT_INT_BITS ) ) {
                    /* send as small integer */
                    writeBits( msg, 0, 1 );
                    writeBits( msg, trunc + FLOAT_INT_BIAS, FLOAT_INT_BITS );
                } else {
                    /* send as full floating point value */
                    writeBits( msg, 1, 1 );
                    writeBits( msg, *toF, 32 );
                }
            }
        } else {
            if (*toF == 0) {
                writeBits( msg, 0, 1 );
            } else {
                writeBits( msg, 1, 1 );
                /* integer */
                writeBits( msg, *toF, field->bits );
            }
        }
    }
}

template < class U, class T >
void DtAbstractProtocol::readEntity( msg_t* msg, T* from, T* to, int number ) {
    if ( number < 0 || number >= MAX_GENTITIES ) {
        d->error( DtDemo::FATAL, "Bad delta entity number: %i", number );
    }

#ifdef MSG_LOG
    Com_Printf( "%3i: entity %d ", msg->readcount, number );
#endif

    if ( readBits( msg, 1 ) == 1 ) {
        /* remove */
#ifdef MSG_LOG
        Com_Printf( "ent remove\n" );
#endif
        memset( to, 0, sizeof( *to ) );
        to->number = MAX_GENTITIES - 1;
        return;
    }

    if ( readBits( msg, 1 ) == 0 ) {
        /* no delta */
#ifdef MSG_LOG
        Com_Printf( "no delta\n" );
#endif
        *to = *from;
        to->number = number;
        return;
    }

    int lastFieldNum = readByte( msg );

    if ( lastFieldNum > U::entityStateFieldsNum ) {
        d->error( DtDemo::FATAL, "Incorrect field count %d > %d", lastFieldNum, U::entityStateFieldsNum );
    }

    to->number = number;

    DtDemo::DtObituaryEvent oEvent;
    netField_t* fld = U::entityStateFields;

    for ( int i = 0; i < lastFieldNum; ++i, ++fld ) {
        if ( readBits( msg, 1 ) ) {
            if ( !fld->bits ) {
                if ( readBits( msg, 1 ) != 0 ) {
                    if ( readBits( msg, 1 ) == 0 ) {
                        readBits( msg, FLOAT_INT_BITS );
                    }
                    else {
                        readBits( msg, 32 );
                    }
                }
#ifdef MSG_LOG
                Com_Printf( "%s:float ", fld->name );
#endif
            } else {
                int val = ( readBits( msg, 1 ) == 0 ) ? 0 : readBits( msg, fld->bits );
#ifdef MSG_LOG
                Com_Printf( "%s:%i ", fld->name, val );
#endif

                if ( d->currentParseType != DtDemo::FindFrags ) {
                    continue;
                }

                obutuaryEventCheck( i, val, oEvent );
            }
        }
    }

#ifdef MSG_LOG
                Com_Printf( "\n" );
#endif

    if ( d->currentParseType == DtDemo::FindFrags && oEvent.save ) {
        if ( d->eventTimes[ number ] > d->lastSnapTime - EVENT_VALID_MSEC  ) {
            return;
        }

        d->eventTimes[ number ] = d->lastSnapTime;

        if ( oEvent.victim == d->clientNum ) {
            d->obituaryEvents.insert( d->getLength(), -1 );
        }
        else if ( oEvent.inflictor == d->clientNum ) {
            int mod = oEvent.mod;

            if ( d->playerInClientTeam( oEvent.victim ) ) {
                mod -= 50; // teamkill
            }

            d->obituaryEvents.insert( d->getLength(), mod );
        }
    }
}

template < class U, class T >
void DtAbstractProtocol::deltaEntity( msg_t* msg, clSnapshot_t* frame, int newnum, T* old,
                                      bool unchanged ) {
    T* state;

    // save the parsed entity state into the big circular buffer so
    // it can be used as the source for a later delta

    state = &static_cast< U* >( this )->parseEntities[ d->parseEntitiesNum & ( MAX_PARSE_ENTITIES - 1 ) ];

    if ( unchanged ) {
        *state = *old;
    } else {
        if ( d->currentParseType == DtDemo::CustomWrite ) {
            if ( !d->writeCfg->exportXml ) {
                readDeltaEntity< U >( msg, old, state, newnum );

                int dTime = d->msgEntSnapTime - d->lastSnapTime;

                if ( old->pos.trTime != state->pos.trTime ) {
                    state->pos.trTime += ( state->pos.trTime ) ? dTime : 0;
                }

                if ( old->apos.trTime != state->apos.trTime ) {
                    state->apos.trTime += ( state->apos.trTime ) ? dTime : 0;
                }

                if ( ( state->eType & ~EV_EVENT_BITS ) == ( ET_EVENTS + EV_ITEM_PICKUP_SPEC ) ) {
                    if ( old->time != state->time ) {
                        state->time -= d->lastSnapTime - d->msgSnapTime;
                    }

                    if ( old->time2 != state->time2 ) {
                        state->time2 -= d->lastSnapTime - d->msgSnapTime;
                    }
                }
            }
            else {
                d->xmlWriter->writeEntity( msg, old, state, newnum );
            }
        }
        else {
            readEntity< U >( msg, old, state, newnum );
        }
    }

    if ( state->number == ( MAX_GENTITIES - 1 ) ) {
        return; // entity was delta removed
    }

    ++d->parseEntitiesNum;
    ++frame->numEntities;
}

template < class U, class T >
void DtAbstractProtocol::parsePacketEnitiesImpl( msg_t* msg, clSnapshot_t* oldframe,
                                                 clSnapshot_t* newframe ) {
    int newnum;
    int oldindex;
    int oldnum;
    T* oldstate;

    newframe->parseEntitiesNum = d->parseEntitiesNum;
    newframe->numEntities = 0;

    // delta from the entities present in oldframe
    oldindex = 0;
    oldstate = 0;
    if ( !oldframe ) {
        oldnum = 99999;
    } else {
        if ( oldindex >= oldframe->numEntities ) {
            oldnum = 99999;
        } else {
            int entNum = ( oldframe->parseEntitiesNum + oldindex ) & ( MAX_PARSE_ENTITIES - 1 );
            oldstate = &static_cast< U* >( this )->parseEntities[ entNum ];
            oldnum = oldstate->number;
        }
    }

    forever {
        newnum = readBits( msg, GENTITYNUM_BITS );

        if ( newnum == ( MAX_GENTITIES - 1 ) ) {
            break;
        }

        if ( msg->readcount > msg->cursize ) {
            d->error( DtDemo::WARN, "parsePacketEntities: end of message" );
            d->stopScan = true;
            return;
        }

        while ( oldnum < newnum ) {
            // one or more entities from the old packet are unchanged
            deltaEntity< U >( msg, newframe, oldnum, oldstate, true );

            oldindex++;

            if ( oldindex >= oldframe->numEntities ) {
                oldnum = 99999;
            } else {
                int entNum = ( oldframe->parseEntitiesNum + oldindex ) & ( MAX_PARSE_ENTITIES - 1 );
                oldstate = &static_cast< U* >( this )->parseEntities[ entNum ];
                oldnum = oldstate->number;
            }
        }

        if ( oldnum == newnum ) {
            // delta from previous state
            deltaEntity< U >( msg, newframe, newnum, oldstate, false );

            oldindex++;

            if ( oldindex >= oldframe->numEntities ) {
                oldnum = 99999;
            } else {
                int entNum = ( oldframe->parseEntitiesNum + oldindex ) & ( MAX_PARSE_ENTITIES - 1 );
                oldstate = &static_cast< U* >( this )->parseEntities[ entNum ];
                oldnum = oldstate->number;
            }
            continue;
        }

        if ( oldnum > newnum ) {
            // delta from baseline
            T* baseline;
            baseline = &static_cast< U* >( this )->entityBaselines[ newnum ];
            deltaEntity< U >( msg, newframe, newnum, baseline, false );
            continue;
        }

    }

    // any remaining entities in the old frame are copied over
    while ( oldnum != 99999 ) {
        // one or more entities from the old packet are unchanged
        deltaEntity< U >( msg, newframe, oldnum, oldstate, true );

        oldindex++;

        if ( oldindex >= oldframe->numEntities ) {
            oldnum = 99999;
        } else {
            int entNum = ( oldframe->parseEntitiesNum + oldindex ) & ( MAX_PARSE_ENTITIES - 1 );
            oldstate = &static_cast< U* >( this )->parseEntities[ entNum ];
            oldnum = oldstate->number;
        }
    }
}

template < class U, class T >
void DtAbstractProtocol::emitPacketEntitiesImpl( clSnapshot_t* from, clSnapshot_t* to, msg_t* msg ) {
    T* oldent = 0;
    T* newent = 0;
    int oldindex = 0;
    int newindex = 0;
    int oldnum;
    int newnum;
    int from_num_entities;

    if ( !from ) {
        from_num_entities = 0;
    } else {
        from_num_entities = from->numEntities;
    }

    while ( newindex < to->numEntities || oldindex < from_num_entities ) {
        if ( newindex >= to->numEntities ) {
            newnum = 9999;
        } else {
            int entNum = ( to->parseEntitiesNum + newindex ) & ( MAX_PARSE_ENTITIES - 1 );
            newent = &static_cast< U* >( this )->parseEntities[ entNum ];
            newnum = newent->number;
        }

        if ( oldindex >= from_num_entities ) {
            oldnum = 9999;
        } else {
            int entNum = ( from->parseEntitiesNum + oldindex ) & ( MAX_PARSE_ENTITIES - 1 );
            oldent = &static_cast< U* >( this )->parseEntities[ entNum ];
            oldnum = oldent->number;
        }

        if ( newnum == oldnum ) {
            // delta update from old position
            // because the force parm is qfalse, this will not result
            // in any bytes being emited if the entity has not changed at all

            writeDeltaEntity< U >( msg, oldent, newent, false );
            oldindex++;
            newindex++;
            continue;
        }

        if ( newnum < oldnum ) {
            // this is a new entity, send it from the baseline
            T* baseline;

            baseline = &static_cast< U* >( this )->entityBaselines[ newnum ];
            writeDeltaEntity< U >( msg, baseline, newent, true );
            newindex++;
            continue;
        }

        if ( newnum > oldnum ) {
            // the old entity isn't present in the new message
            writeDeltaEntity< U, T >( msg, oldent, 0, true );
            oldindex++;
            continue;
        }
    }

    writeBits( msg, ( MAX_GENTITIES - 1 ), GENTITYNUM_BITS );
}

template < class T >
void DtAbstractProtocol::fixTrTime( T* baseline ) {
    if ( baseline->pos.trType == TR_SINE ) {
        baseline->pos.trTime += DtDemo::MSG_START_TIME - d->firstSnapTime;

        if ( baseline->pos.trTime < 0 ) {
            baseline->pos.trTime += baseline->pos.trDuration *
                                    ceil( - baseline->pos.trTime /
                                          static_cast< float >( baseline->pos.trDuration ) );
        }
    }

    if ( baseline->apos.trType == TR_SINE ) {
        baseline->apos.trTime += DtDemo::MSG_START_TIME - d->firstSnapTime;

        if ( baseline->apos.trTime < 0 ) {
            baseline->apos.trTime += baseline->apos.trDuration *
                                     ceil( - baseline->apos.trTime /
                                           static_cast< float >( baseline->apos.trDuration ) );
        }
    }
}

template < class U, class T >
void DtAbstractProtocol::parseBaselineImpl( msg_t* msg, msg_t* outMsg ) {
    int newnum = readBits( msg, GENTITYNUM_BITS );
    if ( newnum < 0 || newnum >= MAX_GENTITIES ) {
        d->error( DtDemo::FATAL, "Baseline number out of range: %i\n", newnum );
    }

    T nullstate;
    memset( &nullstate, 0, sizeof( nullstate ) );

    T* nstate = &nullstate;
    T* baseline = &static_cast< U* >( this )->entityBaselines[ newnum ];

    if ( d->currentParseType == DtDemo::CustomWrite ) {
        if ( d->writeCfg->exportXml ) {
            d->xmlWriter->writeEntity( msg, nstate, baseline, newnum, "baseline" );
        }
        else {
            readDeltaEntity< U >( msg, nstate, baseline, newnum );
            fixTrTime( baseline );
            writeDeltaEntity< U >( outMsg, nstate, baseline, true );
        }
    }
    else {
        readDeltaEntity< U >( msg, nstate, baseline, newnum );
    }
}

template < class T >
void DtXmlWriter::writeEntity( msg_t* msg, T* from, T* to, int number,
                               const QString& elementName )
{
    if ( number < 0 || number >= MAX_GENTITIES ) {
        d->error( DtDemo::FATAL, "Bad delta entity number: %i", number );
    }

    writeStartElement( elementName );
    writeAttribute( "number", QString::number( number ) );

    // remove
    if ( d->proto->readBits( msg, 1 ) == 1 ) {
        memset( to, 0, sizeof( *to ) );
        to->number = MAX_GENTITIES - 1;
        writeAttribute( "remove", "true" );
        writeEndElement();
        return;
    }

    // no delta
    if ( d->proto->readBits( msg, 1 ) == 0 ) {
        *to = *from;
        to->number = number;
        writeEndElement();
        return;
    }

    int lastFieldNum = d->proto->readByte( msg );

    if ( lastFieldNum > d->proto->entityFieldsCount() ) {
        d->error( DtDemo::FATAL, "Incorrect field count %d", lastFieldNum );
    }

    to->number = number;
    netField_t* fld = d->proto->entityFields();

    for ( int i = 0; i < lastFieldNum; ++i, ++fld ) {
        if ( d->proto->readBits( msg, 1 ) ) {
            if ( fld->bits == 0 ) {
                // float

                if ( d->proto->readBits( msg, 1 ) == 0 ) {
                    writeField( fld->name, 0.f );
                } else {
                    if ( d->proto->readBits( msg, 1 ) == 0 ) {
                        int val = d->proto->readBits( msg, FLOAT_INT_BITS ) - FLOAT_INT_BIAS;
                        writeField( fld->name, val );
                    }
                    else {
                        // full floating point value
                        union {
                            float floatValue;
                            int intValue;
                        } value;

                        value.intValue = d->proto->readBits( msg, 32 );
                        writeField( fld->name, value.floatValue );
                    }
                }
            } else {
                int val = ( d->proto->readBits( msg, 1 ) == 0 ) ?
                          0 : d->proto->readBits( msg, fld->bits );
                writeField( fld->name, val );
            }
        }
    }

    writeEndElement();
}

#endif // DTABSTRACTPROTOCOL_H
