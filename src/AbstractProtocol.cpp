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

#include "AbstractProtocol.h"
#include "Data.h"
#include "HuffmanStatic.h"

/* using the stringizing operator to save typing... */
#define    PSF(x) #x, (int)(long) &( (playerState_t*) 0 )->x

netField_t playerStateFields[] =
{
{ PSF(commandTime), 32 },
{ PSF(origin[0]), 0 },
{ PSF(origin[1]), 0 },
{ PSF(bobCycle), 8 },
{ PSF(velocity[0]), 0 },
{ PSF(velocity[1]), 0 },
{ PSF(viewangles[1]), 0 },
{ PSF(viewangles[0]), 0 },
{ PSF(weaponTime), -16 },
{ PSF(origin[2]), 0 },
{ PSF(velocity[2]), 0 },
{ PSF(legsTimer), 8 },
{ PSF(pm_time), -16 },
{ PSF(eventSequence), 16 },
{ PSF(torsoAnim), 8 },
{ PSF(movementDir), 4 },
{ PSF(events[0]), 8 },
{ PSF(legsAnim), 8 },
{ PSF(events[1]), 8 },
{ PSF(pm_flags), 666 },
{ PSF(groundEntityNum), GENTITYNUM_BITS },
{ PSF(weaponstate), 4 },
{ PSF(eFlags), 16 },
{ PSF(externalEvent), 10 },
{ PSF(gravity), 16 },
{ PSF(speed), 16 },
{ PSF(delta_angles[1]), 16 },
{ PSF(externalEventParm), 8 },
{ PSF(viewheight), -8 },
{ PSF(damageEvent), 8 },
{ PSF(damageYaw), 8 },
{ PSF(damagePitch), 8 },
{ PSF(damageCount), 8 },
{ PSF(generic1), 8 },
{ PSF(pm_type), 8 },
{ PSF(delta_angles[0]), 16 },
{ PSF(delta_angles[2]), 16 },
{ PSF(torsoTimer), 12 },
{ PSF(eventParms[0]), 8 },
{ PSF(eventParms[1]), 8 },
{ PSF(clientNum), 8 },
{ PSF(weapon), 5 },
{ PSF(viewangles[2]), 0 },
{ PSF(grapplePoint[0]), 0 },
{ PSF(grapplePoint[1]), 0 },
{ PSF(grapplePoint[2]), 0 },
{ PSF(jumppad_ent), 10 },
{ PSF(loopSound), 16 },
{ PSF(jumpTime), 32 },
{ PSF(doubleJumped), 1 }
};

int playerStateFieldsNum = sizeof( playerStateFields ) / sizeof( playerStateFields[ 0 ] );

DtAbstractProtocol::DtAbstractProtocol( DtDemo* parent ) : d( parent ) {
    oldsize = 0;
}

void DtAbstractProtocol::getStrN( char* to, char*& from, int count ) {
    ++from;
    int bytesRead = 0;

    while ( *from && *from != confDelimeter ) {
        if ( bytesRead++ == count ) {
            break;
        }

        *( to++ ) = *( from++ );
    }

    *to = '\0';
}

void DtAbstractProtocol::removeQuotes( QString& str ) {
    if ( str.startsWith( '"' ) ) {
        str.remove( 0, 1 );
    }

    if ( str.endsWith( "\"\n" ) ) {
        str.chop( 2 );
    }
}

void DtAbstractProtocol::updateScores( int cmdIndex, int num ) {
    QByteArray cmd = d->commands.at( cmdIndex ).cmd.toAscii();
    const char* cmdData = cmd.data();

    if ( cmd.startsWith( '"' ) ) {
        ++cmdData;
    }

    if ( num == CS_SCORES1 ) {
        d->scores1 = atoi( cmdData );
    }
    else {
        d->scores2 = atoi( cmdData );
    }
}

void DtAbstractProtocol::readPlayerName( char*& str, char* pName ) {
    while ( *( ++str ) && *str != confDelimeter ) {
        if ( *str == '^' ) {
            if ( str[ 1 ] ) {
                ++str;
            }
            else {
                break;
            }
        }
        else {
            *( pName++ ) = *str;
        }
    }

    *pName = '\0';
}

