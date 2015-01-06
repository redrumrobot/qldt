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

#include "MainWindow.h"
#include "Demo.h"
#include "ScanWidget.h"
#include "Data.h"
#include "About.h"
#include "Archiver.h"
#include "MainOptionsDialog.h"
#include "DirTree.h"
#include "FileDialog.h"
#include "MainTabWidget.h"
#include "FindFragsPanel.h"
#include "PlayerWindow.h"
#include "MainWidget.h"
#include "FindFragsTable.h"
#include "FindTextTable.h"
#include "MainTable.h"
#include "ProgressDialog.h"
#include "EditTab.h"
#include "ClearLineEdit.h"
#include "FindText.h"
#include "FormatDialog.h"
#include "ConvertDialog.h"
#include "HelpDialog.h"
#include "FindDemoDialog.h"
#include "FindTextDialog.h"
#include "FindDemo.h"
#include "FindDemoTable.h"

#include <QMenuBar>
#include <QCloseEvent>
#include <QSettings>
#include <QVBoxLayout>
#include <QTime>
#include <QDirIterator>
#include <QUrl>
#include <QFileDialog>
#include <QEventLoop>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QTimer>
#include <QtConcurrentRun>
#include <QProcess>

#ifdef Q_OS_WIN
#include <QDesktopWidget>
#endif

using namespace dtdata;

QString DtMainWindow::lastOpenFilePath = QDir::homePath();

DtMainWindow::DtMainWindow() {
    initializeStrings();

    defaultTitle = "Demo Tools";
    setTitle();
    setWindowIcon( QIcon( ":/res/qldt.png" ) );
    setAcceptDrops( true );
    setStyleSheet( getStyle( "main" ) );

    QDir baseDir;

    if ( config.getSelectedGame() == Q_LIVE ) {
        currentWorkingDir = config.getQzDemoPath();
        baseDir.setPath( config.getQzBasePath() );
    }
    else {
        currentWorkingDir = config.getQaDemoPath();
        baseDir.setPath( config.getQaBasePath() );
    }

    if ( baseDir.exists() && !QDir( currentWorkingDir ).exists() ) {
        baseDir.mkdir( "demos" );
    }

    createMenus();

    canClose = true;
    canPlayDemo = true;
    initMenus = true;
    dirTreeAlwaysOpened = config.dirTreeAlwaysOpened;

    fileDialog = new DtFileDialog( this );
    mainTab = new DtMainTabWidget( this );
    QVBoxLayout* cLayout = new QVBoxLayout;
    cLayout->setMargin( 3 );
    QWidget* cw = new QWidget( this );

    cLayout->addWidget( mainTab );
    cw->setLayout( cLayout );
    setCentralWidget( cw );
    tempFile.clear();

    int defaultWidth = 1024;
    int defaultHeight = 768;

    resize( config.settings->value( "Main/size", QSize( defaultWidth, defaultHeight ) ).toSize() );

#ifdef Q_OS_WIN
    QRect rect = QApplication::desktop()->availableGeometry( this );
    int dx = rect.center().x() - ( defaultWidth / 2 );
    int dy = rect.center().y() - ( defaultHeight / 2 );

    if ( dx < 0 ) {
        dx = 0;
    }

    if ( dy < 0 ) {
        dy = 0;
    }

    move( config.settings->value( "Main/pos", QPoint( dx, dy ) ).toPoint() );
#endif

    quakeArena = 0;
    otherApp = 0;
    demoPlayer = new DtPlayerWindow( this );

    if ( config.getQzPath().isEmpty() ) {
        QMessageBox::warning( this, tr( "Error" ),
                              tr( "Quake Live Firefox plugin path not found. Set it manually in the options." ) );
    }

    deleteTmpDirs();
}

void DtMainWindow::setTitle( const QString& newTitle ) {
    if ( newTitle.isEmpty() ) {
        setWindowTitle( defaultTitle );
    }
    else {
        setWindowTitle( newTitle + " - " + defaultTitle );
    }
}

void DtMainWindow::paste() {
    if ( files()->isVisible() ) {
        return;
    }

    QString clipboardText = qApp->clipboard()->text().trimmed();

    if ( clipboardText.isEmpty() ) {
        return;
    }

    DtOpenUrlDialog openUrlDialog( this );
    QUrl url( clipboardText );

    if ( url.isValid() ) {
        DtOpenUrlDialog::dialogButtons button = openUrlDialog.exec( url );

        if ( button == DtOpenUrlDialog::BTN_OK && url.isValid() && addUrl( url ) ) {
            addEntryToRecentList( url.toString(), config.recentOpenedUrls );
        }
    }
}

void DtMainWindow::keyPressEvent( QKeyEvent* e ) {
    if ( !e->isAutoRepeat() ) {
        if ( e->modifiers() & Qt::ControlModifier ) {
            switch ( e->key() ) {
                case Qt::Key_V : paste(); break;
                case Qt::Key_W : mainTabWidget->closeCurrentTab(); break;
            }
        }
    }

    QMainWindow::keyPressEvent( e );
}

