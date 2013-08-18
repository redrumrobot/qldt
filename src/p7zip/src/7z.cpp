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

#ifdef Q_OS_LINUX
#include "linux/myWindows/StdAfx.h"

#include "linux/Common/IntToString.h"
#include "linux/Common/MyInitGuid.h"
#include "linux/Common/StringConvert.h"

#include "linux/Windows/DLL.h"
#include "linux/Windows/FileDir.h"
#include "linux/Windows/FileFind.h"
#include "linux/Windows/FileName.h"
#include "linux/Windows/PropVariant.h"
#include "linux/Windows/PropVariantConversions.h"

#include "linux/7zip/Common/FileStreams.h"

#include "linux/7zip/Archive/IArchive.h"

#include "linux/7zip/IPassword.h"
#include "linux/7zip/MyVersion.h"

#define kDllName "/usr/lib/p7zip/7z.so"

#elif defined Q_OS_WIN

#include "win32/Windows/StdAfx.h"

#include "win32/Common/IntToString.h"
#include "win32/Common/MyInitGuid.h"
#include "win32/Common/StringConvert.h"

#include "win32/Windows/DLL.h"
#include "win32/Windows/FileDir.h"
#include "win32/Windows/FileFind.h"
#include "win32/Windows/FileName.h"
#include "win32/Windows/PropVariant.h"
#include "win32/Windows/PropVariantConversions.h"

#include "win32/7zip/Common/FileStreams.h"

#include "win32/7zip/Archive/IArchive.h"

#include "win32/7zip/IPassword.h"
#include "win32/7zip/MyVersion.h"

#define kDllName "7z.dll"

#endif

#include "../include/7z.h"

#include <QFileInfo>
#include <sstream>


// use another CLSIDs, if you want to support other formats (zip, rar, ...).
// {23170F69-40C1-278A-1000-000110070000}
DEFINE_GUID(CLSID_CFormat7z,
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x07, 0x00, 0x00);

using namespace NWindows;

namespace mini7zip {

compressCallback_t* compressCallback;
extractSubarchiveCallback_t* extractSubarchiveCallback;

}

bool g_IsNT = true;

typedef UINT32 (WINAPI * CreateObjectFunc)(
    const GUID *clsID,
    const GUID *interfaceID,
    void **outObject);

void PrintString(const UString &s)
{
  printf("%s", (LPCSTR)GetOemString(s));
}

void PrintString(const AString &s)
{
  printf("%s", (LPCSTR)s);
}

void PrintNewLine()
{
  PrintString("\n");
}

void PrintStringLn(const AString &s)
{
  PrintString(s);
  PrintNewLine();
}

void PrintError(const AString &s)
{
  PrintNewLine();
  PrintString(s);
  PrintNewLine();
}

static HRESULT IsArchiveItemProp(IInArchive *archive, UInt32 index, PROPID propID, bool &result)
{
  NCOM::CPropVariant prop;
  RINOK(archive->GetProperty(index, propID, &prop));
  if (prop.vt == VT_BOOL)
    result = VARIANT_BOOLToBool(prop.boolVal);
  else if (prop.vt == VT_EMPTY)
    result = false;
  else
    return E_FAIL;
  return S_OK;
}

static HRESULT IsArchiveItemFolder(IInArchive *archive, UInt32 index, bool &result)
{
  return IsArchiveItemProp(archive, index, kpidIsDir, result);
}


static const wchar_t *kEmptyFileAlias = L"[Content]";


//////////////////////////////////////////////////////////////
// Archive Open callback class


class CArchiveOpenCallback:
  public IArchiveOpenCallback,
  public ICryptoGetTextPassword,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(ICryptoGetTextPassword)

  STDMETHOD(SetTotal)(const UInt64 *files, const UInt64 *bytes);
  STDMETHOD(SetCompleted)(const UInt64 *files, const UInt64 *bytes);

  STDMETHOD(CryptoGetTextPassword)(BSTR *password);

  bool PasswordIsDefined;
  UString Password;

  CArchiveOpenCallback() : PasswordIsDefined(false) {}
};

