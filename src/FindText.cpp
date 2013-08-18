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

#include "FindText.h"
#include "Data.h"
#include "QuakeEnums.h"
#include "MainTabWidget.h"
#include "ScanWidget.h"
#include "FindTextTable.h"
#include "DemoTable.h"
#include "MainTable.h"
#include "Demo.h"
#include "ProgressDialog.h"
#include "Task.h"

#include <QApplication>
#include <QMutex>

using namespace dtdata;

DtFindText::DtFindText(  QWidget* parent ) :
    scanMutex( new QMutex ),
    task( new DtTask ),
    progressDialog( new DtProgressDialog( tr( "Searching" ), parent ) )
{
    connect( progressDialog, SIGNAL( buttonClicked() ), task, SLOT( stop() ) );
    connect( this, SIGNAL( newScanStarted() ), this, SLOT( updateProgress() ) );
}

DtFindText::~DtFindText() {
    delete task;
    delete scanMutex;
}

void DtFindText::find( const QString& findText, bool searchMatchCase, bool searchIgnoreColors ) {
    currentScanWidget = mainTabWidget->addScanWidget( DtTablesWidget::TT_FINDTEXT );

    progressDialog->start();
    progressDialog->show();
    QApplication::processEvents();

    currentScanWidget->setSearchInfo( tr( "string" ) + ": " + findText );

    matchCase = searchMatchCase;
    ignoreColors = searchIgnoreColors;
    text = findText;
    haveResults = false;
    demosSize = 0.f;
    doneMb = 0.f;
    size_t demosCount = mainDemoTable->rowCount();

    for ( size_t i = 0; i < demosCount; ++i ) {
        demosSize += mainDemoTable->demoAt( i )->fileInfo().size / MiB;
    }

    progressDialog->setData( "0 " + tr( "of" ) + " " + QString::number( demosCount ), 0,
                             DtProgressDialog::CancelButton );

    task->run( demosCount, this, &DtFindText::scanDemo, &DtFindText::scanWaitFunc );
    qApp->processEvents();

    if ( haveResults ) {
        progressDialog->close();
    } else {
        progressDialog->setData( tr( "Nothing found" ), 100, DtProgressDialog::OkButton );
    }

    emit scanFinished();
    currentScanWidget->scanTable->sortColumn( config.findTextTableSortColumn,
                                              static_cast< Qt::SortOrder >(
                                                 config.findTextTableSortOrder ) );
}

void DtFindText::updateProgress() {
    QString lbl = QString::number( task->finishedJobsNum() ) + " " + tr( "of" ) + " "
                  + QString::number( task->jobCount() );
    int prc = static_cast< int >( doneMb / demosSize * 100 );
    progressDialog->setData( lbl, prc, DtProgressDialog::CancelButton );
}

void DtFindText::scanWaitFunc() {
    QApplication::processEvents();
}

void DtFindText::addResult( DtDemo* demo, int time, const QString& player, const QString& cmd,
                            const QString& msg )
{
    scanMutex->lock();
    DtFindTextTable* table = qobject_cast< DtFindTextTable* >( currentScanWidget->scanTable );
    table->addScanRow( demo, time, player, cmd, msg );
    scanMutex->unlock();
    QApplication::processEvents();
    haveResults = true;
}

void DtFindText::scanDemo( int index ) {
    DtDemo* demo = mainDemoTable->demoAt( index );
    bool parseDone = demo->readEditInfo();

    scanMutex->lock();
    doneMb += demo->fileInfo().size / MiB;
    scanMutex->unlock();

    emit newScanStarted();

    if ( parseDone ) {
        const QVector< QPair< int, QString > >& chatStrings = demo->getChatStrings();
        size_t chatStringsCount = chatStrings.count();
        bool q3Chat = ( demo->getProto() == Q3_68 );
        bool q3Cpma = ( demo->getQ3Mod() == MOD_CPMA );
        QString exp;

        if ( !q3Chat ) { /* QL */
            exp = "^(t?chat)\\s\"([^\\s]+)\\s([^\"]+)";
        }
        else if ( q3Cpma ) { /* CPMA */
            exp = "^(chat|mm2)\\s(\\d+)?\\s?(\\d+)?\\s?\"([^\"]+)";
        }
        else { /* VQ3/OSP */
            exp = "^(t?chat)\\s\"([^\"]+)";
        }

        QRegExp chatExp( exp );
        Qt::CaseSensitivity cs = matchCase ? Qt::CaseSensitive : Qt::CaseInsensitive;
        QRegExp textPattern( text, cs, QRegExp::WildcardUnix );

        for ( size_t i = 0; i < chatStringsCount; ++i ) {
            int strTime = chatStrings.at( i ).first;
            QString str = chatStrings.at( i ).second;

            if ( chatExp.indexIn( str ) == -1 ) {
                continue;
            }

            QString command = chatExp.cap( 1 );
            QString chatText;
            QString playerName;
            bool teamCpma = false;

            if ( !q3Chat ) {
                chatText = chatExp.cap( 3 );
            }
            else if ( q3Cpma ) {
                teamCpma = ( command == "mm2" );

                if ( teamCpma ) {
                    playerName = chatExp.cap( 2 );
                }

                chatText = chatExp.cap( 4 );
            }
            else {
                chatText = chatExp.cap( 2 );
            }

            QString msg;

            if ( !q3Cpma ) { /* QL/VQ3/OSP */
                QStringList tokens = chatText.split( chatEscapeChar, QString::SkipEmptyParts );
                playerName = tokens.at( 0 );

                if ( command == "chat" ) {
                    msg = tokens.at( 1 );
                    msg.remove( 0, 2 );
                }
                else if ( tokens.count() > 2 ) {
                    msg = tokens.at( 2 );
                    msg.remove( 0, 2 );
                }
            }
            else { /* CPMA */
                if ( teamCpma ) {
                    msg = chatExp.cap( 3 );
                }
                else {
                    QStringList tokens = chatText.split( ": ", QString::SkipEmptyParts );
                    playerName = tokens.at( 0 );
                    msg = tokens.at( 1 );
                }
            }

            if ( ignoreColors ) {
                cleanStringColors( msg );
            }

            if ( textPattern.exactMatch( msg ) ) {
                cleanStringColors( playerName );
                addResult( demo, strTime, playerName, command, msg );
            }
        }
    }

    scanMutex->lock();
    task->setJobDone();
    scanMutex->unlock();
}
