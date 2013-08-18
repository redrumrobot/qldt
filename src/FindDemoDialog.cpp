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

#include "Data.h"
#include "Demo.h"
#include "FindDemoDialog.h"
#include "MainWindow.h"
#include "MainTabWidget.h"
#include "Task.h"
#include "ProgressDialog.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QToolButton>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QButtonGroup>
#include <QPushButton>
#include <QDateEdit>
#include <QEventLoop>
#include <QDirIterator>
#include <QMutex>
#include <QApplication>

using namespace dtdata;

DtFindDemoDialog::DtFindDemoDialog( QWidget* parent ) : QDialog( parent ),
    task( new DtTask ),
    loadMutex( new QMutex )
{
    setWindowTitle( tr( "Find demos" ) );
    setFixedSize( 350, 300 );
    setWindowModality( Qt::ApplicationModal );

    connect( this, SIGNAL( newDemoLoaded() ), this, SLOT( updateProgress() ), Qt::QueuedConnection );

    progressDialog = new DtProgressDialog( tr( "Indexing directory" ), this );
    connect( progressDialog, SIGNAL( buttonClicked() ), this, SLOT( stopParse() ) );

    QGridLayout* dialogLayout = new QGridLayout;
    int row = 0;

    QLabel* playerLbl = new QLabel( tr( "Player" ), this );
    playerEdit = new QLineEdit( this );
    playerEdit->setMinimumWidth( 180 );

    dialogLayout->addWidget( playerLbl, row, 0, Qt::AlignLeft );
    dialogLayout->addWidget( playerEdit, row++, 1, 1, 2, Qt::AlignLeft );

    QLabel* mapLbl = new QLabel( tr( "Map" ), this );
    mapEdit = new QLineEdit( this );
    mapEdit->setMinimumWidth( 180 );

    dialogLayout->addWidget( mapLbl, row, 0, Qt::AlignLeft );
    dialogLayout->addWidget( mapEdit, row++, 1, 1, 2, Qt::AlignLeft );

    QLabel* serverLbl = new QLabel( tr( "Server" ), this );
    serverEdit = new QLineEdit( this );
    serverEdit->setMinimumWidth( 180 );

    dialogLayout->addWidget( serverLbl, row, 0, Qt::AlignLeft );
    dialogLayout->addWidget( serverEdit, row++, 1, 1, 2, Qt::AlignLeft );

    QLabel* nameLbl = new QLabel( tr( "File name" ), this );
    nameEdit = new QLineEdit( this );
    nameEdit->setMinimumWidth( 180 );

    dialogLayout->addWidget( nameLbl, row, 0, Qt::AlignLeft );
    dialogLayout->addWidget( nameEdit, row++, 1, 1, 2, Qt::AlignLeft );

    QLabel* matchCaseLbl = new QLabel( tr( "Match case" ), this );
    matchCaseCb = new QCheckBox( this );

    dialogLayout->addWidget( matchCaseLbl, row, 0, Qt::AlignLeft );
    dialogLayout->addWidget( matchCaseCb, row++, 1, Qt::AlignLeft );

    gameTypeLbl = new QLabel( tr( "Game type" ), this );
    gameTypeCombo = new QComboBox( this );
    gameTypeCombo->setMinimumWidth( 150 );

    gameTypeCombo->addItem( tr( "Any", "Type|Any" ), GT_ANY );
    gameTypeCombo->addItem( "Free for All", GT_FFA );
    gameTypeCombo->addItem( "Duel", GT_DUEL );
    gameTypeCombo->addItem( "Team Deathmatch", GT_TDM );
    gameTypeCombo->addItem( "Clan Arena", GT_CA );
    gameTypeCombo->addItem( "Capture the Flag", GT_CTF );
    gameTypeCombo->addItem( "Overload", GT_OVERLOAD );
    gameTypeCombo->addItem( "Harvester", GT_HARVESTER );
    gameTypeCombo->addItem( "Freeze Tag", GT_FREEZE );
    gameTypeCombo->addItem( "Domination", GT_DOM );
    gameTypeCombo->addItem( "Attack & Defend", GT_AD );
    gameTypeCombo->addItem( "Red Rover", GT_RR );
    gameTypeCombo->addItem( "Race", GT_RACE );
    gameTypeCombo->addItem( "Instagib", GT_INSTAGIB );
    gameTypeCombo->addItem( "InstaCTF", GT_INSTACTF );
    gameTypeCombo->addItem( "InstaTDM", GT_INSTATDM );

    dialogLayout->addWidget( gameTypeLbl, row, 0, Qt::AlignLeft );
    dialogLayout->addWidget( gameTypeCombo, row++, 1, 1, 2, Qt::AlignLeft );

    q3ModLbl = new QLabel( tr( "Mod" ), this );
    q3ModCombo = new QComboBox( this );
    q3ModCombo->setMinimumWidth( 150 );
    q3ModCombo->addItem( tr( "Any", "Type|Any" ), MOD_ANY );
    q3ModCombo->addItem( "baseq3", MOD_BASEQ3 );
    q3ModCombo->addItem( "osp", MOD_OSP );
    q3ModCombo->addItem( "cpma", MOD_CPMA );
    q3ModCombo->addItem( "defrag", MOD_DEFRAG );
    dialogLayout->addWidget( q3ModLbl, row, 0, Qt::AlignLeft );
    dialogLayout->addWidget( q3ModCombo, row++, 1, 1, 2, Qt::AlignLeft );

    updateWidgetsVisibility();

    QLabel* dateLbl = new QLabel( tr( "Date between" ), this );
    dateFromEdit = new QDateEdit( this );
    dateToEdit = new QDateEdit( this );

    dialogLayout->addWidget( dateLbl, row, 0, Qt::AlignLeft );
    dialogLayout->addWidget( dateFromEdit, row, 1, Qt::AlignLeft );
    dialogLayout->addWidget( dateToEdit, row++, 2, Qt::AlignLeft );

    QLabel* allDirLbl = new QLabel( tr( "All directories" ), this );
    allDirCb = new QCheckBox( this );

    dialogLayout->addWidget( allDirLbl, row, 0, Qt::AlignLeft );
    dialogLayout->addWidget( allDirCb, row++, 1, Qt::AlignLeft );

    dialogLayout->setColumnStretch( dialogLayout->columnCount() + 1, 1 );

    QHBoxLayout* buttonsLayout = new QHBoxLayout;

    QPushButton* findButton = new QPushButton( tr( "Find" ), this );
    connect( findButton, SIGNAL( clicked() ), this, SLOT( startSearch() ) );
    findButton->setMinimumWidth( 120 );

    QPushButton* cancelButton = new QPushButton( tr( "Cancel" ), this );
    connect( cancelButton, SIGNAL( clicked() ), this, SLOT( cancelPressed() ) );
    cancelButton->setMinimumWidth( 120 );

    buttonsLayout->addStretch( 1 );
    buttonsLayout->addWidget( findButton );
    buttonsLayout->addWidget( cancelButton );

    dialogLayout->addItem( new QSpacerItem( 1, 15 ), row++, 0 );
    dialogLayout->addLayout( buttonsLayout, row, 0, 1, 3 );

    setLayout( dialogLayout );
}

