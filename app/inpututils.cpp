/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "inpututils.h"

#include "qcoreapplication.h"
#include "qgsgeometrycollection.h"
#include "qgslinestring.h"
#include "qgspolygon.h"
#include "qgsvectorlayer.h"

#include "qgsquickutils.h"
#include "qgsquickmaptransform.h"
#include "inpututils.h"

#include <Qt>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <algorithm>

QString InputUtils::sLogFile = QStringLiteral();
static const QString DATE_TIME_FORMAT = QStringLiteral( "yyMMdd-hhmmss" );

InputUtils::InputUtils( QObject *parent ): QObject( parent )
{
  mAndroidUtils = std::unique_ptr<AndroidUtils>( new AndroidUtils() );
}

bool InputUtils::removeFile( const QString &filePath )
{
  QFile file( filePath );
  return file.remove( filePath );
}

bool InputUtils::copyFile( const QString &srcPath, const QString &dstPath )
{
  QString modSrcPath = srcPath;
  if ( srcPath.startsWith( "file://" ) )
  {
    modSrcPath = modSrcPath.replace( "file://", "" );
  }

  // https://github.com/lutraconsulting/input/issues/418
  // does not work for iOS files with format
  // file:assets-library://asset/asset.PNG%3Fid=A53AB989-6354-433A-9CB9-958179B7C14D&ext=PNG

  return QFile::copy( modSrcPath, dstPath );
}

QString InputUtils::getFileName( const QString &filePath )
{
  QFileInfo fileInfo( sanitizeName( filePath ) );
  return fileInfo.fileName();
}

QString InputUtils::sanitizeName( const QString &path )
{
#ifdef Q_OS_IOS
  QRegularExpression reAbs( "(.+)\\/asset\\.PNG%.Fid=(\\S+)&ext=" );
  QRegularExpressionMatch matchAbs = reAbs.match( path );
  if ( matchAbs.hasMatch() )
  {
    QString base = matchAbs.captured( 1 );
    QString name = matchAbs.captured( 2 );
    return base + "/" + name + ".png";
  }

  QRegularExpression reRel( "asset\\.PNG%.Fid=(\\S+)&ext=" );
  QRegularExpressionMatch matchRel = reRel.match( path );
  if ( matchRel.hasMatch() )
  {
    QString matched = matchRel.captured( 1 );
    return matched + ".png";
  }
#endif
  return path;
}

QString InputUtils::formatProjectName( const QString &fullProjectName )
{
  if ( fullProjectName.contains( "/" ) )
  {
    QStringList list = fullProjectName.split( "/" );
    if ( list.at( 0 ).isEmpty() )
    {
      return QString( "<b>%1</b>" ).arg( list.at( 1 ) );
    }
    else
    {
      return QString( "%1/<b>%2</b>" ).arg( list.at( 0 ) ).arg( list.at( 1 ) );
    }
  }
  else
  {
    return QString( "<b>%1</b>" ).arg( fullProjectName );
  }
}

QString InputUtils::formatNumber( const double number, int precision )
{
  return QString::number( number, 'f', precision );
}

void InputUtils::setExtentToFeature( const QgsQuickFeatureLayerPair &pair, QgsQuickMapSettings *mapSettings, double panelOffsetRatio )
{

  if ( !mapSettings )
    return;

  if ( !pair.layer() )
    return;

  if ( !pair.feature().isValid() )
    return;

  QgsGeometry geom = pair.feature().geometry();
  if ( geom.isNull() || !geom.constGet() )
    return;

  QgsRectangle bbox = mapSettings->mapSettings().layerExtentToOutputExtent( pair.layer(), geom.boundingBox() );
  QgsRectangle currentExtent = mapSettings->mapSettings().extent();
  QgsPointXY currentExtentCenter = currentExtent.center();
  QgsPointXY featureCenter = bbox.center();

  double panelOffset = ( currentExtent.yMaximum() - currentExtent.yMinimum() ) * panelOffsetRatio / 2;
  double offsetX = currentExtentCenter.x() - featureCenter.x();
  double offsetY = currentExtentCenter.y() - featureCenter.y();
  currentExtent.setXMinimum( currentExtent.xMinimum() - offsetX );
  currentExtent.setXMaximum( currentExtent.xMaximum() - offsetX );
  currentExtent.setYMinimum( currentExtent.yMinimum() - offsetY - panelOffset );
  currentExtent.setYMaximum( currentExtent.yMaximum() - offsetY - panelOffset );
  mapSettings->setExtent( currentExtent );
}


