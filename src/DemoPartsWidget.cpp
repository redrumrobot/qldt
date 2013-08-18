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

#include "DemoPartsWidget.h"
#include "Demo.h"

#include <QPainter>
#include <QMouseEvent>

DtDemoPartsWidget::DtDemoPartsWidget( QWidget* parent ) : QWidget( parent ) {
    setFixedSize( 840, 50 );
    setMouseTracking( true );

    demoRect.setRect( rect().left() + 1, rect().top() + 1,
                      rect().width() - 2, rect().height() - 2 );

    topFrameColor.setRgb( 150, 150, 150 );
    bottomFrameColor.setRgb( 180, 180, 180 );
    cursorColor.setRgb( 230, 230, 230 );
    warmupColor.setRgb( 170, 170, 170 );
    pauseColor.setRgb( 180, 180, 180 );
    lagColor.setRgb( 150, 20, 20 );
    newSelectionColor.setRgb( 0, 0, 255, 50 );
    selectedColor.setRgb( 50, 50, 200, 100 );
    selectedBorderColor.setRgb( 0, 180, 0 );

    selectedPartBrush.setColor( selectedColor );
    selectedPartBrush.setStyle( Qt::SolidPattern );
    newSelectedPartBrush.setColor( newSelectionColor );
    newSelectedPartBrush.setStyle( Qt::SolidPattern );
    warmupBrush.setColor( warmupColor );
    warmupBrush.setStyle( Qt::Dense2Pattern );
    pauseBrush.setColor( pauseColor );
    pauseBrush.setStyle( Qt::Dense2Pattern );
    frameBrush.setColor( QColor( 150, 150, 150 ) );
    frameBrush.setStyle( Qt::SolidPattern );
}

void DtDemoPartsWidget::setDemo( DtDemo* demo ) {
    demoLength = demo->getLength();
    frameTime = demo->getFrameTime();
    warmupLength = demo->getMapRestartTime();
    pauses = demo->getPauses();
    lags = demo->getLags();
    pixMs = static_cast< float >( demoLength ) / width();
    warmupPos = static_cast< int >( warmupLength / pixMs );

    clearParts();
}

void DtDemoPartsWidget::paintEvent( QPaintEvent* ) {
    QPainter painter( this );

    /* widget frame */

    painter.setPen( bottomFrameColor );
    painter.drawLine( rect().bottomLeft(), rect().bottomRight() );
    painter.drawLine( rect().topRight(), rect().bottomRight() );
    painter.setPen( topFrameColor );
    painter.drawLine( rect().topLeft(), rect().topRight() );
    painter.drawLine( rect().topLeft(), rect().bottomLeft() );

    /* background */

    QLinearGradient grad( 0, 0, 0, demoRect.height() );
    grad.setColorAt( 0, QColor( "#a5a5a5" ) );
    grad.setColorAt( 1, QColor( "#6d6d6d" ) );
    QBrush demoGradientBrush( grad );

    painter.fillRect( demoRect, demoGradientBrush );

    /* warmup */

    QRect warmupRect = demoRect;

    warmupRect.setRight( warmupPos );
    painter.fillRect( warmupRect, warmupBrush );
    painter.setPen( warmupColor );
    painter.drawLine( warmupRect.right(), 1, warmupRect.right(), demoRect.height() );

    /* pauses */

    if ( !pauses.isEmpty() ) {
        int pausesCount = pauses.count();
        painter.setPen( pauseColor );

        for ( int i = 0; i < pausesCount; ++i ) {
            QRect pauseRect = demoRect;
            pauseRect.setLeft( pauses.at( i ).start / pixMs );
            pauseRect.setRight( pauses.at( i ).end / pixMs );
            painter.fillRect( pauseRect, pauseBrush );
            painter.drawLine( pauseRect.right(), 1, pauseRect.right(), demoRect.height() );
            painter.drawLine( pauseRect.left(), 1, pauseRect.left(), demoRect.height() );
        }
    }

    /* lags */

    if ( dtdata::config.showLags && !lags.isEmpty() ) {
        int lagsCount = lags.count();
        painter.setPen( lagColor );

        for ( int i = 0; i < lagsCount; ++i ) {
            int left = lags.at( i ) / pixMs;
            painter.drawLine( left, 1, left, demoRect.height() );
        }
    }

    /* selected parts */

    if ( !parts.isEmpty() ) {
        int partsCount = parts.count();

        for ( int i = 0; i < partsCount; ++i ) {
            QRect partRect = demoRect;
            partRect.setLeft( parts.at( i ).startPx );
            partRect.setRight( parts.at( i ).endPx );
            painter.fillRect( partRect, selectedPartBrush );
        }
    }

    /* current selection */

    if ( partSelect ) {
        QRect partRect = demoRect;

        if ( partStartLeft < cursorLeft ) {
            partRect.setLeft( partStartLeft );
            partRect.setRight( cursorLeft );
        }
        else {
            partRect.setLeft( cursorLeft );
            partRect.setRight( partStartLeft );
        }

        painter.fillRect( partRect, newSelectedPartBrush );
    }

    /* selected border */

    if ( partBorderSelected && !borderDragged ) {
        painter.setPen( selectedBorderColor );

        int lineLeft = selectedBorderLeft();
        painter.drawLine( lineLeft, 1, lineLeft, demoRect.height() );
    }

    /* cursor */

    if ( parentWidget()->isEnabled() && drawCursor ) {
        painter.setPen( cursorColor );
        painter.drawLine( cursorLeft, 1, cursorLeft, demoRect.height() );
    }

}

