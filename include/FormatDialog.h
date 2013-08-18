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

#ifndef DTFORMATDIALOG_H
#define DTFORMATDIALOG_H

#include <QDialog>

class QLabel;
class QPushButton;
class QEventLoop;
class QLineEdit;

class DtFormatDialog : public QDialog {
    Q_OBJECT
public:
    DtFormatDialog( QString title, QString& defFormat, QString help, QWidget* parent = 0 );

    enum dialogButtons {
        BTN_CANCEL,
        BTN_OK
    };

    dialogButtons exec();

protected:
    QPushButton* okButton;
    QPushButton* cancelButton;
    QEventLoop* pEventLoop;
    QLineEdit* formatEdit;
    QString& defaultFormat;

    dialogButtons ret;

    void rDone( dialogButtons retc = BTN_CANCEL );

protected slots:
    void okPressed();
    void cancelPressed();
};

#endif // DTFORMATDIALOG_H
