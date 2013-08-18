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
#include "Archiver.h"
#include "FileDialog.h"
#include "zip.h"
#include "unzip.h"
#include "7z.h"

#include <QApplication>

#ifdef Q_OS_LINUX
#define _UNIX
#include "dll.hpp"
#elif defined Q_OS_WIN
#include <windows.h>
#include "unrar.h"
#endif

#include <QDateTime>
#include <QFileInfo>


DtArchiver* DtArchiver::instance;

DtArchiver::DtArchiver( QObject* parent ) : QObject( parent ),
    zFile( 0 ),
    uFile( 0 ),
    rarFile( 0 ),
    l7zFile( 0 )
{
    connect( this, SIGNAL( newFileCompressed( QString, quint64 ) ),
             parent, SLOT( fileCompressed( QString, quint64 ) ) );
    init();
}

QString DtArchiver::beginUnzip( const QString& path ) {
    uFile = unzOpen( path.toUtf8().data() );

    if ( uFile &&
         unzGoToFirstFile( uFile ) == UNZ_OK )
    {
        unzOpenCurrentFile( uFile );
        return currentUnzipFileName();
    }

    return QString();
}

quint64 DtArchiver::currentUnzipFileSize() {
    return lastUnzipFileSize;
}

quint64 DtArchiver::getUncompressedZipSize( const QString& path, bool otherFormats ) {
    uFile = unzOpen( path.toUtf8().data() );
    quint64 size = 0;

    if ( !uFile ) {
        return size;
    }

    unz_global_info info;

    int err = unzGetGlobalInfo( uFile, &info );

    if ( err == UNZ_OK ) {
        for ( quint32 i = 0; i < info.number_entry; ++i ) {
            unz_file_info fInfo;

            err = unzGetCurrentFileInfo( uFile, &fInfo, 0, 0, 0, 0, 0, 0 );
            if ( err != UNZ_OK ) {
                break;
            }

            QByteArray fileName( fInfo.size_filename, 0 );

            err = unzGetCurrentFileInfo( uFile, &fInfo, fileName.data(),
                                        fileName.size(), 0, 0, 0, 0 );
            if ( err != UNZ_OK ) {
                break;
            }

            QString suffix = QFileInfo( fileName ).suffix();

            if ( otherFormats || dtdata::demoProtos.contains( suffix ) ) {
                size += fInfo.uncompressed_size;
            }

            if ( i < info.number_entry - 1 ) {
                if ( unzGoToNextFile( uFile ) != UNZ_OK ) {
                    break;
                }
            }
        }
    }

    closeUnzip();

    return size;
}

QString DtArchiver::currentUnzipFileName() {
    unz_file_info info;
    lastUnzipFileSize = 0;

    int err = unzGetCurrentFileInfo( uFile, &info, 0, 0, 0, 0, 0, 0 );

    if ( err == UNZ_OK ) {
        QByteArray fileName( info.size_filename, 0 );

        err = unzGetCurrentFileInfo( uFile, &info, fileName.data(), fileName.size(), 0, 0, 0, 0 );
        if ( err == UNZ_OK ) {
            lastUnzipFileSize = info.uncompressed_size;
            return QString( fileName );
        }
    }

    return "";
}

QString DtArchiver::unzipNext( const QString& path ) {
    if ( !uFile ) {
        return beginUnzip( path );
    }

    if ( unzCloseCurrentFile( uFile ) != UNZ_OK ||
         unzGoToNextFile( uFile ) != UNZ_OK )
    {
        return "";
    }

    unzOpenCurrentFile( uFile );

    return currentUnzipFileName();
}

bool DtArchiver::newZip( const QString& path, bool add ) {
    compressLevel = dtdata::config.zlibCompressionLevel;
    zFile = zipOpen( path.toLocal8Bit().data(), add ? APPEND_STATUS_ADDINZIP : APPEND_STATUS_CREATE );

    return zFile;
}

bool DtArchiver::addFile( const QString& fileName ) {
    if ( !zFile ) {
        return false;
    }

    zip_fileinfo info;

    QDate date = QDate::currentDate();
    QTime time = QTime::currentTime();

    info.tmz_date.tm_sec = time.second();
    info.tmz_date.tm_min = time.minute();
    info.tmz_date.tm_hour = time.hour();
    info.tmz_date.tm_mday = date.day();
    info.tmz_date.tm_mon = date.month();
    info.tmz_date.tm_year = date.year();

    info.dosDate = 0;
    info.internal_fa = 0;
    info.external_fa = 0;

    QString fn = fileName;

    return zipOpenNewFileInZip( zFile, fn.toLocal8Bit().constData(), &info, 0, 0, 0, 0, 0,
                                compressLevel != 0 ? Z_DEFLATED : 0, compressLevel ) == ZIP_OK;
}

bool DtArchiver::closeFileInZip() {
    return zFile ? zipCloseFileInZip( zFile ) == ZIP_OK : unzCloseCurrentFile( uFile ) == UNZ_OK;
}

qint64 DtArchiver::write( const char* buf, qint64 len ) {
    return zipWriteInFileInZip( zFile, buf, static_cast< uint >( len ) ) == ZIP_OK ? len : -1;
}

int DtArchiver::readZip( char* buf, uint len ) {
    return unzReadCurrentFile( uFile, buf, static_cast< uint >( len ) );
}

void DtArchiver::closeZip() {
    if ( zFile ) {
        zipClose( zFile, 0 );
        zFile = 0;
    }
}