DtFindDemoDialog::~DtFindDemoDialog() {
    delete loadMutex;
    delete task;
}

void DtFindDemoDialog::indexDirectory( QString dir ) {
    QDir demoDir( dir );
    demoDb.setDir( dir );
    currentDir = dir;
    currentDirName = currentDir;
    currentDirName.remove( 0, baseDir.size() + 1 );
    demoFiles = demoDir.entryList( demoNameFilters );

    stopParseHeaders = false;

    task->run( demoFiles.size(), this, &DtFindDemoDialog::loadDemo,
               &DtFindDemoDialog::progressWaitFunc );

    qApp->processEvents();
    demoFiles.clear();
    demoDb.cleanObsoleteRecords();
}

void DtFindDemoDialog::indexAllDirs() {
    QString startDir = ( config.getSelectedGame() == Q_LIVE ) ? config.getQzDemoPath() :
                                                                config.getQaDemoPath();
    baseDir = ( config.getSelectedGame() == Q_LIVE ) ? config.getQzBasePath() :
                                                       config.getQaBasePath();
    QDirIterator it( startDir, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks );

    progressDialog->show();

    if ( currentWorkingDir != startDir ) {
        indexDirectory( startDir );
    }

    while ( it.hasNext() ) {
        it.next();
        QFileInfo entryInfo( it.fileInfo() );
        QString path = it.filePath();

        if ( entryInfo.isDir()          &&
             !path.endsWith( "/." )     &&
             !path.endsWith( "/.." )    &&
             path != currentWorkingDir )
        {
            indexDirectory( path );
        }
    }

    progressDialog->close();
}

