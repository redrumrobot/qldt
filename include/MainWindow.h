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

#ifndef DTMAINWINDOW_H
#define DTMAINWINDOW_H

#include "DemoTable.h"

#include <QMainWindow>
#include <QDialog>

class DtDirTree;
class DtFileDialog;
class DtDemo;
class DtMainTabWidget;
class DtToolsTabWidget;
class DtPlayerWindow;
class DtFileDialog;
class QActionGroup;
class DtProgressDialog;
class DtFindText;
class DtFindDemo;
class DtFindDemoDialog;
class DtFindTextDialog;
class QProcess;

class DtMainWindow : public QMainWindow {
    Q_OBJECT
public:
    DtMainWindow();

    DtFileDialog* files();
    const QString& previewDemo( DtDemo* demo, DtWriteOptions options );
    static void addConfig( const QString& file );
    void setTitle( const QString& newTitle = QString() );
    void toggleMainTableNameColumn();
    void writeXml( DtDemo* demo, const QString& fName, bool zip = false );
    static void addEntryToRecentList( const QString& entry, QStringList& list );

    QProcess* quakeArena;
    QProcess* otherApp;

public slots:
    void onScanFinished();
    void playDemo( tableDemoPos sDemo );
    void playSelectedDemo();
    void actPlaySelectedDemo();
    void showOptionsDialog();
    void applyOptions();
    void applyPlayerOptions();
    void showAbout();
    void applicationMessage( const QString& msg );
    void runDemo();
    void openDemoEditor();
    void updateMainTable();

private:
    DtMainTabWidget* mainTab;
    DtPlayerWindow* demoPlayer;
    DtFileDialog* fileDialog;
    QPointer< DtProgressDialog > progressDialog;
    QPointer< DtFindText > findText;
    QPointer< DtFindDemo > findDemo;
    QPointer< DtFindDemoDialog > findDemoDialog;
    QPointer< DtFindTextDialog > findTextDialog;

    QMenu* fileMenu;
    QMenu* toolsMenu;
    QMenu* connectMenu;
    QMenu* dirMenu;
    QMenu* viewMenu;
    QMenu* optionsMenu;
    QMenu* helpMenu;
    QMenu* recentMenu;
    QMenu* recentFilesMenu;
    QMenu* recentUrlsMenu;
    QMenu* recentConfigsMenu;
    QMenu* exportXmlMenu;
    QMenu* mainTableColumnsMenu;
    QMenu* findDemosTableColumnsMenu;
    QMenu* findFragsTableColumnsMenu;
    QMenu* findTextTableColumnsMenu;

    QAction* actQuit;
    QAction* actAdd;
    QAction* actDelete;
    QAction* actPlayDemo;
    QAction* actHelpContents;
    QAction* actAbout;
    QAction* actQlDemos;
    QAction* actQaDemos;
    QAction* actPreferences;
    QAction* actKeepDirTreeOpened;
    QAction* actOpenFile;
    QAction* actOpenUrl;
    QAction* actOpenConfig;
    QAction* actImportXml;
    QAction* actExportXml;
    QAction* actExportCompressedXml;
    QAction* actFindFrags;
    QAction* actEditDemo;
    QAction* actShowHeaderInfo;
    QAction* actShowClantags;
    QAction* actAutoRename;
    QAction* actDisconnect;
    QAction* actOrganize;
    QAction* actFindText;
    QAction* actConverter;
    QAction* actUpdateTable;

    QString selectedDemoName;
    QString tempFile;
    QString demoSubdir;
    static QString lastOpenFilePath;
    QString defaultTitle;

    bool canClose;
    bool initMenus;
    bool dirTreeAlwaysOpened;
    bool findFragsWidgetVisible;
    bool canPlayDemo;

    void createMenus();
    void saveSettings();

    selectedDemo_t getCurrentSelectedDemo( tableDemoPos pos );
    void deleteTmpFile();
    void deleteTmpDirs();
    bool deleteDir( const QString& dirPath );
    QString getDestDirectory();
    bool addFile( const QString& file );
    bool addUrl( const QUrl& url );
    void addFilesToMainList( const QStringList& newFiles, const QString& destDir );
    bool addUrlsToMainList( const QList< QUrl >& newUrls, const QString& destDir );
    void updateMainList();
    void openConfigs( const QStringList& configFiles );
    void writeXml( bool zip = false );
    void openPath( QStringList& newFiles, QStringList& newConfigs, const QString& path );
    void initializeStrings();

protected:
    void closeEvent( QCloseEvent* e );
    void dragEnterEvent( QDragEnterEvent* e );
    void dropEvent( QDropEvent* e );
    void dragMoveEvent( QDragMoveEvent* e );
    void keyPressEvent( QKeyEvent* e );

protected slots:
    void toggleDirTreeOpened();
    void openFile();
    void openUrl();
    void openConfig();
    void importXml();
    void exportXml();
    void exportCompressedXml();
    void onShowFileMenu();
    void onShowRecentMenu();
    void onShowRecentFilesMenu();
    void onShowRecentUrlsMenu();
    void onShowRecentConfigsMenu();
    void openRecentFile();
    void openRecentUrl();
    void openRecentConfig();
    void openFragsTab();
    void openFindTextDialog();
    void openFindDemoDialog();
    void openConvertDialog();
    void showHelp();
    void onShowToolsMenu();
    void onHideToolsMenu();
    void onShowConnectMenu();
    void toggleHeaderInfo();
    void toggleClantags();
    void showColumnActionClicked();
    void autoRename();
    void organize();
    void paste();
    void allowPlayDemo();
    void hideBusyCursor();
    void disconnectQl();

signals:
    void closing();
    void setDirTreeOpened( bool );
    void setFindFragsVisible( bool );
};

class QPushButton;
class QEventLoop;
class QUrl;
class DtClearLineEdit;

class DtOpenUrlDialog : public QDialog {
    Q_OBJECT
public:
    DtOpenUrlDialog( QWidget* parent = 0 );

    enum dialogButtons {
        BTN_CANCEL,
        BTN_OK
    };

    dialogButtons exec( QUrl& url );

protected:
    QPushButton* okButton;
    QPushButton* cancelButton;
    QEventLoop* pEventLoop;
    DtClearLineEdit* urlEdit;

    dialogButtons ret;

    void rDone( dialogButtons retc = BTN_CANCEL );

protected slots:
    void okPressed();
    void cancelPressed();
};


#endif // DTMAINWINDOW_H