void DtAbstractProtocol::initialize( msg_t* buf, byte* data, int length ) {
    memset( buf, 0, sizeof( *buf ) );
    buf->data = data;
    buf->maxsize = length;
}

/* negative bit values include signs */
void DtAbstractProtocol::writeBits( msg_t* msg, int value, int bits ) {
    oldsize += bits;

    /* this isn't an exact overflow check, but close enough */
    if ( msg->maxsize - msg->cursize < 4 ) {
        msg->overflowed = true;
        return;
    }

    if ( bits == 0 || bits < -31 || bits > 32 ) {
        Com_Error( ERR_DROP, "MSG_WriteBits: bad bits %i", bits );
    }

    /* check for overflows */
    if ( bits != 32 ) {
        if ( bits > 0 ) {
            if ( value > ( ( 1 << bits ) - 1 ) || value < 0 ) {
                overflows++;
            }
        } else {
            int	r = 1 << ( bits - 1 );

            if ( value >  r - 1 || value < -r ) {
                overflows++;
            }
        }
    }

    if ( bits < 0 ) {
        bits = -bits;
    }

    value &= ( 0xffffffff >> ( 32 - bits ) );

    if ( bits & 7 ) {
        int nbits = bits & 7;

        for ( int i = 0; i < nbits; ++i ) {
            huffman.putBit( ( value & 1 ), msg->data, &msg->bit );
            value >>= 1;
        }

        bits = bits - nbits;
    }

    if ( bits ) {
        for ( int i = 0; i < bits; i += 8 ) {
            huffman.offsetTransmit( &huffmanCompressor, ( value & 0xff ), msg->data, &msg->bit );
            value >>= 8;
        }
    }

    msg->cursize = ( msg->bit >> 3 ) + 1;
}

void DtAbstractProtocol::writeByte( msg_t* msg, int c) {
    writeBits( msg, c, 8 );
}

void DtAbstractProtocol::writeShort( msg_t* msg, int c) {
    writeBits( msg, c, 16 );
}

void DtAbstractProtocol::writeLong( msg_t* msg, int c) {
    writeBits( msg, c, 32 );
}

void DtAbstractProtocol::writeString( msg_t* msg, const char* s ) {
    if ( !s ) {
        writeData( msg, "", 1 );
    } else {
        int l;
        char string[ MAX_STRING_CHARS ];

        l = strlen( s );
        if ( l >= MAX_STRING_CHARS ) {
            Com_Printf( "MSG_WriteString: MAX_STRING_CHARS" );
            writeData (msg, "", 1);
            return;
        }

        strncpy( string, s, sizeof( string ) - 1 );
        string[ sizeof( string ) - 1 ] = 0;

        /* get rid of 0xff chars, because old clients don't like them */
        for ( int i = 0; i < l; ++i ) {
            if ( ( ( byte* ) string )[ i ] > 127 ) {
                string[ i ] = '.';
            }
        }

        writeData( msg, string, l + 1 );
    }
}

void DtAbstractProtocol::writeBigString( msg_t* sb, const char* s ) {
    if ( !s ) {
        writeData (sb, "", 1);
    } else {
        int     l,i;
        char    string[BIG_INFO_STRING];

        l = strlen( s );
        if ( l >= BIG_INFO_STRING ) {
            Com_Printf( "MSG_WriteString: BIG_INFO_STRING" );
            writeData (sb, "", 1);
            return;
        }

        strncpy( string, s, sizeof( string ) - 1 );
        string[ sizeof( string ) - 1 ] = 0;

        /* get rid of 0xff chars, because old clients don't like them */
        for ( i = 0 ; i < l ; i++ ) {
            if ( ((byte *)string)[i] > 127 ) {
                string[i] = '.';
            }
        }

        writeData (sb, string, l+1);
    }
}

void DtAbstractProtocol::writeData( msg_t* buf, const void* data, int length ) {
    for ( int i = 0; i < length; i++ ) {
        writeByte( buf, ( ( byte* ) data )[ i ] );
    }
}

