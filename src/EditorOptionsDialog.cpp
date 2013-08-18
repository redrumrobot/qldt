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

#include "Data.h"
#include "EditorOptionsDialog.h"
#include "Table.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QFontDatabase>
#include <QMessageBox>
#include <QCloseEvent>
#include <QHeaderView>
#include <QPainter>
#include <QColorDialog>
#include <QDir>

using namespace dtdata;

DtEditorOptionsDialog::DtEditorOptionsDialog( QWidget* parent ) : DtOptionsDialog( parent ) {
    setWindowTitle( tr( "Preferences" ) );
    setFixedSize( 600, 740 );

    editorOptionsPage = new DtEditorOptionsPage( this );
    mainLayout->insertWidget( 0, editorOptionsPage );
}

void DtEditorOptionsDialog::readConfig() {
    optionsChanged = true;
    editorOptionsPage->readConfig();
    optionsChanged = false;
    btnApply->setEnabled( false );
}

void DtEditorOptionsDialog::writeConfig() {
    editorOptionsPage->writeConfig();
    config.save();
    emit settingsChanged();
}

void DtEditorOptionsDialog::closeEvent( QCloseEvent* event ) {
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

    optionsChanged = false;
    event->accept();
}

void DtEditorOptionsDialog::defaults() {
    int act = QMessageBox::question( this,
                                     tr( "Defaults" ),
                                     tr( "Restore Defaults?" ),
                                     QMessageBox::Yes | QMessageBox::No );

    if ( act == QMessageBox::Yes ) {
        config.textEditorDefaults();
        readConfig();

        emit settingsChanged();
    }
}

