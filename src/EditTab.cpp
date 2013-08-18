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

#include "EditTab.h"
#include "Data.h"
#include "Demo.h"
#include "DemoPartsWidget.h"
#include "Table.h"
#include "MainWindow.h"
#include "MainTable.h"
#include "MainTabWidget.h"
#include "CommandsWidget.h"

#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QStyle>
#include <QMessageBox>
#include <QHeaderView>
#include <QAction>
#include <QMenu>
#include <QContextMenuEvent>
#include <QLineEdit>
#include <QFileDialog>
#include <QFileInfo>
#include <QDateTime>
#include <QButtonGroup>
#include <QRadioButton>
#include <QToolButton>

using namespace dtdata;

DtEditTab::DtEditTab( QWidget* parent ) : QWidget( parent ) {
    QVBoxLayout* mainLayout = new QVBoxLayout;
    QHBoxLayout* topLayout = new QHBoxLayout;
    demoInfoGroup = new QGroupBox;
    demoInfoGroup->setFixedHeight( 180 );
    QGridLayout* infoGrid = new QGridLayout;

    int row = 0;
    int column = 1;

    infoGrid->setHorizontalSpacing( 15 );
    infoGrid->setContentsMargins( 20, 15, 20, 15 );

    QLabel* infoLbl = new QLabel( tr( "Protocol" ) );
    protocolLbl = new QLabel;
    infoGrid->addWidget( infoLbl, row, 0, Qt::AlignLeft );
    infoGrid->addWidget( protocolLbl, row++, column, Qt::AlignLeft );

    infoLbl = new QLabel( tr( "Size" ) );
    sizeLbl = new QLabel;
    infoGrid->addWidget( infoLbl, row, 0, Qt::AlignLeft );
    infoGrid->addWidget( sizeLbl, row++, column, Qt::AlignLeft );

    infoLbl = new QLabel( tr( "Length" ) );
    lengthLbl = new QLabel;
    infoGrid->addWidget( infoLbl, row, 0, Qt::AlignLeft );
    infoGrid->addWidget( lengthLbl, row++, column, Qt::AlignLeft );

    infoLbl = new QLabel( tr( "Server frames" ) );
    framesLbl = new QLabel;
    infoGrid->addWidget( infoLbl, row, 0, Qt::AlignLeft );
    infoGrid->addWidget( framesLbl, row++, column, Qt::AlignLeft );

    infoLbl = new QLabel( tr( "Frame duration" ) );
    frameDurationLbl = new QLabel;
    infoGrid->addWidget( infoLbl, row, 0, Qt::AlignLeft );
    infoGrid->addWidget( frameDurationLbl, row++, column, Qt::AlignLeft );

    row = 0;
    column = 3;

    infoGrid->addItem( new QSpacerItem( 20, 1 ), 0, 2, 9, 1, Qt::AlignCenter );

    infoLbl = new QLabel( tr( "Warmup duration" ) );
    warmupDurationLbl = new QLabel;
    infoGrid->addWidget( infoLbl, row, column, Qt::AlignLeft );
    infoGrid->addWidget( warmupDurationLbl, row++, column + 1, Qt::AlignLeft );

    infoLbl = new QLabel( tr( "Map" ) );
    mapLbl = new QLabel;
    infoGrid->addWidget( infoLbl, row, column, Qt::AlignLeft );
    infoGrid->addWidget( mapLbl, row++, column + 1, Qt::AlignLeft );

    infoLbl = new QLabel( tr( "Game type" ) );
    gameTypeLbl = new QLabel;
    infoGrid->addWidget( infoLbl, row, column, Qt::AlignLeft );
    infoGrid->addWidget( gameTypeLbl, row++, column + 1, Qt::AlignLeft );

    infoLbl = new QLabel( tr( "Game state" ) );
    gameStateLbl = new QLabel;
    infoGrid->addWidget( infoLbl, row, column, Qt::AlignLeft );
    infoGrid->addWidget( gameStateLbl, row++, column + 1, Qt::AlignLeft );

    q3ModTextLbl = new QLabel( tr( "Mod" ) );
    q3ModTextLbl->setVisible( false );
    q3ModLbl = new QLabel;
    q3ModLbl->setVisible( false );
    infoGrid->addWidget( q3ModTextLbl, row, column, Qt::AlignLeft );
    infoGrid->addWidget( q3ModLbl, row++, column + 1, Qt::AlignLeft );

    infoGrid->setColumnStretch( infoGrid->columnCount() + 1, 1 );

    demoInfoGroup->setLayout( infoGrid );

    QGroupBox* writeOptionsGroup = new QGroupBox( tr( "Options" ) );
    writeOptionsGroup->setFixedHeight( 180 );
    QGridLayout* writeOptionsLayout = new QGridLayout;
    writeOptionsLayout->setHorizontalSpacing( 15 );
    writeOptionsLayout->setContentsMargins( 20, 15, 20, 15 );
    row = 0;

    QLabel* removeWarmupLbl = new QLabel( tr( "Skip warmup" ) );
    removeWarmupCb = new QCheckBox;
    connect( removeWarmupCb, SIGNAL( stateChanged( int ) ), this, SLOT( setOptionsCnaged() ) );
    writeOptionsLayout->addWidget( removeWarmupLbl, row, 0, Qt::AlignLeft );
    writeOptionsLayout->addWidget( removeWarmupCb, row++, 1, Qt::AlignLeft );

    QLabel* removePausesLbl = new QLabel( tr( "Skip pauses" ) );
    removePausesCb = new QCheckBox;
    connect( removePausesCb, SIGNAL( stateChanged( int ) ), this, SLOT( setOptionsCnaged() ) );
    writeOptionsLayout->addWidget( removePausesLbl, row, 0, Qt::AlignLeft );
    writeOptionsLayout->addWidget( removePausesCb, row++, 1, Qt::AlignLeft );

    QLabel* removeLagsLbl = new QLabel( tr( "Remove lags" ) );
    removeLagsCb = new QCheckBox;
    connect( removeLagsCb, SIGNAL( stateChanged( int ) ), this, SLOT( setOptionsCnaged() ) );
    writeOptionsLayout->addWidget( removeLagsLbl, row, 0, Qt::AlignLeft );
    writeOptionsLayout->addWidget( removeLagsCb, row++, 1, Qt::AlignLeft );

    QLabel* demoStartTimeLbl = new QLabel( tr( "Game timer initial value" ) );
    demoStartTimeSb = new DtTimeEdit( this );
    connect( demoStartTimeSb, SIGNAL( timeChanged( QTime ) ), this, SLOT( setOptionsCnaged() ) );
    writeOptionsLayout->addWidget( demoStartTimeLbl, row, 0, Qt::AlignLeft );
    writeOptionsLayout->addWidget( demoStartTimeSb, row++, 1, Qt::AlignLeft );
    writeOptionsLayout->setColumnStretch( writeOptionsLayout->columnCount() + 1, 1 );
    writeOptionsLayout->setRowStretch( writeOptionsLayout->rowCount() + 1, 1 );

    writeOptionsGroup->setLayout( writeOptionsLayout );

    topLayout->addWidget( demoInfoGroup );
    topLayout->addWidget( writeOptionsGroup );

    QGroupBox* demoPartsGroup = new QGroupBox;
    QVBoxLayout* partsLayout = new QVBoxLayout;

    demoParts = new DtDemoPartsWidget( this );
    connect( demoParts, SIGNAL( cursorMoved( int, int ) ),
             this, SLOT( updateCursorInfo( int, int ) ) );
    connect( demoParts, SIGNAL( changed() ), this, SLOT( partsChanged() ) );
    connect( demoParts, SIGNAL( partSelected( int ) ),
             this, SLOT( onPartBorderSelected( int ) ) );
    connect( demoParts, SIGNAL( partUnselected() ), this, SLOT( onPartBorderUnselected() ) );
    connect( this, SIGNAL( resizePart( int ) ), demoParts, SLOT( moveSelectedBorder( int ) ) );

    QHBoxLayout* demoPartsWidgetLayout = new QHBoxLayout;

    selectorModeGroup = new QButtonGroup( this );
    QRadioButton* selectPartsButton = new QRadioButton( tr( "Select" ) );
    QRadioButton* cutPartsButton = new QRadioButton( tr( "Cut" ) );

    selectorModeGroup->addButton( selectPartsButton, 0 );
    selectorModeGroup->addButton( cutPartsButton, 1 );

    if ( config.partsSelectorMode == 0 ) {
        selectPartsButton->setChecked( true );
    }
    else {
        cutPartsButton->setChecked( true );
    }

    QVBoxLayout* selectorsLayout = new QVBoxLayout;

    selectorsLayout->addWidget( selectPartsButton );
    selectorsLayout->addWidget( cutPartsButton );

    demoPartsWidgetLayout->addLayout( selectorsLayout, 1 );
    demoPartsWidgetLayout->addWidget( demoParts );

    QHBoxLayout* partsInfo = new QHBoxLayout;

    QHBoxLayout* partOptionsLayout = new QHBoxLayout;
    partTimeSb = new DtTimeEdit( this );
    partSbManual = true;
    connect( partTimeSb, SIGNAL( timeChanged( const QTime& ) ),
             this, SLOT( partSbTimeChanged( const QTime& ) ), Qt::DirectConnection );
    partTimeSb->setEnabled( false );
    partOptionsLayout->addWidget( partTimeSb, 1, Qt::AlignLeft );
    partOptionsLayout->addItem( new QSpacerItem( 1, partTimeSb->height() ) );
    partsInfo->addLayout( partOptionsLayout, 1 );

    cursorTimeLbl = new QLabel;
    cursorTimeLbl->setFixedWidth( 220 );
    partsInfo->addWidget( cursorTimeLbl, Qt::AlignLeft );
    partsInfo->addSpacing( 10 );
    partsInfo->setMargin( 1 );

    partDurationLbl = new QLabel;
    partsInfo->addWidget( partDurationLbl, Qt::AlignLeft );
    partsInfo->addStretch( 5 );

    clearPartsButton = new QToolButton;
    clearPartsButton->setIcon( QIcon( ":/res/clear_left.png" ) );
    clearPartsButton->setIconSize( QSize( 22, 22 ) );
    clearPartsButton->setToolTip( tr( "Clear selection" ) );
    clearPartsButton->setAutoRaise( true );
    connect( clearPartsButton, SIGNAL( clicked() ), demoParts, SLOT( clearParts() ) );
    connect( clearPartsButton, SIGNAL( clicked() ), this, SLOT( onPartBorderUnselected() ) );
    clearPartsButton->setEnabled( false );
    clearPartsButton->setFixedSize( 24, 24 );

    demoPartsWidgetLayout->addWidget( clearPartsButton, 1, Qt::AlignVCenter );
    demoPartsWidgetLayout->addStretch( 1 );
    demoPartsWidgetLayout->setMargin( 0 );

    partsLayout->addLayout( demoPartsWidgetLayout, 1 );
    partsLayout->addLayout( partsInfo, 1 );

    partsLayout->addStretch( 1 );
    demoPartsGroup->setLayout( partsLayout );
    demoPartsGroup->setFixedHeight( 120 );

    mainLayout->addLayout( topLayout, 1 );
    mainLayout->addWidget( demoPartsGroup );

    QTabWidget* commandTabs = new QTabWidget;
    QWidget* chatTab = new QWidget;
    QHBoxLayout* chatLayout = new QHBoxLayout;

    chatTable = new DtChatTable( this );

    chatLayout->addWidget( chatTable );
    chatTab->setLayout( chatLayout );

    commandTabs->addTab( chatTab, tr( "Chat" ) );

    QWidget* cmdTab = new QWidget;
    QHBoxLayout* cmdLayout = new QHBoxLayout;

    commandsWidget = new DtCommandsWidget( this );
    cmdLayout->addWidget( commandsWidget );
    cmdTab->setLayout( cmdLayout );

    commandTabs->addTab( cmdTab, tr( "Commands" ) );

    mainLayout->addWidget( commandTabs );

    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    previewButton = new QPushButton( tr( "Preview" ) );
    previewButton->setMinimumWidth( 120 );
    connect( previewButton, SIGNAL( clicked() ), this, SLOT( preview() ) );
    buttonsLayout->addWidget( previewButton, 1, Qt::AlignLeft );

    saveAsButton = new QPushButton( tr( "Save As" ) );
    saveAsButton->setMinimumWidth( 120 );
    connect( saveAsButton, SIGNAL( clicked() ), this, SLOT( saveAs() ) );
    buttonsLayout->addWidget( saveAsButton, 1, Qt::AlignLeft );

    saveButton = new QPushButton( tr( "Save" ) );
    saveButton->setMinimumWidth( 120 );
    connect( saveButton, SIGNAL( clicked() ), this, SLOT( save() ) );
    buttonsLayout->addWidget( saveButton, 1, Qt::AlignLeft );

    cancelButton = new QPushButton( tr( "Cancel" ) );
    cancelButton->setMinimumWidth( 120 );
    connect( cancelButton, SIGNAL( clicked() ), this, SLOT( cancelChanges() ) );
    buttonsLayout->addWidget( cancelButton, 1, Qt::AlignLeft );
    buttonsLayout->addStretch( 4 );

    mainLayout->addSpacing( 10 );
    mainLayout->addLayout( buttonsLayout );

    int boxSize = ( demoParts->width() + partsLayout->margin() ) / 2;
    demoInfoGroup->setMinimumWidth( boxSize );
    writeOptionsGroup->setMinimumWidth( boxSize );

    setLayout( mainLayout );
    setMouseTracking( true );
}

