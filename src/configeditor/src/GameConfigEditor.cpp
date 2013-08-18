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

#include "GameConfigEditor.h"
#include "EditorOptionsDialog.h"
#include "TabWidget.h"
#include "SyntaxHighlighter.h"
#include "Config.h"
#include "ConfigEditorData.h"
#include "MainWindow.h"
#include "About.h"

#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QMessageBox>
#include <QTextDocumentWriter>
#include <QFileDialog>
#include <QSettings>
#include <QCloseEvent>
#include <QFile>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QCompleter>
#include <QStringListModel>
#include <QScrollBar>
#include <QListWidget>
#include <QApplication>
#include <QClipboard>
#include <QHeaderView>
#include <QTimer>
#include <QTextDocumentFragment>

#ifdef Q_OS_WIN
#include <QApplication>
#include <QDesktopWidget>
#endif

using namespace dtdata;

DtGameConfigEditor::DtGameConfigEditor( QWidget* parent ) : QMainWindow( parent ),
    defaultWindowName( "Config Editor" ),
    welcomePage( 0 )
{
    ceMainWindow = this;
    setWindowTitle( defaultWindowName );
    setWindowIcon( QIcon( ":/res/text-editor.png" ) );
    setStyleSheet( getStyle( "cfgeditor" ) );

    const int defaultWidth = 1024;
    const int defaultHeight = 768;

    resize( config.settings->value( "Editor/size", QSize( defaultWidth, defaultHeight ) ).toSize() );

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

    move( config.settings->value( "Editor/pos", QPoint( dx, dy ) ).toPoint() );
#endif

    modified = false;
    file = new QFile( this );

    tabs = new DtTabWidget( this );
    tabs->setTabsClosable( true );

    searchPanel = new DtEditorSearchPanel( this );
    searchPanel->setVisible( false );
    replacePanel = new DtEditorAdvancedSearchPanel( this );
    replacePanel->setVisible( false );

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->setMargin( 3 );
    mainLayout->addWidget( tabs );
    mainLayout->addWidget( searchPanel );
    mainLayout->addWidget( replacePanel );

    connect( tabs, SIGNAL( tabCloseRequested( int ) ), this, SLOT( tabClosed( int ) ) );
    connect( tabs, SIGNAL( currentChanged( int ) ), this, SLOT( tabChanged( int ) ) );

    setWindowModified( false );

    QMenu* fileMenu = new QMenu( tr( "&File" ), this );
    menuBar()->addMenu( fileMenu );

    QIcon newIcon( ":/res/document-new.png" );
    QIcon openIcon( ":/res/document-open.png" );
    QIcon saveIcon( ":/res/document-save.png" );
    QIcon saveAsIcon( ":/res/document-save-as.png" );
    QIcon saveAllIcon( ":/res/document-save-all.png" );
    QIcon closeIcon( ":/res/dialog-close.png" );
    QIcon undoIcon( ":/res/edit-undo.png" );
    QIcon redoIcon( ":/res/edit-redo.png" );
    QIcon cutIcon( ":/res/editcut.png" );
    QIcon pasteIcon( ":/res/editpaste.png" );
    QIcon selectAllIcon( ":/res/edit-select-all.png" );
    QIcon sortIcon( ":/res/view-sort-ascending.png" );
    QIcon findIcon( ":/res/find.png" );
    QIcon upIcon( ":/res/up.png" );
    QIcon downIcon( ":/res/down.png" );
    QIcon diffIcon( ":/res/diff.png" );
    QIcon commentIcon( ":/res/comment.png" );

    fileMenu->addAction( newIcon, tr( "&New" ), this, SLOT( newConfig() ),
                         QKeySequence( "Ctrl+N" ) );

    fileMenu->addAction( openIcon, tr( "&Open" ), this, SLOT( openConfig() ),
                         QKeySequence( "Ctrl+O" ) );

    recentMenu = fileMenu->addMenu( tr( "Recent" ) );
    connect( recentMenu, SIGNAL( aboutToShow() ), this, SLOT( onShowRecentMenu() ) );

    fileMenu->addSeparator();

    fileMenu->addAction( saveAllIcon, tr( "Save &All" ), this, SLOT( saveAll() ),
                         QKeySequence( "Ctrl+Shift+S" ) );

    fileMenu->addSeparator();

    fileMenu->addAction( saveIcon, tr( "&Save" ), this, SLOT( save() ),
                         QKeySequence( "Ctrl+S" ) );

    fileMenu->addAction( saveAsIcon, tr( "Save As" ), this, SLOT( saveAs() ) );

    fileMenu->addAction( closeIcon, tr("&Close"), this, SLOT( closeCurrentTab() ),
                         QKeySequence( "Ctrl+W" ) );

    fileMenu->addSeparator();

    fileMenu->addAction( tr("&Exit"), this, SLOT( close() ), QKeySequence( "Ctrl+Q" ) );

    QMenu* editMenu = new QMenu( tr( "&Edit" ), this );
    menuBar()->addMenu( editMenu );

    editMenu->addAction( undoIcon, tr( "&Undo" ), this, SLOT( undo() ),
                         QKeySequence( "Ctrl+Z" ) );

    editMenu->addAction( redoIcon, tr( "&Redo" ), this, SLOT( redo() ),
                         QKeySequence( "Ctrl+Shift+Z" ) );

    editMenu->addSeparator();

    editMenu->addAction( cutIcon, tr( "Cu&t" ), this, SLOT( cut() ),
                         QKeySequence( "Ctrl+X" ) );

    editMenu->addAction( icons->getIcon( I_COPY ), tr( "&Copy" ), this, SLOT( copy() ),
                         QKeySequence( "Ctrl+C" ) );

    editMenu->addAction( pasteIcon, tr( "&Paste" ), this, SLOT( paste() ),
                         QKeySequence( "Ctrl+V" ) );

    editMenu->addSeparator();

    editMenu->addAction( selectAllIcon, tr( "Select &All" ), this, SLOT( selectAll() ),
                         QKeySequence( "Ctrl+A" ) );

    editMenu->addSeparator();

    editMenu->addAction( findIcon, tr( "&Find" ), this, SLOT( startSearch() ),
                         QKeySequence( "Ctrl+F" ) );

    editMenu->addAction( downIcon, tr( "Find &Next" ), this, SLOT( findNext() ),
                         QKeySequence( "F3" ) );

    editMenu->addAction( upIcon, tr( "Find P&revious" ), this, SLOT( findPrev() ),
                         QKeySequence( "Shift+F3" ) );

    editMenu->addAction( tr( "&Replace" ), this, SLOT( replaceText() ),
                         QKeySequence( "Ctrl+R" ) );

    editMenu->addSeparator();

    editMenu->addAction( sortIcon, tr( "Sort" ), this, SLOT( sortCfg() ),
                         QKeySequence( "Ctrl+E" ) );

    editMenu->addAction( diffIcon, tr( "Compare" ), this, SLOT( openDiffTab() ),
                         QKeySequence( "Ctrl+T" ) );

    editMenu->addAction( commentIcon, tr( "Comment Selection" ), this, SLOT( commentSelection() ),
                         QKeySequence( "Ctrl+/" ) );

    QMenu* viewMenu = new QMenu( tr( "&View" ), this );
    menuBar()->addMenu( viewMenu );

    aLineWrap = viewMenu->addAction( tr( "Line Wrap" ), this, SLOT( toggleLineWrap() ) );
    aLineWrap->setCheckable( true );

    aHighlight = viewMenu->addAction( tr( "Synax Highlight" ), this, SLOT( toggleHighlight() ) );
    aHighlight->setCheckable( true );

    aAutocomplete = viewMenu->addAction( tr( "Autocomplete" ), this, SLOT( toggleAutocomplete() ) );
    aAutocomplete->setCheckable( true );

    aLineNumbers = viewMenu->addAction( tr( "Line Numbers" ),
                                        this, SLOT( toggleLineNumbers() ) );
    aLineNumbers->setCheckable( true );

    aHighlightLine = viewMenu->addAction( tr( "Highlight Current Line" ),
                                          this, SLOT( toggleHighlightLine() ) );
    aHighlightLine->setCheckable( true );

    viewMenu->addSeparator();

    viewMenu->addAction( tr( "&Preferences" ), this, SLOT( showOptions() ) );

    toolBar = new QToolBar( tr( "File" ), this );
    Qt::ToolBarArea area = static_cast< Qt::ToolBarArea >(
                           config.settings->value( "Editor/toolBarArea",
                           Qt::TopToolBarArea ).toInt() );
    addToolBar( area, toolBar );
    toolBar->setIconSize( QSize( 20, 20 ) );
    toolBar->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
    toolBar->setStyleSheet( "QToolBar { spacing: 5px; }" );

    toolBar->addAction( newIcon, tr( "New" ), this, SLOT( newConfig() ) );
    toolBar->addAction( openIcon, tr( "Open" ), this, SLOT( openConfig() ) );
    toolBar->addSeparator();
    toolBar->addAction( saveIcon, tr( "Save" ), this, SLOT( save() ) );
    toolBar->addAction( saveAsIcon, tr( "Save As" ), this, SLOT( saveAs() ) );
    toolBar->addSeparator();
    toolBar->addAction( closeIcon, tr( "Close" ), this, SLOT( closeCurrentTab() ) );
    toolBar->addSeparator();
    toolBar->addAction( undoIcon, tr( "Undo" ), this, SLOT( undo() ) );
    toolBar->addAction( redoIcon, tr( "Redo" ), this, SLOT( redo() ) );

    DtTextEdit::LineWrapMode cfgLineWrap = static_cast< DtTextEdit::LineWrapMode >(
                                           config.settings->value( "Editor/lineWrap",
                                           DtTextEdit::NoWrap ).toInt() );

    aLineWrap->setChecked( cfgLineWrap == DtTextEdit::WidgetWidth );
    setLineWrapMode( cfgLineWrap );

    QWidget* cWidget = new QWidget( this );
    cWidget->setLayout( mainLayout );

    setCentralWidget( cWidget );

    bool doHighlight = config.settings->value( "Editor/doHighlight", true ).toBool();
    setHighlight( doHighlight );
    aHighlight->setChecked( doHighlight );

    bool doAutocomplete = config.settings->value( "Editor/autocomplete", true ).toBool();
    setAutocomplete( doAutocomplete );
    aAutocomplete->setChecked( doAutocomplete );

    bool showLineNumbers = config.settings->value( "Editor/showLineNumbers", false ).toBool();
    setLineNumbers( showLineNumbers );
    aLineNumbers->setChecked( showLineNumbers );

    bool doHighlightLine = config.settings->value( "Editor/doHighlightLine", true ).toBool();
    setHighlightLine( doHighlightLine );
    aHighlightLine->setChecked( doHighlightLine );

    QMenu* helpMenu = new QMenu( tr( "&Help" ), this );
    menuBar()->addMenu( helpMenu );

    actAbout = helpMenu->addAction( tr( "About" ), this, SLOT( showAbout() ) );

    editorFont = new QFont;
    editorFont->setFixedPitch( true );
    applyFontSettings();
}

