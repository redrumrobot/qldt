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

#ifndef DTFINDDEMODIALOG_H
#define DTFINDDEMODIALOG_H

#include <QDialog>
#include <QPointer>

class QButtonGroup;
class QRadioButton;
class QLineEdit;
class QCheckBox;
class QComboBox;
class QToolButton;
class QLabel;
class QDateEdit;
class QEventLoop;
class DtProgressDialog;
class DtTask;
class QMutex;

struct DtFindDemoOptions {
    int search;
    QString name;
    QString player;
    QString map;
    QString server;
    bool matchCase;
    int gameType;
    int mod;
    int dateFrom;
    int dateTo;
    bool allDirs;
};

class DtFindDemoDialog : public QDialog {
    Q_OBJECT
public:
    DtFindDemoDialog( QWidget* parent = 0 );
    ~DtFindDemoDialog();

    DtFindDemoOptions exec();

protected:
    QLabel* gameTypeLbl;
    QLabel* q3ModLbl;
    QPointer< QEventLoop > pEventLoop;
    QCheckBox* allDirCb;
    QLineEdit* nameEdit;
    QLineEdit* playerEdit;
    QLineEdit* mapEdit;
    QLineEdit* serverEdit;
    QCheckBox* matchCaseCb;
    QComboBox* gameTypeCombo;
    QComboBox* q3ModCombo;
    QDateEdit* dateFromEdit;
    QDateEdit* dateToEdit;
    DtFindDemoOptions opt;
    QStringList demoFiles;
    bool stopParseHeaders;
    DtProgressDialog* progressDialog;
    DtTask* task;
    QMutex* loadMutex;
    QString currentDir;
    QString baseDir;
    QString currentDirName;

    void showEvent( QShowEvent* );
    void closeEvent( QCloseEvent* );
    void indexAllDirs();
    void indexDirectory( QString dir );
    void loadDemo( int index );
    void progressWaitFunc();

protected slots:
    void startSearch();
    void cancelPressed();
    void updateWidgetsVisibility();
    void updateProgress();
    void stopParse();
    void demoParseError();

signals:
    void newDemoLoaded();

};

#endif // DTFINDDEMODIALOG_H