int DtAbstractProtocol::readBits( msg_t* msg, int bits ) {
    int value = 0;
    int nbits = 0;
    bool sign = false;

    if ( bits < 0 ) {
        bits = -bits;
        sign = true;
    }

    if ( bits & 7 ) {
        nbits = bits & 7;

        for ( int i = 0; i < nbits; ++i ) {
            value |= ( huffman.getBit( msg->data, &msg->bit ) << i );
        }

        bits = bits - nbits;
    }

    if ( bits ) {
        for ( int i = 0; i < bits; i+= 8 ) {
            int get;

            huffman.offsetReceive( huffmanCompressor.tree, &get, msg->data, &msg->bit );
            value |= ( get << ( i + nbits ) );
        }
    }

    msg->readcount = ( msg->bit >> 3 ) + 1;

    if ( sign ) {
        if ( value & ( 1 << ( bits - 1 ) ) ) {
            value |= -1 ^ ( ( 1 << bits ) - 1 );
        }
    }

    return value;
}

int DtAbstractProtocol::readByte( msg_t* msg ) {
    int c = (unsigned char) readBits( msg, 8 );

    if ( msg->readcount > msg->cursize ) {
        c = -1;
    }
    return c;
}

int DtAbstractProtocol::readShort( msg_t* msg ) {
    int c = (short) readBits( msg, 16 );

    if ( msg->readcount > msg->cursize ) {
        c = -1;
    }

    return c;
}

int DtAbstractProtocol::readLong( msg_t* msg ) {
    int c = readBits( msg, 32 );

    if ( msg->readcount > msg->cursize ) {
        c = -1;
    }

    return c;
}

char* DtAbstractProtocol::readString( msg_t* msg ) {
    size_t l;
    int c;

    l = 0;
    do {
        c = readByte( msg );        /* use ReadByte so -1 is out of bounds */
        if ( c == -1 || c == 0 ) {
            break;
        }
        /* translate all fmt spec to avoid crash bugs */
        if ( c == '%' ) {
            c = '.';
        }
        /* don't allow higher ascii values */
        if ( c > 127 ) {
            c = '.';
        }

        stringBuf[ l ] = c;
        l++;
    } while ( l < sizeof( stringBuf ) - 1 );

    stringBuf[ l ] = 0;

    return stringBuf;
}

char* DtAbstractProtocol::readBigString( msg_t* msg ) {
    size_t l;
    int c;

    l = 0;
    do {
        c = readByte( msg );        /* use ReadByte so -1 is out of bounds */
        if ( c == -1 || c == 0 ) {
            break;
        }
        /* translate all fmt spec to avoid crash bugs */
        if ( c == '%' ) {
            c = '.';
        }

        bigStringBuf[ l ] = c;
        l++;
    } while ( l < sizeof( bigStringBuf ) - 1 );

    bigStringBuf[ l ] = 0;

    return bigStringBuf;
}

void DtAbstractProtocol::readData( msg_t* msg, void* data, int len ) {
    for ( int i = 0; i < len; i++ ) {
        ( (byte*) data )[ i ] = readByte( msg );
    }
}

