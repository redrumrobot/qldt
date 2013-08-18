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

#include "Archiver.h"
#include "FileDialog.h"
#include "Demo.h"
#include "PlayerData.h"
#include "MainWindow.h"

#include <QDir>
#include <QUrl>
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>
#include <QTime>
#include <QtConcurrentRun>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QHttp>
#include <QNetworkAccessManager>
#include <QNetworkRequest>

#ifdef Q_OS_LINUX
#include <sys/vfs.h>
#elif defined Q_OS_WIN
#include <windows.h>
#endif

using namespace dtdata;

DtFileDialog::DtFileDialog( QWidget* parent ) : QDialog( parent ) {
    setFixedSize( 560, 200 );
    setModal( true );
    setWindowFlags( Qt::Dialog | Qt::WindowMaximizeButtonHint );

    cancelPressed = false;
    running = false;

    QVBoxLayout* mLayout = new QVBoxLayout;

    fileLabel = new QLabel( this );
    fileLabel->setObjectName( "progressFileInfo" );
    fileLabel->setFixedWidth( width() );
    fileProgress = new QProgressBar( this );

    totalLabel = new QLabel( this );
    totalLabel->setObjectName( "progressTotalInfo" );
    totalProgress = new QProgressBar( this );

    cancelButton = new QPushButton( tr( "Cancel" ), this );
    cancelButton->setFixedWidth( 120 );
    connect( cancelButton, SIGNAL( clicked() ), this, SLOT( cancel() ) );

    mLayout->addSpacing( 5 );
    mLayout->addWidget( fileLabel );
    mLayout->addWidget( fileProgress );
    mLayout->addSpacing( 10 );
    mLayout->addWidget( totalLabel );
    mLayout->addWidget( totalProgress );
    mLayout->addStretch( 1 );

    QHBoxLayout* cLayout = new QHBoxLayout;
    cLayout->addWidget( cancelButton, Qt::AlignCenter );

    mLayout->addLayout( cLayout );

    setLayout( mLayout );

    connect( this, SIGNAL( newData( int, int, const QString&, const QString& ) ),
             this, SLOT( updateState( int, int, const QString&, const QString& ) ) );
    connect( this, SIGNAL( newHttpData( int, int, float, const QString&, const QString& ) ),
             this, SLOT( updateState( int, int, float, const QString&, const QString& ) ) );

    tmpName = "/tmp" + QString::number( qrand(), 16 );

    arch = new DtArchiver( this );
    file = new QFile( this );
    outFile = new QFile( this );
    networkManager = new QNetworkAccessManager( this );

    titleTemplate = "%1%";
}

void DtFileDialog::cancel() {
    cancelPressed = true;
    close();
}

void DtFileDialog::downloadFinished() {
    waitHttpData = false;
    dtMainWindow->setWindowTitle( oldMainWindowTitle );
}

void DtFileDialog::updateState( int pFile, int pTotal, const QString& fText,
                                const QString& tText ) {
    if ( fText.isEmpty() ) {
        fileLabel->setVisible( false );
        fileProgress->setVisible( false );
        setFixedSize( 500, 130 );
    }
    else {
        if ( !fileLabel->isVisible() ) {
            fileLabel->setVisible( true );
            setFixedSize( 500, 200 );
        }

        if ( !fileProgress->isVisible() ) {
            fileProgress->setVisible( true );
        }

        fileLabel->setText( fText );
        fileProgress->setValue( pFile );
    }

    totalLabel->setText( tText );
    totalProgress->setValue( pTotal );

    setWindowTitle( titleTemplate.arg( pTotal ) );
}

void DtFileDialog::updateState( int pFile, int pTotal, float pSpeed, const QString& fText,
                                const QString& tText )
{
    updateState( pFile, pTotal, fText, tText );
    QString speedUnits = tr( "KiB" );

    if ( pSpeed > 1024 ) {
        pSpeed /= 1024.f;
        speedUnits = tr( "MiB" );
    }

    setWindowTitle( titleTemplate.arg( pTotal ).arg( pSpeed, 0, 'f', 1 ).arg( speedUnits ) );
}

void DtFileDialog::updateDownloadProgress( qint64 read, qint64 total ) {
    float speed = read * 1000.f / ( downloadDuration * 1024.f );

    if ( total && speedChanged ) {
        QString downloadedSize;

        if ( downloadedFileSize != 0 ) {
            QString sizeUnits = tr( "B" );
            float fSize = downloadedFileSize;

            if ( fSize > 1024 ) {
                fSize /= 1024.f;
                sizeUnits = tr( "KiB" );
            }

            if ( fSize > 1024 ) {
                fSize /= 1024.f;
                sizeUnits = tr( "MiB" );
            }

            downloadedSize = QString( "%1 %2" ).arg( fSize, 0, 'f', 1 ).arg( sizeUnits );
        }

        speedChanged = false;
        int percent = read * 100 / total;

        emit newHttpData( 0, percent, speed, "", downloadedFileName + "\n" + downloadedSize );

        dtMainWindow->setWindowTitle( QString( "%1% - %2" ).arg( percent ).arg( downloadedFileName ) );
    }
}

bool DtFileDialog::checkDestDir( const QString& dest ) {
    if ( dest == currentWorkingDir ) {
        QString msg = tr( "These files already in this directory." );
        QMessageBox::information( this, tr( "Wrong directory" ), msg );

        return false;
    }

    return true;
}

