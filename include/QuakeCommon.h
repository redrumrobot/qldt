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

#ifndef DTQUAKECOMMON_H
#define DTQUAKECOMMON_H

typedef unsigned char byte;

/* the game guarantees that no string from the network will ever
 * exceed MAX_STRING_CHARS */
#define	MAX_STRING_CHARS	1024	/* max length of a string passed to Cmd_TokenizeString */
#define	MAX_STRING_TOKENS	1024	/* max tokens resulting from Cmd_TokenizeString */
#define	MAX_TOKEN_CHARS		1024	/* max length of an individual token */

#define	MAX_INFO_STRING		1024
#define	MAX_INFO_KEY		1024
#define	MAX_INFO_VALUE		1024

#define	BIG_INFO_STRING		8192  /* used for system info key only */
#define	BIG_INFO_KEY		8192
#define	BIG_INFO_VALUE		8192

#define	MAX_NAME_LENGTH		32		/* max length of a client name */

#define	MAX_SAY_TEXT	150


/*
 * these aren't needed by any of the VMs.  put in another header?
 */
#define	MAX_MAP_AREA_BYTES		32		/* bit vector of area visibility */

#ifdef ERR_FATAL
#undef ERR_FATAL			/* this is be defined in malloc.h */
#endif

/* parameters to the main Error routine */
typedef enum {
    ERR_FATAL,					/* exit the entire game with a popup window */
    ERR_DROP,					/* print to console and disconnect from game */
    ERR_SERVERDISCONNECT,		/* don't kill server */
    ERR_DISCONNECT,				/* client disconnected from the server */
    ERR_NEED_CD					/* pop up the need-cd dialog */
} errorParm_t;

typedef float vec3_t[3];

/*
========================================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

========================================================================
*/

#define	SNAPFLAG_RATE_DELAYED	1
#define	SNAPFLAG_NOT_ACTIVE		2	/* snapshot used during connection and for zombies */
#define SNAPFLAG_SERVERCOUNT	4	/* toggled every map_restart so transitions can be detected */

/* */
/* per-level limits */
/* */
#define	MAX_CLIENTS			64		/* absolute limit */
#define MAX_LOCATIONS		64

#define	GENTITYNUM_BITS		10		/* don't need to send any more */
#define	MAX_GENTITIES		(1<<GENTITYNUM_BITS)

/* entitynums are communicated with GENTITY_BITS, so any reserved */
/* values that are going to be communcated over the net need to */
/* also be in this range */
#define	ENTITYNUM_NONE		(MAX_GENTITIES-1)
#define	ENTITYNUM_WORLD		(MAX_GENTITIES-2)
#define	ENTITYNUM_MAX_NORMAL	(MAX_GENTITIES-2)


#define	MAX_MODELS			256		/* these are sent over the net as 8 bits */
#define	MAX_SOUNDS			256		/* so they cannot be blindly increased */

#define	MAX_CONFIGSTRINGS	1024
#define	MAX_GAMESTATE_CHARS	16000

/*========================================================= */

/* bit field limits */
#define	MAX_STATS				16
#define	MAX_PERSISTANT			16
#define	MAX_POWERUPS			16
#define	MAX_WEAPONS				16

#define	MAX_PS_EVENTS			2

#define PS_PMOVEFRAMECOUNTBITS	6

/* playerState_t is the information needed by both the client and server */
/* to predict player motion and actions */
/* nothing outside of pmove should modify these, or some degree of prediction error */
/* will occur */

/* you can't add anything to this without modifying the code in msg.c */

