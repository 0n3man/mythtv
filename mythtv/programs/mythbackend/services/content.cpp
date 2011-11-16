//////////////////////////////////////////////////////////////////////////////
// Program Name: content.cpp
// Created     : Mar. 7, 2011
//
// Copyright (c) 2011 David Blain <dblain@mythtv.org>
//                                          
// This library is free software; you can redistribute it and/or 
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or at your option any later version of the LGPL.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library.  If not, see <http://www.gnu.org/licenses/>.
//
//////////////////////////////////////////////////////////////////////////////

#include "content.h"

#include <QDir>
#include <QImage>
#include <math.h>

#include <compat.h>

#include "mythcorecontext.h"
#include "storagegroup.h"
#include "programinfo.h"
#include "previewgenerator.h"
#include "backendutil.h"
#include "httprequest.h"
#include "util.h"
#include "mythdownloadmanager.h"
#include "metadataimagehelper.h"

/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////

QFileInfo Content::GetFile( const QString &sStorageGroup,
                            const QString &sFileName )
{
    QString sGroup = sStorageGroup;

    if (sGroup.isEmpty())
    {
        LOG(VB_UPNP, LOG_WARNING,
            "GetFile - StorageGroup missing... using 'Default'");
        sGroup = "Default";
    }

    if (sFileName.isEmpty())
    {
        QString sMsg ( "GetFile - FileName missing." );

        LOG(VB_UPNP, LOG_ERR, sMsg);

        throw sMsg;
    }

    // ------------------------------------------------------------------
    // Search for the filename
    // ------------------------------------------------------------------

    StorageGroup storage( sGroup );
    QString sFullFileName = storage.FindFile( sFileName );

    if (sFullFileName.isEmpty())
    {
        LOG(VB_UPNP, LOG_ERR,
            QString("GetFile - Unable to find %1.").arg(sFileName));

        return QFileInfo();
    }

    // ----------------------------------------------------------------------
    // check to see if the file (still) exists
    // ----------------------------------------------------------------------

    if (QFile::exists( sFullFileName ))
    {
        return QFileInfo( sFullFileName );
    }

    LOG(VB_UPNP, LOG_ERR,
        QString("GetFile - File Does not exist %1.").arg(sFullFileName));

    return QFileInfo();
}

/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////

QFileInfo Content::GetImageFile( const QString &sStorageGroup,
                            const QString &sFileName,
                            int nWidth,
                            int nHeight)
{
    QString sGroup = sStorageGroup;

    if (sGroup.isEmpty())
    {
        LOG(VB_UPNP, LOG_WARNING,
            "GetFile - StorageGroup missing... using 'Default'");
        sGroup = "Default";
    }

    if (sFileName.isEmpty())
    {
        QString sMsg ( "GetFile - FileName missing." );

        LOG(VB_UPNP, LOG_ERR, sMsg);

        throw sMsg;
    }

    // ------------------------------------------------------------------
    // Search for the filename
    // ------------------------------------------------------------------

    StorageGroup storage( sGroup );
    QString sFullFileName = storage.FindFile( sFileName );

    if (sFullFileName.isEmpty())
    {
        LOG(VB_UPNP, LOG_ERR,
            QString("GetImageFile - Unable to find %1.").arg(sFileName));

        return QFileInfo();
    }

    // ----------------------------------------------------------------------
    // check to see if the file (still) exists
    // ----------------------------------------------------------------------

    if ((nWidth == 0) && (nHeight == 0))
    {
        if (QFile::exists( sFullFileName ))
        {
            return QFileInfo( sFullFileName );
        }

        LOG(VB_UPNP, LOG_ERR,
            QString("GetImageFile - File Does not exist %1.").arg(sFullFileName));

        return QFileInfo();
    }

    QString sNewFileName = QString( "%1.%2x%3.jpg" )
                              .arg( sFullFileName )
                              .arg( nWidth    )
                              .arg( nHeight   );

    // ----------------------------------------------------------------------
    // check to see if image is already created.
    // ----------------------------------------------------------------------

    if (QFile::exists( sNewFileName ))
        return QFileInfo( sNewFileName );

    // ----------------------------------------------------------------------
    // Must generate Generate Image and save.
    // ----------------------------------------------------------------------

    float fAspect = 0.0;

    QImage *pImage = new QImage( sFullFileName);

    if (!pImage)
        return QFileInfo();

    if (fAspect <= 0)
           fAspect = (float)(pImage->width()) / pImage->height();

    if ( nWidth == 0 )
        nWidth = (int)rint(nHeight * fAspect);

    if ( nHeight == 0 )
        nHeight = (int)rint(nWidth / fAspect);

    QImage img = pImage->scaled( nWidth, nHeight, Qt::IgnoreAspectRatio,
                                Qt::SmoothTransformation);

    QByteArray fname = sNewFileName.toAscii();
    img.save( fname.constData(), "JPG", 60 );

    delete pImage;

    return QFileInfo( sNewFileName );
}

