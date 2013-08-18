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

#include "MainTabWidget.h"
#include "Data.h"
#include "DemoTable.h"
#include "MainWidget.h"
#include "ScanWidget.h"
#include "DirTree.h"
#include "EditTab.h"
#include "Demo.h"
#include "MainWindow.h"

#include <QApplication>
#include <QMenu>
#include <QMessageBox>

using namespace dtdata;

DtMainTabWidget::DtMainTabWidget( QWidget* parent ) : DtTabWidget( parent ) {
    mainTabWidget = this;
    mainWidget = new DtMainWidget( this );
    connect( mainWidget, SIGNAL( scanFinished() ), parent, SLOT( onScanFinished() ) );

    addTab( mainWidget, config.getSelectedGame() == Q_LIVE ? "Live" : "Arena" );

    connect( parent, SIGNAL( closing() ), this, SLOT( saveState() ), Qt::DirectConnection );

    setTabsClosable( true );
    tabBar()->tabButton( 0, QTabBar::RightSide )->hide();

    int tabHeight = style()->sizeFromContents( QStyle::CT_TabBarTab, 0,
                                               QSize( 0, tabBar()->fontMetrics().height() ) ).height();
    tabHeight = ( tabHeight < 27 ) ? 34 : 25;

    tabBar()->setStyleSheet( QString( "QTabBar::tab { min-width: 18ex; height: %1px; }" )
                             .arg( tabHeight ) );

    dirMenu = new QMenu( this );
    connect( dirMenu, SIGNAL( aboutToShow() ), this, SLOT( activateList() ) );
    dirAction = new DtDirAction( this, DT_SELECT );
    dirMenu->addAction( dirAction );

    treeButton = new DtDirTreeButton( this, dirMenu );

    setTreeButtonVisible( !config.dirTreeAlwaysOpened );

    connect( this, SIGNAL( tabCloseRequested( int ) ), this, SLOT( closeTab( int ) ) );
    connect( this, SIGNAL( currentChanged( int ) ), this, SLOT( tabChanged( int ) ) );

    connect( parent, SIGNAL( setDirTreeOpened( bool ) ), mainWidget, SLOT( setDirTreeOpened( bool ) ) );
    connect( parent, SIGNAL( setFindFragsVisible( bool ) ),
             mainWidget, SLOT( setFragsWidgetVisible( bool ) ) );

    connect( this, SIGNAL( actPlaydemoTriggered() ), parent, SLOT( playSelectedDemo() ) );
}

void DtMainTabWidget::setTreeButtonVisible( bool buttonVisible ) {
    setCornerWidget( buttonVisible ? treeButton : 0, Qt::TopLeftCorner );
    treeButton->setVisible( buttonVisible );
    setProperty( "treeButtonVisible", buttonVisible ? "true" : "false" );
    setStyle( QApplication::style() );
}

void DtMainTabWidget::activateList() {
    if ( currentIndex() != 0 ) {
        setCurrentIndex( 0 );
    }
}

void DtMainTabWidget::updateMainTable() {
    mainWidget->indexDemos();
}

void DtMainTabWidget::clearScanWidgetSelections( DtDemoTable* table ) {
    for ( int i = 0; i < scanWidgets.size(); ++i ) {
        if ( scanWidgets.at( i )->demoTable == table ) {
            scanWidgets.at( i )->clearSelections();
            break;
        }
    }
}

void DtMainTabWidget::tabChanged( int index ) {
    if ( index == 0 ) {
        mainWidget->updateIfNeeded();
    }
}

QString DtMainTabWidget::getDirTabName( const QString& path ) {
    return path.right( path.size() - path.lastIndexOf( '/' ) - 1 );
}

void DtMainTabWidget::updateTabName( const QString& path ) {
    bool pathStartsQz = path.startsWith( config.getQzHomePath() );

    config.setSelectedGame( pathStartsQz ? Q_LIVE : Q_ARENA );
    treeButton->updateIcon();

    QString tabName;
    static const int subDirSz = QString( "/baseq3/demos" ).size();
    bool showTitleDirName = false;

    if ( pathStartsQz ) {
        int qzPathSize = config.getQzHomePath().size();

        if ( path.size() == qzPathSize + subDirSz ) {
            tabName = "Live";
        }
        else {
            tabName = getDirTabName( path );
            showTitleDirName = true;
        }
    }
    else {
        int qaPathSize = config.getQaHomePath().size();

        if ( path.size() == qaPathSize + subDirSz ) {
            tabName = "Arena";
        }
        else {
            tabName = getDirTabName( path );
            showTitleDirName = true;
        }
    }

    if ( tabName == defaultNewDirName ) {
        tabName = tr( "New" );
        showTitleDirName = false;
    }

    setTabText( 0, tabName );
    dtMainWindow->setTitle( showTitleDirName ? tabName : "" );
}