DtEditorOptionsPage::DtEditorOptionsPage( QWidget* parent ) : QWidget( parent ) {
    const int spacing = 10;

    QVBoxLayout* pageLayout = new QVBoxLayout;
    QTabWidget* tabs = new QTabWidget( this );
    pageLayout->addWidget( tabs );

    QWidget* mainTab = new QWidget( this );
    QVBoxLayout* mainLayout = new QVBoxLayout;
    QGroupBox* fontGroup = new QGroupBox( tr( "Font" ), this );
    QHBoxLayout* fontLayout = new QHBoxLayout;

    QLabel* fontNameLbl = new QLabel( tr( "Name" ) );
    fontNameCombo = new DtOptionsComboBox( this );
    fontNameCombo->insertItems( 0, QFontDatabase().families() );

    connect( fontNameCombo, SIGNAL( currentIndexChanged( const QString& ) ),
             this, SLOT( fontChanged( const QString& ) ) );

    QLabel* fontSizeLbl = new QLabel( tr( "Size" ) );
    fontSizeCombo = new DtOptionsComboBox( this );

    fontLayout->addSpacing( spacing );
    fontLayout->addWidget( fontNameLbl );
    fontLayout->addWidget( fontNameCombo );
    fontLayout->addSpacing( 20 );
    fontLayout->addWidget( fontSizeLbl );
    fontLayout->addWidget( fontSizeCombo );
    fontLayout->addStretch( 1 );

    fontGroup->setLayout( fontLayout );
    mainLayout->addWidget( fontGroup );

    QGroupBox* tabGroup = new QGroupBox( tr( "Tabs" ), this );
    QHBoxLayout* tabLayout = new QHBoxLayout;

    QLabel* tabSizeLbl = new QLabel( tr( "Tab width" ) );
    tabSizeSpinBox = new DtOptionsSpinBox( this );
    tabSizeSpinBox->setSuffix( " " + tr( "characters" ) );

    tabLayout->addSpacing( spacing );
    tabLayout->addWidget( tabSizeLbl );
    tabLayout->addWidget( tabSizeSpinBox );
    tabLayout->addStretch( 1 );

    tabGroup->setLayout( tabLayout );
    mainLayout->addWidget( tabGroup );

    QGroupBox* colorsGroup = new QGroupBox( tr( "Colors" ), this );
    QGridLayout* colorsLayout = new QGridLayout;

    QLabel* bgColorLbl = new QLabel( tr( "Background" ) );
    bgColorButton = new DtOptionsColorButton( 0, this );

    QLabel* lineNumbersLbl = new QLabel( tr( "Line numbers" ) );
    lineNumbersColorButton = new DtOptionsColorButton( 0, this );

    QLabel* lineNumbersBgLbl = new QLabel( tr( "Line numbers background" ) );
    lineNumbersBgColorButton = new DtOptionsColorButton( 0, this );

    QLabel* highlightLineLbl = new QLabel( tr( "Line highlight" ) );
    highlightLineColorButton = new DtOptionsColorButton( 0, this );

    colorsLayout->addWidget( bgColorLbl, 0, 0, Qt::AlignLeft );
    colorsLayout->addWidget( bgColorButton, 0, 1, Qt::AlignLeft );
    colorsLayout->addWidget( lineNumbersLbl, 1, 0, Qt::AlignLeft );
    colorsLayout->addWidget( lineNumbersColorButton, 1, 1, Qt::AlignLeft );
    colorsLayout->addWidget( lineNumbersBgLbl, 2, 0, Qt::AlignLeft );
    colorsLayout->addWidget( lineNumbersBgColorButton, 2, 1, Qt::AlignLeft );
    colorsLayout->addWidget( highlightLineLbl, 3, 0, Qt::AlignLeft );
    colorsLayout->addWidget( highlightLineColorButton, 3, 1, Qt::AlignLeft );
    colorsLayout->setRowStretch( colorsLayout->rowCount() + 1, 1 );
    colorsLayout->setColumnStretch( colorsLayout->rowCount() + 1, 1 );

    colorsGroup->setLayout( colorsLayout );
    mainLayout->addWidget( colorsGroup );

    mainTab->setLayout( mainLayout );
    tabs->addTab( mainTab, tr( "Font and colors" ) );

    QWidget* highlighterTab = new QWidget( this );
    QVBoxLayout* highlighterPageLayout = new QVBoxLayout;
    highlighterTab->setLayout( highlighterPageLayout );

    QGroupBox* highlightGroup = new QGroupBox( tr( "Colors" ), this );
    QHBoxLayout* highlightLayout = new QHBoxLayout;

    colorsTable = new DtColorsTable( this );

    QStringList contexts;
    contexts << tr( "Normal text" ) << tr( "Numbers" ) << tr( "Strings" ) << tr( "Keys" ) <<
                tr( "Comments" ) << tr( "Actions" ) << tr( "Cvars" ) << tr( "Commands" ) <<
                tr( "Short commands" ) << tr( "Print commands" ) << tr( "Semicolon" ) <<
                tr( "Preprocessor" );

    colorsTable->setRowCount( contexts.size() );
    colorsTable->setFixedHeight( colorsTable->rowHeight( 0 ) * ( contexts.size() + 2 ) );

    QTableWidgetItem* colName;
    DtColorOptionsCheckBox* checkbox;
    DtOptionsColorButton* button;
    DtColorButtonWidget* buttonWidget;
    QHBoxLayout* buttonLayout;
    int row = 0;

    foreach ( const QString& contextName, contexts ) {
        colName = new QTableWidgetItem( contextName );
        colorsTable->setItem( row, DtColorsTable::TC_NAME, colName );

        checkbox = new DtColorOptionsCheckBox( row, colorsTable, this );
        colorsTable->setCellWidget( row, DtColorsTable::TC_BOLD, checkbox );

        checkbox = new DtColorOptionsCheckBox( row, colorsTable, this );
        colorsTable->setCellWidget( row, DtColorsTable::TC_ITALIC, checkbox );

        checkbox = new DtColorOptionsCheckBox( row, colorsTable, this );
        colorsTable->setCellWidget( row, DtColorsTable::TC_UNDERLINE, checkbox );

        button = new DtOptionsColorButton( row, this );
        connect( button, SIGNAL( mouseEntered( int ) ), colorsTable, SLOT( onRowEntered( int ) ) );
        buttonLayout = new QHBoxLayout;
        buttonLayout->setMargin( 0 );
        buttonLayout->addWidget( button, 0, Qt::AlignCenter );
        buttonWidget = new DtColorButtonWidget( row, button, this );
        connect( buttonWidget, SIGNAL( mouseEntered( int ) ),
                 colorsTable, SLOT( onRowEntered( int ) ) );
        buttonWidget->setLayout( buttonLayout );
        colorsTable->setCellWidget( row++, DtColorsTable::TC_COLOR, buttonWidget );
    }

    highlightLayout->addSpacing( spacing );
    highlightLayout->addWidget( colorsTable );
    highlightLayout->addSpacing( spacing );

    highlightGroup->setLayout( highlightLayout );
    highlighterPageLayout->addWidget( highlightGroup );

    QGroupBox* filesGroup = new QGroupBox( tr( "Custom Files" ), this );
    QGridLayout* filesLayout = new QGridLayout;
    filesGroup->setLayout( filesLayout );

    QLabel* useCustomFilesLbl = new QLabel( tr( "Use custom highlight files" ), this );
    useCustomFilesCb = new DtOptionsCheckBox( this );

    filesLayout->addWidget( useCustomFilesLbl, 0, 0, Qt::AlignLeft );
    filesLayout->addWidget( useCustomFilesCb, 0, 1, Qt::AlignLeft );

    QLabel* customFilesPathLbl = new QLabel( tr( "Path to custom files" ), this );
    customFilesPathEdit = new DtPathEdit( "", "", false, this ) ;
    customFilesPathEdit->setTextFieldMinWidth( 280 );

    filesLayout->addWidget( customFilesPathLbl, 1, 0, Qt::AlignLeft );
    filesLayout->addWidget( customFilesPathEdit, 1, 1, Qt::AlignLeft );

    filesLayout->setRowStretch( filesLayout->rowCount() + 1, 1 );
    filesLayout->setColumnStretch( filesLayout->rowCount() + 1, 1 );

    highlighterPageLayout->addWidget( filesGroup );
    highlighterPageLayout->addStretch( 1 );

    tabs->addTab( highlighterTab, tr( "Syntax highlight" ) );

    setLayout( pageLayout );
}

