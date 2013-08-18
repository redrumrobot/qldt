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

#ifndef DTQUAKEENUMS_H_
#define DTQUAKEENUMS_H_

enum qlGameTypes {
    GT_ANY = -2,
    GT_FFA = 0,
    GT_DUEL = 1,
    GT_RACE = 2,
    GT_TDM = 3,
    GT_CA = 4,
    GT_CTF = 5,
    GT_1FCTF = 6,
    GT_OVERLOAD = 7,
    GT_HARVESTER = 8,
    GT_FREEZE = 9,
    GT_DOM = 10,
    GT_AD = 11,
    GT_RR = 12,
    GT_INSTAGIB = 26,
    GT_INSTACTF = 27,
    GT_INSTATDM = 28
};

enum cpmaGameTypes {
    GT_HOONY_CPMA = -1,
    GT_CTF_CPMA = 4,
    GT_CA_CPMA = 5,
    GT_FREEZE_CPMA = 6,
    GT_CSTRIKE_CPMA = 7,
    GT_NTF_CPMA = 8
};

enum gametype_t {
    GT_SINGLE_PLAYER_Q3 = 2,	// single player ffa

    //-- team games go after this --

    GT_TEAM_Q3,			// team deathmatch
    GT_CTF_Q3,				// capture the flag
    GT_1FCTF_Q3,
    GT_OBELISK_Q3,
    GT_HARVESTER_Q3,
    GT_MAX_GAME_TYPE_Q3
};

enum entityStateFieldIndexes_q3 {
    ES_EVENT_Q3 = 9,
    ES_EVENTTYPE_Q3 = 11,
    ES_EVENTPARM_Q3 = 13,
    ES_OTHERENTITYNUM_Q3 = 18,
    ES_OTHERENTITYNUM2_Q3 = 30,
    ES_FRAME_Q3 = 51
};

enum entityStateFieldIndexes {
    ES_EVENT = 10,
    ES_EVENTTYPE = 12,
    ES_EVENTPARM = 14,
    ES_OTHERENTITYNUM = 19,
    ES_OTHERENTITYNUM2 = 31,
    ES_FRAME = 52
};

enum entityEvents_QL {
    EV_FOOTSTEP_QL = 1,
    EV_JUMP_QL = 10,
    EV_ITEM_PICKUP_QL = 15,
    EV_CHANGE_WEAPON_QL = 18,
    EV_FIRE_WEAPON_QL = 20,
    EV_MISSILE_MISS_QL = 48,
    EV_PAIN_QL = 53,
    EV_DEATHx_QL = 54,
    EV_OBITUARY_QL = 58,
    EV_GIB_PLAYER_QL = 63,
    EV_SCOREPLUM_QL = 64,
    EV_ITEM_PICKUP_SPEC = 83
};

typedef enum {
    TEAM_FREE,
    TEAM_RED,
    TEAM_BLUE,
    TEAM_SPECTATOR,

    TEAM_NUM_TEAMS
} team_t;

//
// entityState_t->eType
//
typedef enum {
    ET_GENERAL,
    ET_PLAYER,
    ET_ITEM,
    ET_MISSILE,
    ET_MOVER,
    ET_BEAM,
    ET_PORTAL,
    ET_SPEAKER,
    ET_PUSH_TRIGGER,
    ET_TELEPORT_TRIGGER,
    ET_INVISIBLE,
    ET_GRAPPLE,				// grapple hooked on wall
    ET_TEAM,

    ET_EVENTS				// any of the EV_* events can be added freestanding
                            // by setting eType to ET_EVENTS + eventNum
                            // this avoids having to set eFlags and eventNum
} entityType_t;

// means of death
enum {
    MOD_UNKNOWN,
    MOD_SHOTGUN,
    MOD_GAUNTLET,
    MOD_MACHINEGUN,
    MOD_GRENADE,
    MOD_GRENADE_SPLASH,
    MOD_ROCKET,
    MOD_ROCKET_SPLASH,
    MOD_PLASMA,
    MOD_PLASMA_SPLASH,
    MOD_RAILGUN,
    MOD_LIGHTNING,
    MOD_BFG,
    MOD_BFG_SPLASH,
    MOD_WATER,
    MOD_SLIME,
    MOD_LAVA,
    MOD_CRUSH,
    MOD_TELEFRAG,
    MOD_FALLING,
    MOD_SUICIDE,
    MOD_TARGET_LASER,
    MOD_TRIGGER_HURT,
    MOD_NAIL,
    MOD_CHAINGUN,
    MOD_PROXIMITY_MINE,
    MOD_KAMIKAZE,
    MOD_JUICED,
    MOD_GRAPPLE
};

// entityState_t->event values
// entity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.

// two bits at the top of the entityState->event field
// will be incremented with each change in the event so
// that an identical event started twice in a row can
// be distinguished.  And off the value with ~EV_EVENT_BITS
// to retrieve the actual event number
#define	EV_EVENT_BIT1		0x00000100
#define	EV_EVENT_BIT2		0x00000200
#define	EV_EVENT_BITS		(EV_EVENT_BIT1|EV_EVENT_BIT2)

#define	EVENT_VALID_MSEC	300

