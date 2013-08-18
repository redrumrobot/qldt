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

#ifndef DTEDITOROPTIONSDIALOG_H
#define DTEDITOROPTIONSDIALOG_H

#include "OptionsDialog.h"
#include "Table.h"

#include <QStyledItemDelegate>

class DtTabWidget;
class DtColorsTable;
class DtOptionsColorButton;
class DtColorOptionsCheckBox;
class QTextCharFormat;
class DtEditorOptionsPage;
class QTabWidget;

class DtEditorOptionsDialog : public DtOptionsDialog {
    Q_OBJECT
public:
    DtEditorOptionsDialog( QWidget* parent = 0 );

protected:
    DtEditorOptionsPage* editorOptionsPage;

    void closeEvent( QCloseEvent* event );

protected slots:
    void defaults();
    void readConfig();
    void writeConfig();

signals:
    void settingsChanged();
};

class DtEditorOptionsPage : public QWidget {
    Q_OBJECT
public:
    DtEditorOptionsPage( QWidget* parent = 0 );

public slots:
    void setOptionsChanged();
    void colorButtonClicked();
    void readConfig();
    void writeConfig();

protected:
    DtOptionsComboBox* fontNameCombo;
    DtOptionsComboBox* fontSizeCombo;
    DtOptionsSpinBox* tabSizeSpinBox;
    DtColorsTable* colorsTable;
    DtOptionsColorButton* bgColorButton;
    DtOptionsColorButton* lineNumbersColorButton;
    DtOptionsColorButton* lineNumbersBgColorButton;
    DtOptionsColorButton* highlightLineColorButton;
    QTabWidget* tabs;
    DtOptionsCheckBox* useCustomFilesCb;
    DtPathEdit* customFilesPathEdit;

    void setWidgetsState( int& row, const QTextCharFormat& format );
    void storeWidgetsState( int& row, QTextCharFormat& format );

protected slots:
    void fontChanged( const QString& text );

signals:
    void settingsChanged();
};

class DtColorButtonWidget : public QWidget {
    Q_OBJECT
public:
    DtColorButtonWidget( int rowNum, DtOptionsColorButton* cButton, QWidget* parent = 0 );

    DtOptionsColorButton* button();

protected:
    int row;
    DtOptionsColorButton* colorButton;

    void enterEvent( QEvent* );

signals:
    void mouseEntered( int );
};

class DtOptionsColorButton : public DtOptionsButton {
    Q_OBJECT
public:
    DtOptionsColorButton( int rowNum, QWidget* parent = 0 );

    void setColor( const QColor& color );
    QColor getColor() const;

protected:
    int row;

    void paintEvent( QPaintEvent* );
    void enterEvent( QEvent* );
    QColor buttonColor;

signals:
    void mouseEntered( int );
};

class DtColorsTable : public DtTable {
    Q_OBJECT
public:
    DtColorsTable( QWidget* parent = 0 );

    enum tableColumns {
        TC_NAME,
        TC_BOLD,
        TC_ITALIC,
        TC_UNDERLINE,
        TC_COLOR
    };

    DtColorOptionsCheckBox* checkBox( int row, int column );
    DtOptionsColorButton* button( int row );

protected:
    void leaveEvent( QEvent* );

protected slots:
    void onCellEntered( int, int );
    void onRowEntered( int );

};

class DtColorTableDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    DtColorTableDelegate( QWidget* parent = 0 );
    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;

    void highlightRow( int row );

protected:
    int highlihgtedRowNum;
};

class DtColorOptionsCheckBox : public DtOptionsCheckBox {
    Q_OBJECT
public:
    DtColorOptionsCheckBox( int rowNum, QWidget* receiver, QWidget* parent = 0 );

protected:
    int row;

    void enterEvent( QEvent* );

signals:
    void mouseEntered( int );
};

#endif // DTEDITOROPTIONSDIALOG_H
