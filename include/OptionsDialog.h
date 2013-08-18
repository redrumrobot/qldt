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

#ifndef DTOPTIONSDIALOG_H
#define DTOPTIONSDIALOG_H

#include "ClearLineEdit.h"

#include <QDialog>
#include <QPushButton>
#include <QButtonGroup>
#include <QLineEdit>
#include <QToolButton>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QSpinBox>

class QGroupBox;
class QLabel;

class DtPathEdit;
class DtOptionsButton;
class DtOptionsComboBox;
class DtOptionsCheckBox;
class DtOptionsLineEdit;
class DtOptionsClearLineEdit;
class DtOptionsSlider;
class DtOptionsSpinBox;
class QVBoxLayout;

class DtTabWidget;

class DtOptionsDialog : public QDialog {
    Q_OBJECT
public:
    DtOptionsDialog( QWidget* parent = 0 );

public slots:
    void setOptionsChanged();
    int exec();

protected:
    QVBoxLayout* mainLayout;
    DtOptionsButton* btnDefaults;
    DtOptionsButton* btnOK;
    DtOptionsButton* btnApply;
    DtOptionsButton* btnClose;

    bool optionsChanged;

protected slots:
    virtual void defaults() = 0;
    virtual void readConfig() = 0;
    virtual void writeConfig() = 0;
    void apply();
    void ok();
};

class DtPathEdit : public QWidget {
    Q_OBJECT
public:
    DtPathEdit( QString nTitle, QString nText, bool fileDlg, QWidget* parent = 0, int minWidth = 0 );

    void setPath( QString nText );
    QString getPath();
    void setTextFieldMinWidth( int width );
    void setFocus();

public slots:
    void showSelectPathDialog();

protected:
    QLabel* title;
    QLineEdit* textField;
    QToolButton* selectDirBtn;
    bool fileDialog;

    bool eventFilter( QObject* obj, QEvent* e );
};

class DtOptionsButton : public QPushButton {
    Q_OBJECT
public:
    DtOptionsButton( const QString& text, QWidget* parent = 0, const QIcon& icon = QIcon() );
};

class DtOptionsButtonGroup : public QButtonGroup {
    Q_OBJECT
public:
    DtOptionsButtonGroup( QWidget* parent = 0 );
};

class DtOptionsComboBox : public QComboBox {
    Q_OBJECT
public:
    DtOptionsComboBox( QWidget* parent = 0 );
};

class DtOptionsCheckBox : public QCheckBox {
    Q_OBJECT
public:
    DtOptionsCheckBox( QWidget* parent = 0 );
};

class DtOptionsLineEdit : public QLineEdit {
    Q_OBJECT
public:
    DtOptionsLineEdit( const QString& text, QWidget* parent = 0 );
};

class DtOptionsClearLineEdit : public DtClearLineEdit {
    Q_OBJECT
public:
    DtOptionsClearLineEdit( const QString& text, QWidget* parent = 0 );
};

class DtOptionsSlider : public QSlider {
    Q_OBJECT
public:
    DtOptionsSlider( Qt::Orientation o, QWidget* parent = 0 );
};

class DtOptionsSpinBox : public QSpinBox {
    Q_OBJECT
public:
    DtOptionsSpinBox( QWidget* parent = 0 );
};


#endif // DTOPTIONSDIALOG_H