int DtDemoPartsWidget::selectedBorderLeft() {
    if ( selectedBorderIndex == -1 ) {
        return -1;
    }

    return ( selectedBorderIndex & 1 )
           ? parts.at( ( selectedBorderIndex - 1 ) / 2 ).endPx
           : parts.at( selectedBorderIndex / 2 ).startPx;
}

void DtDemoPartsWidget::updateCursorCoords( bool part = true, bool cursor = true ) {
    int partStart = -1;
    int curLeft = -1;

    if ( part ) {
        partStart = static_cast< int >( partStartLeft * pixMs );
        alignTime( partStart, partStartLeft );
    }

    if ( cursor ) {
        curLeft = static_cast< int >( cursorLeft * pixMs );
        alignTime( curLeft, cursorLeft );
    }

    emit cursorMoved( partStart, curLeft );
}

void DtDemoPartsWidget::mouseMoveEvent( QMouseEvent* e ) {
    drawCursor = ( partSelect || demoRect.contains( e->pos(), true ) );
    cursorLeft = e->pos().x();

    if ( partSelect ) {
        updateCursorCoords();
    }
    else {
        updateCursorCoords( false, true );
    }

    int partsCount = parts.count();
    bool overPartBorder = false;

    for ( int i = 0; i < partsCount; ++i ) {
        if ( cursorNearBorder( e->pos().x(), parts.at( i ).startPx ) ||
             cursorNearBorder( e->pos().x(), parts.at( i ).endPx ) )
        {
            overPartBorder = true;
            break;
        }
    }

    setCursor( overPartBorder ? Qt::SplitHCursor : Qt::ArrowCursor );
    update();
}

bool DtDemoPartsWidget::cursorNearBorder( int posX, int borderX ) {
    int borderLeft = ( borderX > 0 ) ? borderX - 1 : 0;
    int borderRight = ( borderX < width() ) ? borderX + 1 : width();

    if ( posX == borderLeft ) {
        cursorLeftShift = -1;
        return true;
    }
    else if ( posX == borderRight ) {
        cursorLeftShift = 1;
        return true;
    }
    else if ( posX > borderLeft && posX < borderRight ) {
        cursorLeftShift = 0;
        return true;
    }

    return false;
}

int DtDemoPartsWidget::selectedPartIndex() {
    if ( selectedBorderIndex == -1 ) {
        return -1;
    }

    return ( selectedBorderIndex & 1 )
            ? ( selectedBorderIndex - 1 ) / 2
            : selectedBorderIndex / 2;
}

DtDemoPart DtDemoPartsWidget::selectedPart() {
    return parts.at( selectedPartIndex() );
}

QVector< int > DtDemoPartsWidget::partBorders() {
    QVector< int > borders;
    int partsCount = parts.count();

    for ( int i = 0; i < partsCount; ++i ) {
        borders << parts.at( i ).startPx << parts.at( i ).endPx;
    }

    return borders;
}

