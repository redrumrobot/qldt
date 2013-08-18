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

#ifndef DTARCHIVER_H
#define DTARCHIVER_H

#include <QObject>
#include <QString>
#include <QMap>

class DtArchiver : public QObject {
    Q_OBJECT
public:
    DtArchiver( QObject* parent = 0 );
    ~DtArchiver();

    static DtArchiver* instance;

    QString unzipNext( const QString& path = QString() );
    quint64 currentUnzipFileSize();
    quint64 getUncompressedZipSize( const QString& path, bool otherFormats = false );
    bool newZip( const QString& path, bool add );
    bool addFile( const QString& fileName );
    bool closeFileInZip();
    void closeZip();
    void closeUnzip();
    qint64 write( const char* buf, qint64 len );
    int readZip( char* buf, uint len );

    QString unrarNext( const QString& path = QString() );
    quint64 getUncompressedRarSize( const QString& path );
    void closeRar();
    int rarSkipFile();
    int unrarFileTo( const QString& newFilePath );
    quint64 lastRarFileSize();

    quint64 getUncompressed7zSize( const QString& path, bool otherFormats = false );
    bool beginUn7z( const QString& path );
    bool beginUpdate7z( const QString& path );
    bool open7z( const QString& path, bool update = false );
    bool extract7z( const QString& path, const QString& dest, bool removePaths );
    bool extract7zSubarchive( const QString& path, const QString& name );
    void close7z();
    quint64 last7zFileSize();

    bool addFilesTo7z( const QString& archivePath, const QMap< QString, QString > demoList,
                       bool move );
    void fileCompressed( QString name, quint64 size );

protected:
    int compressLevel;

    void* zFile;
    void* uFile;
    void* rarFile;
    int l7zFile;
    quint64 lastRarSize;
    quint64 lastUnzipFileSize;
    quint64 last7zipFileSize;

    QString beginUnzip( const QString& path );
    QString currentUnzipFileName();

    QString beginUnrar( const QString& path );
    bool openRar( const QString& path, quint32 openMode );

    QString next7zFile();

    void init();

signals:
    void newFileCompressed( QString, quint64 );
};

#endif // DTARCHIVER_H
