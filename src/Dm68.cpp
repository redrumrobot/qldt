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

#include "Dm68.h"
#include "Demo.h"

/* using the stringizing operator to save typing... */
#define NETF(x) #x, (int)(long) &( (entityState_t*) 0 )->x

netField_t DtDm68::entityStateFields[] =
{
{ NETF(pos.trTime), 32 },
{ NETF(pos.trBase[0]), 0 },
{ NETF(pos.trBase[1]), 0 },
{ NETF(pos.trDelta[0]), 0 },
{ NETF(pos.trDelta[1]), 0 },
{ NETF(pos.trBase[2]), 0 },
{ NETF(apos.trBase[1]), 0 },
{ NETF(pos.trDelta[2]), 0 },
{ NETF(apos.trBase[0]), 0 },
{ NETF(event), 10 },
{ NETF(angles2[1]), 0 },
{ NETF(eType), 8 },
{ NETF(torsoAnim), 8 },
{ NETF(eventParm), 8 },
{ NETF(legsAnim), 8 },
{ NETF(groundEntityNum), GENTITYNUM_BITS },
{ NETF(pos.trType), 8 },
{ NETF(eFlags), 19 },
{ NETF(otherEntityNum), GENTITYNUM_BITS },
{ NETF(weapon), 8 },
{ NETF(clientNum), 8 },
{ NETF(angles[1]), 0 },
{ NETF(pos.trDuration), 32 },
{ NETF(apos.trType), 8 },
{ NETF(origin[0]), 0 },
{ NETF(origin[1]), 0 },
{ NETF(origin[2]), 0 },
{ NETF(solid), 24 },
{ NETF(powerups), 16 },
{ NETF(modelindex), 8 },
{ NETF(otherEntityNum2), GENTITYNUM_BITS },
{ NETF(loopSound), 8 },
{ NETF(generic1), 8 },
{ NETF(origin2[2]), 0 },
{ NETF(origin2[0]), 0 },
{ NETF(origin2[1]), 0 },
{ NETF(modelindex2), 8 },
{ NETF(angles[0]), 0 },
{ NETF(time), 32 },
{ NETF(apos.trTime), 32 },
{ NETF(apos.trDuration), 32 },
{ NETF(apos.trBase[2]), 0 },
{ NETF(apos.trDelta[0]), 0 },
{ NETF(apos.trDelta[1]), 0 },
{ NETF(apos.trDelta[2]), 0 },
{ NETF(time2), 32 },
{ NETF(angles[2]), 0 },
{ NETF(angles2[0]), 0 },
{ NETF(angles2[2]), 0 },
{ NETF(constantLight), 32 },
{ NETF(frame), 16 }
};

int DtDm68::entityStateFieldsNum = sizeof( entityStateFields ) / sizeof( entityStateFields[ 0 ] );

DtDm68::DtDm68( DtDemo* parent ) : DtAbstractProtocol( parent ) {
    parseEntities = new entityState_t[ MAX_PARSE_ENTITIES ];
    entityBaselines = new entityState_t[ MAX_PARSE_ENTITIES ];
    memset( entityBaselines, 0, sizeof( entityBaselines ) * MAX_GENTITIES );
}

DtDm68::~DtDm68() {
    delete [] parseEntities;
    delete [] entityBaselines;
}

int DtDm68::getCpmaTimerValue( char* str ) {
    int ts = 0;

    while ( *( str++ ) ) {
        if ( *str == confDelimeter && *( str + 1 ) == 't' ) {
            if( str[ 2 ] == 's' &&
                str[ 3 ] == confDelimeter )
            {
                str += 4;
                ts = atoi( str );
            }
            else if ( str[ 2 ] == 'd' &&
                      str[ 3 ] == confDelimeter )
            {
                return ts + atoi( str + 4 );
            }
        }
    }

    return ts;
}

int DtDm68::getCpmaInfoVar( const char* var, char* str ) {
    char* start = str;

    while ( *str ) {
        if ( str == start ) {
            if ( *str == var[ 0 ]       &&
                 str[ 1 ] == var[ 1 ]   &&
                 str[ 2 ] == confDelimeter )
            {
                return atoi( str + 3 );
            }
        }
        else if ( *str == confDelimeter ) {
            if ( str[ 1 ] == var[ 0 ] &&
                 str[ 2 ] == var[ 1 ] &&
                 str[ 3 ] == confDelimeter )
            {
                return atoi( str + 4 );
            }
        }

        ++str;
    }

    return -1;
}

