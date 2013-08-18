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

#ifndef DTFRAGSPANEL_H
#define DTFRAGSPANEL_H

#include "TabWidget.h"
#include "DemoData.h"

class DtSourceFilesSelector;
class DtScanProgressDialog;
class DtTask;
class DtDemo;
class DtTask;
class DtProgressDialog;
class QPushButton;
class QComboBox;
class QLineEdit;
class QMutex;
class QGroupBox;
class QGridLayout;
class QButtonGroup;
class QCheckBox;

class DtFragsTab : public QWidget {
    Q_OBJECT
public:
    DtFragsTab( QWidget* parent = 0 );
    ~DtFragsTab();

    void saveSettings();

protected:
    enum filters {
        F_ALL,
        F_SELECTED,
        F_TODAY,
        F_THIS_WEEK,
        F_THIS_MONTH,
        F_RECOREDED_BY,
        F_PLAYERS
    };

    DtSourceFilesSelector* sSelector;
    QPushButton* startScanButton;
    QPushButton* closeButton;
    QComboBox* cbWeapon;
    QComboBox* cbGameType;
    QComboBox* cbMap;
    QLineEdit* leMap;
    QLineEdit* leMaxTime;
    QLineEdit* leMinFrags;
    QCheckBox* directHitsOnlyCb;
    QCheckBox* countTeamKillsCb;
    DtProgressDialog* progressDialog;
    DtTask* task;

    bool haveResults;
    float demosSize;
    float doneMb;

    int minFrags;
    int maxTime;
    int weapon;
    int gameType;

    QMutex* scanMutex;
    DtDemoVec currentFilteredDemos;

    void initNames();
    bool weaponMatch( int weapon, int mod );
    void scanDemo( int index );
    void scanWaitFunc();
    void addResult( DtDemo* demo, int startSeqTime, int fragsInSeq, int deltaTime, int segLength,
                    int weapon );

protected slots:
    void startScan();
    void updateProgress();

signals:
    void scanFinished();
    void newScanStarted();
    void demoGamestateParsed( int );
};

class DtSourceFilesSelector : public QWidget {
    Q_OBJECT
public:
    DtSourceFilesSelector( QWidget* parent = 0 );

    int selectedIndex();
    QVector< QLineEdit* > textFields;

protected:
    QGroupBox* sourceFilesGBox;
    QButtonGroup* sourceFilesGroup;
    QGridLayout* gBox;
    QList< int > textFieldNums;


protected slots:
    void selectionChanged( int id );
};

#endif // DTFRAGSPANEL_H