void DtGameConfigEditor::commentSelection() {
    if ( currentEditor() ) {
        currentEditor()->commentSelection();
    }
}

void DtGameConfigEditor::closeCurrentTab() {
    if ( tabs->isVisible() ) {
        tabClosed( tabs->currentIndex() );
    }
}

void DtGameConfigEditor::onShowRecentMenu() {
    recentMenu->clear();

    foreach ( const QString& fileName, config.recentOpenedConfigs ) {
        recentMenu->addAction( fileName, this, SLOT( openRecent() ) );
    }
}

void DtGameConfigEditor::openRecent() {
    QAction* action = qobject_cast< QAction* >( sender() );

    if ( action ) {
        QFileInfo file( action->iconText() );
        setDir( file.absolutePath() );
        openConfig( file.fileName() );
    }
}

DtGameConfigEditor::~DtGameConfigEditor() {
    delete editorFont;
}

void DtGameConfigEditor::toggleHighlightLine() {
    setHighlightLine( !highlightLine );
    aHighlightLine->setChecked( highlightLine );
    config.settings->setValue( "Editor/doHighlightLine", highlightLine );
}

void DtGameConfigEditor::toggleLineNumbers() {
    setLineNumbers( !lineNumbers );
    aLineNumbers->setChecked( lineNumbers );
    config.settings->setValue( "Editor/showLineNumbers", lineNumbers );
}

void DtGameConfigEditor::toggleAutocomplete() {
    setAutocomplete( !autocomplete );
    aAutocomplete->setChecked( autocomplete );
    config.settings->setValue( "Editor/autocomplete", autocomplete );
}

void DtGameConfigEditor::setAutocomplete( bool s ) {
    if ( autocomplete != s ) {
        autocomplete = s;

        for ( int i = 0; i < tabs->count(); ++i ) {
            editorAt( i )->setAutoComplete( autocomplete );
        }
    }
}

void DtGameConfigEditor::setLineNumbers( bool s ) {
    if ( lineNumbers != s ) {
        lineNumbers = s;

        for ( int i = 0; i < tabs->count(); ++i ) {
            editorAt( i )->setLineNumbers( lineNumbers );
        }
    }
}

void DtGameConfigEditor::setHighlightLine( bool s ) {
    if ( highlightLine != s ) {
        highlightLine = s;

        for ( int i = 0; i < tabs->count(); ++i ) {
            editorAt( i )->setHighlightLine( highlightLine );
        }
    }
}

void DtGameConfigEditor::tabChanged( int index ) {
    if ( diffAt( index ) ) {
        setWindowTitle( tr( "Compare", "Tabname|Compare" ) );
        return;
    }

    QString title = QFileInfo( currentFileName() ).fileName();

    if ( title.isEmpty() ) {
        title = tr( "New config" );
    }

    setWindowTitle( title );

    DtHighlighter* highlighter = syntaxHighlighters.value( currentEditor() );

    if ( highlighter && highlighter->doHighlight != highlight ) {
        highlighter->doHighlight = highlight;
        highlighter->rehighlight();
    }
}

void DtGameConfigEditor::removeEditor( DtTextEdit* editor ) {
    openedFiles.remove( editor );
    syntaxHighlighters.remove( editor );
    delete editor;
}

void DtGameConfigEditor::tabClosed( int index ) {
    if ( DtTextEdit* editor = editorAt( index ) ) {
        if ( editor->document()->isModified() ) {
            int act = confirmSave( tr( "File was changed. Save it?" ) );

            if ( act == QMessageBox::Yes || act == QMessageBox::No ) {

                if ( act == QMessageBox::Yes ) {
                    save( editor );
                }
            }
            else if ( act == QMessageBox::Cancel ) {
                return;
            }
        }

        removeEditor( editor );

        if ( modifiedFilesCount() == 0 ) {
            modified = false;
        }
    }
    else if ( DtComparePage* diff = diffAt( index ) ) {
        delete diff;
    }

    if ( tabs->count() == 0 ) {
        showWelcomePage();
    }
}

int DtGameConfigEditor::modifiedFilesCount() {
    int count = 0;

    for ( int i = 0; i < tabs->count(); ++i ) {
        if ( editorAt( i ) && editorAt( i )->document()->isModified() ) {
            ++count;
        }
    }

    return count;
}

DtTextEdit* DtGameConfigEditor::currentEditor() const {
    return qobject_cast< DtTextEdit* >( tabs->currentWidget() );
}

DtTextEdit* DtGameConfigEditor::editorAt( int index ) const {
    return qobject_cast< DtTextEdit* >( tabs->widget( index ) );
}

DtComparePage* DtGameConfigEditor::diffAt( int index ) const {
    return qobject_cast< DtComparePage* >( tabs->widget( index ) );
}

void DtGameConfigEditor::setModified( bool s ) {
    modified = s;

    if ( !modified ) {
        for ( int i = 0; i < tabs->count(); ++i ) {
            if ( editorAt( i ) ) {
                editorAt( i )->document()->setModified( false );
            }
        }
    }
}

void DtGameConfigEditor::undo() {
    if ( currentEditor() ) {
        currentEditor()->undo();
    }
}

void DtGameConfigEditor::redo() {
    if ( currentEditor() ) {
        currentEditor()->redo();
    }
}

void DtGameConfigEditor::cut() {
    if ( currentEditor() ) {
        clipboard.clear();
        currentEditor()->cut();
    }
}

void DtGameConfigEditor::copy() {
    if ( currentEditor() ) {
        clipboard = currentEditor()->textCursor().selectedText();
        currentEditor()->copy();
    }
}

void DtGameConfigEditor::paste() {
    if ( currentEditor() ) {
        currentEditor()->paste();
    }
}

void DtGameConfigEditor::selectAll() {
    if ( currentEditor() ) {
        currentEditor()->selectAll();
    }
}