void DtAbstractProtocol::writeDeltaPlayerstate( msg_t* msg, struct playerState_s* from, struct playerState_s* to ) {
    int              i;
    playerState_t    dummy;
    int              statsbits;
    int              persistantbits;
    int              ammobits;
    int              powerupbits;
    int              numFields;
    int              c;
    netField_t       *field;
    int              *fromF, *toF;
    float            fullFloat;
    int              trunc, lc;

    if (!from) {
        from = &dummy;
        memset( &dummy, 0, sizeof( dummy ) );
    }

    c = msg->cursize;

    numFields = sizeof( playerStateFields ) / sizeof( playerStateFields[0] );

    lc = 0;
    for ( i = 0, field = playerStateFields ; i < numFields ; i++, field++ ) {
        fromF = (int *)( (byte *)from + field->offset );
        toF = (int *)( (byte *)to + field->offset );
        if ( *fromF != *toF ) {
            lc = i+1;
        }
    }

    writeByte( msg, lc );    /* # of changes */

    oldsize += numFields - lc;

    for ( i = 0, field = playerStateFields ; i < lc ; i++, field++ ) {
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
        } else {
            /* integer */
            writeBits( msg, *toF, field->bits == 666 ? ( d->realProto == 90 ? 24 : 16 ) : field->bits );
        }
    }
    c = msg->cursize - c;

    /* */
    /* send the arrays */
    /* */
    statsbits = 0;
    for (i=0 ; i<16 ; i++) {
        if (to->stats[i] != from->stats[i]) {
            statsbits |= 1<<i;
        }
    }
    persistantbits = 0;
    for (i=0 ; i<16 ; i++) {
        if (to->persistant[i] != from->persistant[i]) {
            persistantbits |= 1<<i;
        }
    }
    ammobits = 0;
    for (i=0 ; i<16 ; i++) {
        if (to->ammo[i] != from->ammo[i]) {
            ammobits |= 1<<i;
        }
    }
    powerupbits = 0;
    for (i=0 ; i<16 ; i++) {
        if (to->powerups[i] != from->powerups[i]) {
            powerupbits |= 1<<i;
        }
    }

    if (!statsbits && !persistantbits && !ammobits && !powerupbits) {
        writeBits( msg, 0, 1 );    /* no change */
        oldsize += 4;
        return;
    }
    writeBits( msg, 1, 1 );    /* changed */

    if ( statsbits ) {
        writeBits( msg, 1, 1 );    /* changed */
        writeShort( msg, statsbits );
        for (i=0 ; i<16 ; i++)
            if (statsbits & (1<<i) )
                writeShort (msg, to->stats[i]);
    } else {
        writeBits( msg, 0, 1 );    /* no change */
    }


    if ( persistantbits ) {
        writeBits( msg, 1, 1 );    /* changed */
        writeShort( msg, persistantbits );
        for (i=0 ; i<16 ; i++)
            if (persistantbits & (1<<i) )
                writeShort (msg, to->persistant[i]);
    } else {
        writeBits( msg, 0, 1 );    /* no change */
    }


    if ( ammobits ) {
        writeBits( msg, 1, 1 );    /* changed */
        writeShort( msg, ammobits );
        for (i=0 ; i<16 ; i++)
            if (ammobits & (1<<i) )
                writeShort (msg, to->ammo[i]);
    } else {
        writeBits( msg, 0, 1 );    /* no change */
    }


    if ( powerupbits ) {
        writeBits( msg, 1, 1 );    /* changed */
        writeShort( msg, powerupbits );

        for ( i = 0 ; i < 16 ; i++ )
            if ( powerupbits & ( 1 << i ) )
                writeLong( msg, to->powerups[ i ] );
    } else {
        writeBits( msg, 0, 1 );    /* no change */
    }
}

