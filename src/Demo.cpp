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

#include "Demo.h"
#include "Data.h"
#include "XmlReader.h"
#include "XmlWriter.h"
#include "AbstractProtocol.h"
#include "Dm68.h"
#include "Dm73.h"
#include "Dm73MapFix.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QTime>
#include <QFileInfo>
#include <QDir>
#include <QXmlStreamWriter>

DtDemo::DtDemo( const QFileInfo& fInf ) :
    referenceCount( 0 ),
    state( NONE ),
    writeCfg( 0 ),
    proto( 0 ),
    broken( false ),
    demoFile( 0 ),
    outFile( 0 ),
    xmlReader( 0 ),
    xmlWriter( 0 ),
    demoProto( Q_UNKNOWN ),
    q3Mod( MOD_BASEQ3 ),
    infoPlayerCount( 0 ),
    parsing( false ),
    gameTimeSet( false ),
    firstSnapTime( 0 ),
    lastSnapTime( 0 ),
    serverMessageSequence( 0 ),
    serverCommandSequence( 0 ),
    parseEntitiesNum( 0 ),
    snapCount( 0 ),
    levelStartTime( 0 ),
    gameType( 0 ),
    instaGib( false ),
    countdownStopped( false ),
    backupSnaps( 0 ),
    gamestateMsg( 0 ),
    eventTimes( 0 ),
    mapRestartTime( -1 ),
    msgCutted( false ),
    frameTime( 0 ),
    gamestateMsgSaved( false ),
    advertDelay( 0 ),
    recordedBySubscriber( false )
{
    setFileInfo( fInf );
}

void DtDemo::setFileInfo( const QFileInfo& fInf ) {
    fInfo.filePath = fInf.absolutePath();
    fInfo.baseName = fInf.completeBaseName();
    fInfo.fileExt = fInf.suffix();
    fInfo.size = fInf.size();
    fInfo.lastModified = fInf.lastModified().toTime_t();
    fInfo.fName = QString( "%1.%2" ).arg( fInfo.baseName, fInfo.fileExt );
    fInfo.fullFilePath = QString( "%1/%2" ).arg( fInfo.filePath, fInfo.fName );
}

void DtDemo::setDbData( const DtDemoDbData& demoData ) {
    clientName = demoData.player;
    gameType = demoData.type;
    q3Mod = static_cast< quake3Mods >( demoData.mod );
    mapName = demoData.map;
    levelStartTime = demoData.date;
    hostName = demoData.server;
    demoProto = static_cast< DtDemoProto >( demoData.protocol );
    broken = demoData.broken;
    state |= INFO_FETCHED;
}

void DtDemo::error( errorLevel level, const char* fmt, ... ) {
    char str[ 1024 ];
    va_list ap;

    if ( !fmt ) {
        return;
    }

    va_start( ap, fmt );
    vsprintf( str, fmt, ap );
    va_end( ap );

    printf( "Error while parsing %s:\n%s\n", fInfo.fileName().toUtf8().data(), str );

    if ( level == FATAL ) {
        throw demoParseError();
    }
}

void DtDemo::readGamestateString( msg_t* msg, int& dataCount ) {
    int len;
    int csIndex = proto->readShort( msg );

    if ( csIndex < 0 || csIndex >= MAX_CONFIGSTRINGS ) {
        error( FATAL, "Config string index > MAX_CONFIGSTRINGS\n" );
    }

    len = strlen( proto->readBigString( msg ) );

    if ( len + 1 + dataCount > MAX_GAMESTATE_CHARS ) {
        error( FATAL, "MAX_GAMESTATE_CHARS exceeded\n" );
    }

    dataCount += len + 1;
}

int DtDemo::writeGamestateStrings( msg_t* msg, int& dataCount, msg_t* outMsg ) {
    int commandCount = writeCfg->commands.count();

    for ( int i = 0; i < commandCount; ++i ) {
        if ( writeCfg->commands.at( i ).time == -1 ) {
            proto->writeByte( outMsg, svc_configstring );
            proto->writeShort( outMsg, writeCfg->commands.at( i ).csIndex );
            proto->writeBigString( outMsg, writeCfg->commands.at( i ).cmd.toAscii().data() );
        }
        else {
            lastWrittenCommand = i;
            break;
        }
    }

    readGamestateString( msg, dataCount );

    forever {
        int command = proto->readByte( msg );

        if ( command == svc_configstring ) {
            readGamestateString( msg, dataCount );
        }
        else {
            return command;
        }
    }
}

