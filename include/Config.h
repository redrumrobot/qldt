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

#ifndef DTCONFIG_H
#define DTCONFIG_H

#include <QString>
#include <QHash>
#include <QVariant>
#include <QTextCharFormat>
#include <QBitArray>
#include <QDebug>

class QSettings;
class DtCrypt;

enum {
    Q_LIVE,
    Q_ARENA
};

enum {
    F_ZIP,
    F_7Z,
    F_RAR
};

struct findFragsSettings {
    bool tabOpened;
    quint32 segAddTimeBegin;
    quint32 segAddTimeEnd;
    int selectorIndex;
    int cbWeapon;
    int cbGameType;
    int cbMap;
    QString leMap;
    QString leMaxTime;
    QString leMinFrags;
    QString playerName;
    QString playerNames;
    bool directHitsOnly;
    bool countTeamKills;
};

struct charFormat {
    bool bold;
    bool italic;
    bool underline;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};

struct textEditorSettings {
    QString fontFamily;
    int fontSize;
    int tabSize;
    QColor backgroundColor;
    QColor lineNumbersBgColor;
    QColor lineNumbersColor;
    QColor lineHighlightColor;
    QTextCharFormat normalText;
    QTextCharFormat number;
    QTextCharFormat string;
    QTextCharFormat key;
    QTextCharFormat comment;
    QTextCharFormat action;
    QTextCharFormat cvar;
    QTextCharFormat command;
    QTextCharFormat shortCommand;
    QTextCharFormat say;
    QTextCharFormat semicolon;
    QTextCharFormat preprocessor;
    bool searchMatchCase;
    bool useCustomFiles;
    QString customFilesPath;
};

class DtConfig {
public:
    DtConfig();

    QSettings* settings;

    QString language;

    int lastRecentMenuIndex;
    QStringList recentOpenedFiles;
    QStringList recentOpenedUrls;
    QStringList recentOpenedConfigs;
    QBitArray mainTableVisibleColumns;
    QBitArray findFragsTableVisibleColumns;
    QBitArray findTextTableVisibleColumns;
    QBitArray findDemoTableVisibleColumns;
    bool headerInfoVisible;
    bool showClanTags;

    int mainTableSortColumn;
    int mainTableSortOrder;
    int findFragsTableSortColumn;
    int findFragsTableSortOrder;
    int findTextTableSortColumn;
    int findTextTableSortOrder;
    int findDemoTableSortColumn;
    int findDemoTableSortOrder;
    bool confirmOnDelete;
    int zlibCompressionLevel;
    bool dirTreeAlwaysOpened;
    bool dropDemosToNewDir;
    bool archiveRemovePaths;
    QString lastRenameFormat;
    QString lastDuelRenameFormat;
    QString lastOrganizeFormat;

    bool editorShowGameTime;
    findFragsSettings frags;
    textEditorSettings textEditor;
    quint32 textSegAddTimeBegin;
    quint32 textSegAddTimeEnd;
    bool showLags;

    QString lastFindTextString;
    bool findTextIgnoreColors;
    bool findTextMatchCase;

    bool qzFullscreen;
    bool qaFullscreen;
    int qzWindowedMode;
    int qaWindowedMode;
    int qzFullscreenMode;
    int qaFullscreenMode;

    QString qzGameConfig;
    QString qaGameConfig;

    QString otherAppTitle;
    bool otherAppDm68;
    bool otherAppDm73;
    bool otherAppDoubleClick;
    bool otherAppMenu;
    bool otherAppPreview;
    QString otherAppPath;
    QString otherAppCmdLine;
    bool otherAppFromDemos;

    QString lastPackPath;
    QString lastExportPath;
    QString lastImportPath;

    QColor tableAlternateColor;
    QColor tableSelectionColor;
    QColor tableSelectionBorderColor;
    QColor tableTextColor;

    int tableAlternateColorFactor;
    int tableSelectionColorFactor;

    int partsSelectorMode;
    QString timeDisplayFormat;

    int lastArchiverFormat;

    void save( bool saveDefaults = false );
    void defaults();
    void saveTextEditorSettings( bool saveDefaults = false );
    void loadDefaults();
    void loadLanguage();
    void textEditorDefaults();

    void setSelectedGame( int game );
    int getSelectedGame() const;
    void setQzPath( const QString& path );
    const QString& getQzPath() const;
    void setQaPath( const QString& path );
    const QString& getQaPath() const;
    void setQaHomePath( const QString& path );
    const QString& getQaHomePath() const;
    void setQzFSBasePath( const QString& path );
    const QString& getQzFSBasePath() const;
    const QString& getQzBasePath() const;
    const QString& getQzHomePath() const;
    const QString& getQaBasePath() const;
    const QString& getQzDemoPath() const;
    const QString& getQaDemoPath() const;

    QByteArray formatToByteArray( unsigned char red = 0, unsigned char green = 0, unsigned char blue = 0,
                                  bool bold = false, bool italic = false, bool underline = false );

    QByteArray textCharFormatToByteArray( const QTextCharFormat& textFormat );
    QTextCharFormat byteArrayToFormat( const QByteArray& array );
    QString getAppDataPath();

private:
    int selectedGame;
    QString qzPath;
    QString qaPath;
    QString qzFSBasePath;
    QString qzHomePath;
    QString qaHomePath;
    QString qzBasePath;
    QString qaBasePath;
    QString qzDemoPath;
    QString qaDemoPath;

    QString getQzDefaultFSBasePath();
    QString getQzDefaultExePath();
    QString getDefaultLanguage();

    QHash< QString, QVariant > defaultVars;

    void updatePaths();

    Q_DISABLE_COPY( DtConfig )
};

#endif // DTCONFIG_H