bool DtDemoPartsWidget::mousePressedNearBorder( int cursorPos ) {
    QVector< int > borders = partBorders();
    int bordersCount = borders.count();

    for ( int i = 0; i < bordersCount; ++i ) {
        if ( cursorNearBorder( cursorPos, borders.at( i ) ) ) {
            selectedBorderIndex = i;
            return true;
        }
    }

   selectedBorderIndex = -1;
   return false;
}

void DtDemoPartsWidget::resizeSelectedPart() {
    partBorderSelected = true;
    borderDragged = true;
    partSelect = true;
    emit partSelected( selectedBorderIndex );

    const DtDemoPart& part = parts.at( selectedPartIndex() );
    partStartLeft = ( part.startPx == selectedBorderLeft() ) ? part.endPx : part.startPx;
    parts.remove( selectedPartIndex(), 1 );
}

void DtDemoPartsWidget::mousePressEvent( QMouseEvent* e ) {
    if ( mousePressedNearBorder( e->pos().x() ) ) {
        resizeSelectedPart();
        return;
    }

    cursorLeftShift = 0;
    selectedBorderIndex = -1;
    partBorderSelected = false;
    partSelect = true;

    emit partUnselected();

    if ( e->pos().x() <= warmupPos ) {
        partStartLeft = warmupPos + 1;
    }
    else {
        partStartLeft = cursorLeft;
    }
}

void DtDemoPartsWidget::moveSelectedBorder( int valueMs ) {
    DtDemoPart part = selectedPart();
    bool partStart = !( selectedBorderIndex & 1 );

    borderDragged = true;

    if ( partStart ) {
        if ( valueMs < part.end ) {
            part.start = valueMs;
        }
        else if ( valueMs == part.end ) {
            int newEnd = valueMs + frameTime;

            if ( newEnd <= demoLength ) {
                part.start = part.end;
                part.end = newEnd;
            }
        }
        else {
            part.start = part.end;
            part.end = valueMs;
        }
    }
    else {
        if ( valueMs > part.start ) {
            part.end = valueMs;
        }
        else if ( valueMs == part.start ) {
            int newStart = valueMs - frameTime;

            if ( newStart >= 0 ) {
                part.end = part.start;
                part.start = newStart;
            }
        }
        else {
            part.end = part.start;
            part.start = valueMs;
        }
    }

    if ( part.start <= warmupLength ) {
        if ( warmupLength + frameTime <= demoLength ) {
            part.start = warmupLength + frameTime;
        }
        else {
            part = selectedPart();
        }
    }

    parts.remove( selectedPartIndex(), 1 );
    addPart( part.start, part.end );

    part.updatePixelSizes();
    releaseBorder( partStart ? part.startPx : part.endPx );
    update();
}

void DtDemoPartsWidget::mouseReleaseEvent( QMouseEvent* e ) {
    if ( demoRect.contains( e->pos() ) ) {
        updateCursorCoords( false, true );
    }
    else {
        updateCursorCoords( false, false );
    }

    if ( partBorderSelected && !borderDragged ) {
        return;
    }

    partSelect = false;
    int buttonReleasePos = e->pos().x() - cursorLeftShift;

    if ( partStartLeft != buttonReleasePos ) {
        int start;
        int end;

        if ( partStartLeft < buttonReleasePos ) {
            start = partStartLeft;
            end = buttonReleasePos;
        }
        else {
            start = buttonReleasePos;
            end = partStartLeft;
        }

        addPartPx( start, end );
        emit changed();

        if ( borderDragged ) {
            releaseBorder( buttonReleasePos );
        }
    }

    update();
}

void DtDemoPartsWidget::releaseBorder( int pos ) {
    QVector< int > borders = partBorders();
    int bordersCount = borders.count();
    selectedBorderIndex = -1;
    partBorderSelected = false;

    for ( int i = 0; i < bordersCount; ++i ) {
        if ( borders.at( i ) == pos ) {
            selectedBorderIndex = i;
            partBorderSelected = true;
            break;
        }
    }

    if ( selectedBorderIndex != -1 ) {
        emit partSelected( selectedBorderIndex );
    }
    else {
        emit partUnselected();
    }

    borderDragged = false;
}

const QVector< DtDemoPart >& DtDemoPartsWidget::getParts() const {
     return parts;
}

void DtDemoPartsWidget::setParts( const QVector< DtDemoPart >& newParts ) {
    parts = newParts;
    update();
}

