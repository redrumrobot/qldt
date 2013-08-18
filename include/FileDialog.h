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

#ifndef DTFILEDIALOG_H
#define DTFILEDIALOG_H

#include "DemoData.h"

#include <QDialog>
#include <QUrl>
#include <QNetworkReply>

class DtDemo;
class DtArchiver;
class QFile;
class QLabel;
class QProgressBar;
class QFileInfo;
class QNetworkAccessManager;
class QNetworkReply;
class QNetworkRequest;

class DtFileDialog : public QDialog {
    Q_OBJECT
public:
    DtFileDialog( QWidget* parent = 0 );
    virtual ~DtFileDialog() {}

    bool checkDestDir( const QString& dest );
    void copy( const DtDemoVec& cpDemos, const QString& dest );
    void copy( const QStringList& cpDemos, const QString& dest );
    void copySegments( DtCpDemoVec& cpDemos, const QString& dest, bool zip = false, bool add = false );
    void packSegments( DtCpDemoVec& cpDemos );
    QStringList move( const DtDemoVec& mvDemos, const QString& dest );
    QStringList remove( const DtDemoVec& delDemos );
    void copyDroppedFiles( const QStringList& droppedFiles, const QString& destDir );
    bool copyDroppedUrls( const QList< QUrl >& droppedUrls, const QString& destDir );

    void pack( const DtDemoVec& pkDemos, bool dir = false, const QString& packDir = QString(),
               const QStringList& demos = QStringList() );
    void packFile( const QString& tmpName, const QString& fileName, const QString& archiveName );
    void packDir( const QString& path, const QStringList& demos );
    void unpack( const QString& path, const QString& dest );
    bool tryUnpackSubarchive( const QString& extension, const QString& path, const QString& fileName );

    /* extract first file with any extension from archive named "path" to directory "dest" */
    QString zipExtractFirstFile( const QString& path, const QString& dest );
    QString l7zExtractFirstFile( const QString& path, const QString& dest );

protected:
    QProgressBar* fileProgress;
    QProgressBar* totalProgress;
    QPushButton* cancelButton;
    QLabel* fileLabel;
    QLabel* totalLabel;
    QString titleTemplate;
    QFile* file;
    QFile* outFile;
    QString tmpName;
    DtArchiver* arch;
    QString curDemoName;
    QString curPath;
    QNetworkAccessManager* networkManager;
    QUrl lastRedirectUrl;
    QString extractedFileName;
    QString oldMainWindowTitle;

    bool waitHttpData;
    bool waitHttpHeader;
    bool httpError;
    bool speedChanged;
    bool cancelPressed;
    bool running;
    quint64 totalSize;
    quint64 totalBytesCopied;
    int downloadDuration;
    QString downloadedFileName;
    int downloadedFileSize;

    bool unpackSubarchivesAction;
    bool subarchivesActionAsked;
    QStringList subarchiveNames;

    enum copyStates {
        CS_COPY,
        CS_CANCEL,
        CS_YESTOALL,
        CS_AUTORENAME
    } copyState;

    class DtHttpError { };

    quint64 getFreeSpace( const QString& dir );
    quint64 getDemosSize( const DtDemoVec& dm );
    quint64 getDemosSize( const QStringList& dm );
    quint64 getDemosSize( const DtCpDemoVec& dm );

    bool checkDir( const DtDemoVec& cDemos, const QString& dest );
    bool checkDir( const DtCpDemoVec& cDemos, const QString& dest );
    bool checkDir( const QStringList& cDemos, const QString& dest );
    void notAffectedMsg( QStringList& na, const QString& nMsg );
    bool copyFile( const QString& fullFileName, const QString& fName, const QString& newName,
                   const QString& dest, bool zip, QStringList& na );
    bool getArchiveName( QString& name, const QString& defName, bool& add );
    void unpackZip( const QString& path, const QString& dest, bool removePaths,
                    bool extractFirstFile = false );
    void unpackRar( const QString& path, const QString& dest, bool removePaths );
    void unpack7z( const QString& path, const QString& dest, bool removePaths,
                  bool extractFirstFile = false );
    void uniqueFileName( const QString& dest, QString& fileName, const QFileInfo& info );
    QNetworkReply* getHttp( const QUrl& url );
    QString filenameFromHTTPContentDisposition( const QString& value );
    void askSubarchivesAction( const QString& archive, const QString& subArchive );
    void unpackSubarchives( const QString& dest );

public slots:
    void fileCompressed( QString name, quint64 size );

protected slots:
    void cancel();
    void updateState( int pFile, int pTotal, const QString& fText, const QString& tText );
    void updateState( int pFile, int pTotal, float pSpeed, const QString& fText, const QString& tText );
    void demoReadPos( int );
    void downloadFinished();
    void readHttpHeader();
    void updateDownloadProgress( qint64 read, qint64 total );
    void onHttpError( QNetworkReply::NetworkError code );

signals:
    void newData( int, int, const QString&, const QString& );
    void newHttpData( int, int, float, const QString&, const QString& );
};

#endif // DTFILEDIALOG_H