DtEditTab::~DtEditTab() {
    config.partsSelectorMode = selectorModeGroup->checkedId();
    stopEdit();
}

void DtEditTab::setOptionsCnaged() {
    optionsChanged = true;
}

DtWriteOptions DtEditTab::getOptions() const {
    DtWriteOptions options;
    const QVector< DtDemoPart >& parts = demoParts->getParts();
    int partCount = parts.count();

    options.removeWarmup = removeWarmupCb->isChecked();
    options.removePauses = removePausesCb->isChecked();
    options.removeLags = removeLagsCb->isChecked();
    options.timerInitialValue = demoStartTimeSb->msec();

    if ( chatTable->isChanged() ) {
        options.chatStrings = chatTable->getStrings();
        options.editChat = true;
    }

    options.writeCommands = commandsWidget->isChanged();

    if ( options.writeCommands ) {
        options.commands = commandsWidget->getCommands();
        return options;
    }

    if ( selectorModeGroup->checkedId() == 0 ) { /* leave selected parts */
        for ( int i = 0; i < partCount; ++i ) {
            options.cutSegments.append( cutSegment( parts.at( i ).start, parts.at( i ).end ) );
        }
    }
    else { /* cut off selected parts */
        if ( !parts.isEmpty() ) {
            options.cutSegments.append( cutSegment( 0, parts.at( 0 ).start ) );

            for ( int i = 1; i < partCount; ++i ) {
                options.cutSegments.append(
                        cutSegment( parts.at( i - 1 ).end, parts.at( i ).start ) );
            }

            if ( parts.at( partCount - 1 ).end != demoLength ) {
                options.cutSegments.append(
                        cutSegment( parts.at( partCount - 1 ).end, demoLength ) );
            }
        }
    }

    return options;
}