void DtDm68::parseConfigVars( char* str, int csIndex ) {
    if ( csIndex == CS_WARMUP ) {
        d->demoWarmupEndTime = atoi( str );

        if ( d->demoWarmupEndTime == -1  ) {
            d->countdownStopped = true;
        }

        return;
    }
    else {
        if ( d->q3Mod != MOD_CPMA && csIndex == CS_LEVEL_START_TIME ) {
            d->demoWarmupEndTime = atoi( str );
            return;
        }
        else if ( d->q3Mod == MOD_CPMA && csIndex == CPMA_GAME_INFO ) {
            d->demoWarmupEndTime = getCpmaInfoVar( "tw", str );

            if ( d->demoWarmupEndTime == -1  ) {
                d->countdownStopped = true;
            }

            d->demoTimerValue = getCpmaTimerValue( str );

            return;
        }
    }

    if ( str[ 0 ] == confDelimeter ) { /* CS_SERVERINFO or CS_SYSTEMINFO */
        char key[ DtDemo::MAX_CONFKEY ];
        char val[ MAX_GAMESTATE_CHARS ];

        while ( *str ) {
            getStrN( key, str, DtDemo::MAX_CONFKEY );

            if ( !*str ) {
                d->error( DtDemo::WARN, "Empty config key" );
                return;
            }

            getStrN( val, str, MAX_GAMESTATE_CHARS );

            if ( !strcmp( key, "mapname" ) ) {
                d->mapName = QString( val ).toLower();
            }
            else if ( !strcmp( key, "sv_hostname" ) ) {
                d->hostName = val;
            }
            else if ( !strcmp( key, "g_gametype" ) ) {
                d->gameType = atoi( val );
            }
            else if ( !strcmp( key, "g_instaGib" ) || !strcmp( key, "g_instagib" ) ) {
                d->instaGib = atoi( val );
            }
            else if ( !strcmp( key, "gamename" ) ) {
                d->q3ModName = val;

                if ( !strcmp( val, "osp" ) ) {
                    d->q3Mod = MOD_OSP;
                }
                else if ( !strcmp( val, "cpma" ) ) {
                    d->q3Mod = MOD_CPMA;
                }
            }

            d->info.append( QPair< QString, QString >( key, val ) );
        }
    }
    else if ( csIndex >= CS_PLAYERS && csIndex < CS_PLAYERS + MAX_CLIENTS ) { /* players */
        char pName[ DtDemo::MAX_CONFKEY ];
        pName[ 0 ] = '\0';
        readPlayerName( ++str, pName );

        int playerNum = csIndex - CS_PLAYERS;
        QString playerInfoNum = QString( "playerName%1" ).arg( playerNum );
        int team = atoi( str + 3 );

        d->playerNames.insert( playerNum, pName );
        d->playerTeams.insert( playerNum, team );

        switch ( team ) {
            case TEAM_SPECTATOR :
                d->cgsSpecs.append( QPair< QString, QString >( playerInfoNum, pName ) );
            break;

            case TEAM_FREE :
            case TEAM_RED :
                d->cgsRedPlayers.append( QPair< QString, QString >( playerInfoNum, pName ) );
            break;

            case TEAM_BLUE :
                d->cgsBluePlayers.append( QPair< QString, QString >( playerInfoNum, pName ) );
            break;

            default : break;
        }
    }
}