quint64 DtFileDialog::getFreeSpace( const QString& dir ) {
#ifdef Q_OS_LINUX
    struct statfs fs;
    statfs( dir.toUtf8().data(), &fs );

    return static_cast< quint64 >( fs.f_bavail * static_cast< quint64 >( fs.f_bsize ) );
#elif defined Q_OS_WIN
    ULARGE_INTEGER availableBytes;
    wchar_t dirPath[ 3 ];

    availableBytes.QuadPart = 0L;
    dirPath[ 0 ] = dir.at( 0 ).toAscii();
    dirPath[ 1 ] = ':';
    dirPath[ 2 ] = '\0';

    if ( !GetDiskFreeSpaceEx( reinterpret_cast< LPCWSTR >( &dirPath ), &availableBytes, 0, 0 ) ) {
        QMessageBox::critical( this, "Error", tr( "Unable to get free space on disk %1" )
                                              .arg( dir.left( 2 ) ) );
        return 0;
    }

    return static_cast< quint64 >( availableBytes.QuadPart );
#endif
}

quint64 DtFileDialog::getDemosSize( const DtDemoVec& dm ) {
    quint64 size = 0;

    for ( int i = 0; i < dm.size(); ++i ) {
        size += dm.at( i )->fileInfo().size;
    }

    return size;
}

quint64 DtFileDialog::getDemosSize( const QStringList& dm ) {
    quint64 size = 0;

    foreach ( const QString& fileName, dm ) {
        size += QFileInfo( fileName ).size();
    }

    return size;
}

quint64 DtFileDialog::getDemosSize( const DtCpDemoVec& dm ) {
    quint64 size = 0;

    for ( int i = 0; i < dm.size(); ++i ) {
        size += dm.at( i ).demo->fileInfo().size;
    }

    return size;
}

bool DtFileDialog::checkDir( const DtCpDemoVec& cDemos, const QString& dest ) {
    QStringList cpDemos;

    for ( int i = 0; i < cDemos.size(); ++i ) {
        cpDemos << cDemos.at( i ).demo->fileInfo().fileName();
    }

    return checkDir( cpDemos, dest );
}

bool DtFileDialog::checkDir( const DtDemoVec& cDemos, const QString& dest ) {
    QStringList cpDemos;

    for ( int i = 0; i < cDemos.size(); ++i ) {
        cpDemos << cDemos.at( i )->fileInfo().fileName();
    }

    return checkDir( cpDemos, dest );
}

bool DtFileDialog::checkDir( const QStringList& cDemos, const QString& dest ) {
    if ( !cDemos.size() ) {
        return false;
    }

    QDir dir( dest );

    if ( !dir.exists() ) {
        QMessageBox::critical( this, tr( "Error" ),
                               tr( "%1 doesn't exists" ).arg( dir.absolutePath() ) );
        return false;
    }

    totalSize = getDemosSize( cDemos );
    quint64 freeSpace = getFreeSpace( dest );

    if ( totalSize >= freeSpace ) {
        float fSz = freeSpace >= GiB ? GiB : MiB;
        float nSz = totalSize >= GiB ? GiB : MiB;

        QString free = QString::number( freeSpace / fSz, 'f', 1 ) +
                         QString( " %1" ).arg( fSz == MiB ? "MB" : "GB" );
        QString needed = QString::number( totalSize / nSz, 'f', 1 ) +
                         QString( " %1" ).arg( nSz == MiB ? "MB" : "GB" );

        QString msg = tr( "Insufficient free disk space:\n\nFree %1\nNeeded: %2" );

        QMessageBox::critical( this, tr( "Error" ), msg.arg( free, needed ) );
        return false;
    }

    return true;
}

void DtFileDialog::notAffectedMsg( QStringList& na, const QString& nMsg ) {
    if ( na.size() ) {
        QString msg = nMsg + ":\n\n";

        for ( int i = 0; i < na.size(); ++i ) {
            msg += na.at( i ) + "\n";

            if ( i == 10 ) {
                if ( na.size() > 10 ) {
                    msg += "...\n(" + QString::number( na.size() ) + " " + tr( "files" ) + ")";
                }

                break;
            }
        }

        QMessageBox::warning( this, tr( "Error" ), msg );
    }
}

void DtFileDialog::demoReadPos( int pos ) {
    emit newData( pos, totalBytesCopied * 100 / totalSize, curDemoName, curPath );
}

void DtFileDialog::packSegments( DtCpDemoVec& cpDemos ) {
    if ( !cpDemos.size() ) {
        return;
    }

    QString path;
    bool add = false;

    DtDemo* demo = cpDemos.at( 0 ).demo;

    QString defName = cpDemos.size() > 1 ? QDir( demo->fileInfo().filePath ).dirName() :
                      demo->fileInfo().baseName;

    if ( !getArchiveName( path, defName, add ) ) {
        return;
    }

    copySegments( cpDemos, path, true, add );
}