void DtMainTabWidget::onDirSelected( const QString& path ) {
    dirMenu->close();

    if ( currentWorkingDir != path ) {
        if ( !QFileInfo( path ).permission( QFile::ReadUser ) ) {
            QMessageBox::warning( this, tr( "Error" ),
                                  tr( "You have no permission to open that directory" ) );
            return;
        }

        updateTabName( path );
        currentWorkingDir = path;
        updateMainTable();
    }
}

void DtMainTabWidget::saveState() {
    mainWidget->saveSplitterSizes();

    saveScanListsSplitters();

    foreach ( DtEditTab* editor, demoEditors ) {
        delete editor;
    }
}

void DtMainTabWidget::saveScanListsSplitters() {
    for ( int i = 0; i < scanWidgets.size(); ++i ) {
        DtScanWidget* scanWidget;

        if ( ( scanWidget = scanWidgets.at( i ) ) != 0 ) {
            scanWidget->saveSplitterSizes();
            return;
        }
    }
}

void DtMainTabWidget::closeCurrentTab() {
    closeTab( currentIndex() );
}

void DtMainTabWidget::closeTab( int index ) {
    if ( widget( index )->inherits( "DtScanWidget" ) ) {
        closeScanList( index );
    }
    else {
        closeDemoEditor( index );
    }
}

DtScanWidget* DtMainTabWidget::currentScanWidget() {
    return qobject_cast< DtScanWidget* >( widget( currentIndex() ) );
}

void DtMainTabWidget::closeScanList( int index ) {
    DtScanWidget* scanTab = qobject_cast< DtScanWidget* >( widget( index ) );

    if ( !scanTab ) {
        return;
    }

    for ( int i = 0; i < scanWidgets.size(); ++i ) {
        if ( scanTab == scanWidgets.at( i ) ) {
            if ( !scanTab->isDone() ) {
                //message
            }
            else {
                if ( currentPlayDemoTable == scanTab->demoTable ) {
                    currentPlayDemoTable = 0;
                }

                scanTab->demoTable->removeAllRows();
                removeTab( index );
                delete scanTab;
                scanWidgets.removeAt( i );
            }
        }
    }
}

void DtMainTabWidget::closeDemoEditor( int index ) {
    DtEditTab* editTab = qobject_cast< DtEditTab* >( widget( index ) );

    if ( !editTab ) {
        return;
    }

    for ( int i = 0; i < demoEditors.size(); ++i ) {
        if ( editTab == demoEditors.at( i ) ) {
            removeTab( index );
            delete editTab;
            demoEditors.removeAt( i );
        }
    }
}

DtScanWidget* DtMainTabWidget::addScanWidget( int type ) {
    saveScanListsSplitters();

    DtScanWidget* newWidget = new DtScanWidget( type, this );
    scanWidgets.append( newWidget );

    int ind = addTab( newWidget, tr( "Search results" ) );
    setCurrentIndex( ind );

    return newWidget;
}

DtEditTab* DtMainTabWidget::addDemoEditor( DtDemo* demo ) {
    foreach ( DtEditTab* demoEditor, demoEditors ) {
        if ( demoEditor->currentDemo() == demo ) {
            setCurrentIndex( indexOf( demoEditor ) );
            return demoEditor;
        }
    }

    DtEditTab* editor = new DtEditTab( this );

    if ( editor->startEdit( demo ) ) {
        demoEditors.append( editor );

        QString demoName = demo->fileInfo().baseName;

        if ( demoName.size() > 15 ) {
            demoName = demoName.left( 15 ) + "...";
        }

        int ind = addTab( editor, demoName );
        setCurrentIndex( ind );
    }
    else {
        delete editor;
        editor = 0;
    }

    return editor;
}

DtMainTabWidget::~DtMainTabWidget() {
    for ( int i = 0; i < scanWidgets.size(); ++i ) {
        delete scanWidgets.at( i );
    }

    for ( int i = 0; i < demoEditors.size(); ++i ) {
        delete demoEditors.at( i );
    }

    scanWidgets.clear();
}