void DtEditTab::preview() {
    lastPreviewPath = dtMainWindow->previewDemo( demo, getOptions() );
    lastPreviewModifiedTime = QFileInfo( lastPreviewPath ).lastModified().toTime_t();
}

void DtEditTab::saveChat( QString fName ) {
    QFile log( fName );

    if ( !log.open( QFile::WriteOnly ) ) {
        return;
    }

    const QVector< QPair< int, QString > >& chatStrings = demo->getChatStrings();
    int chatStringsCount = chatStrings.count();

    QString exp;
    bool q3Chat = ( demo->getProto() == Q3_68 );
    bool q3Cpma = ( demo->getQ3Mod() == MOD_CPMA );

    if ( !q3Chat ) { /* QL */
        exp = "^(t?chat)\\s\"([^\\s]+)\\s([^\"]+)";
    }
    else if ( q3Cpma ) { /* CPMA */
        exp = "^(chat|mm2)\\s(\\d+)?\\s?(\\d+)?\\s?\"([^\"]+)";
    }
    else { /* VQ3/OSP */
        exp = "^(t?chat)\\s\"([^\"]+)";
    }

    QRegExp chatExp( exp );

    for ( int i = 0; i < chatStringsCount; ++i ) {
        if ( chatExp.indexIn( chatStrings.at( i ).second ) != -1 ) {
            QString chatLine;

            if ( !q3Chat ) {
                chatLine = chatExp.cap( 3 );
            }
            else if ( q3Cpma ) {
                chatLine = chatExp.cap( 4 );

                if ( chatExp.cap( 1 ) == "mm2" ) {
                    chatLine = QString( "%1 %2" ).arg( chatExp.cap( 2 ), chatLine );
                }
            }
            else {
                chatLine = chatExp.cap( 2 );
            }

            chatLine.remove( chatEscapeChar );
            cleanStringColors( chatLine );
            log.write( chatLine.toAscii() );
            log.write( "\n" );
        }
    }

    log.close();
}