void DtGameConfigEditor::toggleLineWrap() {
    setLineWrapMode( lineWrapMode == DtTextEdit::NoWrap ?
                     DtTextEdit::WidgetWidth : DtTextEdit::NoWrap );

    aLineWrap->setChecked( lineWrapMode == DtTextEdit::WidgetWidth );
    config.settings->setValue( "Editor/lineWrap", lineWrapMode );
}

void DtGameConfigEditor::setLineWrapMode( int mode ) {
    if ( lineWrapMode != mode ) {
        lineWrapMode = mode;

        for ( int i = 0; i < tabs->count(); ++i ) {
            editorAt( i )->setLineWrapMode( static_cast< DtTextEdit::LineWrapMode >( mode ) );
        }
    }
}

void DtGameConfigEditor::setHighlight( bool s ) {
    highlight = s;

    DtHighlighter* highlighter = syntaxHighlighters.value( currentEditor() );

    if ( highlighter ) {
        highlighter->doHighlight = highlight;
        highlighter->rehighlight();
    }
}

QString DtGameConfigEditor::currentFileName() const {
    return openedFiles.value( currentEditor() );
}

void DtGameConfigEditor::clear() {
    QList< DtTextEdit* > editors;

    for ( int i = 0; i < tabs->count(); ++i ) {
        editors.append( editorAt( i ) );
    }

    for ( int i = 0; i < editors.size(); ++i ) {
        delete editors.at( i );
    }

    tabs->setTabsClosable( false );
    modified = false;
}

DtTextEdit* DtGameConfigEditor::createEditor( int highlightType ) {
    if ( welcomePage ) {
        hideWelcomePage();
    }

    DtTextEdit* editor = new DtTextEdit( this );
    applySettingsToEditor( editor );
    editor->setLineWrapMode( static_cast< DtTextEdit::LineWrapMode >( lineWrapMode ) );
    editor->setHighlightLine( highlightLine );
    editor->setLineNumbers( lineNumbers );

    DtHighlighter* highlighter = new DtHighlighter( editor->document() );
    highlighter->doHighlight = highlight;
    highlighter->highlightType = highlightType;
    syntaxHighlighters.insert( editor, highlighter );

    editor->setAutoComplete( autocomplete );

    return editor;
}

void DtGameConfigEditor::setWindowModified( bool m ) {
    QTextDocument* document = qobject_cast< QTextDocument* >( sender() );

    if( !document ) {
        return;
    }

    for ( int i = 0; i < tabs->count(); ++i ) {
        DtTextEdit* editor = editorAt( i );

        if ( editor && document == editor->document() ) {
            QString tabTitle = QFileInfo( openedFiles.value( editor ) ).fileName();

            if ( m ) {
                tabTitle.prepend( '*' );
            }

            tabs->setTabText( i, tabTitle );

            break;
        }
    }

    setModified( m );
}

void DtGameConfigEditor::toggleHighlight() {
    setHighlight( !highlight );
    aHighlight->setChecked( highlight );
    config.settings->setValue( "Editor/doHighlight", highlight );
}

int DtGameConfigEditor::confirmSave( const QString& msg ) {
    return QMessageBox::question( this,
                                  tr( "Save file" ),
                                  msg,
                                  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel );
}

void DtGameConfigEditor::saveWindow() {
    config.settings->setValue( "Editor/size", size() );

#ifdef Q_OS_WIN
    config.settings->setValue( "Editor/pos", pos() );
#endif

    config.settings->setValue( "Editor/toolBarArea", toolBarArea( toolBar ) );
    config.save();
}

void DtGameConfigEditor::closeEvent( QCloseEvent* event ) {
    if ( modified ) {
        QString msg = ( modifiedFilesCount() > 1 ) ?
                      tr( "Game configuration files was changed. Save all?" ) :
                      tr( "Game configuration file was changed. Save it?" );

        int act = confirmSave( msg );

        if ( act == QMessageBox::Yes || act == QMessageBox::No ) {

            if ( act == QMessageBox::Yes ) {
                saveAll();
            }

            saveWindow();
            event->accept();
        }
        else if ( act == QMessageBox::Cancel ) {
            event->ignore();
            return;
        }
    }
    else {
        saveWindow();
    }

    searchPanel->clear();
    searchPanel->hide();
    replacePanel->clear();
    replacePanel->hide();
    clipboard.clear();

    modified = false;
    clear();
}

void DtGameConfigEditor::setDir( const QString& dir ) {
    currentDirectory = dir;
}

void DtGameConfigEditor::save( DtTextEdit* editor ) {
    file->setFileName( openedFiles.value( editor ) );

    QTextDocumentWriter writer( file, "plaintext" );

    if ( writer.write( editor->document() ) ) {
        editor->document()->setModified( false );
    }
}

void DtGameConfigEditor::save() {
    if ( !currentEditor() ) {
        return;
    }

    if ( currentFileName().isEmpty() ) {
        saveAs();
        return;
    }

    save( currentEditor() );
    setWindowTitle( QFileInfo( currentFileName() ).fileName() );

    if ( modified ) {
        bool haveModified = false;

        for ( int i = 0; i < tabs->count(); ++i ) {
            if ( editorAt( i )->document()->isModified() ) {
                haveModified = true;
            }
        }

        if ( !haveModified ) {
            modified = false;
        }
    }

    emit fileSaved();
}

void DtGameConfigEditor::saveAll() {
    if ( !modified || !currentEditor() ) {
        return;
    }

    for ( int i = 0; i < tabs->count(); ++i ) {
        DtTextEdit* editor = editorAt( i );

        if ( editor->document()->isModified() ) {
            tabs->setCurrentIndex( i );
            editor->setFocus();
            save();
        }
    }

    setModified( false );
}

void DtGameConfigEditor::saveAs() {
    if ( !currentEditor() ) {
        return;
    }

    QString fName = QFileDialog::getSaveFileName( this, tr( "Save as" ),
                                                  currentDirectory, defaultConfigFormat );
    if ( fName.isEmpty() ) {
        return;
    }

    bool hasExtension = false;

    foreach ( const QString& filter, defaultConfigFilters ) {
        if ( fName.endsWith( filter, Qt::CaseInsensitive ) ) {
            hasExtension = true;
        }
    }

    if ( !hasExtension ) {
        fName += ".cfg";
    }

    int highlightType = !QFileInfo( fName ).suffix().compare( "cfg" ) ? HT_CFG : HT_MENU;

    DtHighlighter* highlighter = syntaxHighlighters.value( currentEditor() );
    highlighter->highlightType = highlightType;
    highlighter->rehighlight();

    openedFiles.insert( currentEditor(), fName );
    save();
}

const QIcon& DtGameConfigEditor::dirIcon() {
    int iconId = I_QZ_SMALL;

    if ( !config.getQaBasePath().isEmpty() &&
         currentDirectory.startsWith( config.getQaBasePath() ) )
    {
        iconId = I_Q3_SMALL;
    }

    return icons->getIcon( iconId );
}

void DtGameConfigEditor::newConfig() {
    DtTextEdit* editor = createEditor( HT_CFG );

    connect( editor->document(), SIGNAL( modificationChanged( bool ) ),
             this, SLOT( setWindowModified( bool ) ) );

    editor->document()->setModified( true );
    openedFiles.insert( editor, "" );
    setWindowTitle( "New config" );
    int index = tabs->addTab( editor, "*Config" );

    tabs->setCurrentIndex( index );
    tabs->setTabIcon( index, dirIcon() );
    editor->setFocus();

    if ( tabs->count() > 1 ) {
        tabs->setTabsClosable( true );
    }

    if ( isHidden() ) {
        show();
    }

    activateWindow();
    raise();
}

void DtGameConfigEditor::openConfig( const QString& fileName ) {
    QString fullPath = currentDirectory + "/" + fileName;

    for ( int i = 0; i < tabs->count(); ++i ) {
        QString openedFile = openedFiles.value( editorAt( i ), "" );

        if ( !openedFile.isEmpty() && !openedFile.compare( fullPath ) ) {
            tabs->setCurrentIndex( i );
            return;
        }
    }

    file->setFileName( fullPath );

    if ( !file->open( QFile::ReadOnly ) ) {
        return;
    }

    setWindowTitle( fileName );

    int highlightType = !QFileInfo( fullPath ).suffix().compare( "cfg" ) ? HT_CFG : HT_MENU;

    DtTextEdit* editor = createEditor( highlightType );
    editor->setPlainText( file->readAll() );
    editor->document()->setModified( false );

    connect( editor->document(), SIGNAL( modificationChanged( bool ) ),
             this, SLOT( setWindowModified( bool ) ) );

    file->close();

    openedFiles.insert( editor, fullPath );
    int index = tabs->addTab( editor, fileName );

    tabs->setCurrentIndex( index );
    tabs->setTabIcon( index, dirIcon() );
    editor->setFocus();

    addEntryToRecentList( fullPath, config.recentOpenedConfigs );

    if ( isHidden() ) {
        show();
    }
}