void DtFileDialog::copySegments( DtCpDemoVec& cpDemos, const QString& dest, bool zip, bool add ) {
    if ( !checkDir( cpDemos, !zip ? dest : QFileInfo( dest ).absolutePath() ) ) {
        return;
    }

    titleTemplate = !zip ? tr( "Copy" ) : tr( "Pack" );
    titleTemplate += " %1%";

    totalBytesCopied = 0;

    updateState( 0, 0, cpDemos.first().demo->fileInfo().fileName( false ), dest );
    show();
    repaint();
    QApplication::flush();

    if ( zip && config.lastArchiverFormat == F_ZIP && !arch->newZip( dest, add ) ) {
        return;
    }

    QMap< QString, QString > demoList;

    for ( int i = 0; i < cpDemos.size(); ++i ) {
        if ( cancelPressed ) {
            break;
        }

        DtDemo* demo = cpDemos.at( i ).demo;
        connect( demo, SIGNAL( readPosition( int ) ), this, SLOT( demoReadPos( int ) ) );

        curDemoName = demo->fileInfo().fileName( false );
        curPath = dest;

        QStringList copied = demo->copySegment(
                const_cast< DtWriteOptions* >( &cpDemos.at( i ).options ) );

        if ( zip ) {
            QStringList nc;

            foreach ( const QString& fName, copied ) {
                QString tmpName = QDir::tempPath() + "/" + fName;

                if ( config.lastArchiverFormat == F_ZIP ) {
                    copyFile( tmpName, fName, "", dest, true, nc );
                    QFile::remove( tmpName );
                }
                if ( config.lastArchiverFormat == F_7Z ) {
                    demoList.insert( tmpName, fName );
                }
            }
        }

        totalBytesCopied += demo->fileInfo().size;
    }

    if ( cancelPressed ) {
        cancelPressed = false;
    }

    disconnect( this, SLOT( demoReadPos( int ) ) );

    if ( config.lastArchiverFormat == F_7Z ) {
        curPath = dest;
        QtConcurrent::run( arch, &DtArchiver::addFilesTo7z, dest, demoList, true );
        return;
    }

    if ( zip && config.lastArchiverFormat == F_ZIP ) {
        arch->closeZip();
    }

    close();
}

bool DtFileDialog::copyFile( const QString& fullFileName, const QString& fName,
                             const QString& newName, const QString& dest, bool zip,
                             QStringList& na )
{
    quint64 fileBytesCopied = 0;
    quint64 addedToTotal = 0;
    bool destReady = false;

    if ( !zip ) {
        outFile->setFileName( dest + tmpName );
        destReady = outFile->open( QFile::WriteOnly );
    }
    else {
        destReady = arch->addFile( fName );
    }

    bool err = false;
    file->setFileName( fullFileName );

    if ( file->open( QFile::ReadOnly ) && destReady ) {
        char block[ 4096 ];
        qint64 readCount = 0;

        int updateDlg = 0;

        while ( !file->atEnd() ) {
            qint64 read = file->read( block, sizeof( block ) );

            if ( read <= 0 ) {
                break;
            }

            readCount += read;

            if ( !zip ) {
                if ( read != outFile->write( block, read ) ) {
                    err = true;
                    break;
                }
            }
            else if ( read != arch->write( block, read ) ) {
                err = true;
                break;
            }

            fileBytesCopied += read;
            totalBytesCopied += read;
            addedToTotal += read;

            const int blockNum = 700 * 1024 / sizeof( block );

            if ( ++updateDlg > blockNum ) {
                updateDlg = 0;

                emit newData( fileBytesCopied * 100 / file->size(),
                              totalBytesCopied * 100 / totalSize, fName, dest );
            }
        }

        if ( readCount != file->size() ) {
            err = true;
        }

        file->close();

        if ( !zip ) {
            outFile->close();
        }
        else {
            arch->closeFileInZip();
        }
    }
    else {
        err = true;
    }

    if ( !zip && !err ) {
        QString newFileName = newName;

        if ( copyState == CS_COPY ) {
            if ( QFile::exists( newFileName ) ) {
                QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::YesToAll
                                                       | QMessageBox::No | QMessageBox::Cancel;
                QString msg = tr( "File %1 already exists\nOverwrite it?" );
                QMessageBox msgBox( QMessageBox::Question, tr( "File exists" ),
                                    msg.arg( QFileInfo( newFileName ).fileName() ),
                                    buttons, this );
                QPushButton* renameButton = msgBox.addButton( tr( "Auto rename all" ),
                                                              QMessageBox::ActionRole );
                int button = msgBox.exec();

                if ( !button && msgBox.clickedButton() == renameButton ) {
                    copyState = CS_AUTORENAME;
                }
                else {
                    switch ( button ) {
                        case QMessageBox::Yes       : QFile::remove( newFileName ); break;
                        case QMessageBox::No        : break;
                        case QMessageBox::YesToAll  : copyState = CS_YESTOALL; break;
                        case QMessageBox::Cancel    : copyState = CS_CANCEL; break;
                        default: break;
                    }
                }
            }
        }

        if ( copyState == CS_YESTOALL ) {
            QFile::remove( newFileName );
        }
        else if ( copyState == CS_AUTORENAME && QFile::exists( newFileName ) ) {
            QFileInfo fileInfo( newFileName );

            int num = 0;
            QString fileNameBase = fileInfo.completeBaseName();
            QString filePath = fileInfo.absolutePath();
            QString fileSuffix = fileInfo.suffix();

            do {
                newFileName = QString( "%1/%2%3.%4" ).arg( filePath, fileNameBase )
                                                     .arg( ++num )
                                                     .arg( fileSuffix );
            }
            while ( QFile::exists( newFileName ) );
        }
        else if ( copyState == CS_CANCEL ) {
            cancelPressed = true;
            outFile->setPermissions( QFile::WriteOwner );
            outFile->remove();
            return 0;
        }

        if ( !outFile->rename( newFileName ) ) {
            err = true;
        }
    }

    if ( err ) {
        fileBytesCopied = file->size();
        totalBytesCopied += fileBytesCopied - addedToTotal;

        if ( !zip ) {
            outFile->setPermissions( QFile::WriteOwner );
            outFile->remove();
        }

        na << fName;
    }

    if ( file->size() > 0 && totalSize ) {
        emit newData( fileBytesCopied * 100 / file->size(),
                      totalBytesCopied * 100 / totalSize, fName, dest );
    }

    return !err;
}