void DtEditTab::saveAs() {
    QString demoFilter = QString( "Quake Demo (*.%1)" ).arg( demo->fileInfo().fileExt );
    QString xmlFilter = tr( "XML file (*.xml)" );
    QString txtChatFilter = tr( "Chat log (*.txt)" );

    QFileDialog saveDialog( this, tr( "Save as" ), currentWorkingDir );
    saveDialog.setNameFilters( QStringList() << demoFilter << xmlFilter << txtChatFilter );
    saveDialog.setViewMode( QFileDialog::List );
    saveDialog.setFileMode( QFileDialog::AnyFile );
    saveDialog.setAcceptMode( QFileDialog::AcceptSave );
    saveDialog.selectFile( "demo" );

    QString fName;

    if( saveDialog.exec() ) {
        fName = saveDialog.selectedFiles().first();
    }

    if ( fName.isEmpty() || !isAcceptedEncoding( QFileInfo( fName ).fileName() ) ) {
        return;
    }

    if ( saveDialog.selectedNameFilter() == xmlFilter ) {
        if ( !fName.endsWith( ".xml", Qt::CaseInsensitive ) ) {
            fName += ".xml";
        }

        dtMainWindow->writeXml( demo, fName );
        return;
    }

    if ( saveDialog.selectedNameFilter() == txtChatFilter ) {
        if ( !fName.endsWith( ".txt", Qt::CaseInsensitive ) ) {
            fName += ".txt";
        }

        saveChat( fName );
        return;
    }

    if ( !fName.endsWith( "." + demo->fileInfo().fileExt, Qt::CaseInsensitive ) ) {
        fName += "." + demo->fileInfo().fileExt;
    }

    if ( QFile::exists( fName ) ) {
        QString demoModName = getModifiedName( fName );
        DtDemo* demoFile = openedDemos.value( demoModName, 0 );

        if ( demoFile ) {
            if ( demoFile->referenceCount > 0 ) {
                QMessageBox::warning( this, tr( "File is in use" ),
                                      tr( "Can't overwrite an used demo" ) );
                return;
            }
        }

        int act = QMessageBox::question( this, tr( "File exists" ),
                                         tr( "File with the given name already exists."
                                             " Overwrite it?" ),
                                         QMessageBox::Yes | QMessageBox::No );
        if ( act == QMessageBox::Yes ) {
            QFile::remove( fName );

            if ( demoFile ) {
                openedDemos.remove( demoModName );
            }
        }
        else {
            return;
        }
    }

    dtMainWindow->setCursor( Qt::WaitCursor );
    QFileInfo preview( lastPreviewPath );

    if ( preview.exists() && preview.lastModified().toTime_t() == lastPreviewModifiedTime ) {
        QFile::copy( lastPreviewPath, fName );
    }
    else {
        DtWriteOptions options = getOptions();
        options.newFileName = fName;
        options.newFileName.remove( "." + demo->fileInfo().fileExt );

        mainTabWidget->setEnabled( false );
        demo->writeSegment( &options );
        mainTabWidget->setEnabled( true );
    }

    dtMainWindow->setCursor( Qt::ArrowCursor );

    if ( QFileInfo( fName ).absolutePath() == currentWorkingDir ) {
        mainTabWidget->updateMainTable();
    }
}