void DtMainWindow::createMenus() {
    fileMenu = menuBar()->addMenu( tr( "&File" ) );
    connect( fileMenu, SIGNAL( aboutToShow() ), this, SLOT( onShowFileMenu() ) );
    actOpenFile = fileMenu->addAction( tr( "Open demo" ), this, SLOT( openFile() ) );
    actOpenUrl = fileMenu->addAction( tr( "Open link" ), this, SLOT( openUrl() ) );
    actOpenConfig = fileMenu->addAction( tr( "Open config" ), this, SLOT( openConfig() ) );

    recentMenu = fileMenu->addMenu( tr( "Recent" ) );
    recentFilesMenu = recentMenu->addMenu( tr( "Recent files" ) );
    connect( recentFilesMenu, SIGNAL( aboutToShow() ), this, SLOT( onShowRecentFilesMenu() ) );
    recentUrlsMenu = recentMenu->addMenu( tr( "Recent links" ) );
    connect( recentUrlsMenu, SIGNAL( aboutToShow() ), this, SLOT( onShowRecentUrlsMenu() ) );
    recentConfigsMenu = recentMenu->addMenu( tr( "Recent configs" ) );
    connect( recentConfigsMenu, SIGNAL( aboutToShow() ), this, SLOT( onShowRecentConfigsMenu() ) );

    connect( recentMenu, SIGNAL( aboutToShow() ), this, SLOT( onShowRecentMenu() ) );
    fileMenu->addSeparator();
    actImportXml = fileMenu->addAction( tr( "Import..." ), this, SLOT( importXml() ) );
    exportXmlMenu = fileMenu->addMenu( tr( "Export..." ) );
    exportXmlMenu->addAction( tr( "Export XML" ), this, SLOT( exportXml() ) );
    exportXmlMenu->addAction( tr( "Export compressed XML" ), this, SLOT( exportCompressedXml() ) );
    fileMenu->addSeparator();
    actPlayDemo = fileMenu->addAction( tr( "Play demo" ), this, SLOT( actPlaySelectedDemo() ) );
    fileMenu->addSeparator();
    actQuit = fileMenu->addAction( tr( "&Quit" ), this, SLOT( close() ) );

    toolsMenu = menuBar()->addMenu( tr( "&Tools" ) );
    connect( toolsMenu, SIGNAL( aboutToShow() ), this, SLOT( onShowToolsMenu() ) );
    connect( toolsMenu, SIGNAL( aboutToHide() ), this, SLOT( onHideToolsMenu() ) );
    actFindFrags = toolsMenu->addAction( tr( "&Find demos" ), this, SLOT( openFindDemoDialog() ),
                                         QKeySequence( "Ctrl+S" ) );
    actFindFrags = toolsMenu->addAction( tr( "&Find frags" ), this, SLOT( openFragsTab() ) );
    actFindFrags->setCheckable( true );
    actFindText = toolsMenu->addAction( tr( "Find &chat" ), this, SLOT( openFindTextDialog() ) );
    actEditDemo = toolsMenu->addAction( tr( "&Edit demo" ), this, SLOT( openDemoEditor() ),
                                        QKeySequence( "F4" ) );
    actConverter = toolsMenu->addAction( tr( "Co&nvert" ), this, SLOT( openConvertDialog() ) );

    toolsMenu->addSeparator();

    actAutoRename = toolsMenu->addAction( tr( "&Auto Rename" ), this, SLOT( autoRename() ) );
    actOrganize = toolsMenu->addAction( tr( "O&rganize" ), this, SLOT( organize() ) );

    toolsMenu->addSeparator();

    actUpdateTable = toolsMenu->addAction( tr( "&Update table" ), this,
                                          SLOT( updateMainTable() ), QKeySequence( "F5" ) );

    optionsMenu = menuBar()->addMenu( tr( "&Options" ) );
    viewMenu = optionsMenu->addMenu( tr( "View" ) );

    actKeepDirTreeOpened = viewMenu->addAction( tr( "Directory tree" ),
                                                this, SLOT( toggleDirTreeOpened() ) );
    actKeepDirTreeOpened->setCheckable( true );
    actKeepDirTreeOpened->setChecked( config.dirTreeAlwaysOpened );

    viewMenu->addSeparator();

    mainTableColumnsMenu = viewMenu->addMenu( tr( "Main table columns" ) );

    for ( int i = 0; i < defaultMainTableColumnNames.count(); ++i ) {
        QAction* actShowColumn = mainTableColumnsMenu->addAction( defaultMainTableColumnNames.at( i ),
                                                                  this,
                                                                  SLOT( showColumnActionClicked() ) );
        actShowColumn->setCheckable( true );
        actShowColumn->setChecked( config.mainTableVisibleColumns.at( i ) );
    }

    findDemosTableColumnsMenu = viewMenu->addMenu( tr( "Find demos table columns" ) );

    for ( int i = 0; i < defaultFindDemoTableColumnNames.count(); ++i ) {
        QAction* actShowColumn = findDemosTableColumnsMenu->addAction( defaultFindDemoTableColumnNames.at( i ),
                                                                       this,
                                                                       SLOT( showColumnActionClicked() ) );
        actShowColumn->setCheckable( true );
        actShowColumn->setChecked( config.findDemoTableVisibleColumns.at( i ) );
    }

    findFragsTableColumnsMenu = viewMenu->addMenu( tr( "Find frags table columns" ) );

    for ( int i = 0; i < defaultFindFragsTableColumnNames.count(); ++i ) {
        QAction* actShowColumn = findFragsTableColumnsMenu->addAction( defaultFindFragsTableColumnNames.at( i ),
                                                                       this,
                                                                       SLOT( showColumnActionClicked() ) );
        actShowColumn->setCheckable( true );
        actShowColumn->setChecked( config.findFragsTableVisibleColumns.at( i ) );
    }

    findTextTableColumnsMenu = viewMenu->addMenu( tr( "Find text table columns" ) );

    for ( int i = 0; i < defaultFindTextTableColumnNames.count(); ++i ) {
        QAction* actShowColumn = findTextTableColumnsMenu->addAction( defaultFindTextTableColumnNames.at( i ),
                                                                       this,
                                                                       SLOT( showColumnActionClicked() ) );
        actShowColumn->setCheckable( true );
        actShowColumn->setChecked( config.findTextTableVisibleColumns.at( i ) );
    }

    viewMenu->addSeparator();

    actShowHeaderInfo = viewMenu->addAction( tr( "Show demo header info" ),
                                             this, SLOT( toggleHeaderInfo() ) );
    actShowHeaderInfo->setCheckable( true );
    actShowHeaderInfo->setChecked( config.headerInfoVisible );

    actShowClantags = viewMenu->addAction( tr( "Show clan tags" ),
                                             this, SLOT( toggleClantags() ) );
    actShowClantags->setCheckable( true );
    actShowClantags->setChecked( config.showClanTags );

    optionsMenu->addSeparator();
    actPreferences = optionsMenu->addAction( QIcon( ":/res/preferences-system.png" ),
                                             tr( "&Preferences" ), this, SLOT( showOptionsDialog() ) );

    helpMenu = menuBar()->addMenu( tr( "&Help" ) );
    actHelpContents = helpMenu->addAction( tr( "Contents" ), this, SLOT( showHelp() ),
                                           QKeySequence( "F1" ) );
    helpMenu->addSeparator();
    actAbout = helpMenu->addAction( tr( "About" ), this, SLOT( showAbout() ) );
}

void DtMainWindow::updateMainTable() {
    mainTabWidget->updateMainTable();
}

void DtMainWindow::toggleClantags() {
    config.showClanTags = actShowClantags->isChecked();
}

void DtMainWindow::onShowFileMenu() {
    actPlayDemo->setEnabled( mainDemoTable->selectionModel()->hasSelection() );
    exportXmlMenu->setEnabled( mainDemoTable->selectionModel()->hasSelection() );
}

void DtMainWindow::openConvertDialog() {
    DtDemo* demo = mainDemoTable->demoAt( mainDemoTable->getSelectedRow() );

    if ( !demo ) {
        return;
    }

    DtConvertDialog dialog( demo, this );
    dialog.exec();
}

void DtMainWindow::openFindDemoDialog() {
    if ( !findDemoDialog ) {
        findDemoDialog = new DtFindDemoDialog( this );
    }

    DtFindDemoOptions opt = findDemoDialog->exec();

    if ( opt.search ) {
        if ( !findDemo ) {
            findDemo = new DtFindDemo( this );
            connect( findDemo, SIGNAL( scanFinished() ), this, SLOT( onScanFinished() ) );
        }

        findDemo->find( opt );
    }
}

void DtMainWindow::openFindTextDialog() {
    if ( !findTextDialog ) {
        findTextDialog = new DtFindTextDialog( this );
    }

    if ( findTextDialog->exec() ) {
        if ( !findText ) {
            findText = new DtFindText( this );
            connect( findText, SIGNAL( scanFinished() ), this, SLOT( onScanFinished() ) );
        }

        findText->find( config.lastFindTextString, config.findTextMatchCase,
                        config.findTextIgnoreColors );
    }
}