void DtAbstractProtocol::readDeltaPlayerstate( msg_t* msg, struct playerState_s* from, struct playerState_s* to ) {
    int            i, lc;
    int            bits;
    netField_t    *field;
    int            numFields;
    int            *fromF, *toF;
    int            trunc;
    playerState_t  dummy;

    if ( !from ) {
        from = &dummy;
        memset( &dummy, 0, sizeof( dummy ) );
    }
    *to = *from;

#ifdef MSG_LOG
    int startBit;
    int endBit;

    if ( msg->bit == 0 ) {
        startBit = msg->readcount * 8 - GENTITYNUM_BITS;
    } else {
        startBit = ( msg->readcount - 1 ) * 8 + msg->bit - GENTITYNUM_BITS;
    }

    Com_Printf( "%3i: playerstate ", msg->readcount );
#endif

    numFields = sizeof( playerStateFields ) / sizeof( playerStateFields[0] );
    lc = readByte( msg );

    for ( i = 0, field = playerStateFields ; i < lc ; i++, field++ ) {
        fromF = (int *)( (byte *)from + field->offset );
        toF = (int *)( (byte *)to + field->offset );

        if ( !readBits( msg, 1 ) ) {
            /* no change */
            *toF = *fromF;
        } else {
            if ( field->bits == 0 ) {
                /* float */
                if ( readBits( msg, 1 ) == 0 ) {
                    /* integral float */
                    trunc = readBits( msg, FLOAT_INT_BITS );
                    /* bias to allow equal parts positive and negative */
                    trunc -= FLOAT_INT_BIAS;
                    *(float *)toF = trunc;
#ifdef MSG_LOG
                    Com_Printf( "%s:%i ", field->name, trunc );
#endif
                } else {
                    /* full floating point value */
                    *toF = readBits( msg, 32 );
#ifdef MSG_LOG
                    Com_Printf( "%s:%f ", field->name, *(float *)toF );
#endif
                }
            } else {
                /* integer */
                *toF = readBits( msg, field->bits == 666 ? ( d->realProto == 90 ? 24 : 16 ) : field->bits );
#ifdef MSG_LOG
                Com_Printf( "%s:%i ", field->name, *toF );
#endif
            }
        }
    }

    for ( i=lc,field = &playerStateFields[lc]; i<numFields; i++, field++) {
        fromF = (int *)( (byte *)from + field->offset );
        toF = (int *)( (byte *)to + field->offset );
        /* no change */
        *toF = *fromF;
    }

    /* read the arrays */
    if ( readBits( msg, 1 ) ) {
        /* parse stats */
        if ( readBits( msg, 1 ) ) {
            bits = readShort (msg);
            for (i=0 ; i<16 ; i++) {
                if (bits & (1<<i) ) {
                    to->stats[i] = readShort(msg);
                }
            }
        }

        /* parse persistant stats */
        if ( readBits( msg, 1 ) ) {
            bits = readShort (msg);
            for (i=0 ; i<16 ; i++) {
                if (bits & (1<<i) ) {
                    to->persistant[i] = readShort(msg);
                }
            }
        }

        /* parse ammo */
        if ( readBits( msg, 1 ) ) {
            bits = readShort (msg);
            for (i=0 ; i<16 ; i++) {
                if (bits & (1<<i) ) {
                    to->ammo[i] = readShort(msg);
                }
            }
        }

        /* parse powerups */
        if ( readBits( msg, 1 ) ) {
            bits = readShort (msg);
            for (i=0 ; i<16 ; i++) {
                if (bits & (1<<i) ) {
                    to->powerups[i] = readLong(msg);
                }
            }
        }
    }

#ifdef MSG_LOG
    if ( msg->bit == 0 ) {
        endBit = msg->readcount * 8 - GENTITYNUM_BITS;
    } else {
        endBit = ( msg->readcount - 1 ) * 8 + msg->bit - GENTITYNUM_BITS;
    }
    Com_Printf( " (%i bits)\n", endBit - startBit  );
#endif
}

void DtAbstractProtocol::skipPlayerState( msg_t* msg ) {
    int lastPlayerField = readByte( msg );

    if ( lastPlayerField > playerStateFieldsNum ) {
        d->error( DtDemo::FATAL, "max allowed field num is %d", playerStateFieldsNum );
    }

    netField_t* fld = playerStateFields;

    for ( int i = 0; i < lastPlayerField; ++i, ++fld ) {
        if ( readBits( msg, 1 ) ) {
            if ( fld->bits == 0 ) {
                if ( readBits( msg, 1 ) == 0 ) {
                    readBits( msg, FLOAT_INT_BITS );
                }
                else {
                    readBits( msg, 32 );
                }
            }
            else {
                readBits( msg, fld->bits == 666 ? ( d->realProto == 90 ? 24 : 16 ) : fld->bits );
            }
        }
    }

    if ( readBits( msg, 1 ) ) {
        for ( int i = 0; i < 3; ++i ) {
            if ( readBits( msg, 1 ) ) {
                int bVal = readShort( msg );
                for ( int j = 0; j < 16; ++j ) {
                    if ( bVal & ( 1 << j ) ) {
                        readShort( msg );
                    }
                }
            }
        }

        if ( readBits( msg, 1 ) ) {
            int bVal = readShort( msg );
            for ( int j = 0; j < 16; ++j ) {
                if ( bVal & ( 1 << j ) ) {
                    readLong( msg );
                }
            }
        }
    }
}

void DtAbstractProtocol::Com_Printf( const char *fmt, ... ) {
    va_list    argptr;

    va_start (argptr, fmt);
    vprintf (fmt, argptr);
    va_end (argptr);
}


void DtAbstractProtocol::Com_Error( int code, const char *fmt, ... ) {
    va_list    argptr;

    printf("Com_Error(%d)\n", code);
    va_start (argptr, fmt);
    vprintf (fmt, argptr);
    va_end (argptr);
}