bool DtDemo::parseGamestate( msg_t* msg, msg_t* outMsg ) {
    serverCommandSequence = proto->readLong( msg );
    serverCmdSeq = 0;

    if ( currentParseType == CustomWrite ) {
        if ( writeCfg->exportXml ) {
            xmlWriter->writeGamestateHeader();
        }
        else {
            if ( writeCfg->setGamestateString || writeCfg->converter == DC_BETAMAPFIX ) {
                serverCmdSeq = serverCommandSequence;
                copyMessages = true;
            }

            proto->writeByte( outMsg, svc_gamestate );
            proto->writeLong( outMsg, serverCmdSeq++ );
        }
    }

    int dataCount = 1;

    forever {
        int command = proto->readByte( msg );

        if ( currentParseType == CustomWrite ) {
            if ( !writeCfg->exportXml ) {
                if ( command != svc_configstring ) {
                    proto->writeByte( outMsg, command );
                }
                else if ( writeCfg->writeCommands ) {
                    command = writeGamestateStrings( msg, dataCount, outMsg );
                    proto->writeByte( outMsg, command );
                }
            }
        }

        if ( command == svc_EOF ) {
            break;
        }

        switch ( command ) {
            case svc_configstring :
                proto->parseConfigString( msg, dataCount, outMsg );
                break;
            case svc_baseline :
                proto->parseBaseline( msg, outMsg );
                break;
            default :
                error( FATAL, "Bad gamestate command byte %d", command );
            break;
        }
    }

    clientNum = proto->readLong( msg );
    int checksumFeed = proto->readLong( msg );

    if ( currentParseType == CustomWrite ) {
        if ( writeCfg->exportXml ) {
            xmlWriter->writeGamestateFooter( clientNum, checksumFeed );
        }
        else {
            proto->writeLong( outMsg, clientNum );
            proto->writeLong( outMsg, checksumFeed );
        }
    }

    return ( currentParseType != ParseGamestate );
}

int DtDemo::getLength() const {
    return lastSnapTime - firstSnapTime;
}

void DtDemo::addPause() {
    pauses.append( cutSegment( lastPauseSegmentStartTime - firstSnapTime,
                               lastPauseSegmentEndTime - firstSnapTime ) );
}