void DtFileDialog::copy( const DtDemoVec& cpDemos, const QString& dest ) {
    QStringList cpDemoList;

    for ( int i = 0; i < cpDemos.size(); ++i ) {
        cpDemoList << cpDemos.at( i )->fileInfo().fileName();
    }

    copy( cpDemoList, dest );
}

void DtFileDialog::copy( const QStringList& cpDemos, const QString& dest ) {
    if ( !checkDir( cpDemos, dest ) ) {
        return;
    }

    copyState = CS_COPY;
    QStringList notCopied;

    titleTemplate = tr( "Copy" ) + " %1%";
    totalBytesCopied = 0;

    updateState( 0, 0, QFileInfo( cpDemos.at( 0 ) ).fileName(), dest );

    QTime copyTime;
    copyTime.start();

    for ( int i = 0; i < cpDemos.size(); ++i ) {
        if ( cancelPressed ) {
            break;
        }

        const QString& fName = QFileInfo( cpDemos.at( i ) ).fileName();
        const QString newName = dest + "/" + fName;

        copyFile( cpDemos.at( i ), fName, newName, dest, false, notCopied );

        if ( copyTime.elapsed() > 100 ) {
            show();
        }
    }

    if ( !cancelPressed ) {
        notAffectedMsg( notCopied, tr( "Some files haven't been copied" ) );
    }
    else {
        cancelPressed = false;
    }

    close();
}

void DtFileDialog::copyDroppedFiles( const QStringList& droppedFiles, const QString& destDir ) {
    QFileInfo fileInfo;
    QStringList demoFiles;
    QStringList archiveFiles;

    foreach ( const QString& fileName, droppedFiles ) {
        fileInfo.setFile( fileName );

        if ( demoProtos.contains( fileInfo.suffix() ) ) {
            demoFiles << fileName;
        }
        else {
            archiveFiles << fileName;
        }
    }

    copy( demoFiles, destDir );

    foreach ( const QString& archive, archiveFiles ) {
        unpack( archive, destDir );
    }
}

void DtFileDialog::readHttpHeader() {
    QNetworkReply* reply = qobject_cast< QNetworkReply* >( sender() );

    if ( waitHttpHeader &&reply && reply->hasRawHeader( "Content-Type" ) ) {
        waitHttpHeader = false;
    }
}

void DtFileDialog::onHttpError( QNetworkReply::NetworkError code ) {
    if ( code != QNetworkReply::OperationCanceledError ) {
        httpError = true;
    }
}

QNetworkReply* DtFileDialog::getHttp( const QUrl& url ) {
    QNetworkRequest request;
    request.setUrl( url );
    const QString& uAgent = config.qzCustomUserAgent ? config.qzUserAgent : firefoxUserAgent;
    request.setRawHeader( "User-Agent", uAgent.toUtf8().data() );

    waitHttpHeader = true;

    QNetworkReply* reply = networkManager->get( request );
    connect( reply, SIGNAL( metaDataChanged() ), this, SLOT( readHttpHeader() ) );
    connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
             this, SLOT( onHttpError( QNetworkReply::NetworkError ) ) );

    try {
        while( waitHttpHeader ) {
            if ( cancelPressed || httpError ) {
                throw DtHttpError();
            }

            QApplication::processEvents( QEventLoop::WaitForMoreEvents );
        }

        QUrl redirectUrl = reply->attribute( QNetworkRequest::RedirectionTargetAttribute ).toUrl();

        if ( !redirectUrl.isEmpty() && lastRedirectUrl == redirectUrl ) {
            throw DtHttpError();
        }

        if ( !redirectUrl.isEmpty() && redirectUrl.isValid() ) {
            if ( redirectUrl.scheme().isEmpty() || redirectUrl.host().isEmpty() ) {
                redirectUrl.setScheme( url.scheme() );
                redirectUrl.setHost( url.host() );
            }

            lastRedirectUrl = redirectUrl;
            reply->abort();
            reply->deleteLater();
            reply = getHttp( redirectUrl );

            if ( reply == 0 ) {
                return 0;
            }
        }
    }
    catch ( DtHttpError ) {
        if ( httpError ) {
            QMessageBox::warning( this, tr( "Error" ), reply->errorString() );
        }

        reply->abort();
        reply->deleteLater();

        return 0;
    }

    connect( reply, SIGNAL( finished() ), this, SLOT( downloadFinished() ) );
    connect( reply, SIGNAL( downloadProgress( qint64, qint64 ) ),
             this, SLOT( updateDownloadProgress( qint64, qint64 ) ) );

    oldMainWindowTitle = dtMainWindow->windowTitle();

    return reply;
}

