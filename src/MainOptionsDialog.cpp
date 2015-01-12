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

#include "MainOptionsDialog.h"
#include "Data.h"
#include "PlayerData.h"
#include "TabWidget.h"
#include "EditorOptionsDialog.h"
#include "MainWindow.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QSettings>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QStackedLayout>
#include <QCloseEvent>
#include <QDirIterator>
#include <QRadioButton>

using namespace dtdata;

DtMainOptionsDialog::DtMainOptionsDialog( QWidget* parent ) : DtOptionsDialog( parent ) {
    setWindowTitle( tr( "Preferences" ) );
    setWindowModality( Qt::WindowModal );

    QSize defaultSize( 1024, 768 );
    setMinimumSize( defaultSize );
    resize( config.settings->value( "OptionsDialog/size", defaultSize ).toSize() );

    mainStackedLayout = new QStackedLayout;
    optionsChanged = true;

    createAppearancePage();
    createFilesPage();
    createPlaybackPage();
    createEditPage();
    createTextEditorPage();

    QHBoxLayout* listLayout = new QHBoxLayout;
    optionsList = new DtListWidget( this );
    connect( optionsList, SIGNAL( currentRowChanged( int ) ),
             mainStackedLayout, SLOT( setCurrentIndex( int ) ) );

    QStringList options;
    options << tr( "Appearance" ) << tr( "Files" ) << tr( "Playback" ) << tr( "Edit", "Tabs|Edit" )
            << tr( "Text editor" );

    optionsList->insertItems( 0, options );
    optionsList->item( 0 )->setIcon( QIcon( ":/res/options-appearance.png" ) );
    optionsList->item( 1 )->setIcon( QIcon( ":/res/options-files.png" ) );
    optionsList->item( 2 )->setIcon( QIcon( ":/res/options-playback.png" ) );
    optionsList->item( 3 )->setIcon( QIcon( ":/res/options-edit.png" ) );
    optionsList->item( 4 )->setIcon( QIcon( ":/res/options-texteditor.png" ) );
    optionsList->setCurrentRow( config.settings->value( "OptionsDialog/currentPage", 0 ).toInt() );

    listLayout->addWidget( optionsList );
    listLayout->addLayout( mainStackedLayout );

    mainLayout->insertLayout( 0, listLayout );

    editorConnected = false;
}

void DtMainOptionsDialog::noCompressChecked( int state ) {
    compressSlider->setDisabled( state );
    compressLbl->setDisabled( state );
    compressFasterLbl->setDisabled( state );
    compressBetterLbl->setDisabled( state );
}

void DtMainOptionsDialog::createAppearancePage() {
    QWidget* appearancePage = new QWidget( this );
    mainStackedLayout->addWidget( appearancePage );

    QVBoxLayout* appearanceLayout = new QVBoxLayout;
    QGridLayout* interfaceLayout = new QGridLayout;
    QGroupBox* interfaceGroup = new QGroupBox( tr( "User interface" ) );

    QLabel* languageLbl = new QLabel( tr( "Language" ) );
    languageCombo = new DtOptionsComboBox( this );

    languageCombo->addItems( defaultLanguageNames );

    interfaceLayout->addWidget( languageLbl, 0, 0, Qt::AlignLeft );
    interfaceLayout->addWidget( languageCombo, 0, 1, Qt::AlignLeft );

    interfaceLayout->setColumnStretch( interfaceLayout->columnCount() + 1, 1 );
    interfaceLayout->setRowStretch( interfaceLayout->rowCount() + 1, 1 );

    interfaceGroup->setLayout( interfaceLayout );

    QGroupBox* colorsGroup = new QGroupBox( tr( "Colors" ) );
    QGridLayout* colorsLayout = new QGridLayout;

    QLabel* tableAlternateColorFactorLbl = new QLabel( tr( "Table alternate row color factor" ) );
    tableAlternateColorFactorEdit = new DtOptionsLineEdit( "", this );
    tableAlternateColorFactorEdit->setFixedWidth( 50 );
    tableAlternateColorFactorEdit->setMaxLength( 4 );
    QIntValidator* factorValidator = new QIntValidator( 0, 9999, this );
    tableAlternateColorFactorEdit->setValidator( factorValidator );

    QLabel* tableSelectionColorFactorLbl = new QLabel( tr( "Table selection color factor" ) );
    tableSelectionColorFactorEdit = new DtOptionsLineEdit( "", this );
    tableSelectionColorFactorEdit->setFixedWidth( 50 );
    tableSelectionColorFactorEdit->setMaxLength( 4 );
    tableSelectionColorFactorEdit->setValidator( factorValidator );

    colorsLayout->addWidget( tableAlternateColorFactorLbl, 1, 0, Qt::AlignLeft );
    colorsLayout->addWidget( tableAlternateColorFactorEdit, 1, 1, Qt::AlignLeft );
    colorsLayout->addWidget( tableSelectionColorFactorLbl, 2, 0, Qt::AlignLeft );
    colorsLayout->addWidget( tableSelectionColorFactorEdit, 2, 1, Qt::AlignLeft );

    colorsLayout->setColumnStretch( colorsLayout->columnCount() + 1, 1 );
    colorsLayout->setRowStretch( colorsLayout->rowCount() + 1, 1 );

    colorsGroup->setLayout( colorsLayout );

    appearanceLayout->addWidget( interfaceGroup );
    appearanceLayout->addWidget( colorsGroup );
    appearanceLayout->addStretch( 1 );
    appearancePage->setLayout( appearanceLayout );
}