void DtGameConfigEditor::addEntryToRecentList( const QString& entry, QStringList& list ) {
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

void DtGameConfigEditor::openConfig(){
    if ( currentDirectory.isEmpty() && config.recentOpenedConfigs.size() ) {
        currentDirectory = QFileInfo( config.recentOpenedConfigs.at( 0 ) ).absoluteDir().path();
    }

    QString fName = QFileDialog::getOpenFileName( this, tr( "Open config" ),
                                                  currentDirectory, defaultConfigFormat );
    if ( !fName.isEmpty() ) {
        QFileInfo file( fName );
        setDir( file.absolutePath() );
        openConfig( file.fileName() );
    }
}

void DtGameConfigEditor::openDiffTab() {
    if ( welcomePage ) {
        hideWelcomePage();
    }

    DtComparePage* diffPage = new DtComparePage( currentFileName(), this );

    int index = tabs->addTab( diffPage, tr( "Compare", "Tabname|Compare" ) );

    tabs->setCurrentIndex( index );
    tabs->setTabIcon( index, QIcon( ":/res/diff.png" ) );
}

void DtGameConfigEditor::sortCfg() {
    if ( !currentEditor() ) {
        return;
    }

    DtHighlighter* highlighter = syntaxHighlighters.value( currentEditor() );

    if ( highlighter->highlightType != HT_CFG ) {
        return;
    }

    QString text = currentEditor()->document()->toPlainText();
    QStringList lines = text.split( "\n", QString::SkipEmptyParts );

    QStringList comments;
    QStringList unbinds;
    QStringList unaliases;
    QStringList aliases;
    QStringList binds;
    QStringList customCommands;
    QStringList commands;
    QStringList other;

    QRegExp cmdReg( "^[^\\s]+\\s([\\S]+)" );

    foreach ( const QString& line, lines ) {
        bool otherCmd = false;
        QString trimmedLine = line.trimmed();

        if ( trimmedLine.startsWith( "//" ) ) {
            comments << line;
        }
        else if ( trimmedLine.startsWith( "unbind", Qt::CaseInsensitive ) ) {
            unbinds << line;
        }
        else if ( trimmedLine.startsWith( "unalias", Qt::CaseInsensitive ) ) {
            unaliases << line;
        }
        else if ( trimmedLine.startsWith( "bind", Qt::CaseInsensitive ) ) {
            binds << line;
        }
        else if ( trimmedLine.startsWith( "alias", Qt::CaseInsensitive ) ) {
            aliases << line;
        }
        else if ( trimmedLine.startsWith( "set", Qt::CaseInsensitive ) ) {

            if ( trimmedLine.startsWith( "seta", Qt::CaseInsensitive ) ||
                 trimmedLine.startsWith( "sets", Qt::CaseInsensitive ) ||
                 trimmedLine.startsWith( "setu", Qt::CaseInsensitive ) )
            {
                if ( cmdReg.indexIn( trimmedLine ) != -1 ) {
                    if ( !highlighter->stringHash.contains( cmdReg.cap( 1 ).toLower() ) ) {
                        customCommands << line;
                    }
                    else {
                        commands << line;
                    }
                }
            }
            else {
                otherCmd = true;
            }
        }
        else {
            otherCmd = true;
        }

        if ( otherCmd ) {
            other << line;
        }
    }

#define INSERT_SORTED( cfgLines )                            \
    if ( !cfgLines.isEmpty() ) {                             \
        cfgLines.sort();                                     \
        text.append( cfgLines.join( "\n" ) ).append( "\n" ); \
    }

    text.clear();
    INSERT_SORTED( comments )
    INSERT_SORTED( unbinds )
    INSERT_SORTED( binds )
    INSERT_SORTED( unaliases )
    INSERT_SORTED( aliases )
    INSERT_SORTED( customCommands )
    INSERT_SORTED( commands )
    INSERT_SORTED( other )

    currentEditor()->document()->setPlainText( text );
}

void DtGameConfigEditor::applySettingsToEditor( DtTextEdit* editor ) {
    QPalette pal = editor->viewport()->palette();
    pal.setColor( QPalette::Base, config.textEditor.backgroundColor );
    pal.setColor( QPalette::Text, config.textEditor.normalText.foreground().color() );
    editor->viewport()->setPalette( pal );
    editor->setPalette( pal );
    editor->setFont( *editorFont );
    editor->highlightCurrentLine();

    int tabWidth = QFontMetrics( *editorFont ).averageCharWidth() * config.textEditor.tabSize;
    editor->setTabStopWidth( tabWidth );

    DtHighlighter* highlighter = syntaxHighlighters.value( editor );

    if ( highlighter ) {
        highlighter->updateFormatSettings();
        highlighter->rehighlight();
    }

    editor->update();
}

void DtGameConfigEditor::applyFontSettings() {
    editorFont->setFamily( config.textEditor.fontFamily );
    editorFont->setPointSize( config.textEditor.fontSize );
    editorFont->setWeight( config.textEditor.normalText.fontWeight() );
    editorFont->setItalic( config.textEditor.normalText.fontItalic() );
    editorFont->setUnderline( config.textEditor.normalText.fontUnderline() );
}

void DtGameConfigEditor::applySettings() {
    applyFontSettings();

    for ( int i = 0; i < tabs->count(); ++i ) {
        applySettingsToEditor( editorAt( i ) );
    }
}

void DtGameConfigEditor::showOptions() {
    DtEditorOptionsDialog* optionsDialog = new DtEditorOptionsDialog( this );
    connect( optionsDialog, SIGNAL( settingsChanged() ), this, SLOT( applySettings() ) );

    optionsDialog->exec();

    delete optionsDialog;
}

void DtGameConfigEditor::welcomeItemClicked( QString name ) {
    QFileInfo file( name.right( name.size() - 11 ) );
    setDir( file.absolutePath() );
    openConfig( file.fileName() );
}

bool DtGameConfigEditor::eventFilter( QObject* obj, QEvent* e ) {
    QWidget* item = qobject_cast< QWidget* >( obj );

    if ( item && item->objectName().startsWith( "welcomeItem" )  &&
         e->type() == QEvent::MouseButtonPress )
    {
        welcomeItemClicked( item->objectName() );
        return true;
    }

    return false;
}

static QString cutPath( QString path ) {
    if ( path.size() <= 55 ) {
        return path;
    }

    return path.left( 26 ) + "... " +  path.right( 25 );
}

void DtGameConfigEditor::showWelcomePage() {
    tabs->hide();
    setWindowTitle( defaultWindowName );

    QVBoxLayout* welcomeLayout = new QVBoxLayout;
    welcomePage = new QWidget( this );

    QFrame* filesFrame = new QFrame( welcomePage );
    filesFrame->setFrameStyle( QFrame::StyledPanel | QFrame::Raised );
    filesFrame->setFixedSize( 500, 500 );

    QListWidget* filesWidget = new QListWidget( this );

    QPalette pal = filesWidget->palette();
    pal.setColor( QPalette::Base, QColor( "white" ) );
    filesWidget->setPalette( pal );

    QVBoxLayout* filesLayout = new QVBoxLayout;
    QLabel* welcomeLabel = new QLabel( "<font color=#707070>" + tr( "Last Files" ) + "</font>" );
    welcomeLabel->setContentsMargins( 6, 4, 4, 10 );

    QFont font = welcomeLabel->font();
    font.setBold( true );
    font.setFamily( "Liberation Sans" );
    font.setPixelSize( 18 );
    welcomeLabel->setFont( font );

    filesLayout->addWidget( filesWidget );
    filesFrame->setLayout( filesLayout );

    QListWidgetItem* item = new QListWidgetItem;
    filesWidget->setSelectionMode( QListWidget::NoSelection );
    filesWidget->addItem( item );
    item->setSizeHint( welcomeLabel->sizeHint() );
    filesWidget->setItemWidget( item, welcomeLabel );

    for ( int i = 0; i < config.recentOpenedConfigs.size() && i < 8; ++i ) {
        const QString& recentConfig = config.recentOpenedConfigs.at( i );
        item = new QListWidgetItem;
        filesWidget->addItem( item );

        QWidget* listEntry = new QWidget( this );
        listEntry->setObjectName( "welcomeItem" + recentConfig );
        listEntry->setCursor( Qt::PointingHandCursor );
        listEntry->installEventFilter( this );

        QVBoxLayout* entryLayout = new QVBoxLayout;
        QLabel* entryName = new QLabel( QFileInfo( recentConfig ).fileName() );
        font = entryName->font();
        font.setBold( true );
        font.setFamily( "Liberation Sans" );
        font.setPixelSize( 15 );
        entryName->setFont( font );

        QLabel* entryPath = new QLabel( "<font color=#808080>" + cutPath( recentConfig ) +
                                        "</font>" );
        font.setBold( false );
        entryPath->setFont( font );
        listEntry->setContentsMargins( 6, 0, 4, 0 );

        entryLayout->addWidget( entryName, Qt::AlignLeft );
        entryLayout->addWidget( entryPath, Qt::AlignLeft );
        listEntry->setLayout( entryLayout );

        item->setSizeHint( listEntry->sizeHint() );
        filesWidget->setItemWidget( item, listEntry );
    }

    welcomeLayout->addWidget( filesFrame, 0, Qt::AlignCenter );
    welcomePage->setLayout( welcomeLayout );
    QVBoxLayout* mainLayout = qobject_cast< QVBoxLayout* >( centralWidget()->layout() );
    mainLayout->addWidget( welcomePage );
}

void DtGameConfigEditor::hideWelcomePage() {
    delete welcomePage;
    welcomePage = 0;
    tabs->show();
}

void DtGameConfigEditor::applicationMessage( const QString& msg ) {
    bool argIsEmpty = msg.isEmpty();

    if ( !argIsEmpty ) {
        QFileInfo cmdFile( msg );

        if ( cmdFile.exists() && isConfigFile( cmdFile.suffix() ) ) {
            setDir( cmdFile.absolutePath() );
            openConfig( cmdFile.fileName() );
            return;
        }
        else {
            if ( cmdFile.isDir() && cmdFile.exists() ) {
                setDir( cmdFile.absolutePath() );
                newConfig();
            }

            return;
        }
    }
    else {
        showWelcomePage();
        return;
    }
}

void DtGameConfigEditor::startSearch() {
    if ( !welcomePage ) {
        replacePanel->hide();
        searchPanel->show();

        if ( !replacePanel->searchString().isEmpty() ) {
            searchPanel->setSearchString( replacePanel->searchString() );
        }
    }
}

void DtGameConfigEditor::replaceText() {
    if ( !welcomePage ) {
        searchPanel->hide();
        replacePanel->show();

        if ( !searchPanel->searchString().isEmpty() ) {
            replacePanel->setSearchString( searchPanel->searchString() );
        }

        replacedText.clear();
    }
}

void DtGameConfigEditor::findText( QString searchString ) {
    if ( !currentEditor() ) {
        return;
    }

    if ( searchString.isEmpty() ) {
        QTextCursor cur = currentEditor()->textCursor();
        cur.clearSelection();
        currentEditor()->setTextCursor( cur );
        return;
    }

    QTextDocument::FindFlags options = 0;

    if ( config.textEditor.searchMatchCase ) {
        options |= QTextDocument::FindCaseSensitively;
    }

    QTextCursor cur = currentEditor()->textCursor();
    cur.setPosition( cur.selectionStart() ? cur.selectionStart() - 1 : 0 );
    currentEditor()->setTextCursor( cur );

    currentEditor()->find( searchString, options );
}

void DtGameConfigEditor::findInCurrent( QTextDocument::FindFlags options ) {
    if ( config.textEditor.searchMatchCase ) {
        options |= QTextDocument::FindCaseSensitively;
    }

    QString searchText = ( searchPanel->isVisible() ) ? searchPanel->searchString() :
                                                        replacePanel->searchString();

    if ( !currentEditor()->find( searchText, options ) ) {
        QTextCursor actCur = currentEditor()->textCursor();
        QTextCursor cur = actCur;

        if ( options & QTextDocument::FindBackward ) {
            cur.movePosition( QTextCursor::End );
        }
        else {
            cur.movePosition( QTextCursor::Start );
        }

        currentEditor()->setTextCursor( cur );

        if ( !currentEditor()->find( searchText, options ) ) {
            currentEditor()->setTextCursor( actCur );
        }
    }
}

void DtGameConfigEditor::findNext() {
    if ( currentEditor() ) {
        findInCurrent();
    }
}

void DtGameConfigEditor::findPrev() {
    if ( currentEditor() ) {
        findInCurrent( QTextDocument::FindBackward );
    }
}

void DtGameConfigEditor::clearReplacedText() {
    if ( !currentEditor() ) {
        return;
    }

    if ( !replacedText.isEmpty() ) {
        QTextCharFormat format;
        format.setBackground( QBrush( config.textEditor.backgroundColor ) );

        for ( int i = 0; i < replacedText.size(); ++i ) {
            QPair< int, int > text = replacedText.at( i );
            QTextCursor cur( currentEditor()->document() );
            cur.setPosition( text.first );
            cur.movePosition( QTextCursor::NextCharacter, QTextCursor::KeepAnchor, text.second );
            cur.setCharFormat( format );
        }

        replacedText.clear();
    }
}

void DtGameConfigEditor::replaceNextText() {
    if ( !currentEditor() ) {
        return;
    }

    QTextCursor cur = currentEditor()->textCursor();

    if ( cur.selectedText() != replacePanel->searchString() ) {
        findInCurrent();
        return;
    }

    clearReplacedText();
    QTextCharFormat format;
    format.setBackground( QBrush( QColor( "#00ff00" ) ) );
    replacedText.append( QPair< int, int >( cur.position() - replacePanel->searchString().size(),
                                            replacePanel->replaceString().size() ) );
    cur.insertText( replacePanel->replaceString(), format );
    findInCurrent();
}

void DtGameConfigEditor::replaceAllText() {
    QTextDocument::FindFlags options;
    QTextCursor cur( currentEditor()->document() );
    cur.movePosition( QTextCursor::Start );

    if ( config.textEditor.searchMatchCase ) {
        options |= QTextDocument::FindCaseSensitively;
    }

    clearReplacedText();
    cur = currentEditor()->document()->find( replacePanel->searchString(), cur, options );

    if ( cur.isNull() ) {
        return;
    }

    cur.beginEditBlock();

    forever {
        QTextCharFormat format;
        format.setBackground( QBrush( QColor( "#00ff00" ) ) );
        replacedText.append( QPair< int, int >( cur.position() - replacePanel->searchString().size(),
                                                replacePanel->replaceString().size() ) );

        cur.insertText( replacePanel->replaceString(), format );
        QTextCursor lastCur = currentEditor()->document()->find( replacePanel->searchString(),
                                                                 cur, options );
        if ( lastCur.isNull() ) {
            break;
        }

        cur.setPosition( lastCur.position() - replacePanel->searchString().size() );
        cur.movePosition( QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                          replacePanel->searchString().size() );
    }

    cur.endEditBlock();
}

void DtGameConfigEditor::showAbout() {
    DtAbout( this ).exec();
}

QString DtGameConfigEditor::getClipboardText() const {
    return clipboard;
}

DtEditorSearchPanel::DtEditorSearchPanel( QWidget* parent ) : QWidget( parent ) {
    QHBoxLayout* mainLayout = new QHBoxLayout;
    QLabel* findLabel = new QLabel( tr( "Find:" ) );

    searchTextEdit = new QLineEdit;
    connect( searchTextEdit, SIGNAL( textChanged( QString ) ), parent, SLOT( findText( QString ) ) );
    searchTextEdit->setFixedWidth( 400 );
    QPalette pal = searchTextEdit->palette();
    pal.setColor( QPalette::Base, QColor( "#baf9cd" ) );
    searchTextEdit->setPalette( pal );

    nextButton = new QPushButton( QIcon( ":res/go-down.png" ), tr( "Next" ) );
    connect( nextButton, SIGNAL( clicked() ), parent, SLOT( findNext() ) );
    nextButton->setFixedWidth( 150 );

    prevButton = new QPushButton( QIcon( ":res/go-up.png" ), tr( "Previous" ) );
    connect( prevButton, SIGNAL( clicked() ), parent, SLOT( findPrev() ) );
    prevButton->setFixedWidth( 150 );

    QToolButton* closeButton = new QToolButton;
    connect( closeButton, SIGNAL( clicked() ), this, SLOT( close() ) );
    closeButton->setAutoRaise( true );
    closeButton->setIcon( QIcon( ":res/dialog-close.png" ) );
    closeButton->setIconSize( QSize( 16, 16 ) );
    closeButton->setFixedSize( 26, 26 );

    QLabel* matchCaseLabel = new QLabel( tr( "Match case" ) );
    matchCaseCb = new QCheckBox;
    connect( matchCaseCb, SIGNAL( stateChanged( int ) ), this, SLOT( matchCaseChanged( int ) ) );
    matchCaseCb->setChecked( config.textEditor.searchMatchCase );

    mainLayout->addWidget( closeButton );
    mainLayout->addSpacing( 10 );
    mainLayout->addWidget( findLabel );
    mainLayout->addWidget( searchTextEdit );
    mainLayout->addSpacing( 10 );
    mainLayout->addWidget( nextButton );
    mainLayout->addWidget( prevButton );
    mainLayout->addSpacing( 10 );
    mainLayout->addWidget( matchCaseLabel );
    mainLayout->addWidget( matchCaseCb );
    mainLayout->addStretch( 1 );

    setMinimumHeight( 40 );
    setLayout( mainLayout );
}

void DtEditorSearchPanel::matchCaseChanged( int state ) {
    config.textEditor.searchMatchCase = state;
}

void DtEditorSearchPanel::showEvent( QShowEvent* e ) {
    QString text = ceMainWindow->getClipboardText();

    if ( !text.isEmpty() && text.size() < 100 ) {
        searchTextEdit->setText( text );
    }

    searchTextEdit->setFocus();
    QWidget::showEvent( e );
}

QString DtEditorSearchPanel::searchString() const {
    return searchTextEdit->text();
}

void DtEditorSearchPanel::setSearchString( QString text ) {
    searchTextEdit->setText( text );
}

void DtEditorSearchPanel::clear() {
    searchTextEdit->clear();
}

DtEditorAdvancedSearchPanel::DtEditorAdvancedSearchPanel( QWidget* parent ) : QWidget( parent ) {
    QLabel* findLabel = new QLabel( tr( "Find:" ) );

    searchTextEdit = new QLineEdit;

    nextButton = new QPushButton( QIcon( ":res/go-down.png" ), tr( "Next" ) );
    connect( nextButton, SIGNAL( clicked() ), parent, SLOT( findNext() ) );
    nextButton->setFixedWidth( 150 );

    prevButton = new QPushButton( QIcon( ":res/go-up.png" ), tr( "Previous" ) );
    connect( prevButton, SIGNAL( clicked() ), parent, SLOT( findPrev() ) );
    prevButton->setFixedWidth( 150 );

    replaceButton = new QPushButton( tr( "Replace" ) );
    connect( replaceButton, SIGNAL( clicked() ), parent, SLOT( replaceNextText() ) );
    replaceButton->setFixedWidth( 150 );

    replaceAllButton = new QPushButton( tr( "Replace All" ) );
    connect( replaceAllButton, SIGNAL( clicked() ), parent, SLOT( replaceAllText() ) );
    replaceAllButton->setFixedWidth( 150 );

    QToolButton* closeButton = new QToolButton;
    connect( closeButton, SIGNAL( clicked() ), this, SLOT( close() ) );
    connect( closeButton, SIGNAL( clicked() ), parent, SLOT( clearReplacedText() ) );
    closeButton->setAutoRaise( true );
    closeButton->setIcon( QIcon( ":res/dialog-close.png" ) );
    closeButton->setIconSize( QSize( 16, 16 ) );
    closeButton->setFixedSize( 26, 26 );

    QLabel* matchCaseLabel = new QLabel( tr( "Match case" ) );
    matchCaseCb = new QCheckBox;
    connect( matchCaseCb, SIGNAL( stateChanged( int ) ), this, SLOT( matchCaseChanged( int ) ) );
    matchCaseCb->setChecked( config.textEditor.searchMatchCase );

    QGridLayout* mainLayout = new QGridLayout;
    mainLayout->setHorizontalSpacing( 7 );
    mainLayout->setVerticalSpacing( 0 );

    mainLayout->addWidget( closeButton, 0, 0 );
    mainLayout->addWidget( findLabel, 0, 1 );
    mainLayout->addWidget( searchTextEdit, 0, 2 );
    mainLayout->addWidget( nextButton, 0, 3 );
    mainLayout->addWidget( prevButton, 0, 4 );
    mainLayout->addWidget( matchCaseLabel, 0, 5 );
    mainLayout->addWidget( matchCaseCb, 0, 6 );

    QLabel* replaceLabel = new QLabel( tr( "Replace:" ) );
    replaceTextEdit = new QLineEdit;

    mainLayout->addWidget( replaceLabel, 1, 1 );
    mainLayout->addWidget( replaceTextEdit, 1, 2 );
    mainLayout->addWidget( replaceButton, 1, 3 );
    mainLayout->addWidget( replaceAllButton, 1, 4 );

    setMinimumHeight( 80 );
    setLayout( mainLayout );
    searchTextEdit->setTabOrder( searchTextEdit, replaceTextEdit );
}

void DtEditorAdvancedSearchPanel::matchCaseChanged( int state ) {
    config.textEditor.searchMatchCase = state;
}

void DtEditorAdvancedSearchPanel::showEvent( QShowEvent* e ) {
    QString text = ceMainWindow->getClipboardText();

    if ( !text.isEmpty() && text.size() < 100 ) {
        searchTextEdit->setText( text );
    }

    searchTextEdit->setFocus();
    QWidget::showEvent( e );
}

QString DtEditorAdvancedSearchPanel::searchString() const {
    return searchTextEdit->text();
}

QString DtEditorAdvancedSearchPanel::replaceString() const {
    return replaceTextEdit->text();
}

void DtEditorAdvancedSearchPanel::setSearchString( QString text ) {
    searchTextEdit->setText( text );
}

void DtEditorAdvancedSearchPanel::clear() {
    searchTextEdit->clear();
    replaceTextEdit->clear();
}

DtTextEdit::DtTextEdit( QWidget* parent ) : QPlainTextEdit( parent ),
    completer( 0 ),
    scriptWordsSorted( false )
{
    lineNumbers = new DtLineNumbers( this );

    connect( this, SIGNAL( blockCountChanged( int ) ),
             this, SLOT( updateLineNumbersWidth( int ) ) );

    connect( this, SIGNAL( updateRequest( QRect, int ) ),
             this, SLOT( updateLineNumbers( QRect, int ) ) );

    connect(this, SIGNAL( cursorPositionChanged() ), this, SLOT( highlightCurrentLine() ) );

    updateLineNumbersWidth( 0 );
}

int DtTextEdit::lineNumbersWidth() {
    int digits = 1;
    int max = qMax( 1, blockCount() );

    while ( max >= 10 ) {
        max /= 10;
        ++digits;
    }

    int space = 8 + fontMetrics().width( QLatin1Char( '9' ) ) * digits;

    return space;
}

void DtTextEdit::updateLineNumbersWidth( int ) {
    setViewportMargins( showLineNumbers ? lineNumbersWidth() : 0, 0, 0, 0 );
}

void DtTextEdit::updateLineNumbers( const QRect& rect, int dy ) {
    if ( dy ) {
        lineNumbers->scroll( 0, dy );
    }
    else {
        lineNumbers->update( 0, rect.y(), lineNumbers->width(), rect.height() );
    }

    if ( rect.contains( viewport()->rect() ) ) {
        updateLineNumbersWidth( 0 );
    }
}

void DtTextEdit::resizeEvent( QResizeEvent* e ) {
    QPlainTextEdit::resizeEvent( e );

    QRect cr = contentsRect();
    lineNumbers->setGeometry( QRect( cr.left(), cr.top(), lineNumbersWidth(), cr.height() ) );
}

void DtTextEdit::highlightCurrentLine() {
    QList< QTextEdit::ExtraSelection > extraSelections;

    if ( highlightLine && !isReadOnly() ) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor( config.textEditor.lineHighlightColor ).lighter( 197 );

        selection.format.setBackground( lineColor );
        selection.format.setProperty( QTextFormat::FullWidthSelection, true );
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append( selection );
    }

    setExtraSelections( extraSelections );
}

