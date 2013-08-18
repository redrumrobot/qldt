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

#include "Dm73MapFix.h"
#include "Demo.h"

DtDm73MapFix::DtDm73MapFix( DtDemo* parent ) : DtDm73( parent ) {
}

void DtDm73MapFix::fixModelIndexes( entityState_dm73_t* baseline ) {
    if ( d->mapName == "qzdm14" ) {
        switch ( baseline->number ) {
            case 273 : baseline->modelindex = 19; break;
            case 274 : baseline->modelindex = 20; break;
            case 275 : baseline->modelindex = 21; break;
        }
    }
    else if ( d->mapName == "qzdm17" ) {
        if ( baseline->number == 173 ) {
            baseline->modelindex = 21;
        }
    }
    else if ( d->mapName == "qzteam7" ) {
        switch ( baseline->number ) {
            case 372 : baseline->modelindex = 39; break;
            case 373 : baseline->modelindex = 40; break;
            case 374 : baseline->modelindex = 41; break;
        }
    }
}

void DtDm73MapFix::parseBaseline( msg_t* msg, msg_t* outMsg ) {
    int newnum = readBits( msg, GENTITYNUM_BITS );
    if ( newnum < 0 || newnum >= MAX_GENTITIES ) {
        d->error( DtDemo::FATAL, "Baseline number out of range: %i\n", newnum );
    }

    entityState_dm73_t nullstate;
    memset( &nullstate, 0, sizeof( nullstate ) );

    entityState_dm73_t* nstate = &nullstate;
    entityState_dm73_t* baseline = &entityBaselines[ newnum ];

    readDeltaEntity< DtDm73 >( msg, nstate, baseline, newnum );
    fixTrTime( baseline );
    fixModelIndexes( baseline );
    writeDeltaEntity< DtDm73 >( outMsg, nstate, baseline, true );
}
