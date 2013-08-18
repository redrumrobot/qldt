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

#ifndef DTDEMOPARTSWIDGET_H
#define DTDEMOPARTSWIDGET_H

#include "DemoData.h"

#include <QWidget>
#include <QVector>
#include <QRect>
#include <QColor>
#include <QBrush>

class DtDemo;
class QRect;

struct DtDemoPart {
    int start;
    int end;
    int startPx;
    int endPx;
    float pixMs;

    DtDemoPart( int startTime, int endTime, float px );
    DtDemoPart( float px );
    DtDemoPart();
    bool contains( int time ) const;
    void updatePixelSizes();
};

class DtDemoPartsWidget : public QWidget {
    Q_OBJECT
public:
    DtDemoPartsWidget( QWidget* parent );

    void setDemo( DtDemo* demo );
    const QVector< DtDemoPart >& getParts() const;
    void setParts( const QVector< DtDemoPart >& newParts );
    QVector< int > partBorders();
    int selectedPartIndex();

public slots:
    void clearParts();
    void moveSelectedBorder( int vaueMs );

private:
    bool drawCursor;
    bool partSelect;
    bool partBorderSelected;
    bool borderDragged;
    int selectedBorderIndex;
    int cursorLeft;
    int demoLength;
    int frameTime;
    int warmupLength;
    int warmupPos;
    int partStartLeft;
    float pixMs;
    int cursorLeftShift;

    QRect demoRect;
    QVector< DtDemoPart > parts;
    QVector< cutSegment > pauses;
    QVector< int > lags;
    QColor topFrameColor;
    QColor bottomFrameColor;
    QColor cursorColor;
    QColor warmupColor;
    QColor pauseColor;
    QColor lagColor;
    QColor newSelectionColor;
    QColor selectedColor;
    QColor selectedBorderColor;
    QBrush selectedPartBrush;
    QBrush newSelectedPartBrush;
    QBrush warmupBrush;
    QBrush pauseBrush;
    QBrush frameBrush;

    void addPartPx( int startPx, int endPx );
    void addPart( int start, int end );
    void createPart( DtDemoPart& part );
    void paintEvent( QPaintEvent* e );
    void mouseMoveEvent( QMouseEvent* e );
    void leaveEvent( QEvent* e );
    void enterEvent( QEvent* e );
    void mousePressEvent( QMouseEvent* e );
    void mouseReleaseEvent( QMouseEvent* e );
    bool partContains( int time );
    bool cursorNearBorder( int posX, int borderX );
    int selectedBorderLeft();
    bool mousePressedNearBorder( int cursorPos );
    void resizeSelectedPart();
    void updateCursorCoords( bool part, bool cursor );
    void alignTime( int& alignedTime, int pixTime );
    DtDemoPart selectedPart();
    void releaseBorder( int pos );

signals:
    void cursorMoved( int, int );
    void partBorderClicked( int );
    void changed();
    void partSelected( int );
    void partUnselected();
};

#endif // DTDEMOPARTSWIDGET_H