/* playerState_t is a full superset of entityState_t as it is used by players, */
/* so if a playerState_t is transmitted, the entityState_t can be fully derived */
/* from it. */
typedef struct playerState_s {
    int			commandTime;	/* cmd->serverTime of last executed command */
    int			pm_type;
    int			bobCycle;		/* for view bobbing and footstep generation */
    int			pm_flags;		/* ducked, jump_held, etc */
    int			pm_time;

    vec3_t		origin;
    vec3_t		velocity;
    int			weaponTime;
    int			gravity;
    int			speed;
    int			delta_angles[3];	/* add to command angles to get view direction */
                                    /* changed by spawns, rotating objects, and teleporters */

    int			groundEntityNum;/* ENTITYNUM_NONE = in air */

    int			legsTimer;		/* don't change low priority animations until this runs out */
    int			legsAnim;		/* mask off ANIM_TOGGLEBIT */

    int			torsoTimer;		/* don't change low priority animations until this runs out */
    int			torsoAnim;		/* mask off ANIM_TOGGLEBIT */

    int			movementDir;	/* a number 0 to 7 that represents the reletive angle */
                                /* of movement to the view angle (axial and diagonals) */
                                /* when at rest, the value will remain unchanged */
                                /* used to twist the legs during strafing */

    vec3_t		grapplePoint;	/* location of grapple to pull towards if PMF_GRAPPLE_PULL */

    int			eFlags;			/* copied to entityState_t->eFlags */

    int			eventSequence;	/* pmove generated events */
    int			events[MAX_PS_EVENTS];
    int			eventParms[MAX_PS_EVENTS];

    int			externalEvent;	/* events set on player from another source */
    int			externalEventParm;
    int			externalEventTime;

    int			clientNum;		/* ranges from 0 to MAX_CLIENTS-1 */
    int			weapon;			/* copied to entityState_t->weapon */
    int			weaponstate;

    vec3_t		viewangles;		/* for fixed views */
    int			viewheight;

    /* damage feedback */
    int			damageEvent;	/* when it changes, latch the other parms */
    int			damageYaw;
    int			damagePitch;
    int			damageCount;

    int			stats[MAX_STATS];
    int			persistant[MAX_PERSISTANT];	/* stats that aren't cleared on death */
    int			powerups[MAX_POWERUPS];	/* level.time that the powerup runs out */
    int			ammo[MAX_WEAPONS];

    int			generic1;
    int			loopSound;
    int			jumppad_ent;	/* jumppad entity hit this frame */

    /* not communicated over the net at all */
    int			ping;			/* server to game info for scoreboard */
    int			pmove_framecount;	/* FIXME: don't transmit over the network */
    int			jumppad_frame;
    int			entityEventSequence;
} playerState_t;


/*=================================================================== */

typedef enum {
    TR_STATIONARY,
    TR_INTERPOLATE,				/* non-parametric, but interpolate between snapshots */
    TR_LINEAR,
    TR_LINEAR_STOP,
    TR_SINE,					/* value = base + sin( time / duration ) * delta */
    TR_GRAVITY
} trType_t;

// the parseEntities array must be large enough to hold PACKET_BACKUP frames of
// entities, so that when a delta compressed message arives from the server
// it can be un-deltad from the original
#define	MAX_PARSE_ENTITIES	2048

// snapshots are a view of the server at a given time
typedef struct {
    int     		valid;			// cleared if delta parsing was invalid
    int				snapFlags;		// rate delayed and dropped commands

    int				serverTime;		// server time the message is valid for (in msec)

    int				messageNum;		// copied from netchan->incoming_sequence
    int				deltaNum;		// messageNum the delta is from
    int				ping;			// time from when cmdNum-1 was sent to time packet was reeceived
    byte			areamask[MAX_MAP_AREA_BYTES];		// portalarea visibility bits

    int				cmdNum;			// the next cmdNum the server is expecting
    playerState_t	ps;						// complete information about the current player at this time

    int				numEntities;			// all of the entities that need to be presented
    int				parseEntitiesNum;		// at the time of this snapshot

    int				serverCommandNum;		// execute all commands up to this before
                                            // making the snapshot current
} clSnapshot_t;

#define FLOAT_INT_BITS  13
#define FLOAT_INT_BIAS  (1<<(FLOAT_INT_BITS-1))

typedef struct {
    const char* name;
    int	offset;
    int	bits;			/* 0 = float */
} netField_t;

