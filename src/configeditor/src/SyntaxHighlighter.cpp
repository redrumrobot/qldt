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

#include "SyntaxHighlighter.h"
#include "ConfigEditorData.h"

#include <QFile>
#include <locale>

using namespace dtdata;

QHash< QString, int > DtHighlighter::stringHash;

DtHighlighter::DtHighlighter( QTextDocument* parent ) : QSyntaxHighlighter( parent ) {
    highlightType = HT_CFG;

    if ( stringHash.isEmpty() ) {
        fillHash( TK_CMD, "cmds" );
        fillHash( TK_KEY, "keys" );
        fillHash( TK_CVAR, "cvars" );
        fillHash( TK_ACT, "actions" );
    }

    updateFormatSettings();

    cmdLineReg.setPattern( "^([\\s]*[^\\s]+)([\\s]*[^\\s]*|\\\"[\\w\\W\\s]*\\\")([\\s]*[^\n]*)" );
    unkVarReg.setPattern( "^\\s*[a-z\\d_\\.\\-\\x2b]+\\s*$" );
    numReg.setPattern( "^\\s*[\\\"]*[-]?(\\d*\\.*\\d+)[\\\"]*\\s*$" );
    strReg.setPattern( "^\\s*[\\\"]+[^\\\"]+[\\\"]+\\s*$" );
    commentReg.setPattern( "//[^\n]*" );
    startCommentReg.setPattern( "/\\*" );
    endCommentReg.setPattern( "\\*/" );
    preprocessorReg.setPattern( "#(define|include)[^\n]*" );
    quotedStringReg.setPattern( "\\s*[\\\"]+[^\\\"]+[\\\"]+\\s*" );
    numberStringReg.setPattern( "[-]?(\\d*\\.*\\d+)\\b+" );
}

void DtHighlighter::updateFormatSettings() {
    numberFormat        =   config.textEditor.number;
    stringFormat        =   config.textEditor.string;
    printFormat         =   config.textEditor.say;
    actionFormat        =   config.textEditor.action;
    semicolonFormat     =   config.textEditor.semicolon;
    commentFormat       =   config.textEditor.comment;
    cvarFormat          =   config.textEditor.cvar;
    commandFormat       =   config.textEditor.command;
    wCommandFormat      =   config.textEditor.shortCommand;
    keyFormat           =   config.textEditor.key;
    preprocessorFormat  =   config.textEditor.preprocessor;
}

void DtHighlighter::fillHashRes( tokenType tkType, QString fileName ) {
    QFile idScript( ":/res/idscript/" + fileName + ".txt" );
    idScript.open( QFile::ReadOnly );
    const int maxCmdLine = 1024;
    char str[ maxCmdLine ];
    char strAutocomplete[ maxCmdLine ];
    char* strData = str;
    char* strAutocompleteData = strAutocomplete;

    while ( !idScript.atEnd() ) {
        int pos = 0;
        char c;

        if ( tkType == TK_ACT ) {
            ++strData;  // leave space for + and -
        }

        while ( pos < maxCmdLine && ( idScript.read( &c, 1 ) == 1 ) ) {
            if ( c == '\n' ) {
                *strData = '\0';
                *strAutocompleteData = '\0';
                break;
            }

            *strData++ = tolower( c );
            *strAutocompleteData++ = c;
            ++pos;
        }

        strData = str;
        strAutocompleteData = strAutocomplete;

        if ( tkType == TK_ACT ) {
            str[ 0 ] = '+';
            stringHash.insert( strData, tkType );
            str[ 0 ] = '-';
        }

        stringHash.insert( strData, tkType );

        if ( pos > 3 ) {
            scriptWords.append( strAutocompleteData );
        }
    }

    idScript.close();
}

void DtHighlighter::fillHashCustom( tokenType tkType, QString fileName ) {
    QFile idScript( config.textEditor.customFilesPath + "/" + fileName + ".txt" );

    if ( idScript.open( QFile::ReadOnly ) ) {
        while ( !idScript.atEnd() ) {
            QString line = idScript.readLine().trimmed();
            QString lowerLine = line.toLower();

            if ( tkType == TK_ACT ) {
                stringHash.insert( "+" + lowerLine, tkType );
                stringHash.insert( "-" + lowerLine, tkType );
            }
            else {
                stringHash.insert( lowerLine, tkType );
            }

            if ( line.size() > 3 ) {
                scriptWords.append( line );
            }
        }

        idScript.close();
    }
}