void DtEditTab::save() {
    bool fileInMainTable = ( demo->fileInfo().filePath == currentWorkingDir );

    if ( fileInMainTable && demo->referenceCount > 2 ) {
        QMessageBox::warning( this, tr( "File is in use" ), tr( "Can't save an used demo" ) );
        return;
    }

    int act = QMessageBox::question( this, tr( "Save file" ),
                                     tr( "Are you sure want to overwrite this file?" ),
                                     QMessageBox::Yes | QMessageBox::No );
    if ( act != QMessageBox::Yes ) {
        return;
    }

    setCursor( Qt::WaitCursor );
    mainTabWidget->setEnabled( false );

    QFileInfo preview( lastPreviewPath );
    QString tmpName;

    if ( preview.exists() && preview.lastModified().toTime_t() == lastPreviewModifiedTime ) {
        tmpName = lastPreviewPath;
    }
    else {
        DtWriteOptions options = getOptions();
        tmpName = demo->writeSegment( &options );
    }

    QString demoName = demo->fileInfo().fileName();
    QString modName = getModifiedName( demoName );

    if ( !QFile::remove( demoName ) ) {
        return;
    }

    if ( !QFile::rename( tmpName, demoName ) ) {
        return;
    }

    QFile::remove( tmpName );

    if ( fileInMainTable ) {
        openedDemos.remove( modName );
        mainTabWidget->updateMainTable();
    }

    stopEdit();
    delete demo;

    QFileInfo demoInfo( demoName );
    DtDemo* savedDemo = new DtDemo( demoInfo );

    if ( !savedDemo->parseGamestateMsg() ) {
        delete savedDemo;
        setCursor( Qt::ArrowCursor );
        close();
        return;
    }

    startEdit( savedDemo );
    mainTabWidget->setEnabled( true );
    setCursor( Qt::ArrowCursor );
}

void DtEditTab::cancelChanges() {
    int act = QMessageBox::question( this,
                                     tr( "Cancel changes" ),
                                     tr( "Are your sure want to cancel all changes?" ),
                                     QMessageBox::Yes | QMessageBox::No );

    if ( act == QMessageBox::Yes ) {
        startEdit( demo );
    }
}

void DtEditTab::partsChanged() {
    clearPartsButton->setEnabled( !demoParts->getParts().isEmpty() );

    if ( !demoParts->getParts().isEmpty() && selectorModeGroup->checkedId() == 0 ) {
        int startTimeMs = demoParts->getParts().at( 0 ).start
                          - warmupLength - frameTime + initialTimerValMs;
        demoStartTimeSb->setTime( msecToTime( startTimeMs ) );

        if ( warmupLength > 0 ) {
            removeWarmupCb->setChecked( true );
            removeWarmupCb->setEnabled( false );
        }
    }
    else {
        demoStartTimeSb->setTime( msecToTime( initialTimerValMs ) );

        if ( warmupLength > 0 ) {
            removeWarmupCb->setEnabled( true );
        }
    }
}

void DtEditTab::onPartBorderSelected( int index ) {
    const DtDemoPart& part = demoParts->getParts().at( demoParts->selectedPartIndex() );
    int val = ( index & 1 ) ? part.end : part.start;

    partSbManual = false;
    partTimeSb->setTime( msecToTime( val + initialTimerValMs ) );
    partSbManual = true;
    partTimeSb->setEnabled( true );
}

void DtEditTab::onPartBorderUnselected() {
    partSbManual = false;
    partTimeSb->setEnabled( false );
    partTimeSb->setTime( QTime() );
    partSbManual = true;
}

void DtEditTab::partSbTimeChanged( const QTime& t ) {
    if ( partSbManual ) {
        emit resizePart( t.hour() * 3600000 +
                         t.minute() * 60000 +
                         t.second() * 1000 +
                         t.msec() - initialTimerValMs );
    }
}