void DtDemo::setPowerups( playerState_t* fromPs, playerState_t* toPs, int serverTime ) {
    playerState_t nullState;

    if ( !fromPs ) {
        fromPs = &nullState;
        memset( &nullState, 0, sizeof( nullState ) );
    }

    #define PW( name )                                                          \
        if ( toPs->powerups[ PW_##name ] != fromPs->powerups[ PW_##name ] &&    \
             toPs->powerups[ PW_##name ] != 0 )                                 \
        {                                                                       \
            toPs->powerups[ PW_##name ] = msgSnapTime                           \
                                          + toPs->powerups[ PW_##name ]         \
                                          - serverTime;                         \
        }

    PW( QUAD )
    PW( BATTLESUIT )
    PW( HASTE )
    PW( INVIS )
    PW( REGEN )
    PW( FLIGHT )
}

void DtDemo::parseSnapshot( msg_t* msg, msg_t* outMsg ) {
    int serverTime;
    int deltaNum;
    int areamaskLen;
    clSnapshot_t* old;
    clSnapshot_t newSnap;

    memset( &newSnap, 0, sizeof( newSnap ) );

    unsigned char areamaskData[ MAX_MAP_AREA_BYTES ];

    newSnap.serverCommandNum = serverCommandSequence;
    serverTime = proto->readLong( msg );
    int prevSnapTime = lastSnapTime;

    proto->setGameTime( serverTime );

    newSnap.serverTime = serverTime;
    newSnap.messageNum = serverMessageSequence;
    deltaNum = proto->readByte( msg );

    if ( !deltaNum ) {
        newSnap.deltaNum = -1;
        old = 0;
    } else {
        newSnap.deltaNum = newSnap.messageNum - deltaNum;
        old = &backupSnaps[ newSnap.deltaNum & PACKET_MASK ];
    }

    newSnap.snapFlags = proto->readByte( msg );
    areamaskLen = proto->readByte( msg );

    if ( static_cast< unsigned int >( areamaskLen ) > sizeof( areamaskData ) ) {
        error( FATAL, "Incorrect areamask length" );
    }

    proto->readData( msg, &areamaskData, areamaskLen );
    clSnapshot_t* oldSnap = old;

    if ( currentParseType == CustomWrite ) {
#ifdef MSG_LOG
        printf( "new snap time %d tm %d, %d\n", getLength(), msgSnapTime, lastSnapTime );
#endif
        int curTime = getLength();

        if ( !writeCfg->cutSegments.isEmpty() ) {
            msgCustomWrite = false;
            oldSnap = 0;
            lastPartIndex = -1;

            int segmentsCount = writeCfg->cutSegments.count();

            for ( int i = 0; i < segmentsCount; ++i ) {
                const cutSegment& part = writeCfg->cutSegments.at( i );

                ++lastPartIndex;

                if ( curTime >= part.start && curTime <= part.end ) {
                    msgCustomWrite = true;
                    oldSnap = old;
                    break;
                }
                else if ( lastPartIndex == writeCfg->cutSegments.size() - 1 &&
                         curTime > part.end )
                {
                    stopScan = true;
                }
            }

            if ( !msgCustomWrite ) {
                writeNewPart = true;

                if ( !writeCfg->singleFile ) {
                    msgSnapTime = MSG_START_TIME;
                }
            }
        }

        if ( msgCustomWrite && writeCfg->removeWarmup && lastSnapTime <= mapRestartTime ) {
            msgCustomWrite = false;
            oldSnap = 0;
        }

        bool noDelta = ( messagesInPart - deltaNum < 0 );

        if ( msgCustomWrite && writeCfg->removePauses ) {
            int pausesCount = pauses.count();

            for ( int i = 0; i < pausesCount; ++i ) {
                if ( curTime > pauses.at( i ).start &&
                     curTime < pauses.at( i ).end )
                {
                    oldSnap = 0;
                    msgCustomWrite = false;
                    removedPause = true;
                    break;
                }

                if ( !noDelta ) {
                    removedPause = false;
                }
            }
        }

#ifdef MSG_LOG
        printf( "%d: playerstate\n", msg->readcount );
#endif
        if ( !writeCfg->exportXml ) {
            proto->readDeltaPlayerstate( msg, old ? &old->ps : 0, &newSnap.ps );
        }

        if ( msgCustomWrite ) {
            if ( writeCfg->writeCommands ) {
                msgSnapTime = serverTime;
                msgEntSnapTime = serverTime;
            }
            else if ( prevSnapTime && serverTime != prevSnapTime ) {
                msgEntSnapTime = msgSnapTime;

                int lastFrameTime = serverTime - prevSnapTime;

                if ( !writeCfg->removeLags && lastSnapTime != mapRestartTime ) {
                    if ( lastFrameTime != frameTime ) {
                        msgSnapTime += lastFrameTime;
                    }
                    else {
                        msgSnapTime += frameTime;
                    }
                }
                else {
                    if ( serverTime - newSnap.ps.commandTime > 490 ) {
                        newSnap.ps.commandTime = serverTime - 100;
                    }

                    msgSnapTime += frameTime;
                }

                if ( lastSnapTime == mapRestartTime && !( demoProto == Q3_68 && q3Mod == MOD_CPMA ) ) {
                    msgSnapTime += MAP_RESTART_DURATION;
                }
            }
        }

        if ( !writeCfg->writeCommands && (
             !old || ( old && newSnap.ps.commandTime != old->ps.commandTime ) ) )
        {
            newSnap.ps.commandTime = msgSnapTime - ( serverTime - newSnap.ps.commandTime );
        }

        if ( msgCustomWrite ) {
            if ( !writeCfg->exportXml ) {
                int cDemoLength = getLength();

                if ( writeCfg->writeCommands ) {
                    int commandsCount = writeCfg->commands.count();

                    for ( int i = lastWrittenCommand; i < commandsCount; ++i ) {
                        const DtDemoCommand& command = writeCfg->commands.at( i );

                        if ( command.time == cDemoLength ) {
                            proto->writeByte( outMsg, svc_serverCommand );
                            proto->writeLong( outMsg, serverCmdSeq );
                            proto->writeString( outMsg, command.cmd.toAscii().data() );
                            ++serverCmdSeq;
                            lastWrittenCommand = i + 1;
                        }
                        else if ( command.time > cDemoLength ) {
                            break;
                        }
                    }
                }

                bool writeChat = ( !writeCfg->editChat && writeCfg->writeCommands );

                if ( writeCfg->editChat || writeChat ) {
                    int chatStringsCount = writeChat ? chatStrings.count() :
                                           writeCfg->chatStrings.count();

                    for ( int i = lastWrittenChatString; i < chatStringsCount; ++i ) {
                        const QPair< int, QString >& chatString = writeChat ?
                                                                  chatStrings.at( i ) :
                                                                  writeCfg->chatStrings.at( i );
                        if ( chatString.first == cDemoLength ) {
                            proto->writeByte( outMsg, svc_serverCommand );
                            proto->writeLong( outMsg, serverCmdSeq );
                            proto->writeString( outMsg, chatString.second.toAscii().data() );
                            ++serverCmdSeq;
                            lastWrittenChatString = i + 1;
                        }
                        else if ( chatString.first > cDemoLength ) {
                            break;
                        }
                    }
                }

                proto->writeByte( outMsg, svc_snapshot );
                proto->writeLong( outMsg, msgSnapTime );

                if ( noDelta ) {
                    deltaNum = 0;
                    oldSnap = 0;
                }

                proto->writeByte( outMsg, deltaNum );
                proto->writeByte( outMsg, newSnap.snapFlags );
                proto->writeByte( outMsg, areamaskLen );
                proto->writeData( outMsg, &areamaskData, areamaskLen );

                playerState_t* oldState = oldSnap ? &old->ps : 0;

                setPowerups( oldState, &newSnap.ps, serverTime );
                proto->writeDeltaPlayerstate( outMsg, oldState, &newSnap.ps );
                msgSeq += sequenceIncrement;
                ++messagesInPart;
            }
            else {
                xmlWriter->writeSnapshotHeader( msg, serverTime, deltaNum, newSnap.snapFlags,
                                                areamaskLen, areamaskData );
            }
        }
    }
    else { /* find frags or read edit info */
        if ( prevSnapTime && frameTime <= 0 ) {
            int time = lastSnapTime - prevSnapTime;

            if ( serverFrameTimes.contains( time ) ) {
                ++frameTimeCounts[ time ];
            }
            else if ( time != 0 ) {
                serverFrameTimes.insert( time );
                frameTimeCounts.insert( time, 1 );
            }
        }

        if ( currentParseType == ReadEditInfo ) {
            proto->readDeltaPlayerstate( msg, old ? &old->ps : 0, &newSnap.ps );

            int time = getLength();

            if ( !lastChatStrings.isEmpty() ) {
                foreach ( const QString& chatString, lastChatStrings ) {
                    chatStrings.append( QPair< int, QString >( time, chatString ) );
                }
            }

            if ( !lastCommands.isEmpty() ) {
                int lastCommandsCount = lastCommands.count();

                for ( int i = 0; i < lastCommandsCount; ++i ) {
                    commands.append( DtDemoCommand( time, lastCommands.at( i ).csIndex,
                                                    lastCommands.at( i ).cmd,
                                                    lastCommands.at( i ).big,
                                                    lastCommands.at( i ).bcsNum ) );
                }
            }

            int lastFrameTime = lastSnapTime - prevSnapTime;

            if ( lastSnapTime - firstSnapTime != 0  &&
                 !mapRestartFrame                   &&
                 !intermission                      &&
                 ( lastFrameTime > 430 || serverTime - newSnap.ps.commandTime > 490 ) )
            {
                lags.append( lastSnapTime - firstSnapTime );
            }
        }
        else {
            proto->skipPlayerState( msg );
        }

        if ( pauseCalled && pauseHaveTimer &&
             lastPauseSegmentEndTime <= lastSnapTime ) // unpaused by timer
        {
            pauseCalled  = false;
            addPause();
        }

        if ( currentParseType == ReadEditInfo ) {
            ++snapCount;
        }
    }

#ifdef MSG_LOG
    printf( "%d: packet entities\n", msg->readcount );
#endif

    proto->parsePacketEnities( msg, old, &newSnap );

    if ( currentParseType == CustomWrite && msgCustomWrite && !writeCfg->exportXml ) {
        proto->emitPacketEntities( deltaNum ? old : 0, &newSnap, outMsg );
    }

    if ( currentParseType == CustomWrite && writeCfg->exportXml ) {
        xmlWriter->writeSnapshotFooter();
    }

    int oldMessageNum = snap.messageNum + 1;

    if ( newSnap.messageNum - oldMessageNum >= PACKET_BACKUP ) {
        oldMessageNum = newSnap.messageNum - ( PACKET_BACKUP - 1 );
    }

    snap = newSnap;
#ifdef MSG_LOG
    printf( "   snapshot:%i  delta:%i  ping:%i\n", snap.messageNum, snap.deltaNum, snap.ping );
#endif

    backupSnaps[ snap.messageNum & PACKET_MASK ] = snap;
}

bool DtDemo::parseMessage( msg_t* msg, msg_t* outMsg ) {
    int reliableAcknowledge = proto->readLong( msg );

    if ( currentParseType == CustomWrite ) {
        if ( writeCfg->exportXml ) {
            xmlWriter->writeMessageHeader( serverMessageSequence, reliableAcknowledge );
        }
        else {
            proto->writeLong( outMsg, reliableAcknowledge );
        }
    }

    lastPauseStartTime = -1;
    lastPauseCountdownTime = 0;
    warmupTimeCmd = false;
    mapRestartFrame = false;

    forever {
        if ( msg->readcount > msg->cursize ) {
            error( WARN, "Read past end of server message" );
            break;
        }

        int command = proto->readByte( msg );

        switch ( command ) {
            case svc_EOF :
                if ( currentParseType == CustomWrite ) {
                    if ( writeCfg->exportXml ) {
                        xmlWriter->writeMessageFooter();
                    }
                    else {
                        proto->writeByte( outMsg, svc_EOF );
                    }
                }
#ifdef MSG_LOG
                printf( "%d:END OF MESSAGE\n------------------\n", msg->readcount - 1 );
#endif
                return true;

            case svc_nop :
                if ( currentParseType == CustomWrite ) {
                    if ( writeCfg->exportXml ) {
                        xmlWriter->writeNop();
                    }
                    else {
                        proto->writeByte( outMsg, svc_nop );
                    }
                }
#ifdef MSG_LOG
                printf( "svc_nop\n" );
#endif
                break;

            case svc_gamestate :
                if ( !gamestateMsgSaved && currentParseType == CustomWrite && !writeCfg->singleFile ) {
                    gamestateMsg->allowoverflow = msg->allowoverflow;
                    gamestateMsg->overflowed = msg->overflowed;
                    gamestateMsg->maxsize = msg->maxsize;
                    gamestateMsg->cursize = msg->cursize;
                    gamestateMsg->readcount = 0;
                    gamestateMsg->bit = 0;

                    memcpy( gamestateMsg->data, msg->data, MAX_MSGLEN );
                    gamestateMsgSaved = true;
                }
#ifdef MSG_LOG
                printf( "%d svc_gamestate\n", msg->readcount - 1 );
#endif
                if ( !parseGamestate( msg, outMsg ) ) {
                    return false;
                }

                break;

            case svc_serverCommand :
#ifdef MSG_LOG
                printf( "svc_serverCommand\n" );
#endif
                proto->parseCommandString( msg, outMsg );
                break;

            case svc_snapshot :
#ifdef MSG_LOG
                printf( "svc_snapshot\n" );
#endif
                parseSnapshot( msg, outMsg );

                if ( stopScan ) {
                    return false;
                }

                break;

            default :
                error( WARN, "Unknown command %d", command );
                return false;
        }
    }

    return !stopScan;
}

bool DtDemo::open() {
    if ( demoFile->isOpen() ) {
        return true;
    }

    demoProto = dtdata::demoProtos.value( fInfo.fileExt, Q_UNKNOWN );
    realProto = demoProto;

    if ( demoProto == Q_UNKNOWN ) {
        error( WARN, "Unknown extension %s", fInfo.fileExt.toUtf8().data() );
        return false;
    }

    if ( !demoFile->open( QFile::ReadOnly ) ) {
        error( WARN, "Couldn't open file %s", fInfo.fileName().toUtf8().data() );
        return false;
    }

    return true;
}

int DtDemo::readBlock( msg_t& msg, byte* msgData ) {
    proto->initialize( &msg, msgData, MAX_MSGLEN );

    if ( demoFile->read( reinterpret_cast< char* >( &msg.cursize ), 4 ) != 4 ) {
        return -1;
    }

    if ( msg.cursize != -1 ) {
        if ( msg.cursize > msg.maxsize ) {
            error( FATAL, "msg.cursize > msg.maxsize" );
        }

        int read = demoFile->read( reinterpret_cast< char* >( msg.data ), msg.cursize );

        if ( read != msg.cursize ) {
            return -1;
        }

        msg.readcount = 0;
    }

    return msg.cursize;
}

template < class T >
void newArray( T*& arr, int size ) {
    arr = new T [ size ];
}

void DtDemo::initializeIo() {
    demoFile = new QFile( fInfo.fileName() );

    if ( currentParseType == CustomWrite ) {
        outFile = new QFile;

        if ( writeCfg->exportXml ) {
            xmlWriter = new DtXmlWriter( this );
        }
        else if ( writeCfg->importXml ) {
            xmlReader = new DtXmlReader( this );
        }
    }
}

void DtDemo::getMemory() {
    if ( currentParseType == CustomWrite ) {
        switch ( writeCfg->converter ) {
            case DC_BETAMAPFIX : proto = new DtDm73MapFix( this ); break;
            case DC_NONE : break;
        }
    }

    if ( !proto ) {
        switch ( demoProto ) {
            case Q3_68 : proto = new DtDm68( this ); break;
            case QZ_73 : proto = new DtDm73( this ); break;
            default : break;
        }
    }

    newArray( backupSnaps, PACKET_BACKUP );
    newArray( eventTimes, MAX_GENTITIES );
    memset( eventTimes, 0, sizeof( int ) * MAX_GENTITIES );

    if ( currentParseType == CustomWrite ) {
        if ( !writeCfg->singleFile ) {
            gamestateMsg = new msg_t;
            memset( gamestateMsg, 0, sizeof( *gamestateMsg ) );
            gamestateMsg->maxsize = MAX_MSGLEN;
            newArray( gamestateMsg->data, MAX_MSGLEN );
        }
    }
}

template < class T >
void freeArray( T*& arr ) {
    delete [] arr;
    arr = 0;
}

void DtDemo::freeMemory() {
    delete proto;
    proto = 0;

    freeArray( backupSnaps );
    freeArray( eventTimes );

    if ( demoFile ) {
        if ( demoFile->isOpen() ) {
            demoFile->close();
        }

        delete demoFile;
        demoFile = 0;
    }

    if ( outFile ) {
        if ( outFile->isOpen() ) {
            outFile->close();
        }

        delete outFile;
        outFile = 0;
    }

    delete xmlWriter;
    delete xmlReader;

    xmlWriter = 0;
    xmlReader = 0;

    if ( writeCfg ) {
        if ( writeCfg->parent() == this ) {
            delete writeCfg;
        }

        writeCfg = 0;
    }

    if ( gamestateMsg ) {
        freeArray( gamestateMsg->data );
        delete gamestateMsg;
        gamestateMsg = 0;
    }

    lastChatStrings.clear();
    lastCommands.clear();
    pastCommands.clear();
    playersInfo.clear();
    flagStatus.clear();
    firstPlace.clear();
    secondPlace.clear();
    cgsRedPlayers.clear();
    cgsBluePlayers.clear();
    cgsSpecs.clear();
    playerNames.clear();
}

void DtDemo::writeMsg( msg_t* msg ) {
    outFile->write( reinterpret_cast< char* >( &msgSeq ), 4 );
    outFile->write( reinterpret_cast< char* >( &msg->cursize ), 4 );
    outFile->write( reinterpret_cast< char* >( msg->data ), msg->cursize );
}

bool DtDemo::parse( parseType type ) {
    if ( parsing ) {
        return false;
    }

    if ( type == ParseGamestate && state & INFO_PARSED ) {
        return true;
    }

    if ( type == FindFrags && state & FRAGS_PARSED ) {
        lastSnapTime = infoLastSnapTime;
        return true;
    }

    if ( type == CustomWrite && writeCfg->importXml ) {
        return false;
    }

    if ( type == ReadEditInfo && state & EDITINFO_PARSED ) {
        lastSnapTime = infoLastSnapTime;
        return true;
    }

    if ( broken && type > ParseGamestate ) {
        return false;
    }

    currentParseType = type;
    savedSegments.clear();

    bool skipGamestate;
    int msgBytes = 0;

    initializeIo();

    if ( !open() ) {
        error( FATAL, "Couldn't open file" );
    }

    getMemory();

    parsing = true;
    stopScan = false;

    fInfo.size = demoFile->size();

    if ( type == ParseGamestate && info.size() ) {
        info.clear();
    }

    skipGamestate = ( type == FindFrags && state & INFO_PARSED );
    msgSeq = 0;

    if ( type == CustomWrite ) {
        if ( writeCfg->singleFile ) {
            outFile->setFileName( outFileName );

            if ( !outFile->open( QFile::WriteOnly ) ) {
                error( FATAL, "Couldn't open file %s for writing", outFileName.toUtf8().data() );
            }
        }

        writeNewPart = true;
        msgSnapTime = writeCfg->writeCommands ? firstSnapTime : MSG_START_TIME;
        messagesInPart = 0;

        if ( writeCfg->writeCommands ) {
            writeCfg->cutSegments.clear();
            writeCfg->removePauses = false;
            writeCfg->removeWarmup = false;
            writeCfg->removeLags = false;
        }

        if ( writeCfg->exportXml ) {
            xmlWriter->setDevice( outFile );
            xmlWriter->writeStartDemo();
        }

        pastCommands.clear();
    }

    msg_t msg;
    byte msgData[ MAX_MSGLEN ];

    msg_t outMsg;
    byte outMsgData[ MAX_MSGLEN ];

    totalBytesRead = 0;
    int oldSequenceNum = -1;
    serverMessageSequence = -1;
    sequenceIncrement = 1;
    pauseCalled = false;
    removedPause = false;
    gamestateMsgSaved = false;
    lastPartIndex = -1;
    lastWrittenChatString = 0;
    lastWrittenCommand = 0;
    lastSnapTime = 0;
    bool firstMessage = true;
    playersInfo.clear();
    playersInfoWritten = false;
    redClanPlayers = 0;
    blueClanPlayers = 0;
    scores1 = 0;
    scores2 = 0;
    caRoundNum = 0;
    lastPauseSegmentStartTime = -1;
    demoPauseStartTime = -1;
    cpmaTimerStopped = -1;
    flagStatus.clear();
    firstPlace.clear();
    secondPlace.clear();
    cgsRedPlayers.clear();
    cgsBluePlayers.clear();
    cgsSpecs.clear();
    playerNames.clear();
    copyMessages = false;
    warmupRemoved = false;
    intermission = false;

    while ( demoFile->pos() < fInfo.size ) {
        if ( serverMessageSequence != -1 ) {
            oldSequenceNum = serverMessageSequence;
        }

        if ( demoFile->read( reinterpret_cast< char* >( &serverMessageSequence ), 4 ) != 4 ) {
            error( FATAL, "Couldn't read file" );
        }

        if ( oldSequenceNum != -1 ) {
            sequenceIncrement = serverMessageSequence - oldSequenceNum;
        }

        if ( type == CustomWrite ) {
            proto->initialize( &outMsg, outMsgData, sizeof( outMsgData ) );
            msgCustomWrite = writeCfg->singleFile;
        }

        if ( readBlock( msg, msgData ) == -1 ) {
            continue;
        }

        if ( type == CustomWrite ) {
            totalBytesRead = demoFile->pos();
            msgBytes += msg.cursize;

            int msgLim = writeCfg->exportXml ? 30 : 300;
            msgLim *= 1024;

            if ( msgBytes >= msgLim ) {
                msgBytes = 0;
                emit readPosition( getReadPosition() );
                QApplication::processEvents();
            }
        }
        else if ( type == ReadEditInfo ) {
            lastChatStrings.clear();
            lastCommands.clear();
        }

        if ( skipGamestate ) {
            skipGamestate = false;
            continue;
        }

        if ( firstMessage                   &&
             type == CustomWrite            &&
             writeCfg->singleFile           &&
             !writeCfg->setGamestateString  &&
             !( writeCfg->converter == DC_BETAMAPFIX ) )
        {
            int start = writeCfg->cutSegments.isEmpty() ? -1 : writeCfg->cutSegments.at( 0 ).start;
            proto->updateGamestateInfo( start );
        }

        bool setGamestateString = ( type == CustomWrite && writeCfg->setGamestateString );
        bool mapFixConverter = ( type == CustomWrite && writeCfg->converter == DC_BETAMAPFIX );

        if ( setGamestateString || mapFixConverter ) {
            msgSeq = serverMessageSequence;
        }

        if ( copyMessages && !firstMessage ) {
            writeMsg( &msg );
            continue;
        }
        else if ( !parseMessage( &msg, &outMsg ) ) {
            break;
        }

        if ( firstMessage ) {
            if ( setGamestateString ) {
                emit gamestateWritten();
            }

            firstMessage = false;
        }

        if ( type == CustomWrite && msgCustomWrite && !writeCfg->exportXml ) {
            if ( writeCfg->singleFile || ( !writeCfg->singleFile && !writeNewPart ) ) {
                writeMsg( &outMsg );
            }
            else if ( writeNewPart ) {
                if ( outFile->isOpen() ) {
                    outFile->close();
                }

                int start = writeCfg->cutSegments.at( lastPartIndex ).start / 1000;
                int end = writeCfg->cutSegments.at( lastPartIndex ).end / 1000;

                QString filePartName = outFileName + QString( "_%1-%2" ).arg( start ).arg( end );
                QString fNameAdd;
                int addNum = 0;
                QString posFileName;

                do {
                    posFileName = filePartName + fNameAdd + "." + fileInfo().fileExt;
                    fNameAdd = QString( "_%1" ).arg( addNum++ );
                }
                while ( QFile::exists( posFileName ) );

                outFile->setFileName( posFileName );
                savedSegments.append( QFileInfo( posFileName ).fileName() );

                if ( !outFile->open( QFile::WriteOnly ) ) {
                    error( FATAL, "Can't open file %s for writing", posFileName.toUtf8().data() );
                }

                proto->updateGamestateInfo( writeCfg->cutSegments.at( lastPartIndex ).start );
                playersInfoWritten = false;

                msg_t gamestate;
                byte gamestateData[ MAX_MSGLEN ];

                proto->initialize( &gamestate, gamestateData, sizeof( gamestateData ) );
                gamestateMsg->readcount = 0;
                gamestateMsg->bit = 0;
                parseMessage( gamestateMsg, &gamestate );

                writeMsg( &gamestate );
                writeMsg( &outMsg );
                writeNewPart = false;
            }

            msgCutted = false;
        }
        else if ( !msgCustomWrite ) {
            msgCutted = true;
            messagesInPart = 0;
        }
    }

    switch ( type ) {
        case ParseGamestate :   state |= INFO_PARSED;                   break;
        case FindFrags :        state |= INFO_PARSED + FRAGS_PARSED;    break;
        case ReadEditInfo :     state |= EDITINFO_PARSED;               break;
        default : break;
    }

    if ( !cgsRedPlayers.isEmpty() ) {
        info += cgsRedPlayers;
    }

    if ( !cgsBluePlayers.isEmpty() ) {
        info += cgsBluePlayers;
    }

    if ( !cgsSpecs.isEmpty() ) {
        info += cgsSpecs;
    }

    if ( clientName.isEmpty() ) {
        clientName = playerNames.value( clientNum );
    }

    if ( type == ReadEditInfo || type == FindFrags ) {
        QHashIterator< int, int > it( frameTimeCounts );
        int maxCount = 0;

        while ( it.hasNext() ) {
            it.next();

            if ( it.value() > maxCount ) {
                maxCount = it.value();
                frameTime = it.key();
            }
        }

        serverFrameTimes.clear();
        infoLastSnapTime = lastSnapTime;

        if ( type == ReadEditInfo ) {
            if ( pauseCalled ) {
                lastPauseSegmentEndTime = lastSnapTime;
                addPause();
            }
        }
    }

    proto->afterParse();

    if ( type == CustomWrite && writeCfg->exportXml ) {
        xmlWriter->writeEndDemo();
    }

    return true;
}

bool DtDemo::startParse( parseType type ) {
    bool parsed = false;

    try {
        if ( ( parsed = parse( type ) ) ) {
            parsing = false;
        }
    }
    catch ( demoParseError ) {
        emit parseError();
    }

    freeMemory();
    return parsed;
}

int DtDemo::getReadPosition() const {
    return totalBytesRead * 100 / fInfo.size;
}

bool DtDemo::isParsing() {
    return parsing;
}

QStringList DtDemo::copySegment( DtWriteOptions* options ) {
    if ( !options ) {
        return QStringList();
    }

    if ( !( state & EDITINFO_PARSED ) && !startParse( ReadEditInfo ) ) {
        return QStringList();
    }

    writeCfg = options;
    outFileName = options->newFileName;

    if ( startParse( CustomWrite ) ) {
        QStringList tmpLst = savedSegments;
        savedSegments.clear();
        return tmpLst;
    }

    return QStringList();
}

QString DtDemo::relatedDemoPath() {
    switch ( demoProto ) {
        case Q3_68 : return dtdata::config.getQaDemoPath();
        case QZ_73 : return dtdata::config.getQzDemoPath();
        default : return "";
    }
}

QString DtDemo::writeSegment( DtWriteOptions* options ) {
    if ( !options ) {
        return "";
    }

    if ( !options->setGamestateString               &&
         !( options->converter == DC_BETAMAPFIX )   &&
         !( state & EDITINFO_PARSED )               &&
         !startParse( ReadEditInfo ) )
    {
        return "";
    }

    QString path = relatedDemoPath() + "/" + dtdata::defaultTmpDirName;
    QDir dir( path );

    if ( !dir.exists() && !dir.mkdir( path ) ) {
        return "";
    }

    writeCfg = options;
    outFileName.clear();

    if ( !writeCfg->newFileName.isEmpty() ) {
        outFileName = QString( "%1.dm_%2" ).arg( writeCfg->newFileName ).arg( demoProto );
    }
    else {
        QString fName = QString( "tmp%1.dm_%2" )
                        .arg( QString::number( qrand(), 16 ) )
                        .arg( demoProto );
        outFileName = path + "/" + fName;
    }

    if ( startParse( CustomWrite ) ) {
        return outFileName;
    }

    return "";
}

bool DtDemo::parseGamestateMsg() {
    return startParse( ParseGamestate );
}

bool DtDemo::findFrags() {
    return startParse( FindFrags );
}

bool DtDemo::readEditInfo() {
    return startParse( ReadEditInfo );
}

bool DtDemo::exportXml( const QString& file ) {
    outFileName = file;
    writeCfg = new DtWriteOptions( this );
    writeCfg->exportXml = true;
    writeCfg->singleFile = true;
    return startParse( CustomWrite );
}

const infoMap& DtDemo::getInfo() const {
    return info;
}

const obituaryEventsMap& DtDemo::getObitEvents() const {
    return obituaryEvents;
}

const sFileInfo& DtDemo::fileInfo() const {
    return fInfo;
}

int DtDemo::getAdvertDelay() const {
    return advertDelay;
}

bool DtDemo::isRecordedBySubscriber() const {
    return recordedBySubscriber;
}

QString DtDemo::getClientName() {
    return clientName;
}

QString DtDemo::md5() {
    if ( !open() ) {
        return "0";
    }

    QByteArray data = demoFile->readAll();
    demoFile->close();

    QByteArray dg = QCryptographicHash::hash( data, QCryptographicHash::Md5 );

    return QString::fromLatin1( dg.toHex() );
}

DtDemo::~DtDemo() {
    freeMemory();
}

bool DtDemo::playerInClientTeam( int playerIndex ) {
    return ( playerTeams.value( clientNum ) == playerTeams.value( playerIndex ) );
}

int DtDemo::getGameType() const {
    int gt = gameType;

    if ( instaGib ) {
        switch ( gt ) {
            case GT_FFA : gt = GT_INSTAGIB; break;
            case GT_CTF : gt = GT_INSTACTF; break;
            case GT_TDM : gt = GT_INSTATDM; break;
        }
    }

    return gt;
}

int DtDemo::getState() const {
    return state;
}

QString DtDemo::getGamestate() const {
    if ( gameInProgress ) {
        return tr( "In progress" );
    }
    else {
        return tr( "Not started" );
    }
}

bool DtDemo::isInProgress() const {
    return gameInProgress;
}

void DtDemo::setBroken( bool s ) {
    broken = s;
}

bool DtDemo::isBroken() const {
    return broken;
}

int DtDemo::getMapRestartTime() const {
    return ( mapRestartTime > 0 ) ? mapRestartTime - firstSnapTime : 0;
}

int DtDemo::getFrameTime() const {
    return ( frameTime == 0 ) ? 25 : frameTime;
}

int DtDemo::getProto() const {
    return demoProto;
}

int DtDemo::getSnapshotCount() const {
    return snapCount;
}

const QVector< cutSegment >& DtDemo::getPauses() const {
    return pauses;
}

const QVector< int >& DtDemo::getLags() const {
    return lags;
}

int DtDemo::getTimerInitialValue() const {
    return timerInitialValue;
}

int DtDemo::getLevelStartTime() const {
    return levelStartTime;
}

const QString& DtDemo::getQ3ModName() const {
    return q3ModName;
}

int DtDemo::getQ3Mod() const {
    return q3Mod;
}

const QVector< QPair< int, QString > >& DtDemo::getChatStrings() const {
    return chatStrings;
}

const QString& DtDemo::getMapName() const {
    return mapName;
}

const QString& DtDemo::getHostName() const {
    return hostName;
}

const DtCmdVec& DtDemo::getCommands() const {
    return commands;
}

bool DtDemo::openXml() {
    if ( demoFile->isOpen() ) {
        return true;
    }

    if ( !demoFile->open( QFile::ReadOnly ) ) {
        error( WARN, "Can't open file %s", fInfo.fileName().toUtf8().data() );
        return false;
    }

    xmlReader->setDevice( demoFile );
    xmlReader->readNext();

    if ( xmlReader->isXmlStart() ) {
        xmlReader->readNext();
    }
    else {
        return false;
    }

    if ( xmlReader->isDemoStart() ) {
        int protocol = xmlReader->xmlReadAttribute( "protocol" );

        switch ( protocol ) {
            case Q3_68  : demoProto = Q3_68; break;
            case QZ_73  : demoProto = QZ_73; break;
            default :
                demoProto = Q_UNKNOWN;
                error( WARN, "Unknown file extension %s", fInfo.fileExt.toUtf8().data() );
                return false;
        }
    }

    return true;
}

bool DtDemo::parseXml() {
    initializeIo();

    if ( !openXml() ) {
        error( FATAL, "Couldn't open file" );
    }

    getMemory();
    xmlReader->initializeFieldNums();

    QString demoExt = QString( ".dm_%1" ).arg( demoProto );
    QString fName = writeCfg->newFileName + "/" + fInfo.baseName;

    if ( !fName.endsWith( demoExt ) ) {
        fName += demoExt;
    }

    QFileInfo outFileInfo( fName );
    QString outFileName = outFileInfo.fileName();
    int addNum = 0;

    while ( QFile::exists( outFileInfo.absolutePath() + "/" + outFileName ) ) {
        outFileName = QString( "%1%2.%3" ).arg( outFileInfo.completeBaseName() )
                      .arg( addNum++ )
                      .arg( outFileInfo.suffix() );
    }

    if ( !addNum ) {
        outFileName = fName;
    }

    outFile->setFileName( outFileName );

    if ( !outFile->open( QFile::WriteOnly ) ) {
        error( FATAL, "Couldn't open file %s for writing", outFileName.toUtf8().data() );
    }

    msg_t msg;
    msg.cursize = 0;
    byte msgData[ MAX_MSGLEN ];

    int msgBytes = 0;

    while ( !xmlReader->atEnd() ) {
        totalBytesRead = demoFile->pos();
        msgBytes += msg.cursize;

        proto->initialize( &msg, msgData, sizeof( msgData ) );

        const int msgLim = 30 * 1024;

        if ( msgBytes >= msgLim ) {
            msgBytes = 0;
            emit readPosition( getReadPosition() );
            QApplication::processEvents();
        }

        xmlReader->parseXmlMessage( &msg );
        writeMsg( &msg );

        if ( xmlReader->isEndDocument() ) {
            break;
        }
    }

    return true;
}

bool DtDemo::importXml( const QString& importPath ) {
    currentParseType = CustomWrite;

    writeCfg = new DtWriteOptions( this );
    writeCfg->importXml = true;
    writeCfg->newFileName = importPath;

    bool parsed = false;

    try {
        if ( ( parsed = parseXml() ) ) {
            parsing = false;
        }
    }
    catch ( demoParseError ) {
        emit parseError();
    }

    freeMemory();
    return parsed;
}

bool DtDemo::isGamesateParsed() const {
    return ( state & INFO_PARSED );
}

bool DtDemo::isInfoAvailable() const {
    return ( state & INFO_FETCHED || state & INFO_PARSED );
}