/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////

QStringList Content::GetFileList( const QString &sStorageGroup )
{

    if (sStorageGroup.isEmpty())
    {
        QString sMsg( "GetFileList - StorageGroup missing.");
        LOG(VB_UPNP, LOG_ERR, sMsg);

        throw sMsg;
    }

    StorageGroup sgroup(sStorageGroup);

    return sgroup.GetFileList("", true);
}

/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////

QFileInfo Content::GetRecordingArtwork ( const QString   &sType,
                                         const QString   &sInetref,
                                         int nSeason,
                                         int nWidth,
                                         int nHeight)
{
    ArtworkMap map = GetArtwork(sInetref, nSeason);

    VideoArtworkType type = kArtworkCoverart;
    QString sgroup;

    if (sType.toLower() == "coverart")
    {
        sgroup = "Coverart";
        type = kArtworkCoverart;
    }
    else if (sType.toLower() == "fanart")
    {
        sgroup = "Fanart";
        type = kArtworkFanart;
    }
    else if (sType.toLower() == "banner")
    {
        sgroup = "Banners";
        type = kArtworkBanner;
    }

    QUrl url(map.value(type).url);
    QString sFileName = url.path();

    return GetImageFile( sgroup, sFileName, nWidth, nHeight);
}

/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////

DTC::ArtworkInfoList* Content::GetRecordingArtworkList( int              nChanId,
                                                        const QDateTime &dStartTime  )
{
    if (nChanId <= 0 || !dStartTime.isValid())
        throw( QString("Channel ID or StartTime appears invalid."));

    ProgramInfo pInfo = ProgramInfo(nChanId, dStartTime);

    return GetProgramArtworkList(pInfo.GetInetRef(), pInfo.GetSeason());
}

DTC::ArtworkInfoList* Content::GetProgramArtworkList( const QString &sInetref,
                                                      int            nSeason  )
{
    ArtworkMap map = GetArtwork(sInetref, nSeason);

    DTC::ArtworkInfoList *pInfos = new DTC::ArtworkInfoList();

    for (ArtworkMap::const_iterator i = map.begin();
         i != map.end(); ++i)
    {
        DTC::ArtworkInfo *pArtInfo = pInfos->AddNewArtworkInfo();
        pArtInfo->setFileName(i.value().url);
        switch (i.key())
        {
            case kArtworkFanart:
                pArtInfo->setStorageGroup("Fanart");
                pArtInfo->setType("fanart");
                pArtInfo->setURL(QString("/Content/GetRecordingArtwork?InetRef=%1"
                              "&Season=%2&Type=fanart").arg(sInetref)
                              .arg(nSeason));
                break;
            case kArtworkBanner:
                pArtInfo->setStorageGroup("Banners");
                pArtInfo->setType("banner");
                pArtInfo->setURL(QString("/Content/GetRecordingArtwork?InetRef=%1"
                              "&Season=%2&Type=banner").arg(sInetref)
                              .arg(nSeason));
                break;
            case kArtworkCoverart:
            default:
                pArtInfo->setStorageGroup("Coverart");
                pArtInfo->setType("coverart");
                pArtInfo->setURL(QString("/Content/GetRecordingArtwork?InetRef=%1"
                              "&Season=%2&Type=coverart").arg(sInetref)
                              .arg(nSeason));
                break;
        }
    }
    return pInfos;
}
/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////