void DtTextEdit::lineNumbersPaintEvent( QPaintEvent* e ) {
    if ( !showLineNumbers ) {
        return;
    }

    QPainter painter( lineNumbers );

    painter.fillRect( e->rect(), config.textEditor.lineNumbersBgColor );

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = static_cast< int >( blockBoundingGeometry( block ).translated( contentOffset() ).top() );
    int bottom = top + static_cast< int >( blockBoundingRect( block ).height() );

    while ( block.isValid() && top <= e->rect().bottom() ) {
        if ( block.isVisible() && bottom >= e->rect().top() ) {
            QString number = QString::number( blockNumber + 1 );

            painter.setPen( config.textEditor.lineNumbersColor );
            painter.setFont( QFont( "Liberation Mono" ) );
            painter.drawText( 0, top, lineNumbers->width() - 4, fontMetrics().height(),
                              Qt::AlignRight, number );
        }

        block = block.next();
        top = bottom;
        bottom = top + static_cast< int >( blockBoundingRect( block ).height() );
        ++blockNumber;
    }
}

void DtTextEdit::setHighlightLine( bool s ) {
    highlightLine = s;
    highlightCurrentLine();
}

void DtTextEdit::setLineNumbers( bool s ) {
    showLineNumbers = s;

    lineNumbers->setVisible( showLineNumbers );
    updateLineNumbersWidth( 0 );
}