QTime DtEditTab::msecToTime( int msec ) {
    int hours = msec / 3600000.f;
    int minutes = msec / 60000.f - hours * 60;
    int seconds = msec / 1000.f - hours * 3600 - minutes * 60;
    int ms = msec - hours * 3600000 - minutes * 60000 - seconds * 1000;

    return QTime( hours, minutes, seconds, ms );
}

void DtEditTab::updateCursorInfo( int partStart, int pos ) {
    if ( pos == -1 ) {
        cursorTimeLbl->setText( "" );
    }
    else {
        int timePos = pos + initialTimerValMs;
        QString timeFormat = tr( "Server time: %1" );

        if ( config.editorShowGameTime ) {
            timePos -= warmupLength;
            cursorTimeLbl->setText( timeFormat.arg(
                    msecToTime( timePos ).toString( config.timeDisplayFormat ) ) );
        }
        else {
            cursorTimeLbl->setText( timeFormat.arg( timePos / 1000.f, 0, 'f', 3 ) );
        }
    }

    if ( partStart == -1 ) {
        partDurationLbl->setText( "" );

    }
    else {
        QString timeFormat = tr( "Length: %1" );
        int time = abs( pos - partStart );

        if ( config.editorShowGameTime ) {
            partDurationLbl->setText( timeFormat.arg(
                    msecToTime( time ).toString( config.timeDisplayFormat ) ) );
        }
        else {
            partDurationLbl->setText( timeFormat.arg( time / 1000.f, 0, 'f', 3 ) );
        }

        if ( !partDurationLbl->isVisible() ) {
            partDurationLbl->show();
        }
    }
}

bool DtEditTab::startEdit( DtDemo* editedDemo ) {
    demo = editedDemo;
    ++demo->referenceCount;

    if ( !demo->readEditInfo() ) {
        QMessageBox::critical( this, tr( "Error" ), tr( "Can't open demo for edit" ) );
        stopEdit();
        close();
        return false;
    }

    frameTime = demo->getFrameTime();
    demoLength = demo->getLength();
    warmupLength = demo->getMapRestartTime();

    demoInfoGroup->setTitle( demo->fileInfo().baseName );

    protocolLbl->setText( QString::number( demo->getProto() ) );
    sizeLbl->setText( tr( "%1 mB" ).arg( demo->fileInfo().size / MiB, 0, 'f', 1 ) );
    lengthLbl->setText( tr( "%1 min" ).arg( demo->getLength() / 60000.f, 0, 'f', 1 ) );
    framesLbl->setText( QString::number( demo->getSnapshotCount() ) );
    frameDurationLbl->setText( tr( "%1 ms" ).arg( demo->getFrameTime() ) );

    QString warmupDurationText = ( demo->getMapRestartTime() ) ?
                                 QString::number( demo->getMapRestartTime() / 1000.f, 'f', 3 ) +
                                 " " + tr( "s" ) : "–";
    warmupDurationLbl->setText( warmupDurationText );

    mapLbl->setText( demo->getMapName() );
    gameTypeLbl->setText( getGameTypeName( demo->getProto(), demo->getQ3Mod(),
                                           demo->getGameType(), false ) );
    gameStateLbl->setText( demo->getGamestate() );

    if ( demo->getProto() == Q3_68 ) {
        q3ModTextLbl->setVisible( true );
        q3ModLbl->setVisible( true );
        q3ModLbl->setText( demo->getQ3ModName() );
    }

    removeWarmupCb->setChecked( false );

    if ( warmupLength == 0 ) {
        removeWarmupCb->setEnabled( false );
    }

    removePausesCb->setChecked( false );

    if ( demo->getPauses().count() == 0 ) {
        removePausesCb->setEnabled( false );
    }

    initialTimerValMs = demo->getTimerInitialValue();
    initialTimerVal = initialTimerValMs / 1000.f;

    if ( demo->getProto() == QZ_73 && !demo->isInProgress() ) {
        initialTimerVal = 0;
    }

    demoStartTimeSb->setTime( msecToTime( initialTimerValMs ) );

    demoParts->setDemo( demo );

    chatTable->setDemo( demo );
    chatTable->setStrings( demo->getChatStrings() );
    commandsWidget->setDemoProto( demo->getProto(), demo->getQ3Mod() );
    commandsWidget->setCommands( demo->getCommands() );

    optionsChanged = false;

    return true;
}

bool DtEditTab::demoModified() {
    return ( optionsChanged                 ||
             demoParts->getParts().count()  ||
             chatTable->isChanged()         ||
             commandsWidget->isChanged() );
}

void DtEditTab::stopEdit() {
    --demo->referenceCount;
}

DtDemo* DtEditTab::currentDemo() const {
    return demo;
}

DtTimeEdit::DtTimeEdit( QWidget* parent ) : QTimeEdit( parent ) {
    setTimeRange( QTime( 0, 0 ), QTime( 23, 59, 59, 999 ) );
    setDisplayFormat( config.timeDisplayFormat );
    setCurrentSection( QTimeEdit::SecondSection );
}