QFileInfo Content::GetVideoArtwork( const QString &sType,
                                    int nId, int nWidth, int nHeight )
{
    LOG(VB_UPNP, LOG_INFO, QString("GetVideoArtwork ID = %1").arg(nId));

    QString sgroup = "Coverart";
    QString column = "coverfile";

    if (sType.toLower() == "coverart")
    {
        sgroup = "Coverart";
        column = "coverfile";
    }
    else if (sType.toLower() == "fanart")
    {
        sgroup = "Fanart";
        column = "fanart";
    }
    else if (sType.toLower() == "banner")
    {
        sgroup = "Banners";
        column = "banner";
    }
    else if (sType.toLower() == "screenshot")
    {
        sgroup = "Screenshots";
        column = "screenshot";
    }

    // ----------------------------------------------------------------------
    // Read Video artwork file path from database
    // ----------------------------------------------------------------------

    MSqlQuery query(MSqlQuery::InitCon());

    QString querystr = QString("SELECT %1 FROM videometadata WHERE "
                               "intid = :ITEMID").arg(column);

    query.prepare(querystr);
    query.bindValue(":ITEMID", nId);

    if (!query.exec())
        MythDB::DBError("GetVideoArtwork ", query);

    if (!query.next())
        return QFileInfo();

    QString sFileName = query.value(0).toString();

    return GetImageFile( sgroup, sFileName, nWidth, nHeight );
}

/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////

QFileInfo Content::GetAlbumArt( int nId, int nWidth, int nHeight )
{
    QString sFullFileName;

    // ----------------------------------------------------------------------
    // Read AlbumArt file path from database
    // ----------------------------------------------------------------------

    MSqlQuery query(MSqlQuery::InitCon());

    query.prepare("SELECT CONCAT_WS('/', music_directories.path, "
                  "music_albumart.filename) FROM music_albumart "
                  "LEFT JOIN music_directories ON "
                  "music_directories.directory_id=music_albumart.directory_id "
                  "WHERE music_albumart.albumart_id = :ARTID;");

    query.bindValue(":ARTID", nId );

    if (!query.exec())
        MythDB::DBError("Select ArtId", query);

    QString sMusicBasePath = gCoreContext->GetSetting("MusicLocation", "");

    if (query.next())
    {
        sFullFileName = QString( "%1/%2" )
                        .arg( sMusicBasePath )
                        .arg( query.value(0).toString() );
    }

    if (!QFile::exists( sFullFileName ))
        return QFileInfo();

    if ((nWidth == 0) && (nHeight == 0))
        return QFileInfo( sFullFileName );

    QString sNewFileName = QString( "%1.%2x%3.png" )
                              .arg( sFullFileName )
                              .arg( nWidth    )
                              .arg( nHeight   );

    // ----------------------------------------------------------------------
    // check to see if albumart image is already created.
    // ----------------------------------------------------------------------

    if (QFile::exists( sNewFileName ))
        return QFileInfo( sNewFileName );

    // ----------------------------------------------------------------------
    // Must generate Albumart Image, Generate Image and save.
    // ----------------------------------------------------------------------

    float fAspect = 0.0;

    QImage *pImage = new QImage( sFullFileName);

    if (!pImage)
        return QFileInfo();

    if (fAspect <= 0)
           fAspect = (float)(pImage->width()) / pImage->height();

    if ( nWidth == 0 )
        nWidth = (int)rint(nHeight * fAspect);

    if ( nHeight == 0 )
        nHeight = (int)rint(nWidth / fAspect);

    QImage img = pImage->scaled( nWidth, nHeight, Qt::IgnoreAspectRatio,
                                Qt::SmoothTransformation);

    QByteArray fname = sNewFileName.toAscii();
    img.save( fname.constData(), "PNG" );

    delete pImage;

    return QFileInfo( sNewFileName );
}

/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////