void DtFindDemoDialog::startSearch() {
    opt.search = true;
    opt.name = nameEdit->text();
    opt.player = playerEdit->text();
    opt.map = mapEdit->text();
    opt.server = serverEdit->text();
    opt.matchCase = matchCaseCb->isChecked();
    opt.gameType = gameTypeCombo->itemData( gameTypeCombo->currentIndex() ).toInt();
    opt.mod = q3ModCombo->itemData( q3ModCombo->currentIndex() ).toInt();
    opt.dateFrom = dateFromEdit->dateTime().toTime_t();
    opt.dateTo = dateToEdit->dateTime().toTime_t();
    opt.allDirs = allDirCb->isChecked();

    if ( opt.allDirs ) {
        indexAllDirs();
    }

    close();
}

void DtFindDemoDialog::updateWidgetsVisibility() {
    bool qlSelected = ( config.getSelectedGame() == Q_LIVE );

    q3ModLbl->setVisible( !qlSelected );
    q3ModCombo->setVisible( !qlSelected );
    gameTypeLbl->setVisible( qlSelected );
    gameTypeCombo->setVisible( qlSelected );
}

void DtFindDemoDialog::showEvent( QShowEvent* ) {
    opt.search = false;
    updateWidgetsVisibility();
    dateToEdit->setDate( QDate::currentDate().addDays( 1 ) );
}

void DtFindDemoDialog::closeEvent( QCloseEvent* ) {
    pEventLoop->exit( 0 );
}

void DtFindDemoDialog::cancelPressed() {
    close();
}

DtFindDemoOptions DtFindDemoDialog::exec() {
    if ( pEventLoop ) {
        return opt;
    }

    show();

    QEventLoop eventLoop;
    pEventLoop = &eventLoop;

    QPointer< DtFindDemoDialog > guard = this;
    eventLoop.exec( QEventLoop::DialogExec );
    pEventLoop = 0;

    if ( guard.isNull() ) {
        return opt;
    }

    return opt;
}

void DtFindDemoDialog::stopParse() {
    stopParseHeaders = true;
}

void DtFindDemoDialog::progressWaitFunc() {
    QApplication::processEvents();
}

void DtFindDemoDialog::loadDemo( int index ) {
    QString fPath = QString( "%1/%2" ).arg( currentDir, demoFiles.at( index ) );

    QFileInfo fInfo( fPath );
    QString modifiedName = getModifiedName( fPath );

    bool newDemo = false;
    DtDemo* demo = openedDemos.value( modifiedName, 0 );

    if ( !demo ) {
        demo = new DtDemo( fInfo );
        connect( demo, SIGNAL( parseError() ), this, SLOT( demoParseError() ) );
        newDemo = true;
    }

    DtDemoDbData demoData( demo->fileInfo().fName, demo->fileInfo().lastModified );

    if ( demoDb.contains( demoData ) ) {
        if( newDemo ) {
            loadMutex->lock();
            openedDemos.insert( modifiedName, demo );
            loadMutex->unlock();
        }

        demo->setDbData( demoData );
    }
    else {
        if ( !stopParseHeaders ) {
            demo->parseGamestateMsg();

            if( newDemo ) {
                loadMutex->lock();
                openedDemos.insert( modifiedName, demo );
                loadMutex->unlock();
            }

            demoData.player = demo->getClientName();
            demoData.type = demo->getGameType();
            demoData.mod = demo->getQ3Mod();
            demoData.map = demo->getMapName();
            demoData.date = demo->getLevelStartTime();
            demoData.server = demo->getHostName();
            demoData.protocol = demo->getProto();
            demoData.broken = demo->isBroken();

            demoDb.insertRecord( demoData );
        }
    }

    loadMutex->lock();
    task->setJobDone();
    loadMutex->unlock();

    emit newDemoLoaded();
}

void DtFindDemoDialog::demoParseError() {
    DtDemo* demo = qobject_cast< DtDemo* >( sender() );

    if ( !demo ) {
        return;
    }

    demo->setBroken( true );
}

void DtFindDemoDialog::updateProgress() {
    QString lbl = currentDirName + " (" +
                  QString::number( task->finishedJobsNum() )
                  + " of " + QString::number( task->jobCount() ) + ")";

    progressDialog->setData( lbl, static_cast< int >(
                             task->finishedJobsNum() * 100 / task->jobCount() ),
                             DtProgressDialog::CancelButton );
}