typedef struct {
    bool allowoverflow; /* if false, do a Com_Error */
    bool overflowed;    /* set to true if the buffer size failed (with allowoverflow set) */
    bool oob;           /* set to true if the buffer size failed (with allowoverflow set) */
    byte* data;
    int	maxsize;
    int cursize;
    int readcount;
    int bit;            /* for bitwise reads and writes */
} msg_t;


/*============================================================================*/

/*
==============================================================

NET

==============================================================
*/

#define	PACKET_BACKUP	32	/* number of old messages that must be kept on client and */
                            /* server for delta comrpession and ping estimation */
#define	PACKET_MASK		(PACKET_BACKUP-1)

#define	MAX_PACKET_USERCMDS		32		/* max number of usercmd_t in a packet */

#define	PORT_ANY			-1

#define	MAX_RELIABLE_COMMANDS	64			/* max string commands buffered for restransmit */

#define	MAX_MSGLEN				16384		/* max length of a message, which may */
                                            /* be fragmented into multiple packets */

#define MAX_DOWNLOAD_WINDOW			8		/* max of eight download frames */
#define MAX_DOWNLOAD_BLKSIZE		2048	/* 2048 byte block chunks */


/*
 * server to client
 */
enum svc_ops_e {
    svc_bad,
    svc_nop,
    svc_gamestate,
    svc_configstring,			/* [short] [string] only in gamestate messages */
    svc_baseline,				/* only in gamestate messages */
    svc_serverCommand,			/* [string] to be executed by client game module */
    svc_download,				/* [short] size [size bytes] */
    svc_snapshot,
    svc_EOF
};


/* these are the only configstrings that the system reserves, all the */
/* other ones are strictly for servergame to clientgame communication */
#define	CS_SERVERINFO		0		/* an info string with all the serverinfo cvars */
#define	CS_SYSTEMINFO		1		/* an info string for server system to client system configuration (timescale, etc) */
#define	CS_MUSIC				2
#define	CS_MESSAGE				3		// from the map worldspawn's message field
#define	CS_MOTD					4		// g_motd string for server message of the day
#define	CS_WARMUP				5		// server time when the match will be restarted
#define	CS_SCORES1				6
#define	CS_SCORES2				7
#define CS_VOTE_TIME			8
#define CS_VOTE_STRING			9
#define	CS_VOTE_YES				10
#define	CS_VOTE_NO				11

#define CS_TEAMVOTE_TIME		12
#define CS_TEAMVOTE_STRING		14
#define	CS_TEAMVOTE_YES			16
#define	CS_TEAMVOTE_NO			18

#define	CS_GAME_VERSION			20
#define	CS_LEVEL_START_TIME		21		// so the timer only shows the current level
#define	CS_INTERMISSION			22		// when 1, fraglimit/timelimit has been hit and intermission will start in a second or two
#define CS_FLAGSTATUS			23		// string indicating flag status in CTF
#define CS_SHADERSTATE			24
#define CS_BOTINFO				25

#define	CS_ITEMS				27		// string of 0's and 1's that tell which items are present

#define	CS_MODELS				32

#define CS_WARMUP_END           13
#define CS_INTERMISSION_QZ      14
#define CS_PAUSE_START          669
#define CS_PAUSE_COUNTDOWN      670
#define CS_CA_ROUND_INFO        661
#define CS_CA_ROUND_START       662
#define	CS_PLAYERS              544
#define	CS_PLAYERS_QZ           529
#define CS_RED_CLAN_PLAYERS     663
#define CS_BLUE_CLAN_PLAYERS    664
#define CS_FLAG_STATUS_QZ       658
#define CS_FIRST_PLACE          659
#define CS_SECOND_PLACE         660
#define CS_AD_WAIT              681
#define CPMA_GAME_INFO          672
#define CPMA_ROUND_INFO         710

extern netField_t	playerStateFields[];
extern int			playerStateFieldsNum;

#endif // DTQUAKECOMMON_H