QFileInfo Content::GetPreviewImage(        int        nChanId,
                                     const QDateTime &dtStartTime,
                                           int        nWidth,
                                           int        nHeight,
                                           int        nSecsIn )
{
    if (!dtStartTime.isValid())
    {
        QString sMsg = QString("GetPreviewImage: bad start time '%1'")
                          .arg( dtStartTime.toString() );

        LOG(VB_GENERAL, LOG_ERR, sMsg);

        throw sMsg;
    }

    // ----------------------------------------------------------------------
    // Read Recording From Database
    // ----------------------------------------------------------------------

    ProgramInfo pginfo( (uint)nChanId, dtStartTime);

    if (!pginfo.GetChanID())
    {
        LOG(VB_GENERAL, LOG_ERR,
            QString( "GetPreviewImage: no recording for start time '%1'" )
                                 .arg( dtStartTime.toString() ));
        return QFileInfo();
    }

    if ( pginfo.GetHostname() != gCoreContext->GetHostName())
    {
        QString sMsg =
            QString("GetPreviewImage: Wrong Host '%1' request from '%2'")
                          .arg( gCoreContext->GetHostName())
                          .arg( pginfo.GetHostname() );

        LOG(VB_UPNP, LOG_ERR, sMsg);

        throw HttpRedirectException( pginfo.GetHostname() );
    }

    QString sFileName = GetPlaybackURL(&pginfo);

    // ----------------------------------------------------------------------
    // check to see if default preview image is already created.
    // ----------------------------------------------------------------------

    QString sPreviewFileName;

    if (nSecsIn <= 0)
    {
        nSecsIn = -1;
        sPreviewFileName = sFileName + ".png";
    }
    else
    {
        sPreviewFileName = QString("%1.%2.png").arg(sFileName).arg(nSecsIn);
    }

    if (!QFile::exists( sPreviewFileName ))
    {
        // ------------------------------------------------------------------
        // Must generate Preview Image, Generate Image and save.
        // ------------------------------------------------------------------
        if (!pginfo.IsLocal() && sFileName.left(1) == "/")
            pginfo.SetPathname(sFileName);

        if (!pginfo.IsLocal())
            return QFileInfo();

        PreviewGenerator *previewgen = new PreviewGenerator( &pginfo, 
                                                             QString(), 
                                                             PreviewGenerator::kLocal);
        previewgen->SetPreviewTimeAsSeconds( nSecsIn          );
        previewgen->SetOutputFilename      ( sPreviewFileName );

        bool ok = previewgen->Run();

        previewgen->deleteLater();

        if (!ok)
            return QFileInfo();
    }

    float fAspect = 0.0;

    QImage *pImage = new QImage(sPreviewFileName);

    if (!pImage)
        return QFileInfo();

    if (fAspect <= 0)
        fAspect = (float)(pImage->width()) / pImage->height();

    if (fAspect == 0)
    {
        delete pImage;
        return QFileInfo();
    }

    bool bDefaultPixmap = (nWidth == 0) && (nHeight == 0);

    if ( nWidth == 0 )
        nWidth = (int)rint(nHeight * fAspect);

    if ( nHeight == 0 )
        nHeight = (int)rint(nWidth / fAspect);

    QString sNewFileName;

    if (bDefaultPixmap)
        sNewFileName = sPreviewFileName;
    else
        sNewFileName = QString( "%1.%2.%3x%4.png" )
                          .arg( sFileName )
                          .arg( nSecsIn   )
                          .arg( nWidth    )
                          .arg( nHeight   );

    // ----------------------------------------------------------------------
    // check to see if scaled preview image is already created.
    // ----------------------------------------------------------------------

    if (QFile::exists( sNewFileName ))
    {
        delete pImage;
        return QFileInfo( sNewFileName );
    }

    PreviewGenerator *previewgen = new PreviewGenerator( &pginfo,
                                                         QString(),
                                                         PreviewGenerator::kLocal);
    previewgen->SetPreviewTimeAsSeconds( nSecsIn             );
    previewgen->SetOutputFilename      ( sNewFileName        );
    previewgen->SetOutputSize          (QSize(nWidth,nHeight));

    bool ok = previewgen->Run();

    previewgen->deleteLater();

    if (!ok)
        return QFileInfo();

    delete pImage;

    return QFileInfo( sNewFileName );
}

/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////

QFileInfo Content::GetRecording( int              nChanId,
                                 const QDateTime &dtStartTime )
{
    if (!dtStartTime.isValid())
        throw( "StartTime is invalid" );

    // ------------------------------------------------------------------
    // Read Recording From Database
    // ------------------------------------------------------------------

    ProgramInfo pginfo( (uint)nChanId, dtStartTime );

    if (!pginfo.GetChanID())
    {
        LOG( VB_UPNP, LOG_ERR, QString("GetRecording - for %1, %2 failed")
                                    .arg( nChanId )
                                    .arg( dtStartTime.toString() ));
        return QFileInfo();
    }

    if ( pginfo.GetHostname() != gCoreContext->GetHostName())
    {
        // We only handle requests for local resources

        QString sMsg =
            QString("GetRecording: Wrong Host '%1' request from '%2'.")
                          .arg( gCoreContext->GetHostName())
                          .arg( pginfo.GetHostname() );

        LOG(VB_UPNP, LOG_ERR, sMsg);

        throw HttpRedirectException( pginfo.GetHostname() );
    }

    QString sFileName( GetPlaybackURL(&pginfo) );

    // ----------------------------------------------------------------------
    // check to see if the file exists
    // ----------------------------------------------------------------------

    if (QFile::exists( sFileName ))
        return QFileInfo( sFileName );

    return QFileInfo();
}

