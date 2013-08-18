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

#ifndef DTDM68_H
#define DTDM68_H

#include <AbstractProtocol.h>
#include <QuakeCommon.h>

typedef struct {
    trType_t	trType;
    int			trTime;
    int			trDuration;			/* if non 0, trTime + trDuration = stop time */
    vec3_t		trBase;
    vec3_t		trDelta;			/* velocity, etc */
} trajectory_t;

/* entityState_t is the information conveyed from the server */
/* in an update message about entities that the client will */
/* need to render in some way */
/* Different eTypes may use the information in different ways */
/* The messages are delta compressed, so it doesn't really matter if */
/* the structure size is fairly large */

typedef struct entityState_s {
    int		number;			/* entity index */
    int		eType;			/* entityType_t */
    int		eFlags;

    trajectory_t	pos;	/* for calculating position */
    trajectory_t	apos;	/* for calculating angles */

    int		time;
    int		time2;

    vec3_t	origin;
    vec3_t	origin2;

    vec3_t	angles;
    vec3_t	angles2;

    int		otherEntityNum;	/* shotgun sources, etc */
    int		otherEntityNum2;

    int		groundEntityNum;	/* -1 = in air */

    int		constantLight;	/* r + (g<<8) + (b<<16) + (intensity<<24) */
    int		loopSound;		/* constantly loop this sound */

    int		modelindex;
    int		modelindex2;
    int		clientNum;		/* 0 to (MAX_CLIENTS - 1), for players and corpses */
    int		frame;

    int		solid;			/* for client side prediction, trap_linkentity sets this properly */

    int		event;			/* impulse events -- muzzle flashes, footsteps, etc */
    int		eventParm;

    /* for players */
    int		powerups;		/* bit flags */
    int		weapon;			/* determines weapon and flash model, etc */
    int		legsAnim;		/* mask off ANIM_TOGGLEBIT */
    int		torsoAnim;		/* mask off ANIM_TOGGLEBIT */

    int		generic1;
} entityState_t;

class DtDm68 : public DtAbstractProtocol {
public:
    DtDm68( DtDemo* parent );
    virtual ~DtDm68();

    void parseConfigString( msg_t* msg, int& dataCount, msg_t* outMsg );
    void parseCommandString( msg_t* msg, msg_t* outMsg );
    void parsePacketEnities( msg_t* msg, clSnapshot_t* oldframe, clSnapshot_t* newframe );
    void emitPacketEntities( clSnapshot_t* from, clSnapshot_t* to, msg_t* msg );
    void updateGamestateInfo( int time );
    void parseBaseline( msg_t* msg, msg_t* outMsg );
    void setGameTime( int sTime );
    void afterParse();
    QHash< QString, int > entityFieldNums();
    QHash< QString, int > playerFieldNums();
    netField_t* entityFields();
    int entityFieldsCount();

private:
    QString cpmaRoundInfo;
    entityState_t* parseEntities;
    entityState_t* entityBaselines;
    static netField_t entityStateFields[];
    static int entityStateFieldsNum;

    void parseConfigVars( char* str, int csIndex );
    inline int getCpmaTimerValue( char* str );
    inline int getCpmaInfoVar( const char* var, char* str );
    void obutuaryEventCheck( int eType, int value, DtDemo::DtObituaryEvent& event );

    friend class DtAbstractProtocol;
};

#endif // DTDM68_H
