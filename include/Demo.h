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

#ifndef DTDEMO_H
#define DTDEMO_H

#include "Data.h"
#include "DemoData.h"

#include <stdio.h>

#include <QStringList>
#include <QMap>
#include <QFile>
#include <QVector>
#include <QPair>
#include <QSet>
#include <QuakeCommon.h>

class QFileInfo;

typedef QVector< QPair< QString, QString > > infoMap;
typedef QMap< int, int > obituaryEventsMap; // < time, mod >
                                            // mod: -1 - death or CA respawn, less than -1 - teamkill
static const QString demoExtBase = "dm_";
static const char confDelimeter = '\\';

struct sFileInfo {
    int size;
    int lastModified;
    QString fileExt;
    QString baseName;
    QString filePath;
    QString fullFilePath;
    QString fName;

    const QString& fileName( bool path = true ) const {
        return path ? fullFilePath : fName ;
    }
};

enum quake3Mods {
    MOD_ANY,
    MOD_BASEQ3,
    MOD_OSP,
    MOD_CPMA,
    MOD_DEFRAG
};

class DtAbstractProtocol;
class DtXmlWriter;
class DtXmlReader;

class DtDemo : public QObject {
    Q_OBJECT
public:
    DtDemo( const QFileInfo& fInf );
    ~DtDemo();

    enum demoState {
        NONE = 1,
        INFO_FETCHED = 2,     /* Info fetched from db */
        INFO_PARSED = 4,      /* Gamestate parsed */
        FRAGS_PARSED = 8,     /* All obituary events found */
        EDITINFO_PARSED = 16  /* All edit essential info gathered */
    };

    quint32 referenceCount;

    bool parseGamestateMsg();
    bool findFrags();
    int getState() const;
    void setBroken( bool s );
    bool isBroken() const;

    const infoMap& getInfo() const;
    const obituaryEventsMap& getObitEvents() const;
    const sFileInfo& fileInfo() const;
    int getMapRestartTime() const;
    int getLevelStartTime() const;

    QString getClientName();
    int getLength() const;

    int getAdvertDelay() const;
    bool isRecordedBySubscriber() const;

    QString writeSegment( DtWriteOptions* options );
    QStringList copySegment( DtWriteOptions* options );
    bool readEditInfo();
    int getReadPosition() const;
    bool isParsing();
    bool isGamesateParsed() const;
    bool isInfoAvailable() const;
    void setFileInfo( const QFileInfo& fInf );
    void setDbData( const DtDemoDbData& demoData );
    int getGameType() const;
    bool exportXml( const QString& file );
    int getFrameTime() const;
    int getProto() const;
    int getSnapshotCount() const;
    const QVector< cutSegment >& getPauses() const;
    const QVector< int >& getLags() const;
    int getTimerInitialValue() const;
    const QVector< QPair< int, QString > >& getChatStrings() const;
    bool importXml( const QString& importPath );
    const QString& getMapName() const;
    const QString& getHostName() const;
    const QString& getQ3ModName() const;
    int getQ3Mod() const;
    QString getGamestate() const;
    bool isInProgress() const;
    const DtCmdVec& getCommands() const;

private:
    enum parseType {
        ParseGamestate,
        FindFrags,
        ReadEditInfo,
        CustomWrite,
        ExportXml
    };

    struct DtObituaryEvent {
        bool save;
        int victim;
        int inflictor;
        int mod;

        DtObituaryEvent() :
                save( false ),
                victim( 0 ),
                inflictor( 0 ),
                mod( -1 )
        { }
    };

    static const int MSG_START_TIME = 1000;
    static const int MAX_CONFKEY = 256;
    static const int MAP_RESTART_DURATION = 400;

    int state;
    obituaryEventsMap obituaryEvents;
    DtWriteOptions* writeCfg;
    DtAbstractProtocol* proto;
    parseType currentParseType;
    bool broken;

    QFile* demoFile;
    QFile* outFile;
    DtXmlReader* xmlReader;
    DtXmlWriter* xmlWriter;
    sFileInfo fInfo;
    QString outFileName;
    QStringList savedSegments;
    DtDemoProto demoProto;
    quake3Mods q3Mod;
    QString q3ModName;