void DtDm68::parseConfigString( msg_t* msg, int& dataCount, msg_t* outMsg ) {
    char* str;
    int len;
    int csIndex = readShort( msg );
    bool playerInfoString = ( csIndex >= CS_PLAYERS &&
                              csIndex < CS_PLAYERS + MAX_CLIENTS );

    if ( csIndex < 0 || csIndex >= MAX_CONFIGSTRINGS ) {
        d->error( DtDemo::FATAL, "Config string index > MAX_CONFIGSTRINGS\n" );
    }

    str = readBigString( msg );
    len = strlen( str );

    if ( len + 1 + dataCount > MAX_GAMESTATE_CHARS ) {
        d->error( DtDemo::FATAL, "MAX_GAMESTATE_CHARS exceeded\n" );
    }

    dataCount += len + 1;

    if ( d->currentParseType == DtDemo::CustomWrite ) {
        if ( d->writeCfg->exportXml ) {
            d->xmlWriter->writeConfigString( csIndex, str );
            return;
        }
        else {
            if ( d->writeCfg->setGamestateString ) {
                playerInfoString = false;
            }

            if ( !playerInfoString ) {
                writeByte( outMsg, svc_configstring );
                writeShort( outMsg, csIndex );
            }
        }

        const char* writeStr = str;
        char csTime[ 20 ];
        char infoStr[ MAX_GAMESTATE_CHARS ];
        QByteArray cfgString;
        int countdownTime = d->msgSnapTime + d->mapRestartTime -
                            d->firstSnapTime; /* time left for warmup */

        #define SET_INT( writeInt )             \
            sprintf( csTime, "%d", writeInt );  \
            writeStr = csTime

        #define SET_STR( writeQStr )                                    \
            int cpLen = ( writeQStr.size() > MAX_GAMESTATE_CHARS ) ?    \
                        MAX_GAMESTATE_CHARS : writeQStr.size();         \
            infoStr[ cpLen ] = '\0';                                    \
            strncpy( infoStr, writeQStr.toAscii().data(), cpLen );      \
                                                                        \
            writeStr = infoStr

        if ( d->writeCfg->setGamestateString ) {
            if ( csIndex == d->writeCfg->gamestateString.first ) {
                SET_STR( d->writeCfg->gamestateString.second );
            }
        }
        else {
            if ( csIndex == CS_FLAGSTATUS ) {
                removeQuotes( d->flagStatus );
                SET_STR( d->flagStatus );
            }
            else if ( d->q3Mod == MOD_CPMA ) {
                QString cmd = str;

                if ( csIndex == CPMA_GAME_INFO ) {
                    int scoreBlue = -1;
                    int scoreRed = -1;
                    int tw = -1;

                    if ( !cpmaRoundInfo.isEmpty() ) {
                        removeQuotes( cpmaRoundInfo );

                        QByteArray cmdStr = cpmaRoundInfo.toAscii();

                        scoreBlue = getCpmaInfoVar( "sb", cmdStr.data() );
                        scoreRed = getCpmaInfoVar( "sr", cmdStr.data() );

                        tw = getCpmaInfoVar( "tw", cmdStr.data() );

                        if ( tw != -1 && tw != 0 ) {
                            cpmaRoundInfo.replace( QRegExp( "tw\\\\-?\\d+" ),
                                                   QString( "tw\\%1" )
                                                   .arg( tw - d->firstSnapTime +
                                                         DtDemo::MSG_START_TIME ) );
                        }
                    }

                    if ( tw == -1 ) {
                        tw = getCpmaInfoVar( "tw", str );

                        if ( tw != -1 && tw != 0 ) {
                            tw = ( d->writeCfg->removeWarmup ) ? 0 : countdownTime;
                        }

                        if ( d->writeCfg->removeWarmup && tw == -1 ) {
                            tw = 0;
                        }
                    }

                    int warmupEndTime = DtDemo::MSG_START_TIME;

                    if ( d->writeCfg->singleFile ) {
                        warmupEndTime -= d->writeCfg->timerInitialValue;
                    }
                    else if ( d->lastPartIndex > -1 ) {
                        warmupEndTime -= d->writeCfg->cutSegments.at( d->lastPartIndex ).start -
                                         d->getMapRestartTime();
                    }

                    d->demoTimerValue = warmupEndTime;

                    if ( scoreBlue != -1 ) {
                        cmd.replace( QRegExp( "sb\\\\\\d+\\\\" ),
                                     QString( "sb\\%1\\" ).arg( scoreBlue ) );
                    }

                    if ( scoreRed != -1 ) {
                        cmd.replace( QRegExp( "sr\\\\\\d+\\\\" ),
                                     QString( "sr\\%1\\" ).arg( scoreRed ) );
                    }

                    cmd.replace( QRegExp( "tw\\\\-?\\d+\\\\" ),
                                 QString( "tw\\%1\\" ).arg( tw ) );

                    cmd.replace( QRegExp( "ts\\\\-?\\d+\\\\td\\\\\\d+" ),
                                 QString( "ts\\%2\\td\\0" ).arg( warmupEndTime ) );
                }
                else if ( csIndex == CPMA_ROUND_INFO ) {
                    cmd = cpmaRoundInfo;
                }

                SET_STR( cmd );
            }
            else {
                switch ( csIndex ) {
                    case CS_WARMUP : {
                        if ( atoi( str ) != -1 ) {
                            SET_INT( ( d->writeCfg->removeWarmup ) ? 0 : countdownTime );
                        }
                    }
                    break;

                    case CS_LEVEL_START_TIME : {
                        int warmupEndTime = DtDemo::MSG_START_TIME;

                        if ( d->writeCfg->singleFile ) {
                            warmupEndTime -= d->writeCfg->timerInitialValue;
                        }
                        else if ( d->lastPartIndex > -1 ) {
                            warmupEndTime -= d->writeCfg->cutSegments.at( d->lastPartIndex )
                                             .start;
                        }

                        SET_INT( warmupEndTime );
                    }
                    break;

                    case CS_SCORES1 : SET_INT( d->scores1 ); break;
                    case CS_SCORES2 : SET_INT( d->scores2 ); break;
                }
            }
        }

        if ( playerInfoString ) {
            /* write the last known info about each player */
            if ( !d->playersInfoWritten ) {
                QMapIterator< int, QString > it( d->playersInfo );

                while ( it.hasNext() ) {
                    it.next();

                    QByteArray playerInfo = it.value().toAscii();

                    if ( playerInfo.endsWith( "\"\n" ) ) {
                        playerInfo.chop( 2 );
                    }

                    const char* infoStr = playerInfo.data();

                    if ( playerInfo.startsWith( '"' ) ) {
                        ++infoStr;
                    }

                    if ( strlen( infoStr ) != 0 ) {
                        writeByte( outMsg, svc_configstring );
                        writeShort( outMsg, it.key() );
                        writeBigString( outMsg, infoStr );
                    }
                }

                d->playersInfoWritten = true;
            }
        }

        if ( !playerInfoString ) {
            writeBigString( outMsg, writeStr );
        }
    }
    else if ( d->currentParseType == DtDemo::ReadEditInfo ) {
        d->commands.append( DtDemoCommand( -1, csIndex, str ) );
    }
    else if ( d->currentParseType == DtDemo::ParseGamestate ||
            ( d->currentParseType == DtDemo::FindFrags &&
              !( d->state & DtDemo::INFO_PARSED ) ) )
    {
        parseConfigVars( str, csIndex );
        d->levelStartTime = d->fInfo.lastModified;
    }
}