void DtEditorOptionsPage::setOptionsChanged() {
    if ( parent()->inherits( "DtOptionsDialog" ) ) {
        qobject_cast< DtOptionsDialog* >( parent() )->setOptionsChanged();
    }
}

void DtEditorOptionsPage::setWidgetsState( int& row, const QTextCharFormat& format ) {
    DtColorOptionsCheckBox* checkbox = colorsTable->checkBox( row, DtColorsTable::TC_BOLD );
    checkbox->setChecked( format.fontWeight() == QFont::Bold );
    checkbox = colorsTable->checkBox( row, DtColorsTable::TC_ITALIC );
    checkbox->setChecked( format.fontItalic() );
    checkbox = colorsTable->checkBox( row, DtColorsTable::TC_UNDERLINE );
    checkbox->setChecked( format.fontUnderline() );

    DtOptionsColorButton* button = colorsTable->button( row );
    button->setColor( format.foreground().color() );

    ++row;
}

void DtEditorOptionsPage::readConfig() {
    fontNameCombo->setCurrentIndex( fontNameCombo->findText( config.textEditor.fontFamily ) );
    fontSizeCombo->setCurrentIndex(
            fontSizeCombo->findText( QString::number ( config.textEditor.fontSize ) ) );
    tabSizeSpinBox->setValue( config.textEditor.tabSize );
    bgColorButton->setColor( config.textEditor.backgroundColor );
    lineNumbersColorButton->setColor( config.textEditor.lineNumbersColor );
    lineNumbersBgColorButton->setColor( config.textEditor.lineNumbersBgColor );
    highlightLineColorButton->setColor( config.textEditor.lineHighlightColor );

    int row = 0;
    setWidgetsState( row, config.textEditor.normalText );
    setWidgetsState( row, config.textEditor.number );
    setWidgetsState( row, config.textEditor.string );
    setWidgetsState( row, config.textEditor.key );
    setWidgetsState( row, config.textEditor.comment );
    setWidgetsState( row, config.textEditor.action );
    setWidgetsState( row, config.textEditor.cvar );
    setWidgetsState( row, config.textEditor.command );
    setWidgetsState( row, config.textEditor.shortCommand );
    setWidgetsState( row, config.textEditor.say );
    setWidgetsState( row, config.textEditor.semicolon );
    setWidgetsState( row, config.textEditor.preprocessor );

    useCustomFilesCb->setChecked( config.textEditor.useCustomFiles );
    customFilesPathEdit->setPath( config.textEditor.customFilesPath );
}

void DtEditorOptionsPage::storeWidgetsState( int& row, QTextCharFormat& format ) {
    DtColorOptionsCheckBox* checkbox = colorsTable->checkBox( row, DtColorsTable::TC_BOLD );
    format.setFontWeight( checkbox->isChecked() ? QFont::Bold : QFont::Normal );
    checkbox = colorsTable->checkBox( row, DtColorsTable::TC_ITALIC );
    format.setFontItalic( checkbox->isChecked() );
    checkbox = colorsTable->checkBox( row, DtColorsTable::TC_UNDERLINE );
    format.setFontUnderline( checkbox->isChecked() );

    DtOptionsColorButton* button = colorsTable->button( row );
    format.setForeground( QBrush( button->getColor() ) );

    ++row;
}