void DtMainWindow::organize() {
    if ( mainTab->currentIndex() != 0 ) {
        mainTab->setCurrentIndex( 0 );
    }

    mainDemoTable->organizeDemos();
}

void DtMainWindow::disconnectQl() {
    applyPlayerOptions();
}

void DtMainWindow::onShowConnectMenu() {
//    actDisconnect->setEnabled( !( demoPlayer && !demoPlayer->isQzLoggedIn() ) );
}

void DtMainWindow::toggleMainTableNameColumn() {
    mainTableColumnsMenu->actions().at( 0 )->activate( QAction::Trigger );
}

void DtMainWindow::autoRename() {
    if ( mainTab->currentIndex() != 0 ) {
        mainTab->setCurrentIndex( 0 );
    }

    mainDemoTable->autoRenameItem();
}

void DtMainWindow::onShowRecentMenu() {
    recentMenu->clear();

    QList< QMenu* > menus;
    menus << recentFilesMenu << recentUrlsMenu << recentConfigsMenu;

    recentMenu->addMenu( menus.at( config.lastRecentMenuIndex ) );

    for ( int i = 0; i < menus.count(); ++i ) {
        if ( i != config.lastRecentMenuIndex ) {
            recentMenu->addMenu( menus.at( i ) );
        }
    }
}

void DtMainWindow::showColumnActionClicked() {
    QAction* act = qobject_cast< QAction* >( sender() );

    if ( !act ) {
        return;
    }

    if ( act->parent() == mainTableColumnsMenu ) {
        int columnIndex = defaultMainTableColumnNames.indexOf( act->iconText() );

        config.mainTableVisibleColumns.setBit( columnIndex,act->isChecked() );
        mainDemoTable->setColumnHidden( columnIndex, !act->isChecked() );
    }
    else {
        int columnIndex;

        if ( act->parent() == findDemosTableColumnsMenu ) {
            columnIndex = defaultFindDemoTableColumnNames.indexOf( act->iconText() );
            config.findDemoTableVisibleColumns.setBit( columnIndex, act->isChecked() );

            foreach ( DtScanWidget* scanWidget, mainTab->scanWidgets ) {
                DtFindDemoTable* table = qobject_cast< DtFindDemoTable* >( scanWidget->demoTable );

                if ( table ) {
                    table->setColumnHidden( columnIndex, !act->isChecked() );
                }
            }
        }
        else if ( act->parent() == findFragsTableColumnsMenu ) {
            columnIndex = defaultFindFragsTableColumnNames.indexOf( act->iconText() );
            config.findFragsTableVisibleColumns.setBit( columnIndex, act->isChecked() );

            foreach ( DtScanWidget* scanWidget, mainTab->scanWidgets ) {
                DtFindFragsTable* table = qobject_cast< DtFindFragsTable* >( scanWidget->demoTable );

                if ( table ) {
                    table->setColumnHidden( columnIndex, !act->isChecked() );
                }
            }
        }
        else if ( act->parent() == findTextTableColumnsMenu ) {
            columnIndex = defaultFindTextTableColumnNames.indexOf( act->iconText() );
            config.findTextTableVisibleColumns.setBit( columnIndex, act->isChecked() );

            foreach ( DtScanWidget* scanWidget, mainTab->scanWidgets ) {
                DtFindTextTable* table = qobject_cast< DtFindTextTable* >( scanWidget->demoTable );

                if ( table ) {
                    table->setColumnHidden( columnIndex, !act->isChecked() );
                }
            }
        }
    }
}

void DtMainWindow::toggleHeaderInfo() {
    config.headerInfoVisible = actShowHeaderInfo->isChecked();

    DtMainWidget* mainWidget = qobject_cast< DtMainWidget* >( mainTab->widget( 0 ) );
    mainWidget->setInfoPanelVisible( config.headerInfoVisible );

    foreach ( DtScanWidget* scanWidget, mainTab->scanWidgets ) {
        scanWidget->setInfoPanelVisible( config.headerInfoVisible );
    }
}

void DtMainWindow::openConfig() {
    QString directory = config.getSelectedGame() == Q_LIVE ?
                        config.getQzBasePath() : config.getQaBasePath();

    QString fName = QFileDialog::getOpenFileName( this, tr( "Open config" ),
                                                  directory, defaultConfigFormat );
    if ( !fName.isEmpty() ) {
        addConfig( fName );
        addEntryToRecentList( fName, config.recentOpenedConfigs );
    }
}

void DtMainWindow::addConfig( const QString& file ) {
    QProcess cfgEditor;

    QString editorFileName =
#ifdef Q_OS_LINUX
    "qldtce";
#else
    "qldtce.exe";
#endif

    cfgEditor.startDetached( QCoreApplication::applicationDirPath() + "/" + editorFileName,
                             QStringList() << file );
}

void DtMainWindow::onShowRecentConfigsMenu() {
    recentConfigsMenu->clear();

    foreach ( const QString& fileName, config.recentOpenedConfigs ) {
        recentConfigsMenu->addAction( fileName, this, SLOT( openRecentConfig() ) );
    }
}

void DtMainWindow::openRecentConfig() {
    QAction* action = qobject_cast< QAction* >( sender() );

    if ( action ) {
        config.lastRecentMenuIndex = 2;
        addConfig( action->iconText() );
        addEntryToRecentList( action->iconText(), config.recentOpenedConfigs );
    }
}

void DtMainWindow::importXml() {
    if ( !QDir( config.lastImportPath ).exists() ) {
        config.lastImportPath = QDir::homePath();
    }

    QString fName = QFileDialog::getOpenFileName( this, tr( "Import demo" ),
                                                  config.lastImportPath, "Quake Demo (*.xml *.zip)" );
    if ( fName.isEmpty() ) {
        return;
    }

    QFileInfo fileInfo( fName );
    QString importedFile;
    bool cleanTmpFile = false;
    QString destDir = getDestDirectory();

    if ( fileInfo.suffix() == "zip" ) {
        QString extractedFile = files()->zipExtractFirstFile( fName, destDir );
        QString extractedFilePath = destDir + "/" + extractedFile;

        if ( extractedFile.isEmpty() || !QFile::exists( extractedFilePath ) ) {
            return;
        }

        importedFile = extractedFilePath;
        cleanTmpFile = true;
    }
    else {
        importedFile = fName;
    }

    QFileInfo importedFileInfo( importedFile );
    DtDemo importedDemo( importedFileInfo );
    config.lastImportPath = importedFileInfo.absolutePath();
    QString progressText = tr( "Importing" ) + " " + QFileInfo( importedFile ).fileName();

    if ( !progressDialog ) {
        progressDialog = new DtProgressDialog( progressText, this );
    }
    else {
        progressDialog->setWindowTitle( progressText );
    }

    progressDialog->setData( "" );
    progressDialog->show();
    connect( &importedDemo, SIGNAL( readPosition( int ) ),
             progressDialog, SLOT( setPos( int ) ) );

    if ( importedDemo.importXml( destDir ) ) {
        updateMainList();
    }
    else {
        QMessageBox::critical( this, tr( "Error" ), tr( "XML parse error" ) );
    }

    progressDialog->close();

    if ( cleanTmpFile ) {
        QFile::remove( importedFile );
    }
}