void DtHighlighter::fillHash( tokenType tkType, QString fileName ) {
    if ( !config.textEditor.useCustomFiles ) {
        fillHashRes( tkType, fileName );
    }
    else {
        fillHashCustom( tkType, fileName );
    }
}

bool DtHighlighter::isType( tokenType tType, const QString& token ) {
    if ( token.isEmpty() ) {
        return false;
    }

    switch ( tType ) {
        case TK_UNKVAR :    return unkVarReg.exactMatch( token );
        case TK_NUM :       return numReg.exactMatch( token );
        case TK_STR :       return strReg.exactMatch( token );
        case TK_KEY : {
            if ( token.at( 0 ) == '"' && token.at( token.size() - 1 ) == '"' ) {
                QString tk = token;
                tk.chop( 1 );
                return ( stringHash.value( tk.remove( 0, 1 ), TK_UNK ) == tType );
            }
        }
        default:            return ( stringHash.value( token, TK_UNK ) == tType );
    }
}

bool DtHighlighter::isPrintCmd( const QString& cmd ) {
    return ( cmd == "say"       ||
             cmd == "say_team"  ||
             cmd == "echo"      ||
             cmd == "tell"      ||
             cmd == "tell_buddy" );
}

void DtHighlighter::chopComments( QString& str ) {
    int commentIndex = str.indexOf( "//", 0 );

    if ( commentIndex != -1 ) {
        str.chop( str.size() - commentIndex );
        str = str.trimmed();
    }
}

int DtHighlighter::cleanStr( const QString& str, QString& rstr ) {
    int i = 0;

    while ( i < str.size() && ( str.at( i ) == ' ' || str.at( i ) == '\t' ) ) {
        ++i;
    }

    rstr = str.toLower().trimmed();

    if ( rstr.endsWith( ';' ) ) {
        rstr.chop( 1 );
        rstr = rstr.trimmed();
    }

    chopComments( rstr );
    return i;
}

void DtHighlighter::highlightString( QString str, int& index ) {
    chopComments( str );

    if ( str.startsWith( '"' ) && str.endsWith( '"' ) ) {
        str.chop( 1 );
        str = str.right( str.size() - 1 );
        ++index;
    }

    highlightLine( str, index, true );
}

void DtHighlighter::setValFormat( const QString& val, int index ) {
    if ( isType( TK_NUM, val ) ) {
        setFormat( index, val.size(), numberFormat );
    }
    else if ( isType( TK_STR, val ) ) {
        setFormat( index, val.size(), stringFormat );
    }
}

void DtHighlighter::splitLine( QString line, QStringList& subLines ) {
    int semicolonPos = 0;

    forever {
        semicolonPos = line.indexOf( ';', semicolonPos + 1 );

        if ( semicolonPos != -1 ) {
            int quotesCount = 0;
            int quotePos = 0;

            forever {
                quotePos = line.indexOf( '"', quotePos + 1 );

                if ( quotePos != -1 && quotePos < semicolonPos ) {
                    ++quotesCount;
                }
                else {
                    break;
                }
            }

            if ( quotesCount != 1 ) {
                subLines << line.left( semicolonPos + 1 );
                line = line.right( line.size() - semicolonPos - 1 );
                semicolonPos = -1;
            }
        }
        else {
            subLines << line;
            break;
        }
    }
}

