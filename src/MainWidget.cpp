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

#include "MainWidget.h"
#include "Data.h"
#include "Demo.h"
#include "DemoTable.h"
#include "Task.h"
#include "ProgressDialog.h"
#include "DirTree.h"
#include "MainTabWidget.h"
#include "MainTable.h"
#include "FindFragsPanel.h"

#include <QApplication>
#include <QTime>
#include <QDir>
#include <QMutex>
#include <QVBoxLayout>

using namespace dtdata;

DtMainWidget::DtMainWidget( QWidget* parent ) : DtTablesWidget( DtTablesWidget::TT_DEMOFILES, parent ),
    task( new DtTask ),
    loadMutex( new QMutex ),
    indexing( false )
{
    updateCentralSplitterSize();
    setSplitterSizes( rSplit, "MainListRSplit", QList< int >() << 120 << 239 );

    progressDialog = new DtProgressDialog( tr( "Reading headers" ), this );
    connect( progressDialog, SIGNAL( buttonClicked() ), this, SLOT( stopParse() ) );

    qRegisterMetaType< DtDemo* >( "DtDemo*" );

    connect( this, SIGNAL( newDemoLoaded() ), this, SLOT( updateProgress() ), Qt::QueuedConnection );
    connect( this, SIGNAL( demoGamestateParsed( DtDemo* ) ), demoTable, SLOT( addDemo( DtDemo* ) ) );
    connect( this, SIGNAL( demoInfoFetched( DtDemo* ) ), demoTable, SLOT( addDemo( DtDemo* ) ) );
    connect( this, SIGNAL( actPlaydemoTriggered() ), parent, SIGNAL( actPlaydemoTriggered() ) );

    if ( config.frags.tabOpened ) {
        showFindFragsWidget();
    }
}

DtMainWidget::~DtMainWidget() {
    delete loadMutex;
    delete task;
    delete dirTree;
}

void DtMainWidget::updateCentralSplitterSize() {
    QList< int > centralSplitterSizes;
    QString splitterCfgName = "MainListCSplit";

    if ( config.dirTreeAlwaysOpened ) {
        centralSplitterSizes << 150 << 605 << 242;
        insertDirTree();
        splitterCfgName.append( "Tree" );
    }
    else {
        centralSplitterSizes << 705 << 292;
    }

    setSplitterSizes( cSplit, splitterCfgName, centralSplitterSizes );
}

void DtMainWidget::setFragsWidgetVisible( bool visible ) {
    if ( !visible ) {
        delete fragsTab;

        if ( config.frags.tabOpened ) {
            config.frags.tabOpened = false;
            config.save();
        }
    }
    else {
        showFindFragsWidget();
        mainTabWidget->setCurrentIndex( 0 );
    }
}

void DtMainWidget::hideFindFragsWidget() {
    setFragsWidgetVisible( false );
}

void DtMainWidget::showFindFragsWidget() {
    if ( !fragsTab ) {
        fragsTab = new DtFragsTab( this );
        connect( fragsTab, SIGNAL( scanFinished() ), this, SIGNAL( scanFinished() ) );

        QVBoxLayout* mainLayout = qobject_cast< QVBoxLayout* >( layout() );

        if ( mainLayout ) {
            mainLayout->addWidget( fragsTab );
            mainLayout->setStretch( 0, 1 );
            mainLayout->setStretch( 1, 0 );
        }
    }
}

void DtMainWidget::setDirTreeOpened( bool opened ) {
    if ( !opened ) {
        saveSplitterState( cSplit, "MainListCSplitTree" );
        delete dirTree;
    }
    else {
        saveSplitterState( cSplit, "MainListCSplit" );
    }

    updateCentralSplitterSize();
}

void DtMainWidget::insertDirTree() {
    if ( !dirTree ) {
        dirTree = new DtDirTree( this, DT_SELECT );

        connect( dirTree, SIGNAL( dirSelected( const QString& ) ),
                 mainTabWidget, SLOT( onDirSelected( const QString& ) ) );

        cSplit->insertWidget( 0, dirTree );
    }
}

void DtMainWidget::updateIfNeeded() {
    if ( demoTable->isUpdateNeeded() ) {
        indexDemos();
        demoTable->setUpdateNeeded( false );
    }
}

void DtMainWidget::stopParse() {
    stopParseHeaders = true;
}

void DtMainWidget::indexDemos() {
    if ( indexing ) {
        return;
    }

    indexing = true;
    demoTable->setUpdatesEnabled( false );
    demoTable->removeAllRows();
    demoTable->deleteMarkedName();
    clearDemoInfo();
    qApp->processEvents();

    if ( openedDemos.size() ) {
        deleteUnusedDemos();
    }

    QDir demoDir( currentWorkingDir );
    demoDb.setDir( currentWorkingDir );
    demoFiles = demoDir.entryList( demoNameFilters );

    listLoadTime = new QTime;
    listLoadTime->start();

    stopParseHeaders = false;

    task->run( demoFiles.size(), this, &DtMainWidget::loadDemo, &DtMainWidget::progressWaitFunc );
    qApp->processEvents();
    demoFiles.clear();
    demoDb.cleanObsoleteRecords();
    delete listLoadTime;

    progressDialog->close();
    demoTable->sortColumn( config.mainTableSortColumn,
                           static_cast< Qt::SortOrder >( config.mainTableSortOrder ) );
    demoTable->setUpdatesEnabled( true );
    indexing = false;
}

void DtMainWidget::progressWaitFunc() {
    const int longTaskTime = 700;

    if ( !progressDialog->isVisible() && listLoadTime->elapsed() > longTaskTime ) {
        progressDialog->show();
    }

    QApplication::processEvents();
}

void DtMainWidget::loadDemo( int index ) {
    QString fPath = QString( "%1/%2" ).arg( currentWorkingDir, demoFiles.at( index ) );

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
        emit demoInfoFetched( demo );
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
            emit demoGamestateParsed( demo );
        }
    }

    loadMutex->lock();
    task->setJobDone();
    loadMutex->unlock();

    emit newDemoLoaded();
}

void DtMainWidget::demoParseError() {
    DtDemo* demo = qobject_cast< DtDemo* >( sender() );

    if ( !demo ) {
        return;
    }

    demo->setBroken( true );
}

void DtMainWidget::updateProgress() {
    QString lbl = QString::number( task->finishedJobsNum() )
                  + " of " + QString::number( task->jobCount() );
    progressDialog->setData( lbl, static_cast< int >(
            task->finishedJobsNum() * 100 / task->jobCount() ), DtProgressDialog::CancelButton );
}

void DtMainWidget::saveSplitterSizes() {
    QString splitterCfgName = "MainListCSplit";

    if ( config.dirTreeAlwaysOpened ) {
        splitterCfgName.append( "Tree" );
    }

    saveSplitterState( cSplit, splitterCfgName );
    saveSplitterState( rSplit, "MainListRSplit" );

    demoTable->saveColumns();
    variablesTable->saveColumnWidths( "CommandsListColumns" );
}