double InputUtils::mapSettingsScale( QgsQuickMapSettings *ms )
{
  if ( !ms ) return 1;
  return 1 / ms->mapUnitsPerPixel();
}

double InputUtils::mapSettingsOffsetX( QgsQuickMapSettings *ms )
{
  if ( !ms ) return 0;
  return -ms->visibleExtent().xMinimum();
}

double InputUtils::mapSettingsOffsetY( QgsQuickMapSettings *ms )
{
  if ( !ms ) return 0;
  return -ms->visibleExtent().yMaximum();
}


static void addLineString( const QgsLineString *line, QVector<double> &data )
{
  data << line->numPoints();
  const double *x = line->xData();
  const double *y = line->yData();
  for ( int i = 0; i < line->numPoints(); ++i )
  {
    data << x[i] << y[i];
  }
}

static void addSingleGeometry( const QgsAbstractGeometry *geom, QgsWkbTypes::GeometryType type, QVector<double> &data )
{
  switch ( type )
  {
    case QgsWkbTypes::PointGeometry:
    {
      const QgsPoint *point = qgsgeometry_cast<const QgsPoint *>( geom );
      if ( point )
      {
        data << 0 << point->x() << point->y();
      }
      break;
    }

    case QgsWkbTypes::LineGeometry:
    {
      const QgsLineString *line = qgsgeometry_cast<const QgsLineString *>( geom );
      if ( line )
      {
        data << 1;
        addLineString( line, data );
      }
      break;
    }

    case QgsWkbTypes::PolygonGeometry:
    {
      const QgsPolygon *poly = qgsgeometry_cast<const QgsPolygon *>( geom );
      if ( poly )
      {
        if ( const QgsLineString *line = qgsgeometry_cast<const QgsLineString *>( poly->exteriorRing() ) )
        {
          data << 2;
          addLineString( line, data );
        }
        for ( int i = 0; i < poly->numInteriorRings(); ++i )
        {
          if ( const QgsLineString *line = qgsgeometry_cast<const QgsLineString *>( poly->interiorRing( i ) ) )
          {
            data << 2;
            addLineString( line, data );
          }
        }
      }
      break;
    }

    case QgsWkbTypes::UnknownGeometry:
    case QgsWkbTypes::NullGeometry:
      break;
  }
}


QVector<double> InputUtils::extractGeometryCoordinates( const QgsQuickFeatureLayerPair &pair, QgsQuickMapSettings *mapSettings )
{
  if ( !mapSettings || !pair.isValid() )
    return QVector<double>();

  QgsGeometry g = pair.feature().geometry();

  QgsCoordinateTransform ct( pair.layer()->crs(), mapSettings->destinationCrs(), mapSettings->transformContext() );
  if ( !ct.isShortCircuited() )
  {
    try
    {
      g.transform( ct );
    }
    catch ( QgsCsException &e )
    {
      Q_UNUSED( e )
      return QVector<double>();
    }
  }

  QVector<double> data;

  const QgsAbstractGeometry *geom = g.constGet();
  QgsWkbTypes::GeometryType geomType = g.type();
  const QgsGeometryCollection *collection = qgsgeometry_cast<const QgsGeometryCollection *>( geom );
  if ( collection && !collection->isEmpty() )
  {
    for ( int i = 0; i < collection->numGeometries(); ++i )
    {
      addSingleGeometry( collection->geometryN( i ), geomType, data );
    }
  }
  else
  {
    addSingleGeometry( geom, geomType, data );
  }

  return data;
}

void InputUtils::setLogFilename( const QString &value )
{
  sLogFile = value;
}

QString InputUtils::logFilename()
{
  return sLogFile;
}

bool InputUtils::createEmptyFile( const QString &filePath )
{
  QFile newFile( filePath );
  if ( !newFile.open( QIODevice::WriteOnly ) )
    return false;

  newFile.close();
  return true;
}

QString InputUtils::filesToString( QList<MerginFile> files )
{
  QStringList resultList;
  for ( MerginFile file : files )
  {
    resultList << file.path;
  }
  return resultList.join( ", " );
}

