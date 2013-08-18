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

#ifndef DTGAMECONFIGEDITOR_H
#define DTGAMECONFIGEDITOR_H

#include "Table.h"

#include <QMainWindow>
#include <QHash>
#include <QDialog>
#include <QPlainTextEdit>

class DtHighlighter;
class DtTabWidget;
class QFile;
class QLineEdit;
class QPushButton;
class DtEditorSearchPanel;
class DtEditorAdvancedSearchPanel;
class QCheckBox;
class DtTextEdit;
class DtLineNumbers;
class QCompleter;
class QListWidgetItem;
class DtComparePage;
class DtPathEdit;
class DtCompareTable;

class DtGameConfigEditor : public QMainWindow {
    Q_OBJECT
public:
    DtGameConfigEditor( QWidget* parent = 0 );
    ~DtGameConfigEditor();

    void setDir( const QString& dir );
    QString getClipboardText() const;

public slots:
    void newConfig();
    void showWelcomePage();
    void hideWelcomePage();
    void openConfig( const QString& fileName );
    void openConfig();
    void applySettings();
    void applicationMessage( const QString& msg );
    void startSearch();
    void findText( QString searchString );
    void findNext();
    void findPrev();
    void showAbout();
    void replaceText();
    void replaceNextText();
    void replaceAllText();
    void clearReplacedText();

protected:
    QString defaultWindowName;
    DtTabWidget* tabs;
    QToolBar* toolBar;
    QString currentDirectory;
    QAction* aLineWrap;
    QAction* aHighlight;
    QAction* aLineNumbers;
    QAction* aHighlightLine;
    QAction* aAutocomplete;
    QAction* actAbout;
    DtEditorSearchPanel* searchPanel;
    DtEditorAdvancedSearchPanel* replacePanel;
    QMenu* recentMenu;
    QWidget* welcomePage;
    DtComparePage* diffPage;

    bool modified;

    int lineWrapMode;
    bool highlight;
    bool lineNumbers;
    bool highlightLine;
    bool autocomplete;

    QFile* file;
    QFont* editorFont;

    QHash< DtTextEdit*, QString > openedFiles;
    QHash< DtTextEdit*, DtHighlighter* > syntaxHighlighters;
    QVector< QPair< int, int > > replacedText;
    QString clipboard;

    DtTextEdit* createEditor( int highlightType );
    DtTextEdit* currentEditor() const;
    DtTextEdit* editorAt( int index ) const;
    DtComparePage* diffAt( int index ) const;
    void applySettingsToEditor( DtTextEdit* editor );
    void applyFontSettings();
    void removeEditor( DtTextEdit* editor );
    void closeEvent( QCloseEvent* event );
    bool eventFilter( QObject*, QEvent* );
    void setModified( bool s );
    void setLineWrapMode( int mode );
    void setHighlight( bool s );
    void setLineNumbers( bool s );
    void setHighlightLine( bool s );
    void setAutocomplete( bool s );
    void clear();
    QString currentFileName() const;
    int confirmSave( const QString& msg );
    int modifiedFilesCount();
    const QIcon& dirIcon();
    void saveWindow();
    void findInCurrent( QTextDocument::FindFlags options = 0 );
    void welcomeItemClicked( QString name );
    void addEntryToRecentList( const QString& entry, QStringList& list );

protected slots:
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void selectAll();
    void tabChanged( int index );
    void tabClosed( int index );
    void setWindowModified( bool );
    void save();
    void save( DtTextEdit* editor );
    void saveAs();
    void saveAll();
    void toggleLineWrap();
    void toggleHighlight();
    void toggleLineNumbers();
    void toggleHighlightLine();
    void toggleAutocomplete();
    void sortCfg();
    void showOptions();
    void onShowRecentMenu();
    void openRecent();
    void closeCurrentTab();
    void openDiffTab();
    void commentSelection();

signals:
    void modificationChanged( bool );
    void fileSaved();
};

class DtEditorSearchPanel : public QWidget {
    Q_OBJECT
public:
    DtEditorSearchPanel( QWidget* parent = 0 );
    void clear();

    QString searchString() const;
    void setSearchString( QString text );

protected:
    QLineEdit* searchTextEdit;
    QPushButton* prevButton;
    QPushButton* nextButton;
    QCheckBox* matchCaseCb;

    void showEvent( QShowEvent* e );

protected slots:
    void matchCaseChanged( int state );

signals:
    void findText( const QString& );
};

class DtEditorAdvancedSearchPanel : public QWidget {
    Q_OBJECT
public:
    DtEditorAdvancedSearchPanel( QWidget* parent = 0 );
    void clear();

    QString searchString() const;
    QString replaceString() const;
    void setSearchString( QString text );

protected:
    QLineEdit* searchTextEdit;
    QLineEdit* replaceTextEdit;
    QPushButton* prevButton;
    QPushButton* nextButton;
    QPushButton* replaceButton;
    QPushButton* replaceAllButton;
    QCheckBox* matchCaseCb;

    void showEvent( QShowEvent* e );

protected slots:
    void matchCaseChanged( int state );

signals:
    void findText( const QString& );
    void replaceText( const QString& text, const QString& replace );
};

class DtTextEdit : public QPlainTextEdit {
    Q_OBJECT
public:
    DtTextEdit( QWidget* parent = 0 );

    void lineNumbersPaintEvent( QPaintEvent* e );
    int lineNumbersWidth();
    void setHighlightLine( bool s );
    void setLineNumbers( bool s );
    void setAutoComplete( bool s );

public slots:
    void highlightCurrentLine();
    void commentSelection();

protected:
    DtLineNumbers* lineNumbers;
    QCompleter* completer;
    bool highlightLine;
    bool showLineNumbers;
    bool doAutoComplete;
    bool scriptWordsSorted;

    void resizeEvent( QResizeEvent* e );
    void keyPressEvent( QKeyEvent* e );
    void focusInEvent( QFocusEvent* e );
    QString textUnderCursor() const;
    void indentSelection();
    void unindentSelection();

protected slots:
    void updateLineNumbersWidth( int newBlockCount );
    void updateLineNumbers( const QRect&, int );
    void insertCompletion( const QString& completion );
};

class DtLineNumbers : public QWidget {
    Q_OBJECT
public:
    DtLineNumbers( DtTextEdit* editor );

    QSize sizeHint() const;

protected:
    void paintEvent( QPaintEvent* e );

    DtTextEdit* textEditor;
};

class DtComparePage : public QWidget {
    Q_OBJECT
public:
    DtComparePage( QString path, QWidget* parent = 0 );

public slots:
    void setOptionsChanged();

protected:
    DtPathEdit* firstPath;
    DtPathEdit* secondPath;
    DtCompareTable* table;
    bool autoShown;

    void showEvent( QShowEvent* e );
    void compare();
    QMap< QString, QString > getCvars( QFile& file );
    QString getCaseName( QString name );
};

class DtCompareTable : public DtTable {
    Q_OBJECT
public:
    DtCompareTable( QWidget* parent = 0 ) : DtTable( parent ) { }

protected slots:
    void commitData( QWidget* ) { }
};

#endif // DTGAMECONFIGEDITOR_H
