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

#ifndef DTEDITTAB_H
#define DTEDITTAB_H

#include "Table.h"
#include "DemoData.h"

#include <QWidget>
#include <QTimeEdit>

class DtDemo;
class DtDemoPartsWidget;
class QGroupBox;
class QLabel;
class QPushButton;
class QDoubleSpinBox;
class QCheckBox;
class DtTable;
class DtChatTable;
class QButtonGroup;
class DtCommandsWidget;
class QToolButton;
class DtTimeEdit;

class DtEditTab : public QWidget {
    Q_OBJECT
public:
    DtEditTab( QWidget* parent );
    ~DtEditTab();

    bool startEdit( DtDemo* editedDemo );
    void stopEdit();
    DtWriteOptions getOptions() const;
    DtDemo* currentDemo() const;

public slots:
    void updateCursorInfo( int partStart, int pos );
    void partsChanged();
    void onPartBorderSelected( int index );
    void onPartBorderUnselected();

protected:
    DtDemo* demo;
    DtDemoPartsWidget* demoParts;
    QGroupBox* demoInfoGroup;
    QLabel* protocolLbl;
    QLabel* sizeLbl;
    QLabel* lengthLbl;
    QLabel* framesLbl;
    QLabel* frameDurationLbl;
    QLabel* warmupDurationLbl;
    QLabel* mapLbl;
    QLabel* gameTypeLbl;
    QLabel* gameStateLbl;
    QLabel* q3ModTextLbl;
    QLabel* q3ModLbl;
    QToolButton* clearPartsButton;
    DtTimeEdit* partTimeSb;
    QLabel* cursorTimeLbl;
    QLabel* partDurationLbl;
    QCheckBox* removeWarmupCb;
    QCheckBox* removePausesCb;
    QCheckBox* removeLagsCb;
    DtTimeEdit* demoStartTimeSb;
    QPushButton* previewButton;
    QPushButton* saveButton;
    QPushButton* saveAsButton;
    QPushButton* cancelButton;
    DtChatTable* chatTable;
    DtCommandsWidget* commandsWidget;
    QButtonGroup* selectorModeGroup;
    bool partSbManual;
    int frameTime;
    int demoLength;
    int warmupLength;
    QString lastPreviewPath;
    quint32 lastPreviewModifiedTime;
    int initialTimerValMs;
    double initialTimerVal;
    bool optionsChanged;

    bool demoModified();
    QTime msecToTime( int msec );
    void saveChat( QString fName );

protected slots:
    void preview();
    void save();
    void saveAs();
    void cancelChanges();
    void partSbTimeChanged( const QTime& t );
    void setOptionsCnaged();

signals:
    void resizePart( int );
};

class DtTimeEdit : public QTimeEdit {
    Q_OBJECT
public:
    DtTimeEdit( QWidget* parent = 0 );

    int msec();
};

class DtChatTable : public DtTable {
    Q_OBJECT
public:
    DtChatTable( QWidget* parent = 0 );

    enum tableColumns {
        TC_TIME,
        TC_CLIENTNUM,
        TC_CMD,
        TC_NAME,
        TC_STR1,
        TC_STR2
    };

    void setStrings( const QVector< QPair< int, QString > >& chatStrings );
    QVector< QPair< int, QString > > getStrings() const;
    bool isChanged();
    void setDemo( DtDemo* sDemo );

protected:
    DtDemo* demo;
    QAction* actNew;
    QAction* actDelete;
    bool changed;
    bool cpmaColumn;

    void contextMenuEvent( QContextMenuEvent* e );

protected slots:
    void deleteSelectedStrings();
    void insertChatString();
    void commitData( QWidget* editorWidget );
};

#endif // DTEDITTAB_H