int DtTimeEdit::msec() {
    QTime t = time();
    return ( ( t.hour() * 60 + t.minute() ) * 60 + t.second() ) * 1000 + t.msec();
}

DtChatTable::DtChatTable( QWidget* parent ) : DtTable( parent ) {
    setSelectionMode( DtTable::ExtendedSelection );
    setColumnCount( 6 );
    setHorizontalHeaderLabels( QStringList() << tr( "Time" ) << tr( "Player" ) << tr( "Command" )
                               << tr( "Name" ) << tr( "Message" ) << "" );
    setMinimumWidth( 900 );
    setSortingEnabled( false );

    horizontalHeader()->setResizeMode( QHeaderView::Stretch );
    horizontalHeader()->setResizeMode( TC_TIME, QHeaderView::Fixed );
    horizontalHeader()->resizeSection( TC_TIME, 80 );

    horizontalHeader()->setResizeMode( TC_CLIENTNUM, QHeaderView::Fixed );
    horizontalHeader()->resizeSection( TC_CLIENTNUM, 60 );

    horizontalHeader()->setResizeMode( TC_CMD, QHeaderView::Fixed );
    horizontalHeader()->resizeSection( TC_CMD, 85 );

    actNew = new QAction( QIcon( ":/res/insert_table_row.png" ), tr( "Add" ), this );
    connect( actNew, SIGNAL( triggered() ), this, SLOT( insertChatString() ) );

    actDelete = new QAction( QIcon( ":/res/delete_table_row.png" ), tr( "Delete" ), this );
    connect( actDelete, SIGNAL( triggered() ), this, SLOT( deleteSelectedStrings() ) );
}

void DtChatTable::setDemo( DtDemo* sDemo ) {
    demo = sDemo;

    if ( demo->getProto() == Q3_68 ) {
        hideColumn( TC_CLIENTNUM );
    }

    cpmaColumn = false;
}

void DtChatTable::deleteSelectedStrings() {
    QList< int > sRows = getSelectedRows();

    for ( int i = sRows.size() - 1; i >= 0; --i ) {
        removeRow( sRows.at( i ) );
        changed = true;
    }
}

void DtChatTable::insertChatString() {
    int row;
    int currentRow = currentIndex().row();
    QString time = "0";
    QString clientNum = "00";

    if ( currentRow != -1 ) {
        time = item( currentRow, TC_TIME )->text();
        clientNum = item( currentRow, TC_CLIENTNUM )->text();

        insertRow( currentRow + 1 );
        row = currentRow + 1;
    }
    else {
        setRowCount( rowCount() + 1 );
        row = rowCount() - 1;
    }

    QTableWidgetItem* timeItem = new QTableWidgetItem( time );
    setItem( row, TC_TIME, timeItem );

    QTableWidgetItem* cmdItem = new QTableWidgetItem( "chat" );
    setItem( row, TC_CMD, cmdItem );

    QTableWidgetItem* numItem = new QTableWidgetItem( clientNum );
    setItem( row, TC_CLIENTNUM, numItem );

    QTableWidgetItem* stringItem = new QTableWidgetItem( "" );
    setItem( row, TC_STR1, stringItem );

    stringItem = new QTableWidgetItem( "" );
    setItem( row, TC_STR2, stringItem );
}