STDMETHODIMP CArchiveOpenCallback::SetTotal(const UInt64 * /* files */, const UInt64 * /* bytes */)
{
  return S_OK;
}

STDMETHODIMP CArchiveOpenCallback::SetCompleted(const UInt64 * /* files */, const UInt64 * /* bytes */)
{
  return S_OK;
}

STDMETHODIMP CArchiveOpenCallback::CryptoGetTextPassword(BSTR *password)
{
  if (!PasswordIsDefined)
  {
    // You can ask real password here from user
    // Password = GetPassword(OutStream);
    // PasswordIsDefined = true;
    PrintError("Password is not defined");
    return E_ABORT;
  }
  return StringToBstr(Password, password);
}


//////////////////////////////////////////////////////////////
// Archive Extracting callback class

class CArchiveExtractCallback:
  public IArchiveExtractCallback,
  public ICryptoGetTextPassword,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(ICryptoGetTextPassword)

  // IProgress
  STDMETHOD(SetTotal)(UInt64 size);
  STDMETHOD(SetCompleted)(const UInt64 *completeValue);

  // IArchiveExtractCallback
  STDMETHOD(GetStream)(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode);
  STDMETHOD(PrepareOperation)(Int32 askExtractMode);
  STDMETHOD(SetOperationResult)(Int32 resultEOperationResult);

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *aPassword);

private:
  CMyComPtr<IInArchive> _archiveHandler;
  UString _directoryPath;  // Output directory
  UString _filePath;       // name inside archive
  UString _diskFilePath;   // full path to file on disk
  bool removePaths;
  bool _extractMode;
  struct CProcessedFileInfo
  {
    FILETIME MTime;
    UInt32 Attrib;
    bool isDir;
    bool AttribDefined;
    bool MTimeDefined;
  } _processedFileInfo;

  COutFileStream *_outFileStreamSpec;
  CMyComPtr<ISequentialOutStream> _outFileStream;

public:
  void Init(IInArchive *archiveHandler, const UString &directoryPath, bool rmPaths);

  UInt64 NumErrors;
  bool PasswordIsDefined;
  UString Password;

  CArchiveExtractCallback() : PasswordIsDefined(false) {}
};

void CArchiveExtractCallback::Init(IInArchive *archiveHandler, const UString &directoryPath,
                                   bool rmPaths)
{
  removePaths = rmPaths;
  NumErrors = 0;
  _archiveHandler = archiveHandler;
  _directoryPath = directoryPath;
  NFile::NName::NormalizeDirPathPrefix(_directoryPath);
}