void DtHighlighter::highlightLine( QString line, int& index, bool subString ) {
    QStringList subLines;

    splitLine( line, subLines );

    foreach ( const QString& subLine, subLines ) {
        if ( cmdLineReg.indexIn( subLine ) == -1 ) {
            continue;
        }

        QString cmd = cmdLineReg.cap( 1 );
        QString arg = cmdLineReg.cap( 2 );
        QString val = cmdLineReg.cap( 3 );

        QString cleanCmd;
        QString cleanArg;
        QString cleanVal;

        int cmdSp = cleanStr( cmd, cleanCmd );
        int argSp = cleanStr( arg, cleanArg );
        int valSp = cleanStr( val, cleanVal );

        index += cmdSp;

        if ( isType( TK_CMD, cleanCmd ) ) {
            setFormat( index, cleanCmd.size(), subString ? commandFormat :
                       !arg.isEmpty() ? commandFormat : wCommandFormat );

            index += cmd.size() - cmdSp + argSp;

            if ( cleanCmd.endsWith( "bind" ) && isType( TK_KEY, cleanArg ) ) {
                setFormat( index, cleanArg.size(), keyFormat );
                index += arg.size() - argSp + valSp;
                highlightString( val.trimmed(), index );
            }
            else if ( isPrintCmd( cleanCmd ) ) {
                setFormat( index, cleanArg.size(), printFormat );
                index += arg.size() - argSp + valSp;
                setFormat( index, cleanVal.size(), printFormat );
                index += val.size() - valSp;
            }
            else if ( isType( TK_CVAR, cleanArg ) ) {
                setFormat( index, cleanArg.size(), cvarFormat );
                index += arg.size() - argSp + valSp;
                setValFormat( cleanVal, index );
                index += val.size() - valSp;
            }
            else if ( !cleanCmd.compare( "vstr" ) ) {
                index += arg.size() - argSp + val.size();
            }
            else if ( !isType( TK_NUM, cleanArg ) && isType( TK_UNKVAR, cleanArg ) ) {
                index += arg.size() - argSp + valSp;
                highlightString( val.trimmed(), index );
            }
            else {
                setValFormat( cleanArg, index );
                index += arg.size() - argSp + val.size();
            }
        }
        else if ( isType( TK_CVAR, cleanCmd ) ) {
            setFormat( index, cleanCmd.size(), cvarFormat );
            index += cmd.size() - cmdSp + argSp;
            setValFormat( cleanArg, index );
            index += arg.size() - argSp + val.size();
        }
        else if ( isType( TK_ACT, cleanCmd ) ) {
            setFormat( index, cleanCmd.size(), actionFormat );
            index += cmd.size() - cmdSp + arg.size() + val.size();
        }
        else {
            index += cmd.size() - cmdSp + arg.size() + val.size();
        }
    }
}

void DtHighlighter::highlightCfgBlock( const QString &text ) {
    QStringList lines = text.split( "\n", QString::SkipEmptyParts );

    int index = 0;

    for ( int i = 0; i < lines.size(); ++i ) {
        highlightLine( lines.at( i ), index );
    }

    index = commentReg.indexIn( text );

    while ( index >= 0 ) {
        int length = commentReg.matchedLength();
        setFormat( index, length, commentFormat );
        index = commentReg.indexIn( text, index + length );
    }

    index = text.indexOf( ';' );

    while ( index >= 0 ) {
        setFormat( index, 1, semicolonFormat );
        index = text.indexOf( ';', index + 1 );
    }
}

void DtHighlighter::highlightReg( const QString &text, const QRegExp &reg, const QTextCharFormat& format ) {
    int index = reg.indexIn( text );

    while ( index >= 0 ) {
        int length = reg.matchedLength();
        setFormat( index, length, format );
        index = reg.indexIn( text, index + length );
    }
}

void DtHighlighter::highlightMenuBlock( const QString &text ) {
    highlightReg( text, numberStringReg, numberFormat );
    highlightReg( text, quotedStringReg, stringFormat );
    highlightReg( text, preprocessorReg, preprocessorFormat );
    highlightReg( text, commentReg, commentFormat );

    setCurrentBlockState( 0 );

    int startIndex = 0;

    if ( previousBlockState() != 1 ) {
        startIndex = text.indexOf( startCommentReg );
    }

    while ( startIndex >= 0 ) {
        int endIndex = text.indexOf( endCommentReg, startIndex );
        int commentLength;

        if ( endIndex == -1 ) {
            setCurrentBlockState( 1 );
            commentLength = text.length() - startIndex;
        }
        else {
            commentLength = endIndex - startIndex + endCommentReg.matchedLength();
        }

        setFormat( startIndex, commentLength, commentFormat );
        startIndex = text.indexOf( startCommentReg, startIndex + commentLength );
    }
}

void DtHighlighter::highlightBlock( const QString& text ) {
    if ( !doHighlight ) {
        return;
    }

    switch ( highlightType ) {
        case HT_NONE :  break;
        case HT_CFG  :  highlightCfgBlock( text ); break;
        case HT_MENU :  highlightMenuBlock( text ); break;
    }
}