QString InputUtils::appInfo()
{
  return QString( "%1/%2 (%3/%4)" ).arg( QCoreApplication::applicationName() ).arg( QCoreApplication::applicationVersion() )
         .arg( QSysInfo::productType() ).arg( QSysInfo::productVersion() );
}

QString InputUtils::bytesToHumanSize( double bytes )
{
  const int precision = 1;
  if ( bytes < 1e-5 )
  {
    return "0.0";
  }
  else if ( bytes < 1024.0 * 1024.0 )
  {
    return QString::number( bytes, 'f', precision ) + " KB";
  }
  else if ( bytes < 1024.0 * 1024.0 * 1024.0 )
  {
    return QString::number( bytes / 1024.0 / 1024.0, 'f', precision ) + " MB";
  }
  else if ( bytes < 1024.0 * 1024.0 * 1024.0 * 1024.0 )
  {
    return QString::number( bytes / 1024.0 / 1024.0 / 1024.0, 'f', precision ) + " GB";
  }
  else
  {
    return QString::number( bytes / 1024.0 / 1024.0 / 1024.0 / 1024.0, 'f', precision ) + " TB";
  }
}

bool InputUtils::hasStoragePermission()
{
  if ( appPlatform() == QStringLiteral( "android" ) )
  {
    return mAndroidUtils->checkPermission( "android.permission.WRITE_EXTERNAL_STORAGE" );
  }
  return true;
}

bool InputUtils::acquireStoragePermission()
{
  if ( appPlatform() == QStringLiteral( "android" ) )
  {
    return mAndroidUtils->requestStoragePermission();
  }
  return true;
}

bool InputUtils::acquireCameraPermission()
{
  if ( appPlatform() == QStringLiteral( "android" ) )
  {
    return mAndroidUtils->requestCameraPermission();
  }
  return true;
}

void InputUtils::quitApp()
{
  QCoreApplication::quit();
}

QString InputUtils::uuidWithoutBraces( const QUuid &uuid )
{
#if QT_VERSION >= QT_VERSION_CHECK( 5, 11, 0 )
  return uuid.toString( QUuid::WithoutBraces );
#else
  QString str = uuid.toString();
  str = str.mid( 1, str.length() - 2 );  // remove braces
  return str;
#endif
}

QString InputUtils::localizedDateFromUTFString( QString timestamp )
{
  if ( timestamp.isEmpty() )
    return QString();

  QDateTime dateTime = QDateTime::fromString( timestamp, Qt::ISODate );
  if ( dateTime.isValid() )
  {
    return dateTime.date().toString( Qt::DefaultLocaleShortDate );
  }
  else
  {
    qDebug() << "Unable to convert UTF " << timestamp << " to QDateTime";
    return QString();
  }
}

QString InputUtils::appVersion()
{
  QString version;
#ifdef INPUT_VERSION
  version = STR( INPUT_VERSION );
#endif
  return version;
}

QString InputUtils::appPlatform()
{
#if defined( ANDROID )
  const QString platform = "android";
#elif defined( Q_OS_IOS )
  const QString platform = "ios";
#elif defined( Q_OS_WIN32 )
  const QString platform = "win";
#elif defined( Q_OS_LINUX )
  const QString platform = "linux";
#elif defined( Q_OS_MAC )
  const QString platform = "macos";
#else
  const QString platform = "unknown";
#endif
  return platform;
}


QString InputUtils::findUniquePath( const QString &path, bool isPathDir )
{
  QFileInfo pathInfo( path );
  if ( pathInfo.exists() )
  {
    int i = 0;
    QFileInfo info( path + QString::number( i ) );
    while ( info.exists() && ( info.isDir() || !isPathDir ) )
    {
      ++i;
      info.setFile( path + QString::number( i ) );
    }
    return path + QString::number( i );
  }
  else
  {
    return path;
  }
}


QString InputUtils::createUniqueProjectDirectory( const QString &baseDataDir, const QString &projectName )
{
  QString projectDirPath = findUniquePath( baseDataDir + "/" + projectName );
  QDir projectDir( projectDirPath );
  if ( !projectDir.exists() )
  {
    QDir dir( "" );
    dir.mkdir( projectDirPath );
  }
  return projectDirPath;
}