void DtMainOptionsDialog::createFilesPage() {
    QWidget* filesPage = new QWidget( this );
    mainStackedLayout->addWidget( filesPage );

    QVBoxLayout* filesLayout = new QVBoxLayout;
    QVBoxLayout* pathsLayout = new QVBoxLayout;
    QGroupBox* pathsGroup = new QGroupBox( tr( "Paths and directories" ) );

    qzExePathEdit = new DtPathEdit( "Quake Live exe", "", true, this, 140 );
    qzFSBasePathEdit = new DtPathEdit( "Quake Live basepath", "", false, this, 140 );
    qaPathEdit = new DtPathEdit( "Quake Arena exe", "", true, this, 140 );
    qaHomePathEdit = new DtPathEdit( "Quake Arena home", "", false, this, 140 );

    QGridLayout* cbLayout = new QGridLayout;
    QLabel* dirTreeOpenedLbl = new QLabel( tr( "Keep directory tree always opened" ) );
    dirTreeOpenedCb = new DtOptionsCheckBox( this );
    cbLayout->addWidget( dirTreeOpenedLbl, 0, 0, Qt::AlignLeft );
    cbLayout->addWidget( dirTreeOpenedCb, 0, 1, Qt::AlignLeft );

    QLabel* dropDemosToNewDirLbl = new QLabel( tr( "Put new demos to the temporary directory" ) );
    dropDemosToNewDirCb = new DtOptionsCheckBox( this );
    cbLayout->addWidget( dropDemosToNewDirLbl, 1, 0, Qt::AlignLeft );
    cbLayout->addWidget( dropDemosToNewDirCb, 1, 1, Qt::AlignLeft );

    cbLayout->setColumnStretch( cbLayout->columnCount() + 1, 1 );
    cbLayout->setRowStretch( cbLayout->rowCount() + 1, 1 );

    pathsLayout->addWidget( qzExePathEdit );
    pathsLayout->addWidget( qzFSBasePathEdit );
    pathsLayout->addWidget( qaPathEdit );
    pathsLayout->addWidget( qaHomePathEdit );
    pathsLayout->addSpacing( 10 );
    pathsLayout->addLayout( cbLayout );

    pathsGroup->setLayout( pathsLayout );
    filesLayout->addWidget( pathsGroup );

    QGridLayout* archLayout = new QGridLayout;
    QGridLayout* archCbLayout = new QGridLayout;
    QGroupBox* archGroup = new QGroupBox( tr( "Archiver" ) );

    QLabel* noCompressionLbl = new QLabel( tr( "No compression, store only" ) );
    noCompressionCb = new DtOptionsCheckBox( this );
    connect( noCompressionCb, SIGNAL( stateChanged( int ) ), this, SLOT( noCompressChecked( int ) ) );

    QHBoxLayout* noCompressLayout = new QHBoxLayout;
    noCompressLayout->setMargin( 0 );
    noCompressLayout->setSpacing( 0 );
    noCompressLayout->addSpacing( 10 );
    noCompressLayout->addWidget( noCompressionCb, 1, Qt::AlignLeft );
    noCompressLayout->addWidget( noCompressionLbl, 1, Qt::AlignLeft );
    noCompressLayout->addStretch( 2 );

    compressLbl = new QLabel( tr( "Compress" ) );
    compressFasterLbl = new QLabel( tr( "faster" ) );
    compressBetterLbl = new QLabel( tr( "better" ) );

    compressSlider = new DtOptionsSlider( Qt::Horizontal, this );
    compressSlider->setMinimum( 1 );
    compressSlider->setMaximum( 9 );
    compressSlider->setMinimumWidth( 150 );
    compressSlider->setTickPosition( QSlider::TicksAbove );
    compressSlider->setTickInterval( 1 );

    QHBoxLayout* compressLayout = new QHBoxLayout;

    compressLayout->addSpacing( 10 );
    compressLayout->addWidget( compressFasterLbl );
    compressLayout->addWidget( compressSlider );
    compressLayout->addWidget( compressBetterLbl );

    QLabel* archRemovePathsLbl = new QLabel( tr( "Save directory structure in unpacked archives" ) );
    archRemovePathsCb = new DtOptionsCheckBox( this );

    archLayout->addWidget( compressLbl, 0, 0, Qt::AlignLeft );
    archLayout->addLayout( compressLayout, 0, 1, 1, 1, Qt::AlignLeft );
    archLayout->addLayout( noCompressLayout, 1, 1, 1, 1, Qt::AlignLeft );
    archLayout->setColumnStretch( archLayout->columnCount() + 1, 1 );
    archLayout->setRowStretch( archLayout->rowCount() + 1, 1 );

    archCbLayout->addWidget( archRemovePathsLbl, 0, 0, Qt::AlignLeft );
    archCbLayout->addWidget( archRemovePathsCb, 0, 1, Qt::AlignLeft );
    archCbLayout->setColumnStretch( archLayout->columnCount() + 1, 1 );
    archCbLayout->setRowStretch( archLayout->rowCount() + 1, 1 );

    QVBoxLayout* archVBox = new QVBoxLayout;
    archVBox->addLayout( archLayout );
    archVBox->addSpacing( 20 );
    archVBox->addLayout( archCbLayout );

    archGroup->setLayout( archVBox );
    filesLayout->addWidget( archGroup );

    QGridLayout* demoListLayout = new QGridLayout;
    QGroupBox* demoListGroup = new QGroupBox( tr( "Demo tables" ) );

    QLabel* confirmDeleteLbl = new QLabel( tr( "Delete demos without confirmation" ) );
    confirmDeleteCb = new DtOptionsCheckBox( this );

    demoListLayout->addWidget( confirmDeleteLbl, 0, 0, Qt::AlignLeft );
    demoListLayout->addWidget( confirmDeleteCb, 0, 1, Qt::AlignLeft );
    demoListLayout->setColumnStretch( archLayout->columnCount() + 1, 1 );
    demoListLayout->setRowStretch( archLayout->rowCount() + 1, 1 );

    demoListGroup->setLayout( demoListLayout );
    filesLayout->addWidget( demoListGroup );

    filesLayout->addStretch( 1 );
    filesPage->setLayout( filesLayout );
}