void DtMainWindow::exportXml() {
    writeXml();
}

void DtMainWindow::exportCompressedXml() {
    writeXml( true );
}

void DtMainWindow::writeXml( bool zip ) {
    DtEditTab* editTab = qobject_cast< DtEditTab* >( mainTabWidget->currentWidget() );
    DtDemo* exportedDemo;

    if ( editTab ) {
        exportedDemo = editTab->currentDemo();
    }
    else if ( mainTabWidget->currentIndex() == 0 ) {
        exportedDemo = mainDemoTable->demoAt( mainDemoTable->getSelectedRow() );

        if ( !exportedDemo ) {
            return;
        }
    }
    else {
        return;
    }

    if ( !QDir( config.lastExportPath ).exists() ) {
        config.lastExportPath = QDir::homePath();
    }

    QString ext = zip ? ".zip" : ".xml";
    QString fileFilter = zip ? tr( "Compressed XML files (*%1)" ) : tr( "XML files (*%1)" );
    QString fName;
    QFileDialog dialog( this, tr( "Export to" ), config.lastExportPath, fileFilter.arg( ext ) );

    dialog.setConfirmOverwrite( false );
    dialog.setFileMode( QFileDialog::AnyFile );
    dialog.selectFile( config.lastExportPath + "/"
                       + exportedDemo->fileInfo().fileName( false ) + ext );

    if ( dialog.exec() ) {
        fName = dialog.selectedFiles().first();
    }

    if ( fName.isEmpty() ) {
        return;
    }

    if ( !fName.endsWith( ext, Qt::CaseInsensitive ) ) {
        fName += ext;
    }

    writeXml( exportedDemo, fName, zip );

    config.lastExportPath = QFileInfo( fName ).absolutePath();
}

void DtMainWindow::writeXml( DtDemo* demo, const QString& fName, bool zip ) {
    if ( QFile::exists( fName ) ) {
        QString msg = tr( "File with the specified name already exists. Overwrite it?" );

        int btn = QMessageBox::question( this, tr( "File exists" ), msg,
                                         QMessageBox::Yes | QMessageBox::Cancel );
        if ( btn != QMessageBox::Yes ) {
            return;
        }

        if ( !QFile::remove( fName ) ) {
            return;
        }
    }

    QString progressText = tr( "Exporting %1" ).arg( demo->fileInfo().fileName( false ) );

    if ( !progressDialog ) {
        progressDialog = new DtProgressDialog( progressText, this );
    }
    else {
        progressDialog->setWindowTitle( progressText );
    }

    progressDialog->setData( "" );
    progressDialog->show();
    connect( demo, SIGNAL( readPosition( int ) ),
             progressDialog, SLOT( setPos( int ) ) );

    QString tmpName = zip ? QString( "%1/tmp%2.xml" )
                      .arg( config.lastExportPath )
                      .arg( QString::number( qrand(), 16 ) ) : fName;

    if ( zip && QFile::exists( tmpName ) ) {
        QFile::remove( tmpName );
    }

    bool exported = demo->exportXml( tmpName );

    progressDialog->close();
    disconnect( demo, SIGNAL( readPosition( int ) ),
                progressDialog, SLOT( setPos( int ) ) );

    if ( zip && exported ) {
        files()->packFile( tmpName, demo->fileInfo().fileName( false ) + ".xml", fName );
        QFile::remove( tmpName );
    }
}

void DtMainWindow::onShowToolsMenu() {
    findFragsWidgetVisible = config.frags.tabOpened;
    actFindFrags->setChecked( config.frags.tabOpened );

    bool mainSelection = mainDemoTable->selectionModel()->hasSelection();

    if ( mainTabWidget->currentIndex() == 0 ) {
        actEditDemo->setEnabled( mainSelection );
    }
    else {
        DtScanWidget* scanWidget = mainTabWidget->currentScanWidget();

        if ( scanWidget ) {
            actEditDemo->setEnabled( scanWidget->demoTable->selectionModel()->hasSelection() );
        }
        else {
            actEditDemo->setEnabled( false );
        }
    }

    actAutoRename->setEnabled( mainSelection );
    actConverter->setEnabled( mainSelection );
}

void DtMainWindow::onHideToolsMenu() {
    actEditDemo->setEnabled( true );
}

void DtMainWindow::dragEnterEvent( QDragEnterEvent* e ) {
    if ( demoDrag ) {
        e->acceptProposedAction();
        return;
    }

    if ( e->mimeData()->hasUrls() ) {
        bool accept = false;

        foreach ( const QUrl& url, e->mimeData()->urls() ) {
            QFileInfo urlInfo( url.toLocalFile() );

            if ( acceptedFileFormat( urlInfo.suffix() ) ) {
                accept = true;
            }
            else if ( urlInfo.isDir() ) {
                QDir droppedDir( urlInfo.absoluteFilePath() );
                QDirIterator it( droppedDir, QDirIterator::Subdirectories );

                while ( it.hasNext() ) {
                    urlInfo.setFile( it.next() );

                    if ( !urlInfo.isFile() ) {
                        continue;
                    }

                    if ( acceptedFileFormat( urlInfo.suffix() ) ) {
                        accept = true;
                        break;
                    }
                }
            }

            if ( accept ) {
                e->acceptProposedAction();
                return;
            }
        }
    }
}

void DtMainWindow::dragMoveEvent( QDragMoveEvent* e ) {
    if ( demoDrag ) {
        DtDemoTable* table = qobject_cast< DtDemoTable* >( demoDrag->parent() );

        if ( table ) {
            table->demoDragMoved( e );
        }

        if ( config.dirTreeAlwaysOpened ) {
            DtMainWidget* mainWidget = qobject_cast< DtMainWidget* >( mainTabWidget->widget( 0 ) );

            if ( mainWidget && mainWidget->dirTree ) {
                mainWidget->dirTree->demoDragMoved( e );
            }
        }
    }
}

QString DtMainWindow::getDestDirectory() {
    QString destDir = config.getSelectedGame() == Q_LIVE ?
                      config.getQzDemoPath() : config.getQaDemoPath();

    if ( config.dropDemosToNewDir ) {
        QString destNewDir = destDir + "/" + defaultNewDirName;
        QDir newDir( destNewDir );

        if ( !newDir.exists() && !newDir.mkdir( destNewDir ) ) {
            QMessageBox::warning( this, tr( "Error" ), tr( "Couldn't create a new temporary directory" ) );
            return "";
        }

        destDir = destNewDir;

        if ( config.dirTreeAlwaysOpened &&
             mainTabWidget->mainWidget  &&
             mainTabWidget->mainWidget->dirTree )
        {
            mainTabWidget->mainWidget->dirTree->showTmpNewDir();
        }
    }
    else {
        destDir = currentWorkingDir;
    }

    return destDir;
}

