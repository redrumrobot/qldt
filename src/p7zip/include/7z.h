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

#ifndef MINI7Z_H
#define MINI7Z_H


namespace mini7zip {

typedef void compressCallback_t( QString name, quint64 size );
typedef bool extractSubarchiveCallback_t( QString path, QString name );

void p7zipInit( compressCallback_t* cCallback, extractSubarchiveCallback_t* eCallback );
unsigned long getUncompressed7zSize( const wchar_t* path, bool otherFormats );
bool openArchive( const wchar_t* path, bool update );
void closeArchive();
bool unpack( QString path, bool removePaths );
bool packFiles( const QMap< QString, QString > demoList, bool move );

}


#endif // MINI7Z_H