/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////

QFileInfo Content::GetMusic( int nId )
{
    QString sBasePath = gCoreContext->GetSetting( "MusicLocation", "");
    QString sFileName;

    // ----------------------------------------------------------------------
    // Load Track's FileName
    // ----------------------------------------------------------------------

    MSqlQuery query(MSqlQuery::InitCon());

    if (query.isConnected())
    {
        query.prepare("SELECT CONCAT_WS('/', music_directories.path, "
                       "music_songs.filename) AS filename FROM music_songs "
                       "LEFT JOIN music_directories ON "
                         "music_songs.directory_id="
                         "music_directories.directory_id "
                       "WHERE music_songs.song_id = :KEY");

        query.bindValue(":KEY", nId );

        if (!query.exec())
        {
            MythDB::DBError("GetMusic()", query);
            return QFileInfo();
        }

        if (query.next())
        {
            sFileName = QString( "%1/%2" )
                           .arg( sBasePath )
                           .arg( query.value(0).toString() );
        }
    }

    // ----------------------------------------------------------------------
    // check to see if the file exists
    // ----------------------------------------------------------------------

    if (QFile::exists( sFileName ))
        return QFileInfo( sFileName );

    return QFileInfo();
}

/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////

QFileInfo Content::GetVideo( int nId )
{
    QString sFileName;

    // ----------------------------------------------------------------------
    // Load Track's FileName
    // ----------------------------------------------------------------------

    MSqlQuery query(MSqlQuery::InitCon());

    if (query.isConnected())
    {
        query.prepare("SELECT filename FROM videometadata WHERE intid = :KEY" );
        query.bindValue(":KEY", nId );

        if (!query.exec())
        {
            MythDB::DBError("GetVideo()", query);
            return QFileInfo();
        }

        if (query.next())
            sFileName = query.value(0).toString();
    }

    if (sFileName.isEmpty())
        return QFileInfo();

    if (!QFile::exists( sFileName ))
        return GetFile( "Videos", sFileName );

    return QFileInfo( sFileName );
}

QString Content::GetHash( const QString &sStorageGroup,
                          const QString &sFileName )
{
    if ((sFileName.isEmpty()) ||
        (sFileName.contains("/../")) ||
        (sFileName.startsWith("../")))
    {
        LOG(VB_GENERAL, LOG_ERR,
            QString("ERROR checking for file, filename '%1' "
                    "fails sanity checks").arg(sFileName));
        return QString();
    }

    QString storageGroup = "Default";

    if (!sStorageGroup.isEmpty())
        storageGroup = sStorageGroup;

    StorageGroup sgroup(storageGroup, gCoreContext->GetHostName());

    QString fullname = sgroup.FindFile(sFileName);
    QString hash = FileHash(fullname);

    if (hash == "NULL")
        return QString();

    return hash;
}

bool Content::DownloadFile( const QString &sURL, const QString &sStorageGroup )
{
    QFileInfo finfo(sURL);
    QString filename = finfo.fileName();
    StorageGroup sgroup(sStorageGroup, gCoreContext->GetHostName(), false);
    QString outDir = sgroup.FindNextDirMostFree();
    QString outFile;

    if (outDir.isEmpty())
    {
        LOG(VB_GENERAL, LOG_ERR,
            QString("Unable to determine directory "
                    "to write to in %1 write command").arg(sURL));
        return false;
    }

    if ((filename.contains("/../")) ||
        (filename.startsWith("../")))
    {
        LOG(VB_GENERAL, LOG_ERR,
            QString("ERROR: %1 write filename '%2' does not "
                    "pass sanity checks.") .arg(sURL).arg(filename));
        return false;
    }

    outFile = outDir + "/" + filename;

    if (GetMythDownloadManager()->download(sURL, outFile))
        return true;

    return false;
}