bool DtFileDialog::copyDroppedUrls( const QList< QUrl >& droppedUrls, const QString& destDir ) {
    if ( droppedUrls.isEmpty() ) {
        return false;
    }

    titleTemplate = tr( "Download" ) + tr( " %1% (%2 %3/s)" );
    totalBytesCopied = 0;

    updateState( 0, 0, 0, "", "\n\n" );
    show();

    bool ret = false;
    QStringList archiveFiles;

    foreach ( const QUrl& url, droppedUrls ) {
        if ( cancelPressed ) {
            break;
        }

        lastRedirectUrl.clear();
        QString tmpFileName = "/tmp" + QString::number( qrand(), 16 );

        waitHttpData = true;
        httpError = false;
        QNetworkReply* reply = getHttp( url );

        if ( reply == 0 ) {
            break;
        }

        QTime downloadTime;
        QTime progressUpdateTime;
        downloadTime.start();
        progressUpdateTime.start();

        speedChanged = false;

        QString contentDisposition = reply->rawHeader( "Content-Disposition" );

        QString fileName = filenameFromHTTPContentDisposition( contentDisposition );
        downloadedFileName = fileName.isEmpty()
                             ? QFileInfo( reply->url().path() ).fileName() : fileName;
        qDebug()<<downloadedFileName<<reply->url().path();
        downloadedFileSize = reply->header( QNetworkRequest::ContentLengthHeader ).toInt();

        while ( waitHttpData ) {
            QApplication::processEvents( QEventLoop::WaitForMoreEvents );

            if ( cancelPressed ) {
                break;
            }

            if ( httpError ) {
                QMessageBox::warning( this, tr( "Error" ), reply->errorString() );
                cancelPressed = true;
                break;
            }

            downloadDuration = downloadTime.elapsed();

            if ( progressUpdateTime.elapsed() > 500 ) {
                progressUpdateTime.restart();
                speedChanged = true;
            }
        }

        if ( cancelPressed ) {
            reply->abort();
            reply->deleteLater();
            break;
        }

        outFile->setFileName( destDir + tmpFileName );

        try {
            if ( !outFile->open( QFile::WriteOnly ) ) {
                throw DtHttpError();
            }

            outFile->setPermissions( QFile::WriteOwner | QFile::ReadOwner );

            if ( cancelPressed ) {
                throw DtHttpError();
                outFile->remove();
            }
        }
        catch ( DtHttpError ) {
            reply->abort();
            reply->deleteLater();
            break;
        }

        outFile->write( reply->readAll() );
        QString newFilePath = QString( "%1/%2" ).arg( destDir, downloadedFileName );

        if ( !outFile->rename( newFilePath ) ) {
            outFile->remove();
        }
        else {
            QString format = QFileInfo( downloadedFileName ).suffix();

            if ( acceptedFileFormat( format ) ) {
                if ( !demoProtos.contains( format ) ) {
                    archiveFiles << newFilePath;
                }
            }
            else {
                outFile->remove();
            }
        }

        reply->deleteLater();
        ret = true;
    }

    cancelPressed = false;

    if ( !archiveFiles.isEmpty() ) {
        foreach ( const QString& archive, archiveFiles ) {
            unpack( archive, destDir );
            QFile::remove( archive );
        }
    }

    close();

    return ret;
}

QString DtFileDialog::filenameFromHTTPContentDisposition( const QString& value ) {
    QStringList keyValuePairs = value.split( ';' );
    quint32 length = keyValuePairs.size();

    for ( quint32 i = 0; i < length; ++i ) {
        int valueStartPos = keyValuePairs.at( i ).indexOf( '=' );

        if ( valueStartPos < 0 ) {
            continue;
        }

        QString key = keyValuePairs.at( i ).left( valueStartPos ).trimmed();

        if ( key.isEmpty() || ( key != "filename" && key != "filename*" ) ) {
            continue;
        }

        if ( key.endsWith( '*' ) ) {
            valueStartPos = keyValuePairs.at( i ).indexOf( "''" ) + 1;
        }

        QString val = keyValuePairs.at( i ).right( keyValuePairs.at( i ).size() - valueStartPos - 1 )
                      .trimmed();

        if ( val.at( 0 ) == '"' ) {
            val.chop( 1 );
            val = val.right( val.size() - 1 );
        }

        return val;
    }

    return "";
}

QStringList DtFileDialog::move( const DtDemoVec& mvDemos, const QString& dest ) {
    QStringList notMoved;
    copyState = CS_COPY;
    titleTemplate = tr( "Move" ) + " %1%";
    totalBytesCopied = 0;

    updateState( 0, 0, mvDemos.at( 0 )->fileInfo().fileName( false ), dest );

    QTime moveTime;
    moveTime.start();

    size_t mvDemosSize = mvDemos.size();
    totalSize = mvDemosSize;

    for ( size_t i = 0; i < mvDemosSize; ++i ) {
        if ( cancelPressed ) {
            for ( size_t j = i; j < mvDemosSize; ++j ) {
                notMoved << mvDemos.at( j )->fileInfo().fileName();
            }

            break;
        }

        QString fullFileName = mvDemos.at( i )->fileInfo().fileName();
        QStringList mvDemosList;
        mvDemosList << fullFileName;

        if ( !checkDir( mvDemosList, dest ) ) {
            notMoved << fullFileName;
            continue;
        }

        QString fName = mvDemos.at( i )->fileInfo().fileName( false );
        QString newName = dest + "/" + fName;

        if ( !QFile( fullFileName ).rename( newName ) ) {
            notMoved << mvDemos.at( i )->fileInfo().fileName();
            continue;
        }

        ++totalBytesCopied;

        emit newData( 100, totalBytesCopied * 100 / totalSize, fName, dest );

        if ( moveTime.elapsed() > 100 ) {
            show();
        }
    }

    if ( !cancelPressed ) {
        notAffectedMsg( notMoved, tr( "Some files haven't been moved" ) );
    }
    else {
        cancelPressed = false;
    }

    close();

    return notMoved;
}