void DtTextEdit::setAutoComplete( bool s ) {
    doAutoComplete = s;

    if ( !scriptWordsSorted && doAutoComplete ) {
        scriptWordsSorted = true;
        scriptWords.sort();
    }

    if ( doAutoComplete && !completer ) {
        completer = new QCompleter( this );
        completer->setModel( new QStringListModel( scriptWords, completer ) );
        completer->setModelSorting( QCompleter::CaseSensitivelySortedModel );
        completer->setCaseSensitivity( Qt::CaseInsensitive );
        completer->setWrapAround( false );
        completer->setWidget( this );
        completer->setCompletionMode( QCompleter::PopupCompletion );

        connect( completer, SIGNAL( activated( QString ) ),
                 this, SLOT( insertCompletion( QString ) ) );
    }
    else if ( !doAutoComplete && completer ) {
        delete completer;
        completer = 0;
    }
}

void DtTextEdit::insertCompletion( const QString& completion ) {
    if ( completer->widget() != this ) {
        return;
    }

    QTextCursor cur = textCursor();

    cur.select( QTextCursor::WordUnderCursor );
    cur.removeSelectedText();
    cur.insertText( completion );
    setTextCursor( cur );
}

QString DtTextEdit::textUnderCursor() const {
    QTextCursor cur = textCursor();
    cur.select( QTextCursor::WordUnderCursor );

    return cur.selectedText();
}