void DtChatTable::setStrings( const QVector< QPair< int, QString > >& chatStrings ) {
    removeAllRows();

    int chatStringsCount = chatStrings.count();
    setRowCount( chatStringsCount );

    int row = 0;
    QString exp;
    bool q3Chat = ( demo->getProto() == Q3_68 );
    bool q3Cpma = ( demo->getQ3Mod() == MOD_CPMA );

    if ( !q3Chat ) { /* QL */
        exp = "^(t?chat)\\s\"([^\\s]+)\\s([^\"]+)";
    }
    else if ( q3Cpma ) { /* CPMA */
        exp = "^(chat|mm2)\\s(\\d+)?\\s?(\\d+)?\\s?\"([^\"]+)";
    }
    else { /* VQ3/OSP */
        exp = "^(t?chat)\\s\"([^\"]+)";
    }

    QRegExp chatExp( exp );

    for ( int i = 0; i < chatStringsCount; ++i ) {
        int strTime = chatStrings.at( i ).first;
        QString str = chatStrings.at( i ).second;

        QTableWidgetItem* timeItem = new QTableWidgetItem(
                QString::number( strTime / 1000.f, 'f', 3 ) );
        setItem( row, TC_TIME, timeItem );

        if ( chatExp.indexIn( str ) == -1 ) {
            continue;
        }

        setItem( row, TC_CMD, new QTableWidgetItem( chatExp.cap( 1 ) ) );

        QString numText;
        QString chatText;
        bool teamCpma = false;

        if ( !q3Chat ) {
            numText = chatExp.cap( 2 );
            chatText = chatExp.cap( 3 );
        }
        else if ( q3Cpma ) {
            teamCpma = ( chatExp.cap( 1 ) == "mm2" );

            if ( teamCpma ) {
                numText = chatExp.cap( 2 );
            }

            chatText = chatExp.cap( 4 );
        }
        else {
            chatText = chatExp.cap( 2 );
        }

        setItem( row, TC_CLIENTNUM, new QTableWidgetItem( numText ) );

        if ( !q3Cpma ) { /* QL/VQ3/OSP */
            QStringList tokens = chatText.split( chatEscapeChar, QString::SkipEmptyParts );

            setItem( row, TC_NAME, new QTableWidgetItem( tokens.at( 0 ) ) );

            QString msg = tokens.at( 1 );

            if ( chatExp.cap( 1 ) == "chat" ) {
                msg.remove( 0, 2 );
            }

            setItem( row, TC_STR1, new QTableWidgetItem( msg ) );

            msg = ( tokens.count() > 2 ) ? tokens.at( 2 ) : "";

            if ( chatExp.cap( 1 ) == "tchat" ) {
                msg.remove( 0, 2 );
            }

            setItem( row, TC_STR2, new QTableWidgetItem( msg ) );
        }
        else { /* CPMA */
            if ( teamCpma ) {
                setItem( row, TC_NAME, new QTableWidgetItem );
                setItem( row, TC_STR1, new QTableWidgetItem( chatText ) );
                setItem( row, TC_STR2, new QTableWidgetItem( chatExp.cap( 3 ) ) );

                if ( !cpmaColumn ) {
                    cpmaColumn = true;
                    setHorizontalHeaderItem( TC_STR2, new QTableWidgetItem( tr( "Location" ) ) );
                    showColumn( TC_CLIENTNUM );
                }
            }
            else {
                QStringList tokens = chatText.split( ": ", QString::SkipEmptyParts );

                setItem( row, TC_NAME, new QTableWidgetItem( tokens.at( 0 ) ) );
                setItem( row, TC_STR1, new QTableWidgetItem( tokens.at( 1 ) ) );
                setItem( row, TC_STR2, new QTableWidgetItem );
            }
        }

        ++row;
    }

    changed = false;
}

QVector< QPair< int, QString > > DtChatTable::getStrings() const {
    int stringsCount = rowCount();
    QVector< QPair< int, QString > > strings;
    QMap< int, QString > stringsMap;

    for ( int i = 0; i < stringsCount; ++i ) {
        QString msg;
        QString clientNum = ( demo->getProto() == Q3_68 ) ?
                            "" : item( i, TC_CLIENTNUM )->text() + " ";
        QString escapeChar = ( demo->getProto() == Q3_68 &&
                               demo->getQ3Mod() == MOD_CPMA ) ? "" : QString( chatEscapeChar );

        if ( item( i, TC_CMD )->text() == "chat" ) {
            msg = QString( "chat \"%1%2%3: %4\"" ).arg( clientNum,
                                                        item( i, TC_NAME )->text(),
                                                        escapeChar,
                                                        item( i, TC_STR1 )->text() );
        }
        else if ( item( i, TC_CMD )->text() == "tchat" ) {
            msg = QString( "tchat \"%1%2%3%4%5%6: %7\"" ).arg( clientNum,
                                                               escapeChar,
                                                               item( i, TC_NAME )->text(),
                                                               escapeChar,
                                                               item( i, TC_STR1 )->text(),
                                                               escapeChar,
                                                               item( i, TC_STR2 )->text() );
        }
        else if ( item( i, TC_CMD )->text() == "mm2" ) {
            msg = QString( "mm2 %1 %2 \"%3\"" ).arg( item( i, TC_CLIENTNUM )->text(),
                                                     item( i, TC_STR2 )->text(),
                                                     item( i, TC_STR1 )->text() );
        }
        else {
            continue;
        }

        int time = item( i, TC_TIME )->text().remove( '.' ).toInt();

        stringsMap.insert( time, msg );
    }

    QMapIterator< int, QString > it( stringsMap );

    while ( it.hasNext() ) {
        it.next();
        strings.append( QPair< int, QString >( it.key(), it.value() ) );
    }

    return strings;
}

void DtChatTable::contextMenuEvent( QContextMenuEvent* e ) {
    QMenu contextMenu( this );
    contextMenu.setMinimumWidth( 100 );
    contextMenu.addAction( actNew );
    contextMenu.addAction( actDelete );
    contextMenu.exec( e->globalPos() );
}

void DtChatTable::commitData( QWidget* editorWidget ) {
    QLineEdit* editor = qobject_cast< QLineEdit* >( editorWidget );
    QString str = editor->text().trimmed();

    if ( ( currentIndex().column() >= 0 && currentIndex().column() <= 2 ) && str.isEmpty() ) {
        return;
    }

    DtTable::commitData( editorWidget );
    changed = true;
}

bool DtChatTable::isChanged() {
    return changed;
}