QStringList DtFileDialog::remove( const DtDemoVec& delDemos ){
    QStringList notRemoved;

    if ( delDemos.size() == 0 ) {
        for ( int i = 0; i < delDemos.size(); ++i ) {
            notRemoved << delDemos.at( i )->fileInfo().fileName();
        }

        return notRemoved;
    }

    QStringList notRemovedPaths;
    titleTemplate = tr( "Delete" ) + " %1%";
    updateState( 0, 0, QString(), delDemos.at( 0 )->fileInfo().fileName( false ) );

    QTime deleteTime;
    deleteTime.start();

    for ( int i = 0; i < delDemos.size(); ++i ) {
        if ( cancelPressed ) {
            break;
        }

        QString fName = delDemos.at( i )->fileInfo().fileName();

        if ( !QFile::remove( fName ) ) {
            notRemoved << QFileInfo( fName ).fileName();
            notRemovedPaths << fName;
        }

        emit newData( 100, i * 100 / delDemos.size(), QString(),
                      delDemos.at( i )->fileInfo().fileName( false ) );

        if ( deleteTime.elapsed() > 100 ) {
            show();
        }
    }

    if ( !cancelPressed ) {
        notAffectedMsg( notRemoved, tr( "Some files haven't been deleted" ) );
    }
    else {
        cancelPressed = false;
    }

    close();

    return notRemovedPaths;
}

bool DtFileDialog::getArchiveName( QString& name, const QString& defName, bool& add ) {
    if ( !QDir( config.lastPackPath ).exists() ) {
        config.lastPackPath = QDir::homePath();
    }

    QString zipNameFilter = tr( "Zip archive (*.zip)" );
    QString p7zipNameFilter = tr( "7-Zip archive (*.7z)" );

    QFileDialog dlg( this, tr( "File name" ), config.lastPackPath );

    QString formatExt;

    switch ( config.lastArchiverFormat ) {
        case F_ZIP :
            dlg.setNameFilter( zipNameFilter + ";;" + p7zipNameFilter );
        break;

        case F_7Z :
            dlg.setNameFilter( p7zipNameFilter + ";;" + zipNameFilter );
        break;

        default : break;
    }

    QList< QUrl > gameDirs;
    gameDirs << QUrl::fromLocalFile( QDir::homePath() );

    if ( !config.getQzHomePath().isEmpty() ) {
        gameDirs << QUrl::fromLocalFile( config.getQzDemoPath() );
    }

    if ( !config.getQaHomePath().isEmpty() ) {
        gameDirs << QUrl::fromLocalFile( config.getQaDemoPath() );
    }

    dlg.setSidebarUrls( gameDirs );
    dlg.setOptions( QFileDialog::DontConfirmOverwrite );
    dlg.setFileMode( QFileDialog::AnyFile );
    dlg.selectFile( config.lastPackPath + "/" + defName );

    if ( dlg.exec() ) {
        name = dlg.selectedFiles().first();
    }

    if ( name.isEmpty() ) {
        return false;
    }

    if ( dlg.selectedNameFilter() == zipNameFilter ) {
        config.lastArchiverFormat = F_ZIP;
        formatExt = ".zip";
    }
    else if ( dlg.selectedNameFilter() == p7zipNameFilter ) {
        config.lastArchiverFormat = F_7Z;
        formatExt = ".7z";
    }

    if ( !name.endsWith( formatExt ) ) {
        name += formatExt;
    }

    if ( QFile::exists( name ) ) {
        QString msg = tr( "Archive with the specified name already exists. Overwrite it?" );

        if ( dlg.selectedNameFilter() == zipNameFilter ) {
            msg += "\n";
            msg += tr( "Click \"No\", to append files to the existing archive." );
        }

        int btn = QMessageBox::question( this, tr( "File exists" ), msg,
                                         QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel );
        if ( btn == QMessageBox::No ) {
            add = true;
        }
        else if ( btn == QMessageBox::Cancel )  {
            return false;
        }
    }

    config.lastPackPath = QFileInfo( name ).absolutePath();

    return true;
}

void DtFileDialog::packFile( const QString& tmpName, const QString& fileName,
                             const QString& archiveName )
{
    QFileInfo fileInfo( tmpName );
    totalSize = fileInfo.size();
    titleTemplate = tr( "Compressing" ) + " %1%";
    totalBytesCopied = 0;

    QStringList notCopied;
    QString dirName = fileInfo.absolutePath();

    updateState( 0, 0, fileName, dirName );
    show();

    if ( arch->newZip( archiveName, false ) ) {
        copyFile( tmpName, fileName, QString(), dirName, true, notCopied );
    }
    else {
        QMessageBox::critical( this, tr( "Error" ), tr( "Unable to create archive" ) );
    }

    arch->closeZip();
    close();
}