void DtDm68::parseCommandString( msg_t* msg, msg_t* outMsg ) {
    int seq = readLong( msg );
    bool oldCommand = d->pastCommands.contains( seq );

    if ( !oldCommand ) {
        d->pastCommands.insert( seq );
    }

    char* s = readString( msg );

    if ( d->currentParseType == DtDemo::CustomWrite ) {
        if ( d->writeCfg->exportXml ) {
            d->xmlWriter->writeCommand( seq, s );
            return;
        }

        if( d->writeCfg->writeCommands ) {
            return;
        }
    }

    bool configString = false;
    bool chatCmd = false;
    int csIndex = -1;

    if ( s == strstr( s, "cs " ) ) {
        configString = true;
        csIndex = atoi( s + 3 );
    }
    else if( s == strstr( s, "chat" )   ||
             s == strstr( s, "tchat" )  ||
             s == strstr( s, "mm2" ) )
    {
        chatCmd = true;
    }

    if ( d->currentParseType == DtDemo::CustomWrite ) {
        if ( ( d->writeCfg->editChat && !chatCmd ) || !d->writeCfg->editChat ) {
            char csTime[ 40 ];
            char infoStr[ 1024 ];

            if ( configString ) {
                #define SET_TIME( var, time )                       \
                    sprintf( csTime, "cs %d \"%d\"\n", var, time ); \
                    s = csTime


                int restartTime = DtDemo::MSG_START_TIME + d->mapRestartTime - d->firstSnapTime;

                if ( d->q3Mod == MOD_CPMA ) {
                    QString cmd = s;

                    if ( csIndex == CPMA_GAME_INFO ) {
                        int tw = getCpmaInfoVar( "tw", s );

                        if ( tw != 0 ) {
                            tw = restartTime;
                        }

                        int te = getCpmaInfoVar( "te", s );

                        if ( te != 0 ) {
                            if ( d->cpmaTimerStopped == -1 ) {
                                te = d->msgSnapTime - d->demoTimerValue;
                                d->demoPauseStartTime = d->msgSnapTime;
                                d->cpmaTimerStopped = te;
                            }
                            else {
                                te = d->cpmaTimerStopped;
                            }
                        }

                        int td = 0;

                        if ( d->cpmaTimerStopped != -1 && d->demoPauseStartTime != -1 ) {
                            td = d->msgSnapTime - d->demoPauseStartTime;
                        }

                        if ( te == 0 ) {
                            if ( d->cpmaTimerStopped != -1 ) {
                                d->cpmaTimerStopped = -1;
                            }

                            d->demoPauseStartTime = -1;
                        }

                        cmd.replace( QRegExp( "tw\\\\-?\\d+\\\\" ),
                                     QString( "tw\\%1\\" ).arg( tw ) );

                        cmd.replace( QRegExp( "ts\\\\-?\\d+\\\\td\\\\\\d+\\\\te\\\\\\d+" ),
                                     QString( "ts\\%2\\td\\%3\\te\\%4" )
                                     .arg( d->demoTimerValue )
                                     .arg( td )
                                     .arg( te ) );

                        int vt = getCpmaInfoVar( "vt", s );

                        if ( vt != 0 ) {
                            cmd.replace( QRegExp( "\\\\vt\\\\\\d+\\\\" ), QString( "\\vt\\%1\\" )
                                         .arg( d->msgSnapTime ) );
                        }
                    }
                    else if ( csIndex == CPMA_ROUND_INFO ) {
                        int tw = getCpmaInfoVar( "tw", s );

                        if ( tw != 0 ) {
                            cmd.replace( QRegExp( "tw\\\\-?\\d+" ),
                                         QString( "tw\\%1" )
                                         .arg( tw - d->lastSnapTime + d->msgSnapTime ) );
                        }
                    }

                    int cpLen = ( cmd.size() > 1024 ) ? 1024 : cmd.size();
                    infoStr[ cpLen ] = '\0';
                    strncpy( infoStr, cmd.toAscii().data(), cpLen );

                    s = infoStr;
                }
                else { /* VQ3, OSP */
                    switch ( csIndex ) {
                        case CS_WARMUP :
                            if ( *( s + 6 ) != '"' ) {
                                SET_TIME( CS_WARMUP, restartTime );
                            }
                        break;

                        case CS_INTERMISSION : d->intermission = true; break;

                        case CS_LEVEL_START_TIME :
                            SET_TIME( CS_LEVEL_START_TIME, d->msgSnapTime - d->frameTime );
                        break;

                        case CS_VOTE_TIME :
                            if ( *( s + 6 ) != '"' ) {
                                SET_TIME( CS_VOTE_TIME, d->msgSnapTime );
                            }
                        break;

                        case CS_TEAMVOTE_TIME :
                            if ( *( s + 7 ) != '"' ) {
                                SET_TIME( CS_TEAMVOTE_TIME, d->msgSnapTime );
                            }
                        break;

                        default : break;
                    }
                }
            }

            if ( !( d->removedPause &&
                ( csIndex == CS_PAUSE_START || csIndex == CS_WARMUP_END ) ) )
            {
                if ( !oldCommand ) {
                    writeByte( outMsg, svc_serverCommand );
                    writeLong( outMsg, d->serverCmdSeq );
                    writeString( outMsg, s );
                    ++d->serverCmdSeq;
                }
            }
        }
    }
    else if ( d->currentParseType == DtDemo::FindFrags   ||
              d->currentParseType == DtDemo::ReadEditInfo )
    {
        if ( d->currentParseType == DtDemo::ReadEditInfo && !chatCmd && !oldCommand ) {
            int index = -1;
            char* str = s;
            bool bcs = ( s == strstr( s, "bcs" ) );
            int bcsNum = 0;

            if ( configString || bcs ) {
                if ( configString ) {
                    index = csIndex;
                    str += 4;
                }
                else {
                    bcsNum = atoi( s + 3 );
                    index = atoi( s + 5 );
                    str += 6;
                }

                while ( *str != ' ' ) {
                    ++str;
                }

                ++str;
            }

            d->lastCommands.append( DtDemoCommand( 0, index, str, bcs, bcsNum ) );
        }

        if ( configString ) {
            switch ( csIndex ) {
                case CS_WARMUP :
                    if ( d->q3Mod != MOD_CPMA ) {
                        d->warmupTimeCmd = ( atoi( s + 6 ) != -1 );
                    }
                break;

                case CS_INTERMISSION : d->intermission = true; break;
            }
        }

        if ( d->warmupTimeCmd && !strncmp( s, "map_restart\n", 12 ) ) {
            d->mapRestartFrame = true;

            if ( d->mapRestartTime == -1 ) {
                d->mapRestartTime = d->lastSnapTime;
            }
        }
        else if ( d->currentParseType == DtDemo::ReadEditInfo && csIndex == CS_PAUSE_COUNTDOWN ) {
            if ( !d->pauseCalled && d->lastPauseStartTime != -1 ) { /* paused */
                d->pauseCalled = true;
                d->lastPauseSegmentStartTime = d->lastPauseStartTime;
                d->pauseHaveTimer = ( d->lastPauseCountdownTime != 0 );
            }
            else if ( d->pauseCalled && d->lastPauseStartTime == -1 ) { /* unpaused manually */
                d->pauseCalled = false;
                d->lastPauseSegmentEndTime = d->lastPauseCountdownTime;
                d->addPause();
            }
        }
        else if ( d->q3Mod == MOD_CPMA ) {
            if ( configString && csIndex == CPMA_GAME_INFO ) {
                if ( d->mapRestartTime == -1 && d->demoWarmupEndTime == -1 ) {
                    d->mapRestartTime = getCpmaInfoVar( "tw", s );
                }

                if ( d->currentParseType == DtDemo::ReadEditInfo ) {
                    int te = getCpmaInfoVar( "te", s );

                    if ( te ) {
                        if ( d->lastPauseSegmentStartTime == -1 ) {
                            d->lastPauseSegmentStartTime = getCpmaInfoVar( "ts", s ) +
                                                           getCpmaInfoVar( "td", s ) + te;
                        }
                    }
                    else if ( d->lastPauseSegmentStartTime > 0 ) {
                        d->lastPauseSegmentEndTime = d->lastSnapTime;
                        d->addPause();
                        d->lastPauseSegmentStartTime = -1;
                    }
                }
            }
            else if ( s == strstr( s, "print \"Timelimit hit\n\"" ) ) {
                d->intermission = true;
            }
        }

        if ( d->currentParseType == DtDemo::ReadEditInfo && chatCmd && !oldCommand ) {
            d->lastChatStrings.append( s );
        }

        if ( d->currentParseType == DtDemo::FindFrags && configString ) {
             if ( d->q3Mod == MOD_CPMA && csIndex == CPMA_ROUND_INFO ) {
                 if ( atoi( s + 8 ) != -1 ) {
                     d->obituaryEvents.insert( d->getLength(), -1 );
                 }
             }
             else if ( csIndex >= CS_PLAYERS && csIndex < CS_PLAYERS + MAX_CLIENTS ) {
                 char pName[ DtDemo::MAX_CONFKEY ];
                 pName[ 0 ] = '\0';
                 char* str = s + 9;
                 readPlayerName( str, pName );
                 int playerNum = csIndex - CS_PLAYERS;
                 int team = atoi( str + 3 );

                 if ( pName[ 0 ] ) {
                     d->playerTeams.insert( playerNum, team );
                 }
                 else {
                     d->playerTeams.remove( playerNum );
                 }
             }
        }
    }
}