void DtArchiver::closeUnzip() {
    if ( uFile ) {
        unzClose( uFile );
        uFile = 0;
    }
}

DtArchiver::~DtArchiver() {
    closeZip();
    closeUnzip();
    closeRar();
}

QString DtArchiver::unrarNext( const QString& path ) {
    lastRarSize = 0;

    if ( !rarFile ) {
        return beginUnrar( path );
    }

    RARHeaderDataEx headerData;
    headerData.CmtBuf = 0;

    if ( RARReadHeaderEx( rarFile, &headerData ) == 0 ) {
        lastRarSize = headerData.UnpSize;
        return headerData.FileName;
    }

    return "";
}

void DtArchiver::closeRar() {
    if ( rarFile ) {
        RARCloseArchive( rarFile );
        rarFile = 0;
    }
}

bool DtArchiver::openRar( const QString& path, quint32 openMode ) {
    RAROpenArchiveDataEx archiveData;

    memset( &archiveData, 0, sizeof( archiveData ) );
    archiveData.CmtBuf = 0;
    archiveData.OpenMode = openMode;
    QByteArray filePath = path.toLocal8Bit();
    archiveData.ArcName = filePath.data();

    rarFile = RAROpenArchiveEx( &archiveData );

    if ( archiveData.OpenResult ) {
        return false;
    }

    return true;
}

QString DtArchiver::beginUnrar( const QString& path ) {
    RARHeaderDataEx headerData;
    headerData.CmtBuf = 0;

    if ( openRar( path, RAR_OM_EXTRACT ) &&
         RARReadHeaderEx( rarFile, &headerData ) == 0 )
    {
        lastRarSize = headerData.UnpSize;
        return headerData.FileName;
    }

    closeRar();
    return "";
}

quint64 DtArchiver::getUncompressedRarSize( const QString& path ) {
    quint64 size = 0;

    RARHeaderDataEx headerData;
    headerData.CmtBuf = 0;

    if ( openRar( path, RAR_OM_LIST ) ) {
        forever {
            if( RARReadHeaderEx( rarFile, &headerData ) != 0 ) {
                break;
            }

            QString suffix = QFileInfo( headerData.FileName ).suffix();

            if ( dtdata::demoProtos.contains( suffix ) ) {
                size += headerData.UnpSize;
            }

            if ( rarSkipFile() != 0 ) {
                break;
            }
        }
    }

    closeRar();
    return size;
}

int DtArchiver::rarSkipFile() {
    return RARProcessFile( rarFile, RAR_SKIP, 0, 0 );
}

int DtArchiver::unrarFileTo( const QString& newFilePath ) {
    return RARProcessFile( rarFile, RAR_EXTRACT, 0, newFilePath.toLocal8Bit().data() );
}

quint64 DtArchiver::lastRarFileSize() {
    return lastRarSize;
}

static void p7zipCompressCallback( QString name, quint64 size ) {
    DtArchiver::instance->fileCompressed( name, size );
}

static bool p7zipExtractArchiveCallback( QString path, QString name ) {
    return DtArchiver::instance->extract7zSubarchive( path, name );
}

bool DtArchiver::extract7zSubarchive( const QString& path, const QString& name ) {
    DtFileDialog* files = qobject_cast< DtFileDialog* >( parent() );

    if ( !files ) {
        return false;
    }

    QFileInfo info( name );
    QString extension = info.suffix();

    return files->tryUnpackSubarchive( extension, path, name );
}

void DtArchiver::fileCompressed( QString name, quint64 size ) {
    emit newFileCompressed( name, size );
    qApp->processEvents();
}

void DtArchiver::init() {
    DtArchiver::instance = this;
    mini7zip::p7zipInit( p7zipCompressCallback, p7zipExtractArchiveCallback );
}

quint64 DtArchiver::getUncompressed7zSize( const QString& path, bool otherFormats ) {
    std::wstring wstr = path.toStdWString();
    return mini7zip::getUncompressed7zSize( wstr.c_str(), otherFormats );
}

bool DtArchiver::extract7z( const QString& path, const QString& dest, bool removePaths ) {
    if ( !l7zFile && !beginUn7z( path ) ) {
        return false;
    }

    bool ret = mini7zip::unpack( dest, removePaths );
    close7z();

    return ret;
}

bool DtArchiver::open7z( const QString& path, bool update ) {
    std::wstring wstr = path.toStdWString();

    if ( mini7zip::openArchive( wstr.c_str(), update ) ) {
        l7zFile = 1;
        return true;
    }

    return false;
}

bool DtArchiver::beginUn7z( const QString& path ) {
    if ( !open7z( path ) ) {
        close7z();
        return false;
    }

    return true;
}

bool DtArchiver::beginUpdate7z( const QString& path ) {
    if ( !open7z( path, true ) ) {
        close7z();
        return false;
    }

    return true;
}

quint64 DtArchiver::last7zFileSize() {
    return last7zipFileSize;
}

void DtArchiver::close7z() {
    if ( l7zFile ) {
        mini7zip::closeArchive();
        l7zFile = 0;
    }
}

bool DtArchiver::addFilesTo7z( const QString& archivePath, const QMap< QString, QString > demoList,
                               bool move ) {
    if ( !l7zFile ) {
        if ( !beginUpdate7z( archivePath ) ) {
            return false;
        }
    }

    bool ret = mini7zip::packFiles( demoList, move );
    close7z();

    return ret;
}