void DtFileDialog::fileCompressed( QString name, quint64 size ) {
    totalBytesCopied += size;
    QFileInfo fInfo( name );

    if ( name.isEmpty() && size == 0 ) {
        close();
    }
    else {
        emit newData( 100, totalBytesCopied * 100 / totalSize, fInfo.fileName(), curPath );
    }
}

void DtFileDialog::pack( const DtDemoVec& pkDemos, bool dir, const QString& packDir,
                         const QStringList& demos )
{
    if ( ( !dir && !pkDemos.size() ) || ( dir && !demos.size() ) ) {
        return;
    }

    QString path;
    bool add = false;
    QString defName;

    if ( dir ) {
        defName = demos.size() > 1 ? QDir( packDir ).dirName() :
                  QFileInfo( demos.first() ).completeBaseName();
    }
    else {
        defName = pkDemos.size() > 1 ? QDir( pkDemos.first()->fileInfo().filePath ).dirName() :
                  pkDemos.at( 0 )->fileInfo().baseName;
    }

    if ( !getArchiveName( path, defName, add ) ) {
        return;
    }

    QStringList notCopied;
    QMap< QString, QString > demoList;

    totalSize = dir ? getDemosSize( demos ) : getDemosSize( pkDemos );
    titleTemplate = tr( "Pack" ) + " %1%";
    totalBytesCopied = 0;

    QString fName = dir ? QFileInfo( demos.at( 0 ) ).fileName() :
                    pkDemos.at( 0 )->fileInfo().fileName( false );

    updateState( 0, 0, fName, path );
    show();

    if ( config.lastArchiverFormat == F_ZIP && !arch->newZip( path, add ) ) {
        return;
    }
    else if ( config.lastArchiverFormat == F_7Z && QFile::exists( path ) ) {
        if ( add ) {

        }
        else {
            QFile::remove( path );
        }
    }

    int size = dir ? demos.size() : pkDemos.size();

    for ( int i = 0; i < size; ++i ) {
        if ( cancelPressed ) {
            break;
        }

        fName = dir ? QString( demos.at( i ) ).remove( packDir ) :
                pkDemos.at( i )->fileInfo().fileName( false );

        QString fileName = dir ? demos.at( i ) : pkDemos.at( i )->fileInfo().fileName();

        if ( config.lastArchiverFormat == F_ZIP ) {
            copyFile( fileName, fName, "", path, true, notCopied );
        }
        else if ( config.lastArchiverFormat == F_7Z ) {
            demoList.insert( fileName, fName );
        }
    }

    if ( !cancelPressed ) {
        notAffectedMsg( notCopied, tr( "Some files haven't been packed" ) );
    }
    else {
        cancelPressed = false;
    }

    if ( config.lastArchiverFormat == F_7Z ) {
        curPath = path;
        QtConcurrent::run( arch, &DtArchiver::addFilesTo7z, path, demoList, false );
        return;
    }

    if ( config.lastArchiverFormat == F_ZIP ) {
        arch->closeZip();
    }

    close();
}

void DtFileDialog::packDir( const QString& path, const QStringList& demos ) {
    pack( DtDemoVec(), true, path, demos );
}

void DtFileDialog::uniqueFileName( const QString& dest, QString& fileName, const QFileInfo& info ) {
    if ( fileName.contains( '/' ) || fileName.contains( '\\' ) ) {
        fileName = info.fileName();
        QString fName = info.completeBaseName();

        int addNum = 0;

        while ( QFile::exists( dest + "/" + fileName ) ) {
            fileName = QString( "%1%2.%3" ).arg( fName ).arg( addNum++ ).arg( info.suffix() );
        }
    }
}

QString DtFileDialog::zipExtractFirstFile( const QString& path, const QString& dest ) {
    unpackZip( path, dest, true, true );
    return extractedFileName;
}

QString DtFileDialog::l7zExtractFirstFile( const QString& path, const QString& dest ) {
    unpack7z( path, dest, true, true );
    return extractedFileName;
}

void DtFileDialog::askSubarchivesAction( const QString& archive, const QString& subArchive ) {
    QString msg = tr( "Archive %1 contains sub-archive %2. Do you want to unpack it?" )
                  .arg( archive, subArchive );

    int act = QMessageBox::question( this, tr( "Unpack sub-archive" ), msg,
                                     QMessageBox::Yes | QMessageBox::YesToAll |
                                     QMessageBox::No | QMessageBox::NoToAll);
    switch ( act ) {
        case QMessageBox::Yes :
            unpackSubarchivesAction = true;
            break;

        case QMessageBox::YesToAll :
            unpackSubarchivesAction = true;
            subarchivesActionAsked = true;
            break;

        case QMessageBox::No :
            unpackSubarchivesAction = false;
            break;

        case QMessageBox::NoToAll :
            unpackSubarchivesAction = false;
            subarchivesActionAsked = true;
            break;

        default : break;
    }
}

bool DtFileDialog::tryUnpackSubarchive( const QString& extension,
                                        const QString& path, const QString& fileName )
{
    bool fileIsArchive = isArchiveFile( extension );

    if ( fileIsArchive ) {
        if ( !subarchivesActionAsked ) {
            askSubarchivesAction( QFileInfo( path ).fileName(), fileName );
        }

        if ( unpackSubarchivesAction ) {
            subarchiveNames << fileName;
        }
    }

    if ( !fileIsArchive || !unpackSubarchivesAction ) {
        return false;
    }

    return true;
}