void DtDm68::obutuaryEventCheck( int eType, int value, DtDemo::DtObituaryEvent& event ) {
    switch ( eType ) {
        case ES_EVENTTYPE_Q3 :
            if ( ( value & ~EV_EVENT_BITS ) == ( ET_EVENTS + EV_OBITUARY ) ) {
                event.save = true;
            }
            break;
        case ES_EVENTPARM_Q3 :         event.mod = value;       break;
        case ES_OTHERENTITYNUM_Q3 :    event.victim = value;    break;
        case ES_OTHERENTITYNUM2_Q3 :   event.inflictor = value; break;
    }
}

void DtDm68::parsePacketEnities( msg_t* msg, clSnapshot_t* oldframe, clSnapshot_t* newframe ) {
    parsePacketEnitiesImpl< DtDm68, entityState_t >( msg, oldframe, newframe );
}

void DtDm68::emitPacketEntities( clSnapshot_t* from, clSnapshot_t* to, msg_t* msg ) {
    emitPacketEntitiesImpl< DtDm68, entityState_t >( from, to, msg );
}

void DtDm68::parseBaseline( msg_t* msg, msg_t* outMsg ) {
    parseBaselineImpl< DtDm68, entityState_t >( msg, outMsg );
}

void DtDm68::setGameTime( int sTime ) {
    if ( !d->gameTimeSet ) {
        if ( d->q3Mod == MOD_CPMA ) {
            d->timerInitialValue = ( sTime < d->demoWarmupEndTime )
                                      ? sTime - d->demoTimerValue
                                      : sTime - d->demoTimerValue - d->demoWarmupEndTime;
        }
        else {
            d->timerInitialValue = ( sTime < d->demoWarmupEndTime ) ? sTime :
                                      sTime - d->demoWarmupEndTime;
        }

        d->firstSnapTime = sTime;
        d->lastSnapTime = sTime;
        d->gameTimeSet = true;
    }
    else {
        d->lastSnapTime = sTime;
    }
}