    infoMap info;
    infoMap cgsRedPlayers;
    infoMap cgsBluePlayers;
    infoMap cgsSpecs;
    int infoPlayerCount;
    int clientNum;
    bool parsing;
    bool gameTimeSet;
    int firstSnapTime;
    int lastSnapTime;
    int infoLastSnapTime;
    int serverMessageSequence;
    int serverCommandSequence;
    int parseEntitiesNum;
    int snapCount;
    int sequenceIncrement;
    int timerInitialValue;
    int demoWarmupEndTime;
    bool gameInProgress;
    int serverCmdSeq;
    QSet< int > pastCommands;
    int lastWrittenChatString;
    int lastWrittenCommand;
    int levelStartTime;
    int gameType;
    bool instaGib;
    bool countdownStopped;
    int demoTimerValue;
    int demoPauseStartTime;
    int cpmaTimerStopped;
    bool warmupTimeCmd;
    bool warmupRemoved;
    QMap< int, QString > playerNames;

    clSnapshot_t snap;
    clSnapshot_t* backupSnaps;
    msg_t* gamestateMsg;
    int* eventTimes;

    DtCmdVec commands;
    QVector< QPair< int, QString > > chatStrings; /* < time, command > */
    QStringList lastChatStrings;
    DtCmdVec lastCommands;
    QVector< cutSegment > pauses;
    QVector< int > lags;
    QSet< int > serverFrameTimes;
    QHash< int, int > frameTimeCounts;
    QString mapName;
    QString hostName;
    QMap< int, QString > playersInfo;
    QString clientName;
    QMap< int, int > playerTeams;

    int redClanPlayers;
    int blueClanPlayers;
    int scores1;
    int scores2;
    QString flagStatus;
    QString firstPlace;
    QString secondPlace;
    int caRoundNum;

    int mapRestartTime;
    bool mapRestartFrame;
    bool msgCustomWrite;
    bool msgCutted;
    bool intermission;
    int msgSnapTime;
    int msgSeq;
    int msgEntSnapTime;
    bool writeNewPart;
    int lastPartIndex;
    bool stopScan;
    quint64 totalBytesRead;
    int frameTime;
    int messagesInPart;
    bool pauseCalled;
    bool pauseHaveTimer;
    int lastPauseStartTime;
    int lastPauseSegmentStartTime;
    int lastPauseSegmentEndTime;
    int lastPauseCountdownTime;
    bool removedPause;
    bool gamestateMsgSaved;
    bool playersInfoWritten;
    int advertDelay;
    bool recordedBySubscriber;
    bool copyMessages;

    class demoParseError { };

    enum errorLevel {
        WARN,
        FATAL
    };

    void error( errorLevel level, const char* fmt, ... );
    void getMemory();
    void freeMemory();
    void initializeIo();
    bool open();
    int readBlock( msg_t& msg, byte* msgData );
    bool parseMessage( msg_t* msg, msg_t* outMsg );
    bool parseGamestate( msg_t* msg, msg_t* outMsg );
    void readGamestateString( msg_t* msg, int& dataCount );
    int writeGamestateStrings( msg_t* msg, int& dataCount, msg_t* outMsg );
    void parseSnapshot( msg_t* msg, msg_t* outMsg );
    bool startParse( parseType type );
    bool parse( parseType type );
    void writeMsg( msg_t* msg );
    void setGameTime( int sTime );
    bool playerInClientTeam( int playerIndex );
    QString md5();
    void addPause();
    void setPowerups( playerState_t* fromPs, playerState_t* toPs, int serverTime );
    bool parseXml();
    bool openXml();
    QString relatedDemoPath();

    friend class DtAbstractProtocol;
    friend class DtDm68;
    friend class DtDm73;
    friend class DtDm73MapFix;
    friend class DtXmlReader;
    friend class DtXmlWriter;

signals:
    void readPosition( int );
    void parseError();
    void gamestateWritten();
};

#endif // DTDEMO_H
