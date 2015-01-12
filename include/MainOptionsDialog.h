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

#ifndef DTMAINOPTIONSDIALOG_H
#define DTMAINOPTIONSDIALOG_H

#include "OptionsDialog.h"

#include <QPointer>
#include <QListWidget>
#include <QStyledItemDelegate>

class DtTabWidget;
class DtListWidget;
class QStackedLayout;
class DtEditorOptionsPage;
class QRadioButton;

class DtMainOptionsDialog : public DtOptionsDialog {
    Q_OBJECT
public:
    DtMainOptionsDialog( QWidget* parent = 0 );

public slots:
    void setOptionsChanged();

protected:
    DtListWidget* optionsList;
    QStackedLayout* mainStackedLayout;

    QTabWidget* playerTabs;

    QLabel* compressLbl;
    QLabel* compressFasterLbl;
    QLabel* compressBetterLbl;
    DtOptionsCheckBox* noCompressionCb;
    DtOptionsSlider* compressSlider;
    DtOptionsCheckBox* archRemovePathsCb;
    DtOptionsCheckBox* confirmDeleteCb;
    DtOptionsCheckBox* dirTreeOpenedCb;
    DtOptionsCheckBox* dropDemosToNewDirCb;
    DtOptionsLineEdit* tableAlternateColorFactorEdit;
    DtOptionsLineEdit* tableSelectionColorFactorEdit;

    DtOptionsCheckBox* qzFullscreenCb;
    DtOptionsCheckBox* qaFullscreenCb;
    DtOptionsComboBox* qzWindowedModeCombo;
    DtOptionsComboBox* qzFullscreenModeCombo;
    DtOptionsComboBox* qaWindowedModeCombo;
    DtOptionsComboBox* qaFullscreenModeCombo;
    DtOptionsComboBox* qzConfigCombo;
    DtOptionsComboBox* qaConfigCombo;
    DtOptionsComboBox* languageCombo;
    DtOptionsButton* qzBtnEditConfig;
    DtOptionsButton* qzBtnNewConfig;
    DtOptionsButton* qaBtnEditConfig;
    DtOptionsButton* qaBtnNewConfig;
    DtOptionsLineEdit* otherAppTitle;
    DtOptionsCheckBox* otherAppDm68Cb;
    DtOptionsCheckBox* otherAppDm73Cb;
    DtOptionsCheckBox* otherAppDblClickCb;
    DtOptionsCheckBox* otherAppMenuCb;
    DtOptionsCheckBox* otherAppPreviewCb;
    DtPathEdit* otherAppPathEdit;
    DtOptionsLineEdit* otherAppCmdLineEdit;
    DtOptionsButtonGroup* otherAppDemoPathBtnGroup;
    QRadioButton* otherAppFromDemosPathBtn;
    QRadioButton* otherAppAbsolutePathBtn;

    DtPathEdit* qzExePathEdit;
    DtPathEdit* qzFSBasePathEdit;
    DtPathEdit* qaPathEdit;
    DtPathEdit* qaHomePathEdit;

    DtOptionsCheckBox* editorShowGameTimeCb;
    DtOptionsCheckBox* editorShowLagsCb;
    DtOptionsLineEdit* addFragsTimeBeginEdit;
    DtOptionsLineEdit* addFragsTimeEndEdit;
    DtOptionsLineEdit* addTextTimeBeginEdit;
    DtOptionsLineEdit* addTextTimeEndEdit;
    DtOptionsLineEdit* timeDisplayFormatEdit;
    DtEditorOptionsPage* editorOptionsPage;

    bool editorConnected;
    bool playerConfigChanged;
    bool playerLoginDataChanged;

    void configsFromDir( QString path, QStringList& list );
    void closeEvent( QCloseEvent* event );
    void createAppearancePage();
    void createFilesPage();
    void createPlaybackPage();
    void createEditPage();
    void createTextEditorPage();
    void createPlayerTabQz();
    void createPlayerTabQa();
    void createPlayerTabOther();
    void updateConfigCb( DtOptionsComboBox* comboBox, const QString& dir );

private slots:
    void defaults();
    void readConfig();
    void writeConfig();
    void qzEditConfig();
    void qzNewConfig();
    void qaEditConfig();
    void qaNewConfig();
    void noCompressChecked( int state );
    void updateConfigComboBoxes();

signals:
    void configChanged();
    void playerWindowConfigChanged();
    void playerWindowLoginDataChanged();
};

class DtListWidget : public QListWidget {
    Q_OBJECT
public:
    DtListWidget( QWidget* parent = 0 );
};

class DtListWidgetDelegate : public QStyledItemDelegate {
public:
    DtListWidgetDelegate( QObject* parent );

    QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;
};

#endif // DTMAINOPTIONSDIALOG_H