void DtMainOptionsDialog::createPlaybackPage() {
    QWidget* playerPage = new QWidget( this );
    mainStackedLayout->addWidget( playerPage );

    playerTabs = new DtTabWidget;

    createPlayerTabQz();
    createPlayerTabQa();
    createPlayerTabOther();

    QVBoxLayout* playerLayout = new QVBoxLayout;
    playerLayout->addWidget( playerTabs );

    for ( int i = 0; i < qzModes.count(); ++i ) {
        QString qzModeStr = QString( "%1x%2" ).arg( qzModes.at( i ).width() )
                                              .arg( qzModes.at( i ).height() );

        qzFullscreenModeCombo->addItem( qzModeStr );
        qzWindowedModeCombo->addItem( qzModeStr, i );

        if ( i < qaModes.count() ) {
            QString qaModeStr = QString( "%1x%2" ).arg( qaModes.at( i ).width() )
                                                  .arg( qaModes.at( i ).height() );

            qaFullscreenModeCombo->addItem( qaModeStr );

            if ( i == QA_640x480 || i == QA_800x600 || i == QA_1024x768 ) {
                qaWindowedModeCombo->addItem( qaModeStr, i );
            }
        }
    }

    playerPage->setLayout( playerLayout );
}

void DtMainOptionsDialog::createEditPage() {
    QWidget* editPage = new QWidget( this );
    mainStackedLayout->addWidget( editPage );

    QVBoxLayout* pageLayout = new QVBoxLayout;
    QGridLayout* editLayout = new QGridLayout;
    QGroupBox* editGroup = new QGroupBox;

    QLabel* editorShowGameTimeLbl = new QLabel( tr( "Show server time in seconds" ) );
    editorShowGameTimeCb = new DtOptionsCheckBox( this );

    QLabel* editorShowLagsLbl = new QLabel( tr( "Show lags" ) );
    editorShowLagsCb = new DtOptionsCheckBox( this );

    QLabel* timeDisplayFormatLbl = new QLabel( tr( "Time format" ) );
    timeDisplayFormatEdit = new DtOptionsLineEdit( "", this );

    editLayout->addWidget( editorShowGameTimeLbl, 0, 0, Qt::AlignLeft );
    editLayout->addWidget( editorShowGameTimeCb, 0, 1, Qt::AlignLeft  );
    editLayout->addWidget( editorShowLagsLbl, 1, 0, Qt::AlignLeft );
    editLayout->addWidget( editorShowLagsCb, 1, 1, Qt::AlignLeft  );
    editLayout->addWidget( timeDisplayFormatLbl, 2, 0, Qt::AlignLeft );
    editLayout->addWidget( timeDisplayFormatEdit, 2, 1, Qt::AlignLeft  );

    editLayout->setColumnStretch( editLayout->columnCount() + 1, 1 );

    editGroup->setLayout( editLayout );

    pageLayout->addWidget( editGroup );
    pageLayout->addSpacing( 10 );

    QGroupBox* findFragsGroup = new QGroupBox( tr( "Find frags" ) );
    QGridLayout* findFragsLayout = new QGridLayout;

    QLabel* addFragsTimeLbl = new QLabel( tr( "Demo segments boundaries extension, sec." ) + ":" );
    addFragsTimeBeginEdit = new DtOptionsLineEdit( "", this );
    addFragsTimeBeginEdit->setValidator( new QIntValidator( this ) );
    addFragsTimeBeginEdit->setMaxLength( 2 );
    addFragsTimeBeginEdit->setFixedWidth( 50 );

    addFragsTimeEndEdit = new DtOptionsLineEdit( "", this );
    addFragsTimeEndEdit->setValidator( new QIntValidator( this ) );
    addFragsTimeEndEdit->setMaxLength( 2 );
    addFragsTimeEndEdit->setFixedWidth( 50 );

    QLabel* addTextBeginLbl = new QLabel( tr( "begin" ) );
    QLabel* addTextEndLbl = new QLabel( tr( "end" ) );

    findFragsLayout->addWidget( addFragsTimeLbl, 0, 0, Qt::AlignLeft );
    findFragsLayout->addItem( new QSpacerItem( 10, 0 ), 0, 1 );
    findFragsLayout->addWidget( addTextBeginLbl, 0, 2, Qt::AlignLeft );
    findFragsLayout->addWidget( addFragsTimeBeginEdit, 0, 3, Qt::AlignLeft );
    findFragsLayout->addItem( new QSpacerItem( 10, 0 ), 0, 4 );
    findFragsLayout->addWidget( addTextEndLbl, 0, 5, Qt::AlignLeft );
    findFragsLayout->addWidget( addFragsTimeEndEdit, 0, 6, Qt::AlignLeft );

    findFragsLayout->setColumnStretch( findFragsLayout->columnCount() + 1, 1 );
    findFragsLayout->setRowStretch( findFragsLayout->rowCount() + 1, 1 );

    findFragsGroup->setLayout( findFragsLayout );

    pageLayout->addWidget( findFragsGroup );

    QGroupBox* findTextGroup = new QGroupBox( tr( "Find chat messages" ) );
    QGridLayout* findTextLayout = new QGridLayout;

    QLabel* addTextTimeLbl = new QLabel( tr( "Demo segments boundaries extension, sec." ) + ":" );
    addTextTimeBeginEdit = new DtOptionsLineEdit( "", this );
    addTextTimeBeginEdit->setValidator( new QIntValidator( this ) );
    addTextTimeBeginEdit->setMaxLength( 2 );
    addTextTimeBeginEdit->setFixedWidth( 50 );

    addTextTimeEndEdit = new DtOptionsLineEdit( "", this );
    addTextTimeEndEdit->setValidator( new QIntValidator( this ) );
    addTextTimeEndEdit->setMaxLength( 2 );
    addTextTimeEndEdit->setFixedWidth( 50 );

    QLabel* addTimeBeginLbl = new QLabel( tr( "begin" ) );
    QLabel* addTimeEndLbl = new QLabel( tr( "end" ) );

    findTextLayout->addWidget( addTextTimeLbl, 0, 0, Qt::AlignLeft );
    findTextLayout->addItem( new QSpacerItem( 10, 0 ), 0, 1 );
    findTextLayout->addWidget( addTimeBeginLbl, 0, 2, Qt::AlignLeft );
    findTextLayout->addWidget( addTextTimeBeginEdit, 0, 3, Qt::AlignLeft );
    findTextLayout->addItem( new QSpacerItem( 10, 0 ), 0, 4 );
    findTextLayout->addWidget( addTimeEndLbl, 0, 5, Qt::AlignLeft );
    findTextLayout->addWidget( addTextTimeEndEdit, 0, 6, Qt::AlignLeft );

    findTextLayout->setColumnStretch( findTextLayout->columnCount() + 1, 1 );
    findTextLayout->setRowStretch( findTextLayout->rowCount() + 1, 1 );

    findTextGroup->setLayout( findTextLayout );

    pageLayout->addWidget( findTextGroup );

    pageLayout->addStretch( 1 );

    editPage->setLayout( pageLayout );
}