void DtMainWindow::updateMainList() {
    if ( !config.dirTreeAlwaysOpened ) {
        currentWorkingDir = getDestDirectory();
    }

    mainTabWidget->updateTabName( currentWorkingDir );
    mainTabWidget->updateMainTable();

    if ( config.dropDemosToNewDir && config.dirTreeAlwaysOpened ) {
        if ( mainTabWidget->mainWidget && mainTabWidget->mainWidget->dirTree ) {
            mainTabWidget->mainWidget->dirTree->updateTmpNewDir();
        }
    }
}

void DtMainWindow::addFilesToMainList( const QStringList& newFiles, const QString& destDir ) {
    if ( newFiles.isEmpty() ) {
        return;
    }

    if ( destDir.isEmpty() ) {
        QMessageBox::warning( this, tr( "Error" ), tr( "Destination path is empty."
                                                       "Did you entered a valid home path in the options?" ) );
        return;
    }

    if ( !QDir( destDir ).exists() ) {
        QMessageBox::warning( this, tr( "Error" ), tr( "Destination path not exists."
                                                       "Did you entered a valid home path in the options?" ) );
        return;
    }

    mainTabWidget->setCurrentIndex( 0 );
    mainDemoTable->setFocus();

    files()->copyDroppedFiles( newFiles, destDir );
    updateMainList();
}

bool DtMainWindow::addUrlsToMainList( const QList< QUrl >& newUrls, const QString& destDir ) {
    mainTabWidget->setCurrentIndex( 0 );
    mainDemoTable->setFocus();

    bool ret = files()->copyDroppedUrls( newUrls, destDir );
    updateMainList();

    return ret;
}

void DtMainWindow::openPath( QStringList& newFiles, QStringList& newConfigs, const QString& path ) {
    QFileInfo fileInfo( path );

    if ( !fileInfo.exists() ) {
        return;
    }

    QString fileExt = fileInfo.suffix();
    QString filePath = fileInfo.absoluteFilePath();

    if ( acceptedFileFormat( fileExt ) ) {
        if ( isConfigFile( fileExt ) ) {
            newConfigs << filePath;
        }
        else {
            newFiles << filePath;
        }
    }
    else if ( fileInfo.isDir() ) {
        QDir droppedDir( filePath );
        QDirIterator it( droppedDir, QDirIterator::Subdirectories );

        while ( it.hasNext() ) {
            fileInfo.setFile( it.next() );
            fileExt = fileInfo.suffix();
            filePath = fileInfo.absoluteFilePath();

            if ( !fileInfo.isFile() ) {
                continue;
            }

            if ( acceptedFileFormat( fileExt ) ) {
                if ( isConfigFile( fileExt ) ) {
                    newConfigs << filePath;
                }
                else {
                    newFiles << filePath;
                }
            }
        }
    }
}

void DtMainWindow::dropEvent( QDropEvent* e ) {
    if ( demoDrag ) {
        DtDemoTable* table = qobject_cast< DtDemoTable* >( demoDrag->parent() );

        if ( table ) {
            table->demoDragDropped( e );
        }

        if ( config.dirTreeAlwaysOpened ) {
            if ( mainTabWidget->mainWidget && mainTabWidget->mainWidget->dirTree ) {
                mainTabWidget->mainWidget->dirTree->demoDragDropped( e );
            }
        }
    }
    else {
        if ( e->mimeData()->hasUrls() ) {
            QStringList droppedFiles;
            QStringList droppedConfigs;
            QString destDir = getDestDirectory();

            foreach ( const QUrl& url, e->mimeData()->urls() ) {
                QString filePath = url.toLocalFile();

                if ( QFileInfo( filePath ).absolutePath() != destDir ) {
                    openPath( droppedFiles, droppedConfigs, url.toLocalFile() );
                }
            }

            addFilesToMainList( droppedFiles, getDestDirectory() );
            openConfigs( droppedConfigs );
        }
    }
}

void DtMainWindow::openConfigs( const QStringList& configFiles ) {
    foreach ( const QString& cfgFile, configFiles ) {
        addConfig( cfgFile );
    }
}

bool DtMainWindow::deleteDir( const QString& dirPath ) {
    QDir dir( dirPath );

    if ( dir.exists() ) {
        if ( !DtDirTree::rmDir( dir ) ) {
            return false;
        }
    }

    return true;
}

void DtMainWindow::deleteTmpDirs() {
    deleteDir( config.getQzDemoPath() + '/' + defaultNewDirName );
    deleteDir( config.getQaDemoPath() + '/' + defaultNewDirName );
    deleteDir( config.getQzDemoPath() + '/' + defaultTmpDirName );
    deleteDir( config.getQaDemoPath() + '/' + defaultTmpDirName );
}

void DtMainWindow::applyOptions() {
    if( dirTreeAlwaysOpened != config.dirTreeAlwaysOpened ) {
        mainTab->setTreeButtonVisible( !config.dirTreeAlwaysOpened );
        emit setDirTreeOpened( config.dirTreeAlwaysOpened );
        dirTreeAlwaysOpened = config.dirTreeAlwaysOpened;
        actKeepDirTreeOpened->setChecked( config.dirTreeAlwaysOpened );
    }
}

void DtMainWindow::applyPlayerOptions() {
    demoPlayer->close();
    demoPlayer->exit();
    delete demoPlayer;
    demoPlayer = new DtPlayerWindow( this );
}

DtFileDialog* DtMainWindow::files() {
    return fileDialog;
}

void DtMainWindow::deleteTmpFile() {
    QFile f( tempFile );

    if ( !f.remove() ) {
        qDebug( "Couldn't delete temporary file" );
    }

    f.close();
    tempFile.clear();
}

void DtMainWindow::playSelectedDemo() {
    playDemo( TD_CUR );
}

void DtMainWindow::actPlaySelectedDemo() {
    playSelectedDemo();
}