static void createSyntaxHighlighterFiles( QString path ) {
    QDir customDir( path );

    if ( !customDir.exists() ) {
        customDir.mkpath( path );
    }

    QStringList fileList;
    fileList << "cvars" << "cmds" << "actions" << "keys";

    foreach ( const QString& file, fileList ) {
        if ( !QFile::exists( path + "/" + file + ".txt" ) ) {
            QString dest = path + "/" + file + ".txt";
            QFile::copy( ":/res/idscript/" + file + ".txt", dest );
            QFile::setPermissions( dest, QFile::ReadOwner | QFile::WriteOwner );
        }
    }
}

void DtEditorOptionsPage::writeConfig() {
    config.textEditor.fontFamily = fontNameCombo->currentText();
    config.textEditor.fontSize = fontSizeCombo->currentText().toInt();
    config.textEditor.tabSize = tabSizeSpinBox->value();
    config.textEditor.backgroundColor = bgColorButton->getColor();
    config.textEditor.lineNumbersColor = lineNumbersColorButton->getColor();
    config.textEditor.lineNumbersBgColor = lineNumbersBgColorButton->getColor();
    config.textEditor.lineHighlightColor = highlightLineColorButton->getColor();

    int row = 0;
    storeWidgetsState( row, config.textEditor.normalText );
    storeWidgetsState( row, config.textEditor.number );
    storeWidgetsState( row, config.textEditor.string );
    storeWidgetsState( row, config.textEditor.key );
    storeWidgetsState( row, config.textEditor.comment );
    storeWidgetsState( row, config.textEditor.action );
    storeWidgetsState( row, config.textEditor.cvar );
    storeWidgetsState( row, config.textEditor.command );
    storeWidgetsState( row, config.textEditor.shortCommand );
    storeWidgetsState( row, config.textEditor.say );
    storeWidgetsState( row, config.textEditor.semicolon );
    storeWidgetsState( row, config.textEditor.preprocessor );

    config.textEditor.useCustomFiles = useCustomFilesCb->isChecked();
    config.textEditor.customFilesPath = customFilesPathEdit->getPath();

    if ( config.textEditor.useCustomFiles && !config.textEditor.customFilesPath.isEmpty() ) {
        createSyntaxHighlighterFiles( config.textEditor.customFilesPath );
    }
}

void DtEditorOptionsPage::fontChanged( const QString& text ) {
    QList< int > sizes = QFontDatabase().pointSizes( text );

    int lastSize = fontSizeCombo->currentText().toInt();
    fontSizeCombo->clear();

    for ( int i = 0; i < sizes.size(); ++i ) {
        fontSizeCombo->insertItem( i, QString::number( sizes.at( i ) ) );

        if ( sizes.at( i ) == lastSize ) {
            fontSizeCombo->setCurrentIndex( i );
        }
    }
}

void DtEditorOptionsPage::colorButtonClicked() {
    DtOptionsColorButton* button = qobject_cast< DtOptionsColorButton* >( sender() );
    QColor color = QColorDialog::getColor( button->getColor(), this );

    if( color.isValid() ) {
        button->setColor( color );
        setOptionsChanged();
    }
}

DtColorButtonWidget::DtColorButtonWidget( int rowNum, DtOptionsColorButton* cButton,
                                          QWidget* parent ) : QWidget( parent ) {
    row = rowNum;
    setMouseTracking( true );
    colorButton = cButton;
}

DtOptionsColorButton* DtColorButtonWidget::button() {
    return colorButton;
}

void DtColorButtonWidget::enterEvent( QEvent* ) {
    emit mouseEntered( row );
}

DtOptionsColorButton::DtOptionsColorButton( int rowNum, QWidget* parent ) : DtOptionsButton( "", parent ) {
    row = rowNum;
    setMinimumHeight( 23 );
    setMouseTracking( true );

    connect( this, SIGNAL( clicked() ), parent, SLOT( colorButtonClicked() ) );
    buttonColor = QColor( 255, 0, 0 );
}

void DtOptionsColorButton::setColor( const QColor& color ) {
    buttonColor = color;
    update();
}

QColor DtOptionsColorButton::getColor() const {
    return buttonColor;
}

void DtOptionsColorButton::paintEvent( QPaintEvent* e ) {
    DtOptionsButton::paintEvent( e );

    QPainter painter( this );
    QRect colorRect( 15, 7, width() - 30, height() - 14 );
    QBrush colorBrush( buttonColor );
    QBrush frameBrush( QColor( 200, 200, 200 ) );

    painter.fillRect( colorRect.left() - 1, colorRect.top() - 1,
                      colorRect.width() + 2, colorRect.height() + 2,
                      frameBrush );
    painter.fillRect( colorRect, colorBrush );
}

