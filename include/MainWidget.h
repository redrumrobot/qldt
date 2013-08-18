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

#ifndef DTMAINWIDGET_H
#define DTMAINWIDGET_H

#include "TablesWidget.h"
#include <QPointer>

class DtTask;
class DtProgressDialog;
class DtFragsTab;
class QMutex;

class DtMainWidget : public DtTablesWidget {
    Q_OBJECT
public:
    DtMainWidget( QWidget* parent = 0 );
    ~DtMainWidget();

    QPointer< DtDirTree > dirTree;
    QPointer< DtFragsTab > fragsTab;

    void indexDemos();
    void saveSplitterSizes();
    void updateIfNeeded();

public slots:
    void insertDirTree();
    void showFindFragsWidget();
    void hideFindFragsWidget();
    void setDirTreeOpened( bool opened );
    void setFragsWidgetVisible( bool visible );
    void demoParseError();
    void stopParse();

protected:
    DtProgressDialog* progressDialog;
    DtTask* task;
    QMutex* loadMutex;
    QStringList demoFiles;
    QTime* listLoadTime;
    bool stopParseHeaders;
    bool indexing;

    void loadDemo( int index );
    void progressWaitFunc();
    void updateCentralSplitterSize();

private slots:
    void updateProgress();

signals:
    void newDemoLoaded();
    void demoGamestateParsed( DtDemo* );
    void demoInfoFetched( DtDemo* );
    void scanFinished();
};

#endif // DTMAINWIDGET_H