void DtMainOptionsDialog::createTextEditorPage() {
    editorOptionsPage = new DtEditorOptionsPage( this );
    mainStackedLayout->addWidget( editorOptionsPage );
}

void DtMainOptionsDialog::updateConfigComboBoxes() {
    updateConfigCb( qzConfigCombo, config.getQzBasePath() );
    updateConfigCb( qaConfigCombo, config.getQaBasePath() );
}

void DtMainOptionsDialog::createPlayerTabQz() {
    QGridLayout* qzPlayerLayout = new QGridLayout;
    int row = 0;

    QLabel* qzWindowedModeLbl = new QLabel( tr( "Windowed Resolution" ) );
    qzWindowedModeCombo = new DtOptionsComboBox( this );
    qzWindowedModeCombo->setFixedWidth( 120 );
    qzWindowedModeCombo->setObjectName( "qzWindowedResolutions" );
    qzPlayerLayout->addWidget( qzWindowedModeLbl, row, 0, Qt::AlignLeft );
    qzPlayerLayout->addWidget( qzWindowedModeCombo, row++, 1, Qt::AlignLeft );

    QLabel* qzFullscreenModeLbl = new QLabel( tr( "Full Screen Resolution" ) );
    qzFullscreenModeCombo = new DtOptionsComboBox( this );
    qzFullscreenModeCombo->setFixedWidth( 120 );
    qzPlayerLayout->addWidget( qzFullscreenModeLbl, row, 0, Qt::AlignLeft );
    qzPlayerLayout->addWidget( qzFullscreenModeCombo, row++, 1, Qt::AlignLeft );

    QLabel* qzFullscreenLbl = new QLabel( tr( "Full Screen Mode" ) );
    qzFullscreenCb = new DtOptionsCheckBox( this );
    qzPlayerLayout->addWidget( qzFullscreenLbl, row, 0, Qt::AlignLeft );
    qzPlayerLayout->addWidget( qzFullscreenCb, row++, 1, Qt::AlignLeft );

    QLabel* qzConfigLbl = new QLabel( tr( "Config" ) );
    qzConfigCombo = new DtOptionsComboBox( this );

    updateConfigCb( qzConfigCombo, config.getQzBasePath() );

    qzBtnEditConfig = new DtOptionsButton( tr( "Edit" ), this );
    qzBtnEditConfig->setIcon( QIcon( ":/res/text-editor.png" ) );
    connect( qzBtnEditConfig, SIGNAL( clicked() ), this, SLOT( qzEditConfig() ) );
    qzBtnNewConfig = new DtOptionsButton( tr( "New" ), this );
    qzBtnNewConfig->setIcon( QIcon( ":/res/document-new.png" ) );
    connect( qzBtnNewConfig, SIGNAL( clicked() ), this, SLOT( qzNewConfig() ) );

    qzPlayerLayout->addWidget( qzConfigLbl, row, 0, Qt::AlignLeft );
    qzPlayerLayout->addWidget( qzConfigCombo, row, 1, Qt::AlignLeft );
    qzPlayerLayout->addWidget( qzBtnEditConfig, row, 2, Qt::AlignLeft );
    qzPlayerLayout->addWidget( qzBtnNewConfig, row++, 3, Qt::AlignLeft );

    qzPlayerLayout->setColumnStretch( qzPlayerLayout->columnCount() + 1, 1 );
    qzPlayerLayout->setRowStretch( qzPlayerLayout->rowCount() + 1, 1 );

    QWidget* qzTab = new QWidget;
    qzTab->setContentsMargins( 8, 8, 2, 2 );
    qzTab->setLayout( qzPlayerLayout );

    playerTabs->addTab( qzTab, icons->getIcon( I_QZ_SMALL ), "Quake Live" );
}

