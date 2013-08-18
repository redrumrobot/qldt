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

#include "Dm73.h"
#include "Demo.h"

#define NETF(x) #x, (int)(long) &((entityState_dm73_t*)0)->x

netField_t DtDm73::entityStateFields[] =
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
{ NETF(pos.gravity), 32 },
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
{ NETF(apos.gravity), 32 },
{ NETF(time2), 32 },
{ NETF(angles[2]), 0 },
{ NETF(angles2[0]), 0 },
{ NETF(angles2[2]), 0 },
{ NETF(constantLight), 32 },
{ NETF(frame), 16 }
};

int DtDm73::entityStateFieldsNum = sizeof( entityStateFields ) / sizeof( entityStateFields[ 0 ] );

DtDm73::DtDm73( DtDemo* parent ) : DtAbstractProtocol( parent ) {
    parseEntities = new entityState_dm73_t[ MAX_PARSE_ENTITIES ];
    entityBaselines = new entityState_dm73_t[ MAX_PARSE_ENTITIES ];
    memset( entityBaselines, 0, sizeof( entityBaselines ) * MAX_GENTITIES );
}

DtDm73::~DtDm73() {
    delete [] parseEntities;
    delete [] entityBaselines;
}

void DtDm73::parseConfigVars( char* str, int len, int csIndex ) {
    switch ( csIndex ) {
        case CS_WARMUP :
            d->demoWarmupEndTime = atoi( str + 6 );
            return;

        case CS_WARMUP_END :
            if ( d->gameInProgress ||
                 ( !d->gameInProgress && d->demoWarmupEndTime < atoi( str ) ) )
            {
                d->demoWarmupEndTime = atoi( str );
                return;
            }
            break;

        case CS_AD_WAIT :
            d->advertDelay = atoi( str );
            return;

        default : break;
    }

    if ( str[ 0 ] == confDelimeter ) {
        if ( csIndex == CS_CA_ROUND_INFO ) {
            return;
        }

        char key[ DtDemo::MAX_CONFKEY ];
        char val[ MAX_GAMESTATE_CHARS ];

        while ( *str ) {
            getStrN( key, str, DtDemo::MAX_CONFKEY );

            if ( !*str ) {
                d->error( DtDemo::WARN, "Empty config key" );
                return;
            }

            getStrN( val, str, MAX_GAMESTATE_CHARS );

            if ( !strcmp( key, "g_gameState" ) ) {
                d->gameInProgress = !strcmp( val, "IN_PROGRESS" );
            }
            else if ( !strcmp( key, "g_levelStartTime" ) ) {
                d->levelStartTime = atoi( val );
            }
            else if ( !strcmp( key, "mapname" ) ) {
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

            d->info.append( QPair< QString, QString >( key, val ) );
        }
    }
    else if ( csIndex >= CS_PLAYERS_QZ && csIndex < CS_PLAYERS_QZ + MAX_CLIENTS ) { /* players */
        char pName[ DtDemo::MAX_CONFKEY ];
        pName[ 0 ] = '\0';
        char* pNamePtr = pName;
        char* strEnd = str + len - 4;
        int playerNum = csIndex - CS_PLAYERS_QZ;
        bool haveClantag = false;

        ++str;

        while ( strEnd > str ) {
            if ( strEnd[ 0 ] == 's'            &&
                 strEnd[ 1 ] == 'u'            &&
                 strEnd[ 2 ] == confDelimeter  &&
                 strEnd[ 3 ] == '1' )
            {
                subscribedPlayers.insert( playerNum );

                if ( !dtdata::config.showClanTags ) {
                    break;
                }
            }

            if ( dtdata::config.showClanTags ) {
                if ( strEnd[ 0 ] == confDelimeter  &&
                     strEnd[ 1 ] == 'c'            &&
                     strEnd[ 2 ] == 'n'            &&
                     strEnd[ 3 ] == confDelimeter )
                {
                    char* playerStr = strEnd + 3;
                    readPlayerName( playerStr, pName );
                    int nameLength = strnlen( pName, 100 );

                    if ( nameLength ) {
                        pNamePtr += nameLength;
                        pNamePtr[ 0 ] = ' ';
                        ++pNamePtr;
                        haveClantag = true;
                    }

                    break;
                }
            }

            --strEnd;
        }

        readPlayerName( str, pNamePtr );

        int team = atoi( str + 3 );

        if ( haveClantag ) {
            d->playerNames.insert( playerNum, pNamePtr );
        }
        else {
            d->playerNames.insert( playerNum, pName );
        }

        d->playerTeams.insert( playerNum, team );

        QString playerInfoNum = QString( "playerName%1" ).arg( playerNum );

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

void DtDm73::parseConfigString( msg_t* msg, int& dataCount, msg_t* outMsg ) {
    char* str;
    int len;
    int csIndex = readShort( msg );
    bool playerInfoString = ( csIndex >= CS_PLAYERS_QZ &&
                              csIndex < CS_PLAYERS_QZ + MAX_CLIENTS );

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
            if ( d->writeCfg->setGamestateString || d->writeCfg->converter == DC_BETAMAPFIX ) {
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
            switch ( csIndex ) {
                case CS_SERVERINFO :
                    if ( !d->gameInProgress && d->writeCfg->removeWarmup ) {
                        QString cs = str;
                        cs.replace( "g_gameState\\PRE_GAME",
                                    "g_gameState\\IN_PROGRESS", Qt::CaseInsensitive );
                        cs.replace( "g_gameState\\COUNT_DOWN",
                                    "g_gameState\\IN_PROGRESS", Qt::CaseInsensitive );

                        cfgString = cs.toAscii();
                        writeStr = cfgString.constData();
                        d->warmupRemoved = true;
                    }
                break;

                case CS_WARMUP : {
                    bool removeCountdown = true;

                    if ( !d->gameInProgress || d->warmupRemoved ) {
                        if ( atoi( str + 6 ) == -1 && !d->warmupRemoved ) {
                            break;
                        }

                        removeCountdown = d->writeCfg->removeWarmup;
                    }

                    sprintf( csTime, "\\time\\%d", ( removeCountdown ) ? 0 : countdownTime );
                    writeStr = csTime;
                }
                break;

                case CS_WARMUP_END : {
                    int warmupEndTime = DtDemo::MSG_START_TIME; /* last serverTime when warmup
                                                                   ended and game started
                                                                */
                    if ( d->writeCfg->singleFile ) {
                        warmupEndTime -= d->writeCfg->timerInitialValue;
                    }
                    else if ( d->lastPartIndex > -1 ) {
                        warmupEndTime -= d->writeCfg->cutSegments.at( d->lastPartIndex ).start -
                                         d->getMapRestartTime();
                    }

                    if ( !d->gameInProgress || d->warmupRemoved ) {
                        if ( atoi( str ) == 0 ) {
                            break;
                        }
                    }

                    d->demoTimerValue = warmupEndTime;

                    SET_INT( warmupEndTime );
                }
                break;

                case CS_RED_CLAN_PLAYERS :  SET_INT( d->redClanPlayers ); break;
                case CS_BLUE_CLAN_PLAYERS : SET_INT( d->blueClanPlayers ); break;
                case CS_SCORES1 :           SET_INT( d->scores1 ); break;
                case CS_SCORES2 :           SET_INT( d->scores2 ); break;

                case CS_CA_ROUND_INFO : {
                    sprintf( csTime, "\\time\\0\\round\\%d", d->caRoundNum );
                    writeStr = csTime;
                }
                break;

                case CS_FLAG_STATUS_QZ : {
                    removeQuotes( d->flagStatus );
                    SET_STR( d->flagStatus );
                }
                break;

                case CS_FIRST_PLACE : {
                     removeQuotes( d->firstPlace );
                     SET_STR( d->firstPlace );
                }
                break;

                case CS_SECOND_PLACE : {
                     removeQuotes( d->secondPlace );
                     SET_STR( d->secondPlace );
                }
                break;

                case CS_AD_WAIT : {
                     SET_INT( 0 );
                }
                break;

                default : break;
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
        parseConfigVars( str, len, csIndex );
    }
}

void DtDm73::parseCommandString( msg_t* msg, msg_t* outMsg ) {
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
             s == strstr( s, "tchat" ) )
    {
        chatCmd = true;
    }

    if ( csIndex == CS_PAUSE_COUNTDOWN ) {
        d->lastPauseCountdownTime = atoi( s + 8 );
        d->lastPauseSegmentEndTime = d->lastPauseCountdownTime;
    }
    else if ( csIndex == CS_PAUSE_START ) {
        d->lastPauseStartTime = atoi( s + 8 );
    }

    if ( d->currentParseType == DtDemo::CustomWrite ) {
        if ( ( d->writeCfg->editChat && !chatCmd ) || !d->writeCfg->editChat ) {
            char csTime[ 40 ];

            if ( configString ) {
                #define SET_TIME( var, time )                       \
                    sprintf( csTime, "cs %d \"%d\"\n", var, time ); \
                    s = csTime

                switch ( csIndex ) {
                    case CS_WARMUP_END : {
                        int time = d->msgSnapTime;

                        if ( d->demoPauseStartTime != -1 ) {
                            time = d->demoTimerValue + d->lastSnapTime +
                                   d->frameTime - d->demoPauseStartTime;
                            d->demoPauseStartTime = -1;
                        }

                        SET_TIME( CS_WARMUP_END, time );
                    }
                    break;

                    case CS_PAUSE_START :
                        if ( *( s + 8 ) != '"' ) {
                            SET_TIME( CS_PAUSE_START, d->msgSnapTime - d->frameTime );

                            if ( d->demoPauseStartTime == -1 ) {
                                d->demoPauseStartTime = d->lastSnapTime;
                            }
                        }
                    break;

                    case CS_PAUSE_COUNTDOWN :
                        SET_TIME( CS_PAUSE_COUNTDOWN, ( d->lastPauseCountdownTime ) ?
                                  d->lastPauseCountdownTime - d->lastSnapTime +
                                  d->msgSnapTime : 0 );
                    break;

                    case CS_CA_ROUND_INFO : {
                        char* str = s + 8;

                        if ( *str == confDelimeter ) {
                            str += 6;
                            int roundStartTime = atoi( str ) - d->lastSnapTime + d->msgSnapTime;

                            while ( *str && *str != confDelimeter ) {
                                ++str;
                            }

                            int roundNum = atoi( str + 7 );

                            sprintf( csTime, "cs %d \"\\time\\%d\\round\\%d\"\n",
                                     CS_CA_ROUND_INFO, roundStartTime, roundNum );
                            s = csTime;
                        }
                    }
                    break;

                    case CS_CA_ROUND_START :
                        if ( atoi( s + 8 ) != -1 ) {
                            SET_TIME( CS_CA_ROUND_START, d->msgSnapTime );
                        }
                    break;

                    case CS_VOTE_TIME :
                        if ( *( s + 6 ) != '"' ) {
                            SET_TIME( CS_VOTE_TIME, d->msgSnapTime );
                        }
                    break;

                    default : break;
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
                case CS_WARMUP : d->warmupTimeCmd = true; break;
                case CS_INTERMISSION_QZ : d->intermission = true; break;
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

        if ( d->currentParseType == DtDemo::ReadEditInfo && chatCmd && !oldCommand ) {
            d->lastChatStrings.append( s );
        }

        if ( d->currentParseType == DtDemo::FindFrags && configString ) {
             if ( csIndex == CS_CA_ROUND_START ) {
                 if ( atoi( s + 8 ) != -1 ) {
                     d->obituaryEvents.insert( d->getLength(), -1 );
                 }
             }
             else if ( csIndex >= CS_PLAYERS_QZ && csIndex < CS_PLAYERS_QZ + MAX_CLIENTS ) {
                 char pName[ DtDemo::MAX_CONFKEY ];
                 pName[ 0 ] = '\0';
                 char* str = s + 9;
                 readPlayerName( str, pName );
                 int playerNum = csIndex - CS_PLAYERS_QZ;
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

void DtDm73::obutuaryEventCheck( int eType, int value, DtDemo::DtObituaryEvent& event ) {
    switch ( eType ) {
        case ES_EVENTTYPE :
            if ( ( value & ~EV_EVENT_BITS ) == ( ET_EVENTS + EV_OBITUARY_QL ) ) {
                event.save = true;
            }
            break;
        case ES_EVENTPARM :         event.mod = value;       break;
        case ES_OTHERENTITYNUM :    event.victim = value;    break;
        case ES_OTHERENTITYNUM2 :   event.inflictor = value; break;
    }
}

void DtDm73::parsePacketEnities( msg_t* msg, clSnapshot_t* oldframe, clSnapshot_t* newframe ) {
    parsePacketEnitiesImpl< DtDm73, entityState_dm73_t >( msg, oldframe, newframe );
}

void DtDm73::emitPacketEntities( clSnapshot_t* from, clSnapshot_t* to, msg_t* msg ) {
    emitPacketEntitiesImpl< DtDm73, entityState_dm73_t >( from, to, msg );
}

void DtDm73::parseBaseline( msg_t* msg, msg_t* outMsg ) {
    parseBaselineImpl< DtDm73, entityState_dm73_t >( msg, outMsg );
}

void DtDm73::setGameTime( int sTime ) {
    if ( !d->gameTimeSet ) {
        d->timerInitialValue = ( sTime < d->demoWarmupEndTime ) ? 0 :
                                  sTime - d->demoWarmupEndTime;

        if ( !d->gameInProgress && d->demoWarmupEndTime == 0 ) { // timer stopped
            d->timerInitialValue = 0;
        }

        d->firstSnapTime = sTime;
        d->lastSnapTime = sTime;
        d->gameTimeSet = true;
    }
    else {
        d->lastSnapTime = sTime;
    }
}

void DtDm73::updateGamestateInfo( int time ) {
    int commandsCount = d->commands.count();

    for ( int i = 0; i < commandsCount; ++i ) {
        if ( d->commands.at( i ).time > time ) {
            break;
        }

        int index = d->commands.at( i ).csIndex;

        if ( index >= CS_PLAYERS_QZ && index < CS_PLAYERS_QZ + MAX_CLIENTS ) {
            d->playersInfo.insert( index, d->commands.at( i ).cmd );
        }
        else switch ( index ) {
            case CS_RED_CLAN_PLAYERS : {
                QByteArray cmd = d->commands.at( i ).cmd.toAscii();
                const char* cmdData = cmd.data();

                if ( cmd.startsWith( '"' ) ) {
                    ++cmdData;
                }

                d->redClanPlayers = atoi( cmdData );
            }
            break;

            case CS_BLUE_CLAN_PLAYERS : {
                QByteArray cmd = d->commands.at( i ).cmd.toAscii();
                const char* cmdData = cmd.data();

                if ( cmd.startsWith( '"' ) ) {
                    ++cmdData;
                }

                d->blueClanPlayers = atoi( cmdData );
            }
            break;

            case CS_FLAG_STATUS_QZ : d->flagStatus = d->commands.at( i ).cmd; break;
            case CS_FIRST_PLACE : d->firstPlace = d->commands.at( i ).cmd; break;
            case CS_SECOND_PLACE : d->secondPlace = d->commands.at( i ).cmd; break;
            case CS_SCORES1 :
            case CS_SCORES2 : updateScores( i, index ); break;
            case CS_CA_ROUND_INFO : {
                QByteArray cmd = d->commands.at( i ).cmd.toAscii();
                const char* cmdData = cmd.data() + 6;

                if ( *cmdData == confDelimeter ) {
                    ++cmdData;
                }

                while ( *cmdData && *cmdData != confDelimeter ) {
                    ++cmdData;
                }

                d->caRoundNum = atoi( cmdData + 7 );
            }
            break;
            default : break;
        }

    }
}

void DtDm73::afterParse() {
    if ( d->currentParseType == DtDemo::ReadEditInfo || d->currentParseType == DtDemo::FindFrags ) {
        if ( d->gameInProgress && d->mapRestartTime != -1 ) {
            d->mapRestartTime = -1;
        }
    }
    else if ( d->currentParseType == DtDemo::ParseGamestate ) {
        if ( subscribedPlayers.contains( d->clientNum ) ) {
            d->recordedBySubscriber = true;
        }
    }
}

QHash< QString, int > DtDm73::entityFieldNums() {
    netField_t* field = entityStateFields;
    QHash< QString, int > fieldNums;

    for ( int i = 0; i < entityStateFieldsNum; ++i, ++field ) {
        fieldNums.insert( field->name, i );
    }

    return fieldNums;
}

QHash< QString, int > DtDm73::playerFieldNums() {
    netField_t* field = playerStateFields;
    QHash< QString, int > fieldNums;

    for ( int i = 0; i < playerStateFieldsNum; ++i, ++field ) {
        fieldNums.insert( field->name, i );
    }

    return fieldNums;
}

netField_t* DtDm73::entityFields() {
    return entityStateFields;
}

int DtDm73::entityFieldsCount() {
    return entityStateFieldsNum;
}