void DtDm68::updateGamestateInfo( int time ) {
    int commandsCount = d->commands.count();

    for ( int i = 0; i < commandsCount; ++i ) {
        if ( d->commands.at( i ).time > time ) {
            break;
        }

        int index = d->commands.at( i ).csIndex;

        if ( index >= CS_PLAYERS && index < CS_PLAYERS + MAX_CLIENTS ) {
            d->playersInfo.insert( index, d->commands.at( i ).cmd );
        }
        else {
            if ( d->q3Mod == MOD_CPMA ) {
                if ( index == CPMA_ROUND_INFO ) {
                    cpmaRoundInfo = d->commands.at( i ).cmd;
                }
            }
            else if ( index == CS_FLAGSTATUS ) {
                d->flagStatus = d->commands.at( i ).cmd;
            }
            else if ( index == CS_SCORES1 || index == CS_SCORES2 ) {
                updateScores( i, index );
            }
        }
    }
}

void DtDm68::afterParse() {
    if ( d->currentParseType == DtDemo::ReadEditInfo || d->currentParseType == DtDemo::FindFrags ) {
        if ( d->q3Mod == MOD_CPMA ) {
            if ( d->mapRestartTime == -1 && d->demoWarmupEndTime > 0 ) {
                d->mapRestartTime = d->demoWarmupEndTime - 1;
            }
            else if ( d->mapRestartTime > 0 ) {
                d->mapRestartTime -= 1;
            }

            if ( d->currentParseType == DtDemo::ReadEditInfo && !d->pauses.isEmpty() ) {
                int pausesCount = d->pauses.count();

                for ( int i = 0; i < pausesCount; ++i ) {
                    d->pauses[ i ].end += d->frameTime * 2;
                }
            }
        }

        d->gameInProgress = true;

        if ( d->mapRestartTime > d->firstSnapTime || d->countdownStopped ) {
            d->gameInProgress = false;
        }
    }
}

QHash< QString, int > DtDm68::entityFieldNums() {
    netField_t* field = entityStateFields;
    QHash< QString, int > fieldNums;

    for ( int i = 0; i < entityStateFieldsNum; ++i, ++field ) {
        fieldNums.insert( field->name, i );
    }

    return fieldNums;
}

QHash< QString, int > DtDm68::playerFieldNums() {
    netField_t* field = playerStateFields;
    QHash< QString, int > fieldNums;

    for ( int i = 0; i < playerStateFieldsNum; ++i, ++field ) {
        fieldNums.insert( field->name, i );
    }

    return fieldNums;
}

netField_t* DtDm68::entityFields() {
    return entityStateFields;
}

int DtDm68::entityFieldsCount() {
    return entityStateFieldsNum;
}