void DtMainOptionsDialog::createPlayerTabOther() {
    QGridLayout* otherAppLayout = new QGridLayout;
    int row = 0;

    QLabel* otherAppTitleLbl = new QLabel( tr( "Title" ) );
    otherAppTitle = new DtOptionsLineEdit( "", this );
    otherAppLayout->addWidget( otherAppTitleLbl, row, 0, Qt::AlignLeft );
    otherAppLayout->addWidget( otherAppTitle, row++, 1, Qt::AlignLeft );

    int minWidth = 90;
    QLabel* extensionsLbl = new QLabel( tr( "Can play" ) );
    QHBoxLayout* extensionsLayout = new QHBoxLayout;
    QLabel* otherAppDm68Lbl = new QLabel( "Q3A demos" );
    otherAppDm68Lbl->setMinimumWidth( minWidth );
    otherAppDm68Cb = new DtOptionsCheckBox( this );
    QLabel* otherAppDm73Lbl = new QLabel( "QL demos" );
    otherAppDm73Lbl->setMinimumWidth( minWidth );
    otherAppDm73Cb = new DtOptionsCheckBox( this );
    extensionsLayout->addWidget( otherAppDm68Cb );
    extensionsLayout->addWidget( otherAppDm68Lbl );
    extensionsLayout->addSpacing( 15 );
    extensionsLayout->addWidget( otherAppDm73Cb );
    extensionsLayout->addWidget( otherAppDm73Lbl );
    otherAppLayout->addWidget( extensionsLbl, row, 0, Qt::AlignLeft );
    otherAppLayout->addLayout( extensionsLayout, row++, 1, Qt::AlignLeft );

    QLabel* useInLbl = new QLabel( tr( "Use in" ) );
    QHBoxLayout* useInLayout = new QHBoxLayout;
    QLabel* otherAppDblClickLbl = new QLabel( tr( "double click" ) );
    otherAppDblClickLbl->setMinimumWidth( minWidth );
    otherAppDblClickCb = new DtOptionsCheckBox( this );
    QLabel* otherAppMenuLbl = new QLabel( tr( "context menu" ) );
    otherAppMenuLbl->setMinimumWidth( minWidth );
    otherAppMenuCb = new DtOptionsCheckBox( this );
    QLabel* otherAppPreviewLbl = new QLabel( tr( "preview" ) );
    otherAppPreviewLbl->setMinimumWidth( minWidth );
    otherAppPreviewCb = new DtOptionsCheckBox( this );
    useInLayout->addWidget( otherAppDblClickCb );
    useInLayout->addWidget( otherAppDblClickLbl );
    useInLayout->addSpacing( 15 );
    useInLayout->addWidget( otherAppMenuCb );
    useInLayout->addWidget( otherAppMenuLbl );
    useInLayout->addSpacing( 15 );
    useInLayout->addWidget( otherAppPreviewCb );
    useInLayout->addWidget( otherAppPreviewLbl );
    otherAppLayout->addWidget( useInLbl, row, 0, Qt::AlignLeft );
    otherAppLayout->addLayout( useInLayout, row++, 1, Qt::AlignLeft );

    QLabel* otherAppPathLbl = new QLabel( tr( "Path to file" ) );
    otherAppPathEdit = new DtPathEdit( "", "", true, this );
    otherAppLayout->addWidget( otherAppPathLbl, row, 0, Qt::AlignLeft );
    otherAppLayout->addWidget( otherAppPathEdit, row++, 1, Qt::AlignLeft );

    QLabel* otherAppCmdLineLbl = new QLabel( tr( "Command line" ) );
    otherAppCmdLineEdit = new DtOptionsLineEdit( "", this );
    otherAppCmdLineEdit->setMinimumWidth( 400 );
    otherAppLayout->addWidget( otherAppCmdLineLbl, row, 0, Qt::AlignLeft );
    otherAppLayout->addWidget( otherAppCmdLineEdit, row++, 1, Qt::AlignLeft );

    QLabel* otherAppDemoPathLbl = new QLabel( tr( "Demo path" ) );
    otherAppDemoPathBtnGroup = new DtOptionsButtonGroup( this );
    QHBoxLayout* otherAppDemoPathLayout = new QHBoxLayout;
    otherAppFromDemosPathBtn = new QRadioButton( tr( "from demos directory" ), this );
    otherAppAbsolutePathBtn = new QRadioButton( tr( "absolute" ), this );
    otherAppDemoPathLayout->addWidget( otherAppFromDemosPathBtn );
    otherAppDemoPathLayout->addSpacing( 15 );
    otherAppDemoPathLayout->addWidget( otherAppAbsolutePathBtn );
    otherAppDemoPathBtnGroup->addButton( otherAppFromDemosPathBtn, 0 );
    otherAppDemoPathBtnGroup->addButton( otherAppAbsolutePathBtn, 1 );
    otherAppLayout->addWidget( otherAppDemoPathLbl, row, 0, Qt::AlignLeft );
    otherAppLayout->addLayout( otherAppDemoPathLayout, row++, 1, Qt::AlignLeft );

    otherAppLayout->setColumnStretch( otherAppLayout->columnCount() + 1, 1 );
    otherAppLayout->setRowStretch( otherAppLayout->rowCount() + 1, 1 );

    QWidget* otherAppTab = new QWidget;
    otherAppTab->setContentsMargins( 8, 8, 2, 2 );
    otherAppTab->setLayout( otherAppLayout );
    playerTabs->addTab( otherAppTab, icons->getIcon( I_OTHER_SMALL ), config.otherAppTitle );
}