void DtFileDialog::unpackSubarchives( const QString& dest ) {
    if ( !subarchiveNames.isEmpty() ) {
        foreach ( const QString& name, subarchiveNames ) {
            QString archivePath = dest + "/" + name;
            unpack( archivePath, dest );
            QFile::remove( archivePath );
        }
    }
}

void DtFileDialog::unpackZip( const QString& path, const QString& dest, bool removePaths,
                              bool extractFirstFile )
{
    updateState( 0, 0, path, dest );

    QTime unpackTime;
    unpackTime.start();

    titleTemplate = tr( "Unpack" ) + " %1%";
    totalBytesCopied = 0;
    totalSize = arch->getUncompressedZipSize( path, extractFirstFile );

    char block[ 8192 ];

    if ( extractFirstFile ) {
        extractedFileName.clear();
    }

    forever {
        if ( cancelPressed ) {
            break;
        }

        QString fileName = arch->unzipNext( path );

        if ( fileName.isEmpty() ) {
            break;
        }

        if ( fileName.endsWith( '/' ) ) { /* directory */
            if ( !removePaths ) {
                QDir( dest ).mkdir( fileName );
            }

            continue;
        }

        QFileInfo info( fileName );
        QString extension = info.suffix();

        if ( !extractFirstFile &&
             !demoProtos.contains( extension ) &&
             !tryUnpackSubarchive( extension, path, fileName ) )
        {
            continue;
        }

        if ( removePaths ) {
            uniqueFileName( dest, fileName, info );
        }

        outFile->setFileName( dest + "/" + fileName );

        if ( outFile->open( QFile::WriteOnly ) ) {
            int updateDlg = 0;
            quint64 fileBytesCopied = 0;

            forever {
                int read = arch->readZip( block, sizeof( block ) );

                if ( read > 0 ) {
                    outFile->write( block, read );
                }
                else {
                    break;
                }

                if ( unpackTime.elapsed() > 100 ) {
                    show();
                }

                fileBytesCopied += read;
                totalBytesCopied += read;

                const int blockNum = 700 * 1024 / sizeof( block );

                if ( ++updateDlg > blockNum ) {
                    updateDlg = 0;

                    if ( totalSize ) {
                        emit newData( fileBytesCopied * 100 / arch->currentUnzipFileSize(),
                                      totalBytesCopied * 100 / totalSize, fileName, dest );
                    }
                }
            }

            outFile->close();

            if ( extractFirstFile ) {
                extractedFileName = fileName;
                break;
            }
        }
    }

    arch->closeUnzip();

    if ( cancelPressed ) {
        cancelPressed = false;
    }
    else {
        unpackSubarchives( dest );
    }

    close();
}

void DtFileDialog::unpackRar( const QString& path, const QString& dest, bool removePaths ) {
    updateState( 0, 0, path, dest );

    QTime unpackTime;
    unpackTime.start();

    titleTemplate = tr( "Unpack" ) + " %1%";
    totalBytesCopied = 0;
    totalSize = arch->getUncompressedRarSize( path );

    forever {
        if ( cancelPressed ) {
            break;
        }

        QString fileName = arch->unrarNext( path );

        if ( fileName.isEmpty() ) {
            break;
        }

        QFileInfo info( fileName );
        QString extension = info.suffix();

        if ( !demoProtos.contains( extension ) && !tryUnpackSubarchive( extension, path, fileName ) ) {
            arch->rarSkipFile();
            continue;
        }

        if ( removePaths ) {
            uniqueFileName( dest, fileName, info );
        }

        QString newFilePath = QString( "%1/%2" ).arg( dest, fileName );

        if ( arch->unrarFileTo( newFilePath ) != 0 ) {
            break;
        }

        if ( unpackTime.elapsed() > 100 ) {
            show();
        }

        totalBytesCopied += arch->lastRarFileSize();

        if ( totalSize ) {
            emit newData( 100, totalBytesCopied * 100 / totalSize, fileName, dest );
        }
    }

    arch->closeRar();

    if ( cancelPressed ) {
        cancelPressed = false;
    }
    else {
        unpackSubarchives( dest );
    }

    close();

}

void DtFileDialog::unpack7z( const QString& path, const QString& dest, bool removePaths,
                             bool extractFirstFile )
{
    updateState( 0, 0, path, dest );

    QTime unpackTime;
    unpackTime.start();

    titleTemplate = tr( "Unpack" ) + " %1%";
    totalBytesCopied = 0;
    totalSize = arch->getUncompressed7zSize( path );

    if ( extractFirstFile ) {
        extractedFileName.clear();
    }

    show();
    arch->extract7z( path, dest, removePaths );

    if ( cancelPressed ) {
        cancelPressed = false;
    }
    else {
        unpackSubarchives( dest );
    }

    close();
}

void DtFileDialog::unpack( const QString& path, const QString& dest ) {
    if ( !QFile::exists( path ) ) {
        return;
    }

    bool rmPaths = config.archiveRemovePaths;
    QString suffix = QFileInfo( path ).suffix();

    unpackSubarchivesAction = false;
    subarchivesActionAsked = false;
    subarchiveNames.clear();

    if ( suffix == "zip" ) {
        unpackZip( path, dest, rmPaths );
    }
    else if ( suffix == "rar" ) {
        unpackRar( path, dest, rmPaths );
    }
    else if ( suffix == "7z" ) {
        unpack7z( path, dest, rmPaths );
    }
}