void DtOptionsColorButton::enterEvent( QEvent* ) {
    emit mouseEntered( row );
}

DtColorsTable::DtColorsTable( QWidget* parent ) : DtTable( parent ) {
    setItemDelegate( new DtColorTableDelegate( this ) );
    setColumnCount( 5 );
    setShowGrid( false );

    setSelectionBehavior( QAbstractItemView::SelectRows );
    setSelectionMode( QAbstractItemView::NoSelection );

    QStringList columnNames;
    columnNames << tr( "Context" ) << "" << "" << "" << tr( "Color" );
    setHorizontalHeaderLabels( columnNames );

    horizontalHeader()->setResizeMode( TC_NAME, QHeaderView::Fixed );
    horizontalHeader()->resizeSection( TC_NAME, 200 );

    horizontalHeaderItem( TC_BOLD )->setIcon( QIcon( ":/res/format-text-bold.png" ) );
    horizontalHeader()->setResizeMode( TC_BOLD, QHeaderView::Fixed );
    horizontalHeader()->resizeSection( TC_BOLD, 25 );

    horizontalHeaderItem( TC_ITALIC )->setIcon( QIcon( ":/res/format-text-italic.png" ) );
    horizontalHeader()->setResizeMode( TC_ITALIC, QHeaderView::Fixed );
    horizontalHeader()->resizeSection( TC_ITALIC, 25 );

    horizontalHeaderItem( TC_UNDERLINE )->setIcon( QIcon( ":/res/format-text-underline.png" ) );
    horizontalHeader()->setResizeMode( TC_UNDERLINE, QHeaderView::Fixed );
    horizontalHeader()->resizeSection( TC_UNDERLINE, 25 );

    connect( this, SIGNAL( cellEntered( int, int ) ),
             this, SLOT( onCellEntered( int, int ) ) );

    setMouseTracking( true );
}

DtColorOptionsCheckBox* DtColorsTable::checkBox( int row, int column ) {
    return qobject_cast< DtColorOptionsCheckBox* >( cellWidget( row, column ) );
}

DtOptionsColorButton* DtColorsTable::button( int row ) {
    DtColorButtonWidget* buttonWidget = qobject_cast< DtColorButtonWidget* >(
                                        cellWidget( row, TC_COLOR ) );
    return buttonWidget->button();
}

void DtColorsTable::onRowEntered( int row ) {
    onCellEntered( row, 0 );
}

void DtColorsTable::onCellEntered( int row, int ) {
    DtColorTableDelegate* delegate = qobject_cast< DtColorTableDelegate* >( itemDelegate() );
    delegate->highlightRow( row );
    setDirtyRegion( viewport()->rect() );
    update();
}

void DtColorsTable::leaveEvent( QEvent* ) {
    DtColorTableDelegate* delegate = qobject_cast< DtColorTableDelegate* >( itemDelegate() );
    delegate->highlightRow( -1 );
    setDirtyRegion( viewport()->rect() );
    update();
}

DtColorTableDelegate::DtColorTableDelegate( QWidget* ) {
    highlihgtedRowNum = -1;
}

void DtColorTableDelegate::highlightRow( int row ) {
    highlihgtedRowNum = row;
}

void DtColorTableDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const {
    painter->save();

    if ( highlihgtedRowNum == index.row() ) {
        QRect rec = option.rect;

        QBrush pBrush = painter->brush();
        pBrush.setStyle( Qt::SolidPattern );

        pBrush.setColor( QColor( "#d9d9d9" ) );

        painter->setPen( Qt::NoPen );
        painter->setBrush( pBrush );
        painter->drawRect( rec );

        painter->setPen( QColor( "black" ) );
    }

    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, index );

    QRect drawRect = opt.rect;
    drawRect.setLeft( drawRect.left() + 5 );

    painter->drawText( drawRect, Qt::AlignVCenter | Qt::TextSingleLine | Qt::AlignLeft, opt.text );
    painter->restore();
}

DtColorOptionsCheckBox::DtColorOptionsCheckBox( int rowNum, QWidget* receiver, QWidget* parent ) :
                                                DtOptionsCheckBox( parent ) {
    setMouseTracking( true );
    row = rowNum;

    connect( this, SIGNAL( mouseEntered( int ) ), receiver, SLOT( onRowEntered( int ) ) );
}

void DtColorOptionsCheckBox::enterEvent( QEvent* ) {
    emit mouseEntered( row );
}
