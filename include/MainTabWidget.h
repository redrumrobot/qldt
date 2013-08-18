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

#ifndef DTMAINTABWIDGET_H
#define DTMAINTABWIDGET_H

#include "TabWidget.h"

class DtMainWidget;
class DtScanWidget;
class DtDemoTable;
class DtDirTreeButton;
class QMenu;
class DtDirAction;
class DtEditTab;
class DtDemo;

class DtMainTabWidget : public DtTabWidget {
    Q_OBJECT
public:
    DtMainTabWidget( QWidget* parent = 0 );
    ~DtMainTabWidget();

    void updateMainTable();
    void clearScanWidgetSelections( DtDemoTable* table );
    DtScanWidget* addScanWidget( int type );
    DtScanWidget* currentScanWidget();
    DtEditTab* addDemoEditor( DtDemo* demo );
    void setTreeButtonVisible( bool buttonVisible );
    DtMainWidget* mainWidget;
    void updateTabName( const QString& path );
    void closeCurrentTab();

    QList< DtScanWidget* > scanWidgets;
    QList< DtEditTab* > demoEditors;

protected:
    DtDirTreeButton* treeButton;
    QMenu* dirMenu;
    DtDirAction* dirAction;

    void saveScanListsSplitters();
    QString getDirTabName( const QString& path );

public slots:
    void saveState();
    void onDirSelected( const QString& path );

private slots:
    void closeScanList( int index );
    void closeDemoEditor( int index );
    void closeTab( int index );
    void tabChanged( int index );
    void activateList();

signals:
    void actPlaydemoTriggered();

};

#endif // DTMAINTABWIDGET_H