typedef enum {
    EV_NONE,

    EV_FOOTSTEP,
    EV_FOOTSTEP_METAL,
    EV_FOOTSPLASH,
    EV_FOOTWADE,
    EV_SWIM,

    EV_STEP_4,
    EV_STEP_8,
    EV_STEP_12,
    EV_STEP_16,

    EV_FALL_SHORT,
    EV_FALL_MEDIUM,
    EV_FALL_FAR,

    EV_JUMP_PAD,			// boing sound at origin, jump sound on player

    EV_JUMP,
    EV_WATER_TOUCH,	// foot touches
    EV_WATER_LEAVE,	// foot leaves
    EV_WATER_UNDER,	// head touches
    EV_WATER_CLEAR,	// head leaves

    EV_ITEM_PICKUP,			// normal item pickups are predictable
    EV_GLOBAL_ITEM_PICKUP,	// powerup / team sounds are broadcast to everyone

    EV_NOAMMO,
    EV_CHANGE_WEAPON,
    EV_FIRE_WEAPON,

    EV_USE_ITEM0,
    EV_USE_ITEM1,
    EV_USE_ITEM2,
    EV_USE_ITEM3,
    EV_USE_ITEM4,
    EV_USE_ITEM5,
    EV_USE_ITEM6,
    EV_USE_ITEM7,
    EV_USE_ITEM8,
    EV_USE_ITEM9,
    EV_USE_ITEM10,
    EV_USE_ITEM11,
    EV_USE_ITEM12,
    EV_USE_ITEM13,
    EV_USE_ITEM14,
    EV_USE_ITEM15,

    EV_ITEM_RESPAWN,
    EV_ITEM_POP,
    EV_PLAYER_TELEPORT_IN,
    EV_PLAYER_TELEPORT_OUT,

    EV_GRENADE_BOUNCE,		// eventParm will be the soundindex

    EV_GENERAL_SOUND,
    EV_GLOBAL_SOUND,		// no attenuation
    EV_GLOBAL_TEAM_SOUND,

    EV_BULLET_HIT_FLESH,
    EV_BULLET_HIT_WALL,

    EV_MISSILE_HIT,
    EV_MISSILE_MISS,
    EV_MISSILE_MISS_METAL,
    EV_RAILTRAIL,
    EV_SHOTGUN,
    EV_BULLET,				// otherEntity is the shooter

    EV_PAIN,
    EV_DEATH1,
    EV_DEATH2,
    EV_DEATH3,
    EV_OBITUARY,

    EV_POWERUP_QUAD,
    EV_POWERUP_BATTLESUIT,
    EV_POWERUP_REGEN,

    EV_GIB_PLAYER,			// gib a previously living player
    EV_SCOREPLUM,			// score plum

//#ifdef MISSIONPACK
    EV_PROXIMITY_MINE_STICK,
    EV_PROXIMITY_MINE_TRIGGER,
    EV_KAMIKAZE,			// kamikaze explodes
    EV_OBELISKEXPLODE,		// obelisk explodes
    EV_OBELISKPAIN,			// obelisk is in pain
    EV_INVUL_IMPACT,		// invulnerability sphere impact
    EV_JUICED,				// invulnerability juiced effect
    EV_LIGHTNINGBOLT,		// lightning bolt bounced of invulnerability sphere
//#endif

    EV_DEBUG_LINE,
    EV_STOPLOOPINGSOUND,
    EV_TAUNT,
    EV_TAUNT_YES,
    EV_TAUNT_NO,
    EV_TAUNT_FOLLOWME,
    EV_TAUNT_GETFLAG,
    EV_TAUNT_GUARDBASE,
    EV_TAUNT_PATROL

} entity_event_t;

// NOTE: may not have more than 16
typedef enum {
    PW_NONE,

    PW_QUAD,
    PW_BATTLESUIT,
    PW_HASTE,
    PW_INVIS,
    PW_REGEN,
    PW_FLIGHT,

    PW_REDFLAG,
    PW_BLUEFLAG,
    PW_NEUTRALFLAG,

    PW_SCOUT,
    PW_GUARD,
    PW_DOUBLER,
    PW_AMMOREGEN,
    PW_INVULNERABILITY,

    PW_NUM_POWERUPS

} powerup_t;

typedef enum {
    WP_NONE,

    WP_GAUNTLET,
    WP_MACHINEGUN,
    WP_SHOTGUN,
    WP_GRENADE_LAUNCHER,
    WP_ROCKET_LAUNCHER,
    WP_LIGHTNING,
    WP_RAILGUN,
    WP_PLASMAGUN,
    WP_BFG,
    WP_GRAPPLING_HOOK,
    WP_NAILGUN,
    WP_PROX_LAUNCHER,
    WP_CHAINGUN,

    WP_NUM_WEAPONS
} weapon_t;

// player_state->persistant[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
// NOTE: may not have more than 16
typedef enum {
    PERS_SCORE,						// !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
    PERS_HITS,						// total points damage inflicted so damage beeps can sound on change
    PERS_RANK,						// player rank or team rank
    PERS_TEAM,						// player team
    PERS_SPAWN_COUNT,				// incremented every respawn
    PERS_PLAYEREVENTS,				// 16 bits that can be flipped for events
    PERS_ATTACKER,					// clientnum of last damage inflicter
    PERS_KILLED,					// count of the number of times you died
    // player awards tracking
    PERS_IMPRESSIVE_COUNT,			// two railgun hits in a row
    PERS_EXCELLENT_COUNT,			// two successive kills in a short amount of time
    PERS_DEFEND_COUNT,				// defend awards
    PERS_ASSIST_COUNT,				// assist awards
    PERS_GAUNTLET_FRAG_COUNT,		// kills with the guantlet
    PERS_CAPTURES,					// captures
    PERS_ATTACKEE_ARMOR             // health/armor of last person we attacked
} persEnum_t;

#endif /* DTQUAKEENUMS_H_ */