void DtMainOptionsDialog::updateConfigCb( DtOptionsComboBox* comboBox, const QString& dir ) {
    QString selectedCfg = comboBox->currentText();
    bool changedState = optionsChanged;

    QStringList configs;
    comboBox->clear();

    if ( dir != "" )
        configsFromDir( dir, configs );

    configs.removeAt( configs.indexOf( "qzconfig.cfg" ) );
    configs.removeAt( configs.indexOf( "repconfig.cfg" ) );
    configs.insert( 0, tr( "None" ) );
    comboBox->addItems( configs );

    int selectedIndex = comboBox->findText( selectedCfg );

    if ( selectedIndex != -1 ) {
        comboBox->setCurrentIndex( selectedIndex );
    }
    else {
        comboBox->setCurrentIndex( 0 );
        setOptionsChanged();
    }

    if ( !changedState ) {
        optionsChanged = false;
        btnApply->setEnabled( false );
    }
}

void DtMainOptionsDialog::createPlayerTabQa() {
    QGridLayout* qaPlayerLayout = new QGridLayout;

    QLabel* qaWindowedModeLbl = new QLabel( tr( "Windowed Resolution" ) );
    qaWindowedModeCombo = new DtOptionsComboBox( this );
    qaWindowedModeCombo->setFixedWidth( 120 );
    qaPlayerLayout->addWidget( qaWindowedModeLbl, 0, 0, Qt::AlignLeft );
    qaPlayerLayout->addWidget( qaWindowedModeCombo, 0, 1, Qt::AlignLeft );

    QLabel* qaFullscreenModeLbl = new QLabel( tr( "Full Screen Resolution" ) );
    qaFullscreenModeCombo = new DtOptionsComboBox( this );
    qaFullscreenModeCombo->setFixedWidth( 120 );
    qaPlayerLayout->addWidget( qaFullscreenModeLbl, 1, 0, Qt::AlignLeft );
    qaPlayerLayout->addWidget( qaFullscreenModeCombo, 1, 1, Qt::AlignLeft );

    QLabel* qaFullscreenLbl = new QLabel( tr( "Full Screen Mode" ) );
    qaFullscreenCb = new DtOptionsCheckBox( this );
    qaPlayerLayout->addWidget( qaFullscreenLbl, 2, 0, Qt::AlignLeft );
    qaPlayerLayout->addWidget( qaFullscreenCb, 2, 1, Qt::AlignLeft );

    QLabel* qaConfigLbl = new QLabel( tr( "Config" ) );
    qaConfigCombo = new DtOptionsComboBox( this );

    updateConfigCb( qaConfigCombo, config.getQaBasePath() );

    qaBtnEditConfig = new DtOptionsButton( tr( "Edit" ), this );
    qaBtnEditConfig->setIcon( QIcon( ":/res/text-editor.png" ) );
    connect( qaBtnEditConfig, SIGNAL( clicked() ), this, SLOT( qaEditConfig() ) );
    qaBtnNewConfig = new DtOptionsButton( tr( "New" ), this );
    qaBtnNewConfig->setIcon( QIcon( ":/res/document-new.png" ) );
    connect( qaBtnNewConfig, SIGNAL( clicked() ), this, SLOT( qaNewConfig() ) );

    qaPlayerLayout->addWidget( qaConfigLbl, 3, 0, Qt::AlignLeft );
    qaPlayerLayout->addWidget( qaConfigCombo, 3, 1, Qt::AlignLeft );
    qaPlayerLayout->addWidget( qaBtnEditConfig, 3, 2, Qt::AlignLeft );
    qaPlayerLayout->addWidget( qaBtnNewConfig, 3, 3, Qt::AlignLeft );

    qaPlayerLayout->setColumnStretch( qaPlayerLayout->columnCount() + 1, 1 );
    qaPlayerLayout->setRowStretch( qaPlayerLayout->rowCount() + 1, 1 );

    QWidget* qaTab = new QWidget;
    qaTab->setContentsMargins( 8, 8, 2, 2 );
    qaTab->setLayout( qaPlayerLayout );

    playerTabs->addTab( qaTab, icons->getIcon( I_Q3_SMALL ), "Quake Arena" );
}

