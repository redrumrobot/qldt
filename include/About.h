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

#ifndef DTABOUT_H
#define DTABOUT_H

#include <QDialog>

class DtAbout: public QDialog {
	Q_OBJECT
public:
	DtAbout( QWidget* parent = 0 );
	~DtAbout();

private:
	QPixmap* bg;
	QString siteUrl;
	QFont pFont;
	QColor pFontColorLightGray;
	QColor pFontColorLink;

	bool overLink;
	bool linkPressed;
	QRect linkRect;

protected:
	void paintEvent( QPaintEvent* );
	void mouseMoveEvent( QMouseEvent* e );
	void leaveEvent( QEvent* );
	void mousePressEvent( QMouseEvent* e );
	void mouseReleaseEvent( QMouseEvent* e );
};

#endif // DTABOUT_H