STDMETHODIMP CArchiveExtractCallback::SetTotal(UInt64 /* size */)
{
  return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::SetCompleted(const UInt64 * /* completeValue */)
{
  return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::GetStream(UInt32 index,
    ISequentialOutStream **outStream, Int32 askExtractMode)
{
  *outStream = 0;
  _outFileStream.Release();

  {
    // Get Name
    NCOM::CPropVariant prop;
    RINOK(_archiveHandler->GetProperty(index, kpidPath, &prop));

    UString fullPath;
    if (prop.vt == VT_EMPTY)
      fullPath = kEmptyFileAlias;
    else
    {
      if (prop.vt != VT_BSTR)
        return E_FAIL;
      fullPath = prop.bstrVal;
    }

    _archiveHandler->GetProperty( index, kpidSize, &prop );
    UInt64 size = ConvertPropVariantToUInt64( prop );

    QString fName = QString::fromStdWString( std::wstring( fullPath ) );
    mini7zip::compressCallback( fName, size );

    if ( removePaths ) {
        std::wstring wstr( fullPath );
        QString fPath = QString::fromStdWString( wstr );
        QFileInfo fInfo( fPath );
        wstr = fInfo.fileName().toStdWString();
        fullPath = wstr.c_str();
    }

    _filePath = fullPath;
  }

  if (askExtractMode != NArchive::NExtract::NAskMode::kExtract)
    return S_OK;

  {
    // Get Attrib
    NCOM::CPropVariant prop;
    RINOK(_archiveHandler->GetProperty(index, kpidAttrib, &prop));
    if (prop.vt == VT_EMPTY)
    {
      _processedFileInfo.Attrib = 0;
      _processedFileInfo.AttribDefined = false;
    }
    else
    {
      if (prop.vt != VT_UI4)
        return E_FAIL;
      _processedFileInfo.Attrib = prop.ulVal;
      _processedFileInfo.AttribDefined = true;
    }
  }

  RINOK(IsArchiveItemFolder(_archiveHandler, index, _processedFileInfo.isDir));

  {
    // Get Modified Time
    NCOM::CPropVariant prop;
    RINOK(_archiveHandler->GetProperty(index, kpidMTime, &prop));
    _processedFileInfo.MTimeDefined = false;
    switch(prop.vt)
    {
      case VT_EMPTY:
        // _processedFileInfo.MTime = _utcMTimeDefault;
        break;
      case VT_FILETIME:
        _processedFileInfo.MTime = prop.filetime;
        _processedFileInfo.MTimeDefined = true;
        break;
      default:
        return E_FAIL;
    }

  }
  {
    // Get Size
    NCOM::CPropVariant prop;
    RINOK(_archiveHandler->GetProperty(index, kpidSize, &prop));
    bool newFileSizeDefined = (prop.vt != VT_EMPTY);
    UInt64 newFileSize;
    if (newFileSizeDefined)
      newFileSize = ConvertPropVariantToUInt64(prop);
  }


  {
    // Create folders for file
    int slashPos = _filePath.ReverseFind(WCHAR_PATH_SEPARATOR);
    if (slashPos >= 0)
      NFile::NDirectory::CreateComplexDirectory(_directoryPath + _filePath.Left(slashPos));
  }

  UString fullProcessedPath = _directoryPath + _filePath;

  if (_processedFileInfo.isDir)
  {
    if ( !removePaths ) {
      NFile::NDirectory::CreateComplexDirectory(fullProcessedPath);
    }
  }
  else
  {

      std::wstring wstr( fullProcessedPath );
      QString fpPath = QString::fromStdWString( wstr );
      QFileInfo fInfo( fpPath );

      if ( fInfo.exists() ) {
          QString newName;
          QString newNamePath;
          QString addNumString;
          QString fileExt = fInfo.suffix();
          QString filePath = fInfo.absolutePath();
          QString newFileName = fInfo.completeBaseName();
          int addNum = 0;

          do {
              newName = QString( "%2%3.%4" ).arg( newFileName, addNumString, fileExt );
              newNamePath = QString( "%1/%2" ).arg( filePath, newName );
              addNumString = QString( "_%1" ).arg( ++addNum );
          } while ( QFile::exists( newNamePath ) );

          std::wstring wstr = newNamePath.toStdWString();
          fullProcessedPath = wstr.c_str();
      }

    _outFileStreamSpec = new COutFileStream;
    CMyComPtr<ISequentialOutStream> outStreamLoc(_outFileStreamSpec);
    if (!_outFileStreamSpec->Open(fullProcessedPath, CREATE_ALWAYS))
    {
      PrintString((UString)L"can not open output file " + fullProcessedPath);
      return E_ABORT;
    }
    _outFileStream = outStreamLoc;
    *outStream = outStreamLoc.Detach();
  }

  _diskFilePath = fullProcessedPath;

  return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::PrepareOperation(Int32 askExtractMode)
{
  _extractMode = false;
  switch (askExtractMode)
  {
    case NArchive::NExtract::NAskMode::kExtract:  _extractMode = true; break;
  };

  return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::SetOperationResult(Int32)
{
  if (_outFileStream != NULL)
  {
    if (_processedFileInfo.MTimeDefined)
      _outFileStreamSpec->SetMTime(&_processedFileInfo.MTime);
    RINOK(_outFileStreamSpec->Close());
  }
  _outFileStream.Release();
  if (_extractMode && _processedFileInfo.AttribDefined)
    NFile::NDirectory::MySetFileAttributes(_diskFilePath, _processedFileInfo.Attrib);
  return S_OK;
}


STDMETHODIMP CArchiveExtractCallback::CryptoGetTextPassword(BSTR *password)
{
  if (!PasswordIsDefined)
  {
    // You can ask real password here from user
    // Password = GetPassword(OutStream);
    // PasswordIsDefined = true;
    PrintError("Password is not defined");
    return E_ABORT;
  }
  return StringToBstr(Password, password);
}



//////////////////////////////////////////////////////////////
// Archive Creating callback class

struct CDirItem
{
  UInt64 Size;
  FILETIME CTime;
  FILETIME ATime;
  FILETIME MTime;
  UString Name;
  UString FullPath;
  UInt32 Attrib;

  bool isDir() const { return (Attrib & FILE_ATTRIBUTE_DIRECTORY) != 0 ; }
};

class CArchiveUpdateCallback:
  public IArchiveUpdateCallback2,
  public ICryptoGetTextPassword2,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP2(IArchiveUpdateCallback2, ICryptoGetTextPassword2)

  // IProgress
  STDMETHOD(SetTotal)(UInt64 size);
  STDMETHOD(SetCompleted)(const UInt64 *completeValue);

  // IUpdateCallback2
  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **enumerator);
  STDMETHOD(GetUpdateItemInfo)(UInt32 index,
      Int32 *newData, Int32 *newProperties, UInt32 *indexInArchive);
  STDMETHOD(GetProperty)(UInt32 index, PROPID propID, PROPVARIANT *value);
  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **inStream);
  STDMETHOD(SetOperationResult)(Int32 operationResult);
  STDMETHOD(GetVolumeSize)(UInt32 index, UInt64 *size);
  STDMETHOD(GetVolumeStream)(UInt32 index, ISequentialOutStream **volumeStream);

  STDMETHOD(CryptoGetTextPassword2)(Int32 *passwordIsDefined, BSTR *password);

public:
  CRecordVector<UInt64> VolumesSizes;
  UString VolName;
  UString VolExt;

  UString DirPrefix;
  const CObjectVector<CDirItem> *DirItems;

  bool PasswordIsDefined;
  UString Password;
  bool AskPassword;

  bool m_NeedBeClosed;

  UStringVector FailedFiles;
  CRecordVector<HRESULT> FailedCodes;

  CArchiveUpdateCallback(): DirItems(0), PasswordIsDefined(false), AskPassword(false) {};

  ~CArchiveUpdateCallback() { Finilize(); }
  HRESULT Finilize();

  void Init(const CObjectVector<CDirItem> *dirItems)
  {
    DirItems = dirItems;
    m_NeedBeClosed = false;
    FailedFiles.Clear();
    FailedCodes.Clear();
  }
};

STDMETHODIMP CArchiveUpdateCallback::SetTotal(UInt64 /* size */)
{
  return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::SetCompleted(const UInt64 * /* completeValue */)
{
  return S_OK;
}


STDMETHODIMP CArchiveUpdateCallback::EnumProperties(IEnumSTATPROPSTG ** /* enumerator */)
{
  return E_NOTIMPL;
}

STDMETHODIMP CArchiveUpdateCallback::GetUpdateItemInfo(UInt32 /* index */,
      Int32 *newData, Int32 *newProperties, UInt32 *indexInArchive)
{
  if (newData != NULL)
    *newData = BoolToInt(true);
  if (newProperties != NULL)
    *newProperties = BoolToInt(true);
  if (indexInArchive != NULL)
    *indexInArchive = (UInt32)-1;
  return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  NWindows::NCOM::CPropVariant prop;

  if (propID == kpidIsAnti)
  {
    prop = false;
    prop.Detach(value);
    return S_OK;
  }

  {
    const CDirItem &dirItem = (*DirItems)[index];
    switch(propID)
    {
      case kpidPath:  prop = dirItem.Name; break;
      case kpidIsDir:  prop = dirItem.isDir(); break;
      case kpidSize:  prop = dirItem.Size; break;
      case kpidAttrib:  prop = dirItem.Attrib; break;
      case kpidCTime:  prop = dirItem.CTime; break;
      case kpidATime:  prop = dirItem.ATime; break;
      case kpidMTime:  prop = dirItem.MTime; break;
    }
  }
  prop.Detach(value);
  return S_OK;
}

HRESULT CArchiveUpdateCallback::Finilize()
{
  if (m_NeedBeClosed)
  {
    m_NeedBeClosed = false;
  }


  return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::GetStream(UInt32 index, ISequentialInStream **inStream)
{
  RINOK(Finilize());

  const CDirItem &dirItem = (*DirItems)[index];

  QString fName = QString::fromStdWString( std::wstring( dirItem.FullPath ) );
  mini7zip::compressCallback( fName, QFile( fName ).size() );

  if (dirItem.isDir())
    return S_OK;

  {
    CInFileStream *inStreamSpec = new CInFileStream;
    CMyComPtr<ISequentialInStream> inStreamLoc(inStreamSpec);
    UString path = DirPrefix + dirItem.FullPath;
    if (!inStreamSpec->Open(path))
    {
      DWORD sysError = ::GetLastError();
      FailedCodes.Add(sysError);
      FailedFiles.Add(path);
      // if (systemError == ERROR_SHARING_VIOLATION)
      {
        PrintNewLine();
        PrintError("WARNING: can't open file");
        // PrintString(NError::MyFormatMessageW(systemError));
        return S_FALSE;
      }
      // return sysError;
    }
    *inStream = inStreamLoc.Detach();
  }
  return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::SetOperationResult(Int32 /* operationResult */)
{
  m_NeedBeClosed = true;
  return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::GetVolumeSize(UInt32 index, UInt64 *size)
{
  if (VolumesSizes.Size() == 0)
    return S_FALSE;
  if (index >= (UInt32)VolumesSizes.Size())
    index = VolumesSizes.Size() - 1;
  *size = VolumesSizes[index];
  return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::GetVolumeStream(UInt32 index, ISequentialOutStream **volumeStream)
{
  wchar_t temp[16];
  ConvertUInt32ToString(index + 1, temp);
  UString res = temp;
  while (res.Length() < 2)
    res = UString(L'0') + res;
  UString fileName = VolName;
  fileName += L'.';
  fileName += res;
  fileName += VolExt;
  COutFileStream *streamSpec = new COutFileStream;
  CMyComPtr<ISequentialOutStream> streamLoc(streamSpec);
  if (!streamSpec->Create(fileName, false))
    return ::GetLastError();
  *volumeStream = streamLoc.Detach();
  return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::CryptoGetTextPassword2(Int32 *passwordIsDefined, BSTR *password)
{
  if (!PasswordIsDefined)
  {
    if (AskPassword)
    {
      // You can ask real password here from user
      // Password = GetPassword(OutStream);
      // PasswordIsDefined = true;
      PrintError("Password is not defined");
      return E_ABORT;
    }
  }
  *passwordIsDefined = BoolToInt(PasswordIsDefined);
  return StringToBstr(Password, password);
}

namespace mini7zip {

CreateObjectFunc createObjectFunc;
bool archiveOpened = false;
bool archiveUpdate = false;
bool firstFile = true;
UInt32 fileIndex;
UInt32 filesCount;
NWindows::NDLL::CLibrary library;
IInArchive* archive;
IOutArchive* outArchive;
CInFileStream* fileStream;
COutFileStream* outFileStream;
bool initialized = false;

void p7zipInit( compressCallback_t* cCallback, extractSubarchiveCallback_t* eCallback ) {
    if ( !library.Load( TEXT( kDllName ) ) ) {
        qDebug() << "Error: couldn't open library" << kDllName;
        return;
    }
#ifdef Q_OS_LINUX
    createObjectFunc = reinterpret_cast< CreateObjectFunc >(
                           library.GetProcAddress( "CreateObject" ) );
#elif defined Q_OS_WIN
    createObjectFunc = reinterpret_cast< CreateObjectFunc >(
                           library.GetProc( "CreateObject" ) );
#endif

    if ( createObjectFunc == 0 ) {
        qDebug() << "Can not get CreateObject";
        return;
    }

    initialized = true;
    compressCallback = cCallback;
    extractSubarchiveCallback = eCallback;
}

bool openInArchive( const wchar_t* path ) {
    if ( createObjectFunc( &CLSID_CFormat7z, &IID_IInArchive, (void **)&archive ) != S_OK ) {
        qDebug() << "Can not get class object";
        return false;
    }

    fileStream = new CInFileStream;
    UString archiveName( path );

    if ( !fileStream->Open( archiveName ) ) {
        qDebug() << "Can not open archive file";
        return false;
    }

    {
        CArchiveOpenCallback* openCallbackSpec = new CArchiveOpenCallback;
        CMyComPtr< IArchiveOpenCallback > openCallback( openCallbackSpec );
        openCallbackSpec->PasswordIsDefined = false;

        if ( archive->Open( fileStream, 0, openCallback ) != S_OK ) {
            qDebug() << "Can not open archive";
            return false;
        }
    }

    filesCount = 0;
    archive->GetNumberOfItems( &filesCount );
    archiveOpened = true;
    fileIndex = 0;
    firstFile = true;
    archiveUpdate = false;

    return true;
}

bool openOutArchive( const wchar_t* path ) {
    if ( createObjectFunc( &CLSID_CFormat7z, &IID_IOutArchive, (void **)&outArchive ) != S_OK ) {
        qDebug() << "Can not get class object";
        return false;
    }

    outFileStream = new COutFileStream;
    UString archiveName( path );

    NFile::NFind::CFileInfoW fi;

    if ( !fi.Find( path ) ) {
        if ( !outFileStream->Create( archiveName, false ) ) {
            qDebug() << "Can not create archive file";
            return false;
        }
    }
    else {
        if ( !outFileStream->Open( archiveName, 0 ) ) {
            qDebug() << "Can not create open file";
            return false;
        }
    }

    archiveUpdate = true;

    return true;
}

bool openArchive( const wchar_t* path, bool update ) {
    if ( !initialized ) {
        return false;
    }

    if ( archiveOpened ) {
        return false;
    }

    if ( update ) {
        return openOutArchive( path );
    }

    return openInArchive( path );
}

void closeArchive() {
    if ( archiveOpened ) {
        if ( archiveUpdate ) {
            outFileStream->Release();
        }
        else {
            fileStream->Release();
        }

        archiveOpened = false;
    }
}

unsigned long getUncompressed7zSize( const wchar_t* path, bool otherFormats ) {
    if ( !openArchive( path, false ) ) {
        return 0;
    }

    UInt64 totalUnPackSize = 0;

    for ( UInt32 i = 0; i < filesCount; ++i ) {
        NWindows::NCOM::CPropVariant prop;
        archive->GetProperty( i, kpidPath, &prop );
        UString filePath = ConvertPropVariantToString( prop );

        const wchar_t* wstr = filePath;
        QFileInfo fInfo( QString::fromStdWString( wstr ) );

        if ( !otherFormats && !dtdata::demoProtos.contains( fInfo.suffix() ) ) {
            continue;
        }

        archive->GetProperty( i, kpidSize, &prop );
        totalUnPackSize += ConvertPropVariantToUInt64( prop );
    }

    closeArchive();

    return totalUnPackSize;
}

bool unpack( QString path, bool removePaths ) {
    CRecordVector< UInt32 > indexes;

    for ( UInt32 i = 0; i < filesCount; ++i ) {
        NWindows::NCOM::CPropVariant prop;
        archive->GetProperty( i, kpidPath, &prop );
        UString filePath = ConvertPropVariantToString( prop );

        const wchar_t* wstr = filePath;
        QFileInfo fInfo( QString::fromStdWString( wstr ) );
        QString ext = fInfo.suffix();

        if ( !dtdata::demoProtos.contains( ext ) &&
             !extractSubarchiveCallback( path, fInfo.fileName() ) )
        {
            continue;
        }

        indexes.Add( i );
    }

    std::wstring wstr = path.toStdWString();
    UString outDir( wstr.c_str() );

    CArchiveExtractCallback* aec = new CArchiveExtractCallback;
    CMyComPtr< IArchiveExtractCallback > ec = aec;
    aec->Init( archive, outDir, removePaths );

    HRESULT result = archive->Extract( &indexes.Front(), indexes.Size(), false, aec );
    //compressCallback( "", 0 );

    return ( result == S_OK );
}

bool packFiles( const QMap< QString, QString > demoList, bool move ) {
    UStringVector commandStrings;
    int level = dtdata::config.zlibCompressionLevel;

    if ( level > 1 && !( level & 1 ) ) {
        ++level;
    }

    std::wstringstream wStream;
    wStream << L"x" << level;

    commandStrings.Add( wStream.str().c_str() );

    CRecordVector< const wchar_t* > names;
    names.Add( (const wchar_t *)commandStrings[0] );

    NWindows::NCOM::CPropVariant* values = new NWindows::NCOM::CPropVariant[ 1 ];
    NWindows::NCOM::CPropVariant propVariant;
    values[ 0 ] = propVariant;

    CMyComPtr< ISetProperties > setProperties;
    outArchive->QueryInterface( IID_ISetProperties, (void **)&setProperties );
    setProperties->SetProperties( &names.Front(), values, names.Size() );

    delete [] values;

    CObjectVector< CDirItem > dirItems;
    QMapIterator< QString, QString > it( demoList );

    while ( it.hasNext() ) {
        it.next();

        CDirItem di;
        std::wstring wstr = it.key().toStdWString();
        UString filePath( wstr.c_str() );
        UString name = GetUnicodeString( filePath, CP_OEMCP );

        wstr = it.value().toStdWString();
        UString fileName( wstr.c_str() );

        NFile::NFind::CFileInfoW fi;

        if ( !fi.Find( name ) ) {
            continue;
        }

        di.Attrib = fi.Attrib;
        di.Size = fi.Size;
        di.CTime = fi.CTime;
        di.ATime = fi.ATime;
        di.MTime = fi.MTime;
        di.Name = fileName;
        di.FullPath = name;
        dirItems.Add( di );
    }

    CArchiveUpdateCallback* updateCallbackSpec = new CArchiveUpdateCallback;
    CMyComPtr< IArchiveUpdateCallback2 > updateCallback( updateCallbackSpec );
    updateCallbackSpec->Init( &dirItems );

    HRESULT result = outArchive->UpdateItems( outFileStream, dirItems.Size(), updateCallback );
    updateCallbackSpec->Finilize();
    compressCallback( "", 0 );

    if ( move ) {
        it.toFront();

        while ( it.hasNext() ) {
            it.next();
            QFile::remove( it.key() );
        }
    }

    return ( result == S_OK );
}

}