void DtMainOptionsDialog::readConfig() {
    optionsChanged = true;

    languageCombo->setCurrentIndex( languageCombo->findText( config.language ) );
    tableAlternateColorFactorEdit->setText( QString::number( config.tableAlternateColorFactor ) );
    tableSelectionColorFactorEdit->setText( QString::number( config.tableSelectionColorFactor ) );

    confirmDeleteCb->setChecked( !config.confirmOnDelete );
    noCompressionCb->setChecked( !config.zlibCompressionLevel );
    compressSlider->setValue( !config.zlibCompressionLevel ? 6 : config.zlibCompressionLevel );
    compressSlider->setEnabled( config.zlibCompressionLevel );
    archRemovePathsCb->setChecked( !config.archiveRemovePaths );

    qzExePathEdit->setPath( config.getQzPath() );
    qzFSBasePathEdit->setPath( config.getQzFSBasePath() );
    qaPathEdit->setPath( config.getQaPath() );
    qaHomePathEdit->setPath( config.getQaHomePath() );
    dirTreeOpenedCb->setChecked( config.dirTreeAlwaysOpened );
    dropDemosToNewDirCb->setChecked( config.dropDemosToNewDir );

    qzFullscreenCb->setChecked( config.qzFullscreen );
    qaFullscreenCb->setChecked( config.qaFullscreen );
    qzWindowedModeCombo->setCurrentIndex( qzWindowedModeCombo->findData( config.qzWindowedMode ) );
    qzFullscreenModeCombo->setCurrentIndex( config.qzFullscreenMode );
    qaWindowedModeCombo->setCurrentIndex( qaWindowedModeCombo->findData( config.qaWindowedMode ) );
    qaFullscreenModeCombo->setCurrentIndex( config.qaFullscreenMode );

    int cfgIndex = qzConfigCombo->findText( config.qzGameConfig );

    if ( cfgIndex != -1 ) {
        qzConfigCombo->setCurrentIndex( cfgIndex );
    }

    cfgIndex = qaConfigCombo->findText( config.qaGameConfig );

    if ( cfgIndex != -1 ) {
        qaConfigCombo->setCurrentIndex( cfgIndex );
    }

    otherAppTitle->setText( config.otherAppTitle );
    otherAppDm68Cb->setChecked( config.otherAppDm68 );
    otherAppDm73Cb->setChecked( config.otherAppDm73 );
    otherAppDblClickCb->setChecked( config.otherAppDoubleClick );
    otherAppMenuCb->setChecked( config.otherAppMenu );
    otherAppPreviewCb->setChecked( config.otherAppPreview );
    otherAppPathEdit->setPath( config.otherAppPath );
    otherAppCmdLineEdit->setText( config.otherAppCmdLine );

    if ( config.otherAppFromDemos ) {
        otherAppFromDemosPathBtn->setChecked( true );
    }
    else {
        otherAppAbsolutePathBtn->setChecked( true );
    }

    editorShowGameTimeCb->setChecked( !config.editorShowGameTime );
    editorShowLagsCb->setChecked( config.showLags );
    addFragsTimeBeginEdit->setText( QString::number( config.frags.segAddTimeBegin ) );
    addFragsTimeEndEdit->setText( QString::number( config.frags.segAddTimeEnd ) );
    addTextTimeBeginEdit->setText( QString::number( config.textSegAddTimeBegin ) );
    addTextTimeEndEdit->setText( QString::number( config.textSegAddTimeEnd ) );
    timeDisplayFormatEdit->setText( config.timeDisplayFormat );

    editorOptionsPage->readConfig();

    optionsChanged = false;
    playerConfigChanged = false;
    playerLoginDataChanged = false;
    btnApply->setEnabled( false );
}

