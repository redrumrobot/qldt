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

#ifndef DTFINDTEXT_H
#define DTFINDTEXT_H

#include "DemoData.h"

class QMutex;
class DtTask;
class DtProgressDialog;

class DtFindText : public QObject {
    Q_OBJECT
public:
    DtFindText( QWidget* parent = 0 );
    ~DtFindText();

    void find( const QString& findText, bool searchMatchCase, bool searchIgnoreColors );

protected:
    bool matchCase;
    bool ignoreColors;
    bool haveResults;
    float demosSize, doneMb;
    QMutex* scanMutex;
    DtTask* task;
    DtProgressDialog* progressDialog;
    QString text;

    void scanDemo( int index );
    void scanWaitFunc();
    void addResult( DtDemo* demo, int time, const QString& player, const QString& cmd,
                    const QString& msg );

protected slots:
    void updateProgress();

signals:
    void scanFinished();
    void newScanStarted();
};

#endif // DTFINDTEXT_H
