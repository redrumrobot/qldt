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

#ifndef DTCONFIGEDITORDATA_H
#define DTCONFIGEDITORDATA_H

#include "Config.h"

#include <QIcon>
#include <QHash>

enum iconImages {
    I_QZ_SMALL,
    I_Q3_SMALL,
    I_COPY
};

enum DtDemoProto {
    Q_UNKNOWN = 0,
    Q3_68 = 68,
    QZ_73 = 73
};

class DtImages {
public:
    DtImages();
    const QIcon& getIcon( int index );
private:
    QPixmap* iconsSheet;
    QIcon rIcons[ 3 ];
};

class DtGameConfigEditor;

namespace dtdata {

extern DtConfig config;
extern DtGameConfigEditor* ceMainWindow;
extern QString defaultConfigFormat;
extern QStringList defaultConfigFilters;
extern QStringList defaultLanguageNames;
extern QHash< QString, DtDemoProto > demoProtos;
extern QStringList scriptWords;

void initializeData();
QString getStyle( const char* cName );
bool isConfigFile( const QString& format );
extern DtImages* icons;

}

#endif // DTCONFIGEDITORDATA_H
