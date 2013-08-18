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

#include "MainWindow.h"
#include "Data.h"
#include "Demo.h"
#include "AbstractProtocol.h"

#include <qtsingleapplication.h>
#include <QDateTime>
#include <QFileInfo>
#include <QUrl>

using namespace dtdata;

static void showHelp() {
    qDebug( "Usage: qldt [KEY] [FILE] ...\n" );

    qDebug( "-i    Import specified demo from XML" );
    qDebug( "-e    Export specified demo to XML" );
    qDebug( "-o    Export output file name or path for import (optional)" );
}

static void showError( const char* error ) {
    qDebug( error );
    showHelp();
}

static void parseArguments( const QStringList& args ) {
    QString outputFile;
    QString inputFile;
    bool importDemo = false;
    bool exportDemo = false;

    for ( int i = 1; i < args.size(); ++i ) {
        const QString& arg = args.at( i );

        if ( arg == "-i" || arg == "-e" ) {
            if ( i + 1 == args.size() ) {
                showError( "No input file specfied\n" );
                break;
            }

            if ( !QFile::exists( args.at( i + 1 ) ) ) {
                showError( "Couldn't find specified file\n" );
                break;
            }

            inputFile = args.at( i + 1 );

            if ( arg == "-i" ) {
                if ( !exportDemo ) {
                    importDemo = true;
                }
            }
            else {
                if ( !importDemo ) {
                    exportDemo = true;
                }
            }
        }
        else if ( arg == "-h" ) {
            showHelp();
            break;
        }
        else if ( arg == "-o" ) {
            if ( i + 1 == args.size() ) {
                break;
            }

            outputFile = args.at( i + 1 );
        }
    }

    if ( ( !importDemo && !exportDemo ) || inputFile.isEmpty() ) {
        return;
    }

    QFileInfo inputFileInfo( inputFile );

    if ( importDemo ) {
        DtDemo demo( inputFileInfo );
        QString path = ( outputFile.isEmpty() ) ? QCoreApplication::applicationDirPath() : outputFile;

        if ( !demo.importXml( path ) ) {
            showError( "XML parse error\n" );
        }
    }
    else if ( exportDemo ) {
        DtDemo demo( inputFileInfo );
        QString defaultName = QCoreApplication::applicationDirPath() + "/" +
                              inputFileInfo.fileName() + ".xml";
        QString path = ( outputFile.isEmpty() ) ? defaultName : outputFile;

        if ( !demo.exportXml( path ) ) {
            showError( "Demo parse error\n" );
        }
    }
}

int main( int argc, char* argv[] ) {
    QtSingleApplication app( argc, argv );
    QStringList args = app.arguments();
    QStringList knownArgs;
    knownArgs << "-h" << "-i" << "-e" << "-z" << "-o";
    QString cmdArg;

    if ( !initializeData() ) {
        return 1;
    }

    if ( args.size() >= 2 ) {
        if ( knownArgs.contains( args.at( 1 ) ) ) {
            parseArguments( args );
            return 0;
        }

        cmdArg = args.at( 1 );
    }

    bool argIsConfigFile = false;

    if ( !cmdArg.isEmpty() ) {
        QFileInfo cmdFile( cmdArg );

        if ( cmdFile.exists() && acceptedFileFormat( cmdFile.suffix() ) ) {
            if ( isConfigFile( cmdFile.suffix() ) ) {
                argIsConfigFile = true;
            }
        }
        else {
            QRegExp mailtoReg( "^mailto:\\?body=([\\w\\W]+)&" );

            if ( mailtoReg.indexIn( cmdArg ) != -1 ) {
                cmdArg = QUrl::fromPercentEncoding( mailtoReg.cap( 1 ).toAscii() );

                if ( !cmdArg.startsWith( "http://" ) ) {
                    cmdArg += "http://";
                }
            }
            else {
                return 0;
            }
        }
    }

    if ( app.sendMessage( cmdArg ) ) {
        return 0;
    }

    qsrand( QDateTime::currentDateTime().toTime_t() );
    createMainWindow( cmdArg );

    return app.exec();
}