selectedDemo_t DtMainWindow::getCurrentSelectedDemo( tableDemoPos pos ) {
    bool noLoop = false;
    bool stopPlay = false;

    if ( pos == TD_NEXT_NOLOOP ) {
        pos = TD_NEXT;
        noLoop = true;
    }

    selectedDemo_t selected;
    selected.demo = 0;

    if ( pos == TD_CUR ) {
        DtTablesWidget* tablesWidget = qobject_cast< DtTablesWidget* >( mainTabWidget->currentWidget() );

        if ( tablesWidget ) {
            currentPlayDemoTable = tablesWidget->demoTable;
            currentPlayDemoTable->setCurrentDemoPlaying();
        }
    }

    DtDemoTable* table = currentPlayDemoTable;

    if ( !table || !table->getCurrentPlayDemoIndex() ) {
        return selected;
    }

    if ( pos == TD_NEXT ) {
        int nextDemoRow = currentPlayDemoTable->getCurrentPlayDemoIndex()->row() + 1;

        if ( noLoop && ( nextDemoRow > currentPlayDemoTable->rowCount() - 1 ) ) {
            stopPlay = true;
        }
        else {
            currentPlayDemoTable->setNextDemoPlaying();
        }
    }
    else if ( pos == TD_PREV ) {
        currentPlayDemoTable->setPrevDemoPlaying();
    }

    bool part = table->inherits( "DtScanTable" );
    int startTime = 0;
    int endTime = 0;
    int timerValue = 0;
    int tableIndex = table->getCurrentPlayDemoIndex()->row();

    if ( !stopPlay ) {
        selected.demo = table->demoAt( tableIndex );
    }

    if ( !stopPlay && part ) {
        if ( table->inherits( "DtFindFragsTable" ) ) {
            QTableWidgetItem* timeItem = table->item( tableIndex, DtFindFragsTable::CT_TIME );
            startTime = timeItem->data( Qt::UserRole ).toList().at( 1 ).toInt();

            int addTimeBegin = config.frags.segAddTimeBegin * 1000;
            int addTimeEnd = config.frags.segAddTimeEnd * 1000;

            QTableWidgetItem* lengthItem = table->item( tableIndex, DtFindFragsTable::CT_SEGMENTLENGTH );

            endTime = startTime
                      + lengthItem->data( Qt::UserRole ).toList().at( 1 ).toInt()
                      + addTimeEnd;

            if ( startTime - addTimeBegin > selected.demo->getMapRestartTime() ) {
                startTime -= addTimeBegin;
            }

            timerValue = DtDemoTable::stringToMsec( timeItem->text() );
        }
        else { /* DtFindTextTable */
            QTableWidgetItem* timeItem = table->item( tableIndex, DtFindTextTable::CT_TIME );
            int time = timeItem->data( Qt::UserRole ).toList().at( 1 ).toInt();
            int addTimeBegin = config.textSegAddTimeBegin * 1000;
            int addTimeEnd = config.textSegAddTimeEnd * 1000;
            startTime = time - addTimeBegin;
            endTime = time + addTimeEnd;
            timerValue = DtDemoTable::stringToMsec( timeItem->text() );
        }
    }

    selected.part = part;
    selected.startTime = startTime;
    selected.endTime = endTime;
    selected.timerValue = timerValue;

    return selected;
}

void DtMainWindow::allowPlayDemo() {
    canPlayDemo = true;
}

void DtMainWindow::hideBusyCursor() {
    setCursor( Qt::ArrowCursor );
}

void DtMainWindow::runDemo() {
    if ( DtDemo* demo = qobject_cast< DtDemo* >( sender() ) ) {
        demo->disconnect( this, SLOT( runDemo() ) );

        if ( !tempFile.isEmpty() ) {
            demoPlayer->playDemo( tempFile, demo->fileInfo().baseName );
        }
    }
}

void DtMainWindow::playDemo( tableDemoPos sDemo ) {
    if ( !canPlayDemo ) {
        return;
    }

    canPlayDemo = false;

    QTimer::singleShot( 500, this, SLOT( allowPlayDemo() ) );

    selectedDemo_t selected = getCurrentSelectedDemo( sDemo );

    if ( !selected.demo ) {
        if ( currentPlayDemoTable ) {
            currentPlayDemoTable->clearMark();
            currentPlayDemoTable->stopDemoPlay();
        }

        if ( demoPlayer->isVisible() ) {
            demoPlayer->close();
        }

        return;
    }

    currentPlayDemoTable->clearMark();

    if ( !tempFile.isEmpty() ) {
        deleteTmpFile();
    }

    if ( !selected.part ) {
        setCursor( Qt::BusyCursor );

        if ( selected.demo->getProto() == Q3_68 ) {
            demoPlayer->playDemo( selected.demo->fileInfo().fileName() );
        }
        else {
            DtDemo* demo = selected.demo;

//            if ( !demo->isGamesateParsed() ) {
//                demo->parseGamestateMsg();
//            }

            demoPlayer->playDemo( demo->fileInfo().fileName() );
        }
    }
    else {
        setCursor( Qt::WaitCursor );
        setEnabled( false );

        DtWriteOptions opts;
        opts.cutSegments.append( cutSegment( selected.startTime, selected.endTime ) );
        opts.timerInitialValue = selected.timerValue;
        opts.removeWarmup = true;

        tempFile = selected.demo->writeSegment( &opts );
        demoPlayer->playDemo( tempFile, selected.demo->fileInfo().baseName );
        setEnabled( true );
    }

    if ( selected.demo->getProto() == Q3_68 ) {
        QTimer::singleShot( 3000, this, SLOT( hideBusyCursor() ) );
    }
    else {
        hideBusyCursor();
    }

    if ( currentPlayDemoTable && currentPlayDemoTable->getCurrentPlayDemoIndex() ) {
        int row = currentPlayDemoTable->getCurrentPlayDemoIndex()->row();
        currentPlayDemoTable->selectRow( row );
    }
}

const QString& DtMainWindow::previewDemo( DtDemo* demo, DtWriteOptions options ) {
    setCursor( Qt::WaitCursor );
    setEnabled( false );

    if ( !tempFile.isEmpty() ) {
        deleteTmpFile();
    }

    tempFile = demo->writeSegment( &options );
    currentPlayDemoTable = 0;
    playDemoRequestSource = RS_PREVIEW;
    demoPlayer->playDemo( tempFile, demo->fileInfo().baseName );
    setEnabled( true );
    setCursor( Qt::ArrowCursor );

    return tempFile;
}

void DtMainWindow::onScanFinished() {
    DtScanWidget* table = mainTab->scanWidgets.size() ? mainTab->scanWidgets.back() : 0;

    if ( table && !table->isDone() ) {
        table->setDone( true );
    }
}

void DtMainWindow::addEntryToRecentList( const QString& entry, QStringList& list ) {
    int index;

    if ( ( index = list.indexOf( entry ) ) != -1 ) {
        list.move( index, 0 );
    }
    else {
        list.prepend( entry );
    }

    if ( list.size() > 10 ) {
        list.removeLast();
    }
}

void DtMainWindow::openFile() {
    QString format;

    format = "Quake Demos (";
    QStringList filters = demoNameFilters;
    filters << "*.zip" << "*.rar" << "*.7z";

    foreach ( const QString& filter, filters ) {
        format.append( filter ).append( " " );
    }

    format.append( ")" );
    QString fName = QFileDialog::getOpenFileName( this, tr( "Add file" ), lastOpenFilePath, format );

    if ( !fName.isEmpty() && addFile( fName ) ) {
        addEntryToRecentList( fName, config.recentOpenedFiles );
        lastOpenFilePath = QFileInfo( fName ).absolutePath();
    }
}

void DtMainWindow::openUrl() {
    DtOpenUrlDialog openUrlDialog( this );
    QUrl url;

    DtOpenUrlDialog::dialogButtons button = openUrlDialog.exec( url );

    if ( button == DtOpenUrlDialog::BTN_OK && url.isValid() && addUrl( url ) ) {
        addEntryToRecentList( url.toString(), config.recentOpenedUrls );
    }
}

