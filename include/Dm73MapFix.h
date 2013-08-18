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

#ifndef DTDM73MAPFIX_H
#define DTDM73MAPFIX_H

#include <Dm73.h>
#include <QuakeCommon.h>

class DtDm73MapFix : public DtDm73 {
public:
    DtDm73MapFix( DtDemo* parent );

    void parseBaseline( msg_t* msg, msg_t* outMsg );
    inline void fixModelIndexes( entityState_dm73_t* baseline );
};

#endif // DTDM73MAPFIX_H
