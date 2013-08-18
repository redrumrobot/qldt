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

#ifndef DTPROGRESSDIALOG_H
#define DTPROGRESSDIALOG_H

#include <QDialog>

class QProgressBar;
class QLabel;

class DtProgressDialog : public QDialog {
    Q_OBJECT
public:
    enum dialogButtons {
        NoButton,
        OkButton,
        CancelButton
    };

    DtProgressDialog( const QString& title, QWidget* parent = 0 );

    void start();
    void setData( const QString& lbl, int p = 0, dialogButtons btn = NoButton );

public slots:
    void setPos( int p = 0 );

private:
    bool done;

    QProgressBar* pb;
    QLabel* infoLabel;
    QPushButton* mButton;

    QString okBtnText;
    QString cancelBtnText;

private slots:
    void mButtonClicked();

signals:
    void buttonClicked();
};

#endif // DTPROGRESSDIALOG_H