void InputUtils::onQgsLogMessageReceived( const QString &message, const QString &tag, Qgis::MessageLevel level )
{
  QString levelStr;
  switch ( level )
  {
    case Qgis::MessageLevel::Warning:
      levelStr = "Warning";
      break;
    case Qgis::MessageLevel::Critical:
      levelStr = "Error";
      break;
    default:
      break;
  }

  log( "QGIS " + tag, levelStr + ": " + message );
}

bool InputUtils::cpDir( const QString &srcPath, const QString &dstPath, bool onlyDiffable )
{
  bool result  = true;
  QDir parentDstDir( QFileInfo( dstPath ).path() );
  if ( !parentDstDir.mkpath( dstPath ) )
  {
    log( "cpDir", QString( "Cannot make path %1" ).arg( dstPath ) );
    return false;
  }

  QDir srcDir( srcPath );
  const QFileInfoList fileInfoList = srcDir.entryInfoList( QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden );
  foreach ( const QFileInfo &info, fileInfoList )
  {
    QString srcItemPath = srcPath + "/" + info.fileName();
    QString dstItemPath = dstPath + "/" + info.fileName();
    if ( info.isDir() )
    {
      if ( !cpDir( srcItemPath, dstItemPath ) )
      {
        log( "cpDir", QString( "Cannot copy a dir from %1 to %2" ).arg( srcItemPath ).arg( dstItemPath ) );
        result = false;
      }
    }
    else if ( info.isFile() )
    {
      if ( onlyDiffable && !MerginApi::isFileDiffable( info.fileName() ) )
        continue;

      if ( !QFile::copy( srcItemPath, dstItemPath ) )
      {
        if ( !QFile::remove( dstItemPath ) )
        {
          log( "cpDir", QString( "Cannot remove a file from %1" ).arg( dstItemPath ) );
          result =  false;
        }
        if ( !QFile::copy( srcItemPath, dstItemPath ) )
        {
          log( "cpDir", QString( "Cannot overwrite a file %1 with %2" ).arg( dstItemPath ).arg( dstItemPath ) );
          result =  false;
        }
      }
      QFile::setPermissions( dstItemPath, QFile::ReadUser | QFile::WriteUser | QFile::ReadOwner | QFile::WriteOwner );
    }
    else
    {
      log( "cpDir", QString( "Unhandled item %1 in cpDir" ).arg( info.filePath() ) );
    }
  }
  return result;
}

QString InputUtils::renameWithDateTime( const QString &srcPath, const QDateTime &dateTime )
{
  if ( QFile::exists( srcPath ) )
  {
    QFileInfo info( srcPath );
    QString timestamp = ( dateTime.isValid() ) ? dateTime.toString( DATE_TIME_FORMAT ) : QDateTime::currentDateTime().toString( DATE_TIME_FORMAT );
    QString newFilename = QString( "%1.%2" ).arg( timestamp ).arg( info.suffix() );
    QString newPath( info.absolutePath() + "/" + newFilename );

    if ( QFile::rename( srcPath, newPath ) ) return newPath;
  }

  return QString();
}

QString InputUtils::downloadInProgressFilePath( const QString &projectDir )
{
  return projectDir + "/.mergin/.project.downloading";
}

void InputUtils::showNotification( const QString &message )
{
  emit showNotificationRequested( message );
}

qreal InputUtils::groundSpeedFromSource( QgsQuickPositionKit *positionKit )
{
  if ( positionKit == nullptr ) return 0;

  if ( positionKit->source()->lastKnownPosition().isValid() )
  {
    return positionKit->source()->lastKnownPosition().attribute( QGeoPositionInfo::Attribute::GroundSpeed );
  }
  return 0;
}

void InputUtils::log( const QString &topic, const QString &info )
{
  QString logFilePath;
  QByteArray data;
  data.append( QString( "%1 %2: %3\n" ).arg( QDateTime().currentDateTimeUtc().toString( Qt::ISODateWithMs ) ).arg( topic ).arg( info ) );

  qDebug() << data;
  appendLog( data, sLogFile );
}

void InputUtils::appendLog( const QByteArray &data, const QString &path )
{
  QFile file( path );
  if ( !file.open( QIODevice::Append ) )
  {
    return;
  }

  file.write( data );
  file.close();
}