void DtDemoPartsWidget::clearParts() {
    drawCursor = false;
    partSelect = false;
    partBorderSelected = false;
    borderDragged = false;
    selectedBorderIndex = -1;

    parts.clear();
    update();
    emit changed();
}

void DtDemoPartsWidget::alignTime( int& alignedTime, int pixTime ) {
    int time = alignedTime % frameTime;

    if ( time ) {
        if ( ( ( alignedTime - time ) / pixMs ) == pixTime ) {
            alignedTime -= time;
        }
        else {
            alignedTime += frameTime - time;
        }
    }
}

void DtDemoPartsWidget::addPartPx( int startPx, int endPx ) {
    DtDemoPart part( pixMs );
    part.start = startPx * pixMs;
    alignTime( part.start, startPx );
    part.end = endPx * pixMs;
    alignTime( part.end, endPx );

    if ( part.start <= warmupLength ) {
        part.start = warmupLength + frameTime;
    }

    if ( part.end > demoLength ) {
        part.end = demoLength;
    }

    createPart( part );
}

void DtDemoPartsWidget::addPart( int start, int end ) {
    DtDemoPart part( pixMs );
    part.start = start;
    part.end = end;

    createPart( part );
}

void DtDemoPartsWidget::createPart( DtDemoPart& part ) {
    int pausesCount = pauses.count();

    for ( int i = 0; i < pausesCount; ++i ) {
        if ( part.start >= pauses.at( i ).start && part.start <= pauses.at( i ).end ) {
            part.start = pauses.at( i ).start + frameTime;
        }

        if ( part.end >= pauses.at( i ).start && part.end <= pauses.at( i ).end ) {
            part.end = pauses.at( i ).end;
        }
    }

    part.updatePixelSizes();

    if ( parts.isEmpty() ) {
        parts.append( part );
        return;
    }

    QVector< DtDemoPart > newParts;
    int partsCount = parts.size();
    bool added = false;

    for ( int i = 0; i < partsCount; ++i ) {
        if ( added ) {
            newParts.append( parts.at( i ) );
            continue;
        }

        if ( parts.at( i ).contains( part.start ) ) {
            int skipBefore = -1;

            for ( int j = i; j < partsCount; ++j ) {
                if ( parts.at( j ).contains( part.end ) ) {
                    newParts.append( DtDemoPart( parts.at( i ).start, parts.at( j ).end, pixMs ) );
                    i = j;
                    added = true;
                    break;
                }
                else if ( parts.at( j ).end < part.end ) {
                    skipBefore = j;
                }
            }

            if ( !added ) {
                newParts.append( DtDemoPart( parts.at( i ).start, part.end, pixMs ) );

                if ( skipBefore != -1 ) {
                    i = skipBefore;
                }

                added = true;
            }
        }
        else if ( parts.at( i ).contains( part.end ) ) {
            newParts.append( DtDemoPart( part.start, parts.at( i ).end, pixMs ) );
            added = true;
        }
        else {
            if ( part.end < parts.at( i ).start ) {
                newParts.append( part );
                newParts.append( parts.at( i ) );
                added = true;
            }
            else if ( part.start > parts.at( i ).start || part.end < parts.at( i ).end ) {
                newParts.append( parts.at( i ) );
            }

            if ( i == partsCount - 1 && !added ) {
                newParts.append( part );
            }
        }
    }

    parts = newParts;
}

void DtDemoPartsWidget::leaveEvent( QEvent* ) {
    drawCursor = false;
    updateCursorCoords( false, false );
    update();
}

void DtDemoPartsWidget::enterEvent( QEvent* ) {
    drawCursor = true;
    update();
}

DtDemoPart::DtDemoPart( int startTime, int endTime, float px ) :
        start( startTime ),
        end( endTime ),
        pixMs( px )
{
    updatePixelSizes();
}

DtDemoPart::DtDemoPart( float px ) :
        start( 0 ),
        end( 0 ),
        startPx( 0 ),
        endPx( 0 ),
        pixMs( px )
{ }

DtDemoPart::DtDemoPart() :
        start( 0 ),
        end( 0 ),
        startPx( 0 ),
        endPx( 0 ),
        pixMs( 1 )
{ }

bool DtDemoPart::contains( int time ) const {
    return ( time >= start && time <= end );
}

void DtDemoPart::updatePixelSizes() {
    startPx = start / pixMs;
    endPx = end / pixMs;
}