void DtMainOptionsDialog::writeConfig() {
    config.language = languageCombo->currentText();
    config.tableAlternateColorFactor = tableAlternateColorFactorEdit->text().toInt();
    config.tableSelectionColorFactor = tableSelectionColorFactorEdit->text().toInt();

    config.confirmOnDelete = !confirmDeleteCb->isChecked();

    if ( noCompressionCb->isChecked() ) {
        config.zlibCompressionLevel = 0;
    }
    else {
        config.zlibCompressionLevel = noCompressionCb->isChecked() ? 0 : compressSlider->value();
    }

    config.archiveRemovePaths = !archRemovePathsCb->isChecked();
    config.setQzPath( qzExePathEdit->getPath() );
    config.setQzFSBasePath( qzFSBasePathEdit->getPath()  );
    config.setQaPath( qaPathEdit->getPath() );
    config.setQaHomePath( qaHomePathEdit->getPath() );
    config.dirTreeAlwaysOpened = dirTreeOpenedCb->isChecked();
    config.dropDemosToNewDir = dropDemosToNewDirCb->isChecked();

    config.qzFullscreen = qzFullscreenCb->isChecked();
    config.qaFullscreen = qaFullscreenCb->isChecked();
    config.qzWindowedMode = qzWindowedModeCombo->itemData( qzWindowedModeCombo->currentIndex() ).toInt();
    config.qzFullscreenMode = qzFullscreenModeCombo->currentIndex();
    config.qaWindowedMode = qaWindowedModeCombo->itemData( qaWindowedModeCombo->currentIndex() ).toInt();
    config.qaFullscreenMode = qaFullscreenModeCombo->currentIndex();

    config.qzGameConfig = qzConfigCombo->currentText();

    if ( qzConfigCombo->currentIndex() == 0 ) {
        config.qzGameConfig.clear();
    }

    config.qaGameConfig = qaConfigCombo->currentText();

    if ( qaConfigCombo->currentIndex() == 0 ) {
        config.qaGameConfig.clear();
    }

    config.otherAppTitle = otherAppTitle->text();
    config.otherAppDm68 = otherAppDm68Cb->isChecked();
    config.otherAppDm73 = otherAppDm73Cb->isChecked();
    config.otherAppDoubleClick = otherAppDblClickCb->isChecked();
    config.otherAppMenu = otherAppMenuCb->isChecked();
    config.otherAppPreview = otherAppPreviewCb->isChecked();
    config.otherAppPath = otherAppPathEdit->getPath();
    config.otherAppCmdLine = otherAppCmdLineEdit->text();
    config.otherAppFromDemos = !( otherAppDemoPathBtnGroup->checkedId() );

    config.editorShowGameTime = !editorShowGameTimeCb->isChecked();
    config.showLags = editorShowLagsCb->isChecked();
    config.timeDisplayFormat = timeDisplayFormatEdit->text();
    config.frags.segAddTimeBegin = addFragsTimeBeginEdit->text().toInt();
    config.frags.segAddTimeEnd = addFragsTimeEndEdit->text().toInt();
    config.textSegAddTimeBegin = addTextTimeBeginEdit->text().toInt();
    config.textSegAddTimeEnd = addTextTimeEndEdit->text().toInt();

    config.save();
    emit configChanged();

    if ( playerLoginDataChanged ) {
        playerLoginDataChanged = false;
        emit playerWindowLoginDataChanged();
    }

    if ( playerConfigChanged ) {
        playerConfigChanged = false;
        emit playerWindowConfigChanged();
    }

    editorOptionsPage->writeConfig();
}

void DtMainOptionsDialog::configsFromDir( QString path, QStringList& list ) {
    QDirIterator it( path, QDirIterator::Subdirectories );

    path += '/';

    while( it.hasNext() ) {
        it.next();

        if ( it.fileInfo().isFile() && it.fileInfo().suffix() == "cfg" &&
             it.fileInfo().fileName() != "autoexec.cfg" )
        {
            list << it.fileInfo().filePath().remove( path );
        }
    }

    qSort( list );
}

void DtMainOptionsDialog::setOptionsChanged() {
    DtOptionsDialog::setOptionsChanged();

    if ( sender() &&
         ( sender()->objectName() == "qzWindowedResolutions" ||
           sender()->objectName() == "qzPanelAutoHide"       ||
           sender()->objectName() == "qzPanelStyle" ) )
    {
        playerConfigChanged = true;
    }
    else if ( sender() &&
              ( sender()->objectName() == "qzEmailEdit" ||
                sender()->objectName() == "qzPassEdit" ) )
    {
        playerLoginDataChanged = true;
    }

}

void DtMainOptionsDialog::defaults() {
    int act = QMessageBox::question( this,
                                     tr( "Settings reset" ),
                                     tr( "Reset all settings to defaults?" ),
                                     QMessageBox::Yes | QMessageBox::No );

    if ( act == QMessageBox::Yes ) {
        config.defaults();
        readConfig();
        playerConfigChanged = true;
        playerLoginDataChanged = true;
    }
}

void DtMainOptionsDialog::closeEvent( QCloseEvent* event ) {
    if ( optionsChanged ) {
        int act = QMessageBox::question( this,
                                         tr( "Options changed" ),
                                         tr( "Save changes?" ),
                                         QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel );
        if ( act == QMessageBox::Yes ) {
            apply();
        }
        else if ( act == QMessageBox::Cancel ) {
            event->ignore();
            return;
        }
    }

    config.settings->setValue( "OptionsDialog/size", size() );
    config.settings->setValue( "OptionsDialog/currentPage", optionsList->currentRow() );
    optionsChanged = false;
    event->accept();
}

void DtMainOptionsDialog::qzEditConfig() {
    if ( qzConfigCombo->currentIndex() != 0 ) {
        DtMainWindow::addConfig( config.getQzBasePath() + "/" + qzConfigCombo->currentText() );
    }
    else {
        qzNewConfig();
    }
}

void DtMainOptionsDialog::qzNewConfig() {
    DtMainWindow::addConfig( config.getQzBasePath() );
}

void DtMainOptionsDialog::qaEditConfig() {
    if ( qaConfigCombo->currentIndex() != 0 ) {
        DtMainWindow::addConfig( config.getQaBasePath() + "/" + qaConfigCombo->currentText() );
    }
    else {
        qaNewConfig();
    }
}

void DtMainOptionsDialog::qaNewConfig() {
    DtMainWindow::addConfig( config.getQaBasePath() );
}

DtListWidget::DtListWidget( QWidget* parent ) : QListWidget( parent ) {
    setFixedWidth( 190 );
    setItemDelegate( new DtListWidgetDelegate( this ) );
    setIconSize( QSize( 24, 24 ) );
}

DtListWidgetDelegate::DtListWidgetDelegate( QObject* parent ) : QStyledItemDelegate( parent ) {
}

QSize DtListWidgetDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QSize size = QStyledItemDelegate::sizeHint( option, index );
    size.setHeight( qMax( size.height(), 40 ) );
    return size;
}