void DtTextEdit::focusInEvent( QFocusEvent* e ) {
    if ( completer ) {
        completer->setWidget( this );
    }

    QPlainTextEdit::focusInEvent( e );
}

void DtTextEdit::indentSelection() {
    QTextCursor cur = textCursor();
    QString selection = cur.selection().toPlainText();
    QStringList selectedLines = selection.split( "\n" );
    int tabs = 0;

    for ( ; tabs < selectedLines.size(); ++tabs ) {
        selectedLines[ tabs ].prepend( '\t' );
    }

    int pos = cur.position();
    int apos = cur.anchor();

    cur.insertText( selectedLines.join( "\n" ) );

    if ( pos > apos ) {
        cur.setPosition( apos, QTextCursor::MoveAnchor );
        cur.movePosition( QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                          pos - apos + tabs );
    }
    else {
        cur.setPosition( apos + tabs, QTextCursor::MoveAnchor );
        cur.movePosition( QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor,
                          apos - pos + tabs );
    }

    setTextCursor( cur );
}

void DtTextEdit::unindentSelection() {
    QTextCursor cur = textCursor();
    QString selection = cur.selection().toPlainText();
    QStringList selectedLines = selection.split( "\n" );
    int tabs = 0;

    for ( int i = 0; i < selectedLines.size(); ++i ) {
        if ( selectedLines.at( i ).startsWith( "\t" ) ) {
            selectedLines[ i ].remove( 0, 1 );
            ++tabs;
        }
    }

    int pos = cur.position();
    int apos = cur.anchor();

    cur.insertText( selectedLines.join( "\n" ) );

    if ( pos > apos ) {
        cur.setPosition( apos, QTextCursor::MoveAnchor );
        cur.movePosition( QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                          pos - apos - tabs );
    }
    else {
        cur.setPosition( apos - tabs, QTextCursor::MoveAnchor );
        cur.movePosition( QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor,
                          apos - pos - tabs );
    }

    setTextCursor( cur );
}

void DtTextEdit::commentSelection() {
    QTextCursor cur = textCursor();
    QString selection = cur.selection().toPlainText();

    if ( selection.size() == 0 ) {
        int pos = cur.position();
        int size = 0;
        cur.movePosition( QTextCursor::StartOfLine, QTextCursor::MoveAnchor );
        cur.movePosition( QTextCursor::EndOfLine, QTextCursor::KeepAnchor );

        selection = cur.selection().toPlainText();

        if ( selection.size() ) {
            if ( selection.trimmed().startsWith( "//" ) ) {
                int index = selection.indexOf( "//" );

                if ( index != -1 ) {
                    selection.remove( index, 2 );
                    size = -2;
                }
            }
            else {
                selection.prepend( "//" );
                size = 2;
            }

            cur.insertText( selection );
            cur.setPosition( pos + size, QTextCursor::MoveAnchor );
            setTextCursor( cur );
        }

        return;
    }

    QStringList selectedLines = selection.split( "\n" );
    int comments = 0;

    for ( int i = 0; i < selectedLines.size(); ++i ) {
        QString line = selectedLines.at( i ).trimmed();

        if ( line.isEmpty() || line.startsWith( "//" ) ) {
            ++comments;
        }
    }

    int slashes = 0;

    if ( comments == selectedLines.size() ) {
        for ( int i = 0; i < selectedLines.size(); ++i ) {
            int index = selectedLines[ i ].indexOf( "//" );

            if ( index != -1 ) {
                selectedLines[ i ].remove( index, 2 );
                slashes -= 2;
            }
        }
    }
    else {
        for ( int i = 0; i < selectedLines.size(); ++i ) {
            if ( !selectedLines.at( i ).isEmpty() ) {
                selectedLines[ i ].prepend( "//" );
                slashes += 2;
            }
        }
    }

    int pos = cur.position();
    int apos = cur.anchor();

    cur.insertText( selectedLines.join( "\n" ) );

    if ( pos > apos ) {
        cur.setPosition( apos, QTextCursor::MoveAnchor );
        cur.movePosition( QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                          pos - apos + slashes );
    }
    else {
        cur.setPosition( apos + slashes, QTextCursor::MoveAnchor );
        cur.movePosition( QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor,
                          apos - pos + slashes );
    }

    setTextCursor( cur );
}

void DtTextEdit::keyPressEvent( QKeyEvent* e ) {
    if ( completer && completer->popup()->isVisible() ) {
       switch ( e->key() ) {
           case Qt::Key_Enter :
           case Qt::Key_Return :
           case Qt::Key_Escape :
           case Qt::Key_Tab :
           case Qt::Key_Backtab : e->ignore(); return;
           default : break;
       }
    }

    ceMainWindow->clearReplacedText();
    bool ctrlModifier = e->modifiers() & Qt::ControlModifier;
    bool shiftModifier = e->modifiers() & Qt::ShiftModifier;
    bool ctrlOrShift = ( ctrlModifier || shiftModifier );

    if ( shiftModifier && e->key() == Qt::Key_Delete && !textCursor().hasSelection() ) {
        QTextCursor cur = textCursor();
        cur.select( QTextCursor::LineUnderCursor );
        cur.removeSelectedText();
        cur.movePosition( QTextCursor::NextBlock, QTextCursor::MoveAnchor );
        cur.deletePreviousChar();
        return;
    }

    if ( ( e->key() == Qt::Key_Tab || e->key() == Qt::Key_Backtab ) &&
         textCursor().hasSelection() )
    {
        if ( !shiftModifier ) {
            indentSelection();
        }
        else {
            unindentSelection();
        }

        return;
    }

    QPlainTextEdit::keyPressEvent( e );

    if ( e->key() == Qt::Key_Return ||
         e->key() == Qt::Key_Enter  ||
         e->key() == Qt::Key_Backspace )
    {
        return;
    }

    if ( !completer                             ||
         ( ctrlOrShift && e->text().isEmpty() ) ||
         e->key() == Qt::Key_Delete             ||
         ctrlModifier )
    {
        return;
    }

    static QString endOfWord( " ~!@#$%^&*(){}|+-:\"<>?,./;'[]\\=" );
    bool hasModifier = ( e->modifiers() != Qt::NoModifier ) && !ctrlOrShift;
    QString completionPrefix = textUnderCursor();

    if ( hasModifier                                ||
         e->text().isEmpty()                        ||
         completionPrefix.length() < 3              ||
         endOfWord.contains( e->text().right( 1 ) ) ||
         ( completer->completionModel()->rowCount() == 1 &&
           completer->completionModel()->index( 0, 0 ).data().toString() == completionPrefix ) )
    {
        completer->popup()->hide();
        return;
    }

    if ( completionPrefix != completer->completionPrefix() ) {
        completer->setCompletionPrefix( completionPrefix );
        completer->popup()->setCurrentIndex( completer->completionModel()->index( 0, 0 ) );
    }

    QRect completeRect = cursorRect();
    completeRect.setWidth( completer->popup()->sizeHintForColumn( 0 )
                           + completer->popup()->verticalScrollBar()->sizeHint().width() * 2 );
    completer->complete( completeRect );
}

