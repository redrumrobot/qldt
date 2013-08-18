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

#ifndef DTSYNTAXHIGHLIGHTER_H
#define DTSYNTAXHIGHLIGHTER_H

#include <QSyntaxHighlighter>

enum highlightTypes {
    HT_NONE,
    HT_CFG,
    HT_MENU
};

class DtHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    DtHighlighter( QTextDocument* parent = 0 );

    bool doHighlight;
    int highlightType;

    enum tokenType {
        TK_UNK,
        TK_CMD,
        TK_KEY,
        TK_CVAR,
        TK_ACT,
        TK_UNKVAR,
        TK_NUM,
        TK_STR
    };

    static QHash< QString, int > stringHash;

    void updateFormatSettings();

protected:
    QRegExp cmdLineReg;
    QRegExp unkVarReg;
    QRegExp numReg;
    QRegExp strReg;
    QRegExp commentReg;
    QRegExp startCommentReg;
    QRegExp endCommentReg;
    QRegExp preprocessorReg;
    QRegExp quotedStringReg;
    QRegExp numberStringReg;

    QTextCharFormat cvarFormat;
    QTextCharFormat actionFormat;
    QTextCharFormat commandFormat;
    QTextCharFormat wCommandFormat;
    QTextCharFormat keyFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat semicolonFormat;
    QTextCharFormat bindFormat;
    QTextCharFormat commentFormat;
    QTextCharFormat stringFormat;
    QTextCharFormat printFormat;
    QTextCharFormat preprocessorFormat;

    inline bool isPrintCmd( const QString& cmd );
    inline bool isType( tokenType tkType, const QString& cmd );
    inline void highlightString( QString str, int& index );
    inline void chopComments( QString& str );
    inline int cleanStr( const QString& str, QString& rstr );
    inline void setValFormat( const QString& val, int index );
    inline void splitLine( QString line, QStringList& subLines );

    void fillHash( tokenType tkType, QString fileName );
    void fillHashRes( tokenType tkType, QString fileName );
    void fillHashCustom( tokenType tkType, QString fileName );
    void highlightLine( QString line, int& index, bool subString = false );
    void highlightBlock( const QString &text );
    void highlightCfgBlock( const QString &text );
    void highlightMenuBlock( const QString &text );
    void highlightReg( const QString &text, const QRegExp &reg, const QTextCharFormat& format );
};

#endif // DTSYNTAXHIGHLIGHTER_H
