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

#include "Version.h"
#include "About.h"

#include <QPainter>
#include <QMouseEvent>
#include <QDesktopServices>
#include <QUrl>

DtAbout::DtAbout( QWidget* parent ) : QDialog( parent ) {
    setWindowTitle( tr( "About" ) );
    setFixedSize( 400, 200 );
    setMouseTracking( true );

    siteUrl = "http://qldt.sf.net";

    QPixmap qzTitle( ":/res/ql_logo.png" );
    QPixmap aboutBg( ":/res/about_bg.png" );
    QPixmap year( ":/res/about_year.png" );

    overLink = false;
    linkPressed = false;

    QColor pFontColor( "#52525a" );
    QColor pFontShadow( "#acacac" );

    pFontColorLightGray.setNamedColor( "#c0c0c0" );
    pFontColorLink.setNamedColor( "blue" );

    bg = new QPixmap( width(), height() );

    QPainter painter( bg );

    painter.drawPixmap( 0, 0, aboutBg );
    painter.drawPixmap( width() / 2 - qzTitle.width() / 2, 0, qzTitle );
    painter.drawPixmap( width() - year.width(), height() - year.height(), year );

    pFont.setPointSize( 9 );
    pFont.setFamily( "Liberation Sans" );
    pFont.setBold( true );

    painter.setFont( pFont );

    int verX = 114;
    int verY = 112;

    painter.setPen( pFontShadow );
    QRect rect( verX + 1, verY + 1, width(), 15 );
    painter.drawText( rect, Qt::AlignCenter, VERSION );

    painter.setPen( pFontColor );
    rect.setRect( verX, verY, width(), 15 );
    painter.drawText( rect, Qt::AlignCenter, VERSION );

    painter.end();

    pFont.setPointSize( 9 );

    QFontMetrics fontMetrics( pFont );
    linkRect.setRect( 3, height() - fontMetrics.height() - 1,
                      fontMetrics.width( siteUrl ), fontMetrics.height() );
}

void DtAbout::paintEvent( QPaintEvent* ) {
    QPainter painter( this );

    painter.drawPixmap( rect(), *bg );
    painter.setFont( pFont );
    painter.setPen( overLink ? pFontColorLink : pFontColorLightGray );
    painter.drawText( linkRect, Qt::AlignLeft, siteUrl );
}

void DtAbout::mouseMoveEvent( QMouseEvent* e ) {
    if ( overLink != linkRect.contains( e->pos() ) ) {
        overLink = linkRect.contains( e->pos() );
        setCursor( overLink ? Qt::PointingHandCursor : Qt::ArrowCursor );
        update();
    }
}

void DtAbout::leaveEvent( QEvent* ) {
    if ( overLink ) {
        overLink = false;
        update();
    }

    setCursor( Qt::ArrowCursor );
}

void DtAbout::mousePressEvent( QMouseEvent* e ) {
    if ( e->button() == Qt::LeftButton ) {
        linkPressed = true;
    }
}

void DtAbout::mouseReleaseEvent( QMouseEvent* e ) {
    if ( e->button() == Qt::LeftButton && linkPressed ) {
        linkPressed = false;

        if ( overLink ) {
            QDesktopServices::openUrl( siteUrl );
        }
        else {
            close();
        }
    }
}

DtAbout::~DtAbout() {
    delete bg;
}