DtLineNumbers::DtLineNumbers( DtTextEdit* editor ) : QWidget( editor ) {
    textEditor = editor;
}

QSize DtLineNumbers::sizeHint() const {
    return QSize( textEditor->lineNumbersWidth(), 0 );
}

void DtLineNumbers::paintEvent( QPaintEvent* e ) {
    textEditor->lineNumbersPaintEvent( e );
}

DtComparePage::DtComparePage( QString path, QWidget* parent ) : QWidget( parent ),
    autoShown( false )
{
    QVBoxLayout* mainLayout = new QVBoxLayout;
    QHBoxLayout* pathLayout = new QHBoxLayout;

    firstPath = new DtPathEdit( "", path, true, this );
    secondPath = new DtPathEdit( "", "", true, this );

    pathLayout->addWidget( firstPath );
    pathLayout->addSpacing( 20 );
    pathLayout->addWidget( secondPath );

    mainLayout->addLayout( pathLayout );

    table = new DtCompareTable( this );
    table->horizontalHeader()->setClickable( false );
    table->horizontalHeader()->setSortIndicatorShown( false );
    table->horizontalHeader()->setStretchLastSection( true );
    table->setFixedWidth( 640 );
    mainLayout->addWidget( table, 1, Qt::AlignHCenter );

    setLayout( mainLayout );
}

void DtComparePage::showEvent( QShowEvent* e ) {
    if ( !autoShown && !firstPath->getPath().isEmpty() ) {
        secondPath->setPath( QFileInfo( firstPath->getPath() ).absoluteDir().path() + "/*.cfg" );
        secondPath->setFocus();
        QTimer::singleShot( 0, secondPath, SLOT( showSelectPathDialog() ) );
        autoShown = true;
    }

    QWidget::showEvent( e );
}

void DtComparePage::setOptionsChanged() {
    compare();
}

static void removeQuotes( QString& str ) {
    if ( str.startsWith( '"' ) && str.endsWith( '"' ) ) {
        str.chop( 1 );
        str = str.right( str.size() - 1 );
    }
}

static void chopComments( QString& str ) {
    int commentIndex = str.indexOf( "//", 0 );

    if ( commentIndex != -1 ) {
        str.chop( str.size() - commentIndex );
        str = str.trimmed();
    }
}

QMap< QString, QString > DtComparePage::getCvars( QFile& file ) {
    QMap< QString, QString > cvars;
    DtHighlighter script;
    QRegExp cmdLineReg( "^([\\s]*[^\\s]+)([\\s]*[^\\s]*|\\\"[\\w\\W\\s]*\\\")([\\s]*[^\n]*)" );

    while ( !file.atEnd() ) {
        QString line = file.readLine().trimmed();
        chopComments( line );

        if ( cmdLineReg.indexIn( line ) != -1 ) {
            QString var;
            QString val;

            if ( !cmdLineReg.cap( 1 ).compare( "seta", Qt::CaseInsensitive )   ||
                 !cmdLineReg.cap( 1 ).compare( "set", Qt::CaseInsensitive )    ||
                 !cmdLineReg.cap( 1 ).compare( "sets", Qt::CaseInsensitive )   ||
                 !cmdLineReg.cap( 1 ).compare( "setu", Qt::CaseInsensitive ) )
            {
                var = cmdLineReg.cap( 2 ).trimmed().toLower();
                val = cmdLineReg.cap( 3 ).trimmed().toLower();
            }
            else {
                var = cmdLineReg.cap( 1 ).trimmed().toLower();
                val = cmdLineReg.cap( 2 ).trimmed().toLower();
            }

            int type = script.stringHash.value( var, DtHighlighter::TK_UNK );

            if ( type == DtHighlighter::TK_CVAR ) {
                removeQuotes( val );
                cvars.insert( var, val );
            }
        }
    }

    file.close();
    return cvars;
}

QString DtComparePage::getCaseName( QString name ) {
    int scriptWordsSize = scriptWords.size();

    for ( int i = 0; i < scriptWordsSize; ++i ) {
        if ( scriptWords.at( i ).toLower() == name ) {
            return scriptWords.at( i );
        }
    }

    return name;
}

void DtComparePage::compare() {
    QFile firstFile( firstPath->getPath() );
    QFile secondFile( secondPath->getPath() );

    table->setColumnCount( 0 );
    table->setRowCount( 0 );

    if ( !firstFile.exists()                                    ||
         !secondFile.exists()                                   ||
         QFileInfo( firstPath->getPath() ).suffix() != "cfg"    ||
         QFileInfo( secondPath->getPath() ).suffix() != "cfg"   ||
         !firstFile.open( QFile::ReadOnly )                     ||
         !secondFile.open( QFile::ReadOnly ) )
    {
        return;
    }

    QStringList columns;
    columns << tr( "Variable" ) << QFileInfo( firstFile.fileName() ).fileName()
            << QFileInfo( secondFile.fileName() ).fileName();

    table->setColumnCount( 3 );
    table->setHorizontalHeaderLabels( columns );

    table->horizontalHeader()->setResizeMode( 0, QHeaderView::Fixed );
    table->horizontalHeader()->resizeSection( 0, 250 );

    table->horizontalHeader()->setResizeMode( 1, QHeaderView::Fixed );
    table->horizontalHeader()->resizeSection( 1, 180 );

    table->horizontalHeader()->setResizeMode( 2, QHeaderView::Fixed );
    table->horizontalHeader()->resizeSection( 2, 180 );

    QMap< QString, QString > firstCvars = getCvars( firstFile );
    QMap< QString, QString > secondCvars = getCvars( secondFile );

    QMapIterator< QString, QString > it( firstCvars );
    QLocale cLocale( QLocale::C );

    while ( it.hasNext() ) {
        it.next();

        QString firstValue = secondCvars.value( it.key() );
        QString secondValue = it.value();
        bool equal;
        bool firstOk;
        bool secondOk;

        double firstDouble = cLocale.toDouble( firstValue, &firstOk );
        double secondDouble = cLocale.toDouble( secondValue, &secondOk );

        if ( firstOk && secondOk ) {
            equal = ( firstDouble == secondDouble );
        }
        else {
            equal = ( firstValue == secondValue );
        }

        if ( !equal ) {
            int row = table->rowCount();
            table->setRowCount( row + 1 );

            QTableWidgetItem* item = new QTableWidgetItem( getCaseName( it.key() ) );
            table->setItem( row, 0, item );

            item = new QTableWidgetItem( it.value() );
            table->setItem( row, 1, item );

            item = new QTableWidgetItem( secondCvars.value( it.key(), "–" ) );
            table->setItem( row, 2, item );
        }
    }

    QMapIterator< QString, QString > secondIt( secondCvars );

    while ( secondIt.hasNext() ) {
        secondIt.next();

        if ( !firstCvars.contains( secondIt.key() ) ) {
            int row = table->rowCount();
            table->setRowCount( row + 1 );

            QTableWidgetItem* item = new QTableWidgetItem( getCaseName( secondIt.key() ) );
            table->setItem( row, 0, item );

            item = new QTableWidgetItem( "–" );
            table->setItem( row, 1, item );

            item = new QTableWidgetItem( secondIt.value() );
            table->setItem( row, 2, item );
        }
    }

    table->sortByColumn( 0, Qt::AscendingOrder );
}

