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

#ifndef DTKEYBINDINGSDIALOG_H
#define DTKEYBINDINGSDIALOG_H

#include "EditorOptionsDialog.h"

#include <QWidgetAction>
#include <QHeaderView>

class DtBindsTable;
class DtCustomBindsTable;
class DtOptionsBindButton;
class DtBindAction;
class DtBindWidget;
class QTabWidget;
class QMenu;

class DtKeyBindingsDialog : public DtOptionsDialog {
    Q_OBJECT
public:
    DtKeyBindingsDialog( QWidget* parent = 0 );

    static QString getKeyName( int key );

protected:
    QTabWidget* tabs;
    DtBindsTable* bindsTable;
    DtCustomBindsTable* customBindsTable;
    DtOptionsSpinBox* soundVolumeStepSb;
    DtBindAction* bindAction;
    QMenu* bindMenu;
    DtOptionsBindButton* currentBindButton;
    DtOptionsButton* addActionButton;
    int lastKeyCode;
    bool keyboardGrabbed;
    QMap< int, int > playerKeys;
    QMap< int, int > playerAlternateKeys;
    QMap< int, QString > customKeyPressActions;
    QMap< int, QString > customKeyReleaseActions;
    bool newAction;

    void readKeyNames( int action, int& row );
    bool event( QEvent* e );
    bool eventFilter( QObject* obj, QEvent* e );
    void closeEvent( QCloseEvent* event );
    void addBindButtons( QTableWidget* table, int row, int from );
    int maxMapKey( const QMap< int, QString >& map );

public slots:
    void addCustomAction();
    void removeAction( int row );
    void updateAction( int row, int column, const QString& cmd );
    void deleteRow();

protected slots:
    void defaults();
    void readConfig();
    void writeConfig();
    void captureKeyPress();
    void captureEnd();
};

class DtBindButtonWidget : public QWidget {
    Q_OBJECT
public:
    DtBindButtonWidget( int rowNum, DtOptionsBindButton* cButton, QWidget* parent = 0 );

    DtOptionsBindButton* button();

protected:
    int row;
    DtOptionsBindButton* bindButton;

    void enterEvent( QEvent* );

signals:
    void mouseEntered( int );
};

class DtOptionsBindButton : public DtOptionsButton {
    Q_OBJECT
public:
    DtOptionsBindButton( int rowNum, int colNum, int actCode, QWidget* parent = 0 );

    void setKey( int nKey );
    void setAction( int nAct );
    int getKey() const;
    int getRow() const;
    int getColumn() const;
    int getAction() const;

protected:
    int row;
    int column;
    int action;
    DtOptionsBindButton* bindButton;

    void enterEvent( QEvent* );
    int key;

signals:
    void mouseEntered( int );
};

class DtBindsTable : public DtTable {
    Q_OBJECT
public:
    DtBindsTable( QWidget* parent = 0 );

    enum tableColumns {
        TC_ACTION,
        TC_KEY,
        TC_ALTKEY
    };

    QTableWidgetItem* editIndex;
    QAction* actDelete;
    QAction* actNew;

    DtOptionsBindButton* button( int row, int column );
    void clearHighlight();

protected:
    void leaveEvent( QEvent* );

protected slots:
    void onCellEntered( int, int );
    void onRowEntered( int );
};

class DtCustomBindsTable : public DtBindsTable {
    Q_OBJECT
public:
    DtCustomBindsTable( QWidget* parent = 0 );

    enum tableColumns {
        TC_KEYPRESS_ACTION,
        TC_KEYRELEASE_ACTION,
        TC_KEY,
        TC_ALTKEY
    };

protected:
    void commitData( QWidget* editor );
    void mouseDoubleClickEvent( QMouseEvent* e );
    void contextMenuEvent( QContextMenuEvent* e );

signals:
    void newAction( int, int, const QString& );
};

class DtBindsTableHeader : public QHeaderView {
    Q_OBJECT
public:
    DtBindsTableHeader( Qt::Orientation orientation, QWidget * parent = 0 );
    ~DtBindsTableHeader();

protected:
    QPixmap* keyDown;
    QPixmap* keyUp;

    void paintSection( QPainter * painter, const QRect & rect, int logicalIndex ) const;
};

class DtBindWidget : public QWidget {
    Q_OBJECT
public:
    DtBindWidget( QWidget* parent );
};

class DtBindAction : public QWidgetAction {
    Q_OBJECT
public:
    DtBindAction( QWidget* parent );

private:
    DtBindWidget* bindWidget;

protected:
    QWidget* createWidget( QWidget* parent );
};

#endif // DTKEYBINDINGSDIALOG_H