bool DtMainWindow::addFile( const QString& file ) {
    QString destDir = getDestDirectory();

    if ( QFileInfo( file ).absolutePath() != destDir ) {
        if ( !destDir.isEmpty() ) {
            addFilesToMainList( QStringList() << file, destDir );
            return true;
        }
    }

    return false;
}

bool DtMainWindow::addUrl( const QUrl& url ) {
    QString destDir = getDestDirectory();

    if ( !destDir.isEmpty() ) {
        return addUrlsToMainList( QList< QUrl >() << url, destDir );
    }

    return false;
}

void DtMainWindow::openRecentFile() {
    QAction* action = qobject_cast< QAction* >( sender() );

    if ( action ) {
        config.lastRecentMenuIndex = 0;
        addFile( action->iconText() );
        addEntryToRecentList( action->iconText(), config.recentOpenedFiles );
    }
}

void DtMainWindow::openRecentUrl() {
    QAction* action = qobject_cast< QAction* >( sender() );

    if ( action ) {
        config.lastRecentMenuIndex = 1;
        addUrl( action->iconText() );
        addEntryToRecentList( action->iconText(), config.recentOpenedUrls );
    }
}

void DtMainWindow::onShowRecentFilesMenu() {
    recentFilesMenu->clear();

    foreach ( const QString& fileName, config.recentOpenedFiles ) {
        recentFilesMenu->addAction( fileName, this, SLOT( openRecentFile() ) );
    }
}

void DtMainWindow::onShowRecentUrlsMenu() {
    recentUrlsMenu->clear();

    foreach ( QString url, config.recentOpenedUrls ) {
        url.replace( '&', "&&" );
        recentUrlsMenu->addAction( url, this, SLOT( openRecentUrl() ) );
    }
}

void DtMainWindow::openFragsTab() {
    config.frags.tabOpened = !config.frags.tabOpened;
    config.save();

    if ( findFragsWidgetVisible != config.frags.tabOpened ) {
        emit setFindFragsVisible( config.frags.tabOpened );
        findFragsWidgetVisible = config.frags.tabOpened;
        actFindFrags->setChecked( config.frags.tabOpened );
    }
}

void DtMainWindow::openDemoEditor() {
    DtScanWidget* scanTab = mainTabWidget->currentScanWidget();
    DtDemo* demo;

    if ( !scanTab ) {
        demo = mainDemoTable->demoAt( mainDemoTable->getSelectedRow() );
    }
    else {
        demo = scanTab->demoTable->demoAt( scanTab->demoTable->getSelectedRow() );
    }

    if ( !demo ) {
        return;
    }

    setCursor( Qt::BusyCursor );
    mainTabWidget->addDemoEditor( demo );
    setCursor( Qt::ArrowCursor );
}

void DtMainWindow::showHelp() {
    DtHelpDialog help( this );
    help.exec();
}

void DtMainWindow::toggleDirTreeOpened() {
    config.dirTreeAlwaysOpened = !config.dirTreeAlwaysOpened;
    config.save();
    applyOptions();
}

void DtMainWindow::showOptionsDialog() {
    DtMainOptionsDialog optionsDialog( this );
    connect( &optionsDialog, SIGNAL( configChanged() ), this, SLOT( applyOptions() ) );
    connect( &optionsDialog, SIGNAL( playerWindowLoginDataChanged() ), demoPlayer,
             SIGNAL( clearLoginData() ), Qt::DirectConnection );
    connect( &optionsDialog, SIGNAL( playerWindowConfigChanged() ), this, SLOT( applyPlayerOptions() ),
             Qt::DirectConnection );
    optionsDialog.exec();
}

void DtMainWindow::saveSettings() {
    config.settings->beginGroup( "Main");
    config.settings->setValue( "size", size() );

#ifdef Q_OS_WIN
    config.settings->setValue( "pos", pos() );
#endif

    config.settings->endGroup();

    DtMainWidget* mainWidget = qobject_cast< DtMainWidget* >( mainTab->widget( 0 ) );
    delete mainWidget->fragsTab;

    emit closing();

    config.save();
}

void DtMainWindow::closeEvent( QCloseEvent* e ) {
    if ( canClose ) {
        qApp->processEvents();
        currentPlayDemoTable = 0;
        demoPlayer->exit();
        delete demoPlayer;
        saveSettings();
        e->accept();

        deleteTmpDirs();
    } else {
        e->ignore();
    }
}

void DtMainWindow::applicationMessage( const QString& msg ) {
    if ( msg.isEmpty() ) {
        return;
    }

    QStringList newFiles;
    QStringList newConfigs;
    QString destDir = getDestDirectory();

    if ( !msg.startsWith( "http://" ) ) {
        if ( QFileInfo( msg ).absolutePath() != destDir ) {
            openPath( newFiles, newConfigs, msg );
        }
    }
    else {
        QUrl url( msg );

        if ( !files()->isVisible() && url.isValid() && addUrl( url ) ) {
            addEntryToRecentList( url.toString(), config.recentOpenedUrls );
        }
    }

    addFilesToMainList( newFiles, destDir );
    openConfigs( newConfigs );
}

void DtMainWindow::showAbout() {
    DtAbout( this ).exec();
}

