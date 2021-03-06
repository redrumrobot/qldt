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

#ifndef DTFINDTEXTDIALOG_H
#define DTFINDTEXTDIALOG_H

#include <QPointer>
#include <QDialog>

class QCheckBox;
class QLineEdit;
class QEventLoop;

class DtFindTextDialog : public QDialog {
    Q_OBJECT
public:
    DtFindTextDialog( QWidget* parent = 0 );

    bool exec();

protected:
    QCheckBox* ignoreColorsCb;
    QCheckBox* matchCaseCb;
    QLineEdit* searchStringEdit;
    QPointer< QEventLoop > pEventLoop;
    bool ret;

    void closeEvent( QCloseEvent* );
    void showEvent( QShowEvent* );

protected slots:
    void startSearch();
};

#endif // DTFINDTEXTDIALOG_H