void DtMainWindow::initializeStrings() {
#define weaponName( id, fullName, shortName )   \
    weaponNames.insert( id, fullName );         \
    weaponNames.insert( id, shortName )

    weaponNames.insert( -1, tr( "Any" ) );

    weaponName( MOD_GAUNTLET,       "Gauntlet",         "GT" );
    weaponName( MOD_MACHINEGUN,     "Machinegun",       "MG" );
    weaponName( MOD_SHOTGUN,        "Shotgun",          "SG" );
    weaponName( MOD_GRENADE,        "Grenade Launcher", "GL" );
    weaponName( MOD_ROCKET,         "Rocket Launcher",  "RL" );
    weaponName( MOD_RAILGUN,        "Railgun",          "RG" );
    weaponName( MOD_PLASMA,         "Plasma Gun",       "PG" );
    weaponName( MOD_LIGHTNING,      "Lightning Gun",    "LG" );
    weaponName( MOD_HMG,            "Heavy Machinegun", "HMG" );
    weaponName( MOD_BFG,            "BFG 10K",          "BFG" );
    weaponName( MOD_CHAINGUN,       "Chaingun",         "CG" );
    weaponName( MOD_NAIL,           "Nailgun",          "NG" );
    weaponName( MOD_PROXIMITY_MINE, "Proxy Launcher",   "PL" );
    weaponName( MOD_KAMIKAZE,       "Kamikaze",         "KAM" );
    weaponName( MOD_TELEFRAG,       "Telefrag",         "TEL" );

#define gameTypeName( id, fullName, shortName )   \
    gameTypeNames.insert( id, fullName );         \
    gameTypeNames.insert( id, shortName )

#define gameTypeNameQa( id, fullName, shortName )   \
    gameTypeNamesQa.insert( id, fullName );         \
    gameTypeNamesQa.insert( id, shortName )

#define gameTypeNameCpma( id, fullName, shortName )   \
    gameTypeNamesCpma.insert( id, fullName );         \
    gameTypeNamesCpma.insert( id, shortName )

    gameTypeNames.insert( GT_ANY, tr( "Any", "Typenames|Any" ) );
    gameTypeName( GT_FFA,       "Free for All",     "FFA" );
    gameTypeName( GT_DUEL,      "Duel",             "Duel" );
    gameTypeName( GT_TDM,       "Team Deathmatch",  "TDM" );
    gameTypeName( GT_CA,        "Clan Arena",       "CA" );
    gameTypeName( GT_CTF,       "Capture the Flag", "CTF" );
    gameTypeName( GT_INSTAGIB,  "Instagib",         "IFFA" );
    gameTypeName( GT_INSTACTF,  "InstaCTF",         "ICTF" );
    gameTypeName( GT_INSTATDM,  "InstaTDM",         "ITDM" );
    gameTypeName( GT_1FCTF,     "One Flag CTF",     "1FCTF" );
    gameTypeName( GT_OVERLOAD,  "Overload",         "OL" );
    gameTypeName( GT_HARVESTER, "Harvester",        "HAR" );
    gameTypeName( GT_FREEZE,    "Freeze Tag",       "FT" );
    gameTypeName( GT_DOM,       "Domination",       "DOM" );
    gameTypeName( GT_AD,        "Attack & Defend",  "AD" );
    gameTypeName( GT_RR,        "Red Rover",        "RR" );
    gameTypeName( GT_RACE,      "Race",             "Race" );

    gameTypeNamesQa.insert( GT_ANY,       tr( "Any", "Typenames|Any" ) );
    gameTypeNameQa( GT_FFA,               "Free for All",     "FFA" );
    gameTypeNameQa( GT_DUEL,              "Duel",             "Duel" );
    gameTypeNameQa( GT_SINGLE_PLAYER_Q3,  "Single Player",    "SP" );
    gameTypeNameQa( GT_TEAM_Q3,           "Team Deathmatch",  "TDM" );
    gameTypeNameQa( GT_CTF_Q3,            "Capture the Flag", "CTF" );
    gameTypeNameQa( GT_1FCTF_Q3,          "One Flag CTF",     "1FCTF" );
    gameTypeNameQa( GT_OBELISK_Q3,        "Overload",         "OVR" );
    gameTypeNameQa( GT_HARVESTER_Q3,      "Harvester",        "HARV" );

    gameTypeNamesCpma.insert( GT_ANY,       tr( "Any", "Typenames|Any" ) );
    gameTypeNameCpma( GT_HOONY_CPMA,        "HoonyMode",         "HOONY" );
    gameTypeNameCpma( GT_FFA,               "Free for All",      "FFA" );
    gameTypeNameCpma( GT_DUEL,              "Duel",              "Duel" );
    gameTypeNameCpma( GT_TDM,               "Team Deathmatch",   "TDM" );
    gameTypeNameCpma( GT_CTF_CPMA,          "Capture the Flag",  "CTF" );
    gameTypeNameCpma( GT_CA_CPMA,           "Clan Arena",        "CA" );
    gameTypeNameCpma( GT_FREEZE_CPMA,       "Freeze Tag",        "FT" );
    gameTypeNameCpma( GT_CSTRIKE_CPMA,      "Capture Strike",    "CS" );
    gameTypeNameCpma( GT_NTF_CPMA,          "Not Team Fortress", "NTF" );

    defaultMainTableColumnNames
            << tr( "Name" ) << tr( "Recorded by" )
            << tr( "Type" ) << tr( "Map" )
            << tr( "Date" ) << tr( "Server" )
            << tr( "Protocol" );

    defaultFindDemoTableColumnNames
            << tr( "Name" ) << tr( "Directory" )
            << tr( "Recorded by" )
            << tr( "Type" ) << tr( "Map" )
            << tr( "Date" ) << tr( "Server" )
            << tr( "Protocol" );

    defaultFindFragsTableColumnNames
            << tr( "Name" ) << tr( "Recorded by" )
            << tr( "Type" ) << tr( "Map" )
            << tr( "Time" ) << tr( "Frags" )
            << tr( "Max Time" ) << tr( "Length" )
            << tr( "Weapon" ) << tr( "Protocol" );

    defaultFindTextTableColumnNames
            << tr( "Name" ) << tr( "Recorded by" )
            << tr( "Type" ) << tr( "Map" )
            << tr( "Date" ) << tr( "Time" )
            << tr( "Command" ) << tr( "Message" )
            << tr( "Length" ) << tr( "Protocol" );
}

DtOpenUrlDialog::DtOpenUrlDialog( QWidget* parent ) : QDialog( parent ) {
    setWindowTitle( tr( "Add link" ) );
    setFixedSize( 450, 100 );

    QVBoxLayout* dialogLayout = new QVBoxLayout;

    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    okButton = new QPushButton( "OK" );
    connect( okButton, SIGNAL( clicked() ), this, SLOT( okPressed() ) );
    okButton->setMinimumWidth( 120 );
    cancelButton = new QPushButton( tr( "Cancel" ) );
    connect( cancelButton, SIGNAL( clicked() ), this, SLOT( cancelPressed() ) );
    cancelButton->setMinimumWidth( 120 );

    urlEdit = new DtClearLineEdit;
    dialogLayout->addSpacing( 5 );
    dialogLayout->addWidget( urlEdit );

    buttonsLayout->addStretch( 3 );
    buttonsLayout->addWidget( okButton, 1, Qt::AlignRight );
    buttonsLayout->addWidget( cancelButton, 1, Qt::AlignRight );
    dialogLayout->addSpacing( 10 );
    dialogLayout->addLayout( buttonsLayout );
    setLayout( dialogLayout );

    ret = BTN_CANCEL;
    pEventLoop = 0;
}

void DtOpenUrlDialog::okPressed() {
    rDone( BTN_OK );
}

void DtOpenUrlDialog::cancelPressed() {
    rDone( BTN_CANCEL );
}

DtOpenUrlDialog::dialogButtons DtOpenUrlDialog::exec( QUrl& url ) {
    if ( pEventLoop ) {
        return BTN_CANCEL;
    }

    if ( !url.isEmpty() ) {
        urlEdit->setText( url.toString() );
    }

    show();

    QEventLoop eventLoop;
    pEventLoop = &eventLoop;
    QPointer< DtOpenUrlDialog > guard = this;
    eventLoop.exec( QEventLoop::DialogExec );
    pEventLoop = 0;

    if ( guard.isNull() ) {
        return BTN_CANCEL;
    }

    url.setUrl( urlEdit->text(),  QUrl::TolerantMode );

    return ret;
}

void DtOpenUrlDialog::rDone( dialogButtons retc ) {
    ret = retc;
    pEventLoop->exit( 0 );
    close();
}

