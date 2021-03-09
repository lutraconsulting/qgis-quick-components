/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef INPUTUTILS_H
#define INPUTUTILS_H

#define STR1(x)  #x
#define STR(x)  STR1(x)

#include <QObject>
#include <QtGlobal>
#include <QUuid>
#include "inputhelp.h"
#include "merginapi.h"
#include "androidutils.h"
#include "qgsquickfeaturelayerpair.h"
#include "qgsquickmapsettings.h"
#include "qgsquickpositionkit.h"
#include "qgis.h"

class InputUtils: public QObject
{
    Q_OBJECT
  public:
    explicit InputUtils( QObject *parent = nullptr );
    ~InputUtils() = default;

    Q_INVOKABLE bool removeFile( const QString &filePath );
    Q_INVOKABLE bool copyFile( const QString &srcPath, const QString &dstPath );

    Q_INVOKABLE QString getFileName( const QString &filePath );
    Q_INVOKABLE QString formatProjectName( const QString &fullProjectName );

    Q_INVOKABLE void setExtentToFeature( const QgsQuickFeatureLayerPair &pair, QgsQuickMapSettings *mapSettings, double panelOffsetRatio );

    // utility functions to extract information from map settings
    // (in theory this data should be directly available from QgsQuick.MapTransform
    // but they are not currently, so this is a workaround we need for display of markers)

    Q_INVOKABLE double mapSettingsScale( QgsQuickMapSettings *ms );
    Q_INVOKABLE double mapSettingsOffsetX( QgsQuickMapSettings *ms );
    Q_INVOKABLE double mapSettingsOffsetY( QgsQuickMapSettings *ms );

    /**
     * Extract geometry coordinates from the given feature.
     *
     * The output can encode also multi-part geometries or even geometry collections.
     * We pass a single array out of the function, so this is the encoding of coordinates:
     * - point:  <value 0> <x> <y>
     * - linestring: <value 1> <number of points> <x1> <y1> ... <xn> <yn>
     * - polygon: <value 2> <number of points> <x1> <y1> ... <xn> <yn>
     *
     * The output is a chain of sub-geometries. Polygon's holes (interior rings) are treated just
     * like exterior ring because when we create a singla path, Qt automatically detects which rings
     * are holes by using even-odd fill rule.
     *
     * If the layer's CRS is not the same as map CRS, the geometry will be first transformed to map CRS.
     */
    Q_INVOKABLE QVector<double> extractGeometryCoordinates( const QgsQuickFeatureLayerPair &pair, QgsQuickMapSettings *mapSettings );

    /**
     * Renames a file located at a given path with a dateTime. Tend to be use to avoid name conflicts.
     * \param srcPath Absolute path to a file.
     * \param dateTime new name of a file. If is invalid, current dateTime is used.
     * \result Either absolute path of a rename file or empty string.
     */
    Q_INVOKABLE static QString renameWithDateTime( const QString &srcPath, const QDateTime &dateTime = QDateTime() );

    /**
     * Returns name of temporary file indicating first time download of project is in progress
     * \param projectName
     */
    static QString downloadInProgressFilePath( const QString &projectDir );

    /**
     * Shows notification
     */
    Q_INVOKABLE void showNotification( const QString &message );

    /**
     * Returns speed from positionKit's QGeoPositionInfo.
     * \param QgsQuickPositionKit positionKit.
     * \result The ground speed, in meters/sec.
     */
    Q_INVOKABLE qreal groundSpeedFromSource( QgsQuickPositionKit *positionKit );

    /**
     * Converts bytes to  human readable size (e.g. 1GB, 500MB)
     */
    Q_INVOKABLE static QString bytesToHumanSize( double bytes );

    Q_INVOKABLE bool hasStoragePermission();

    Q_INVOKABLE bool acquireStoragePermission();

    Q_INVOKABLE bool acquireCameraPermission();

    Q_INVOKABLE void quitApp();

    /**
     * Method copies all entries from given source path to destination path. If cannot copy a file for the first time,
     * removes it and tries again (overwrite a file). If failes again, skips the file, sets result to false and continue.
     * \param srcPath Source path
     * \param dstPath Destination path
     * \result True if operation was fully successful otherwise false.
     */
    static bool cpDir( const QString &srcPath, const QString &dstPath, bool onlyDiffable = false );

    /**
     * Add a log entry to internal log text file
     *
     * \see setLogFilename()
     */
    static void log( const QString &topic, const QString &info );

    /**
     * Sets the filename of the internal text log file
     */
    static void setLogFilename( const QString &value );

    static QString logFilename();

    static bool createEmptyFile( const QString &filePath );

    static QString filesToString( QList<MerginFile> files );

    static QString appInfo();

    static QString uuidWithoutBraces( const QUuid &uuid );

    static QString localizedDateFromUTFString( QString timestamp );

    /** InputApp version */
    static QString appVersion();

    /** InputApp platform */
    static QString appPlatform();

    /**
    * Returns given path if doesn't exists, otherwise the slightly modified non-existing path by adding a number to given path.
    * \param QString path
    * \param QString isPathDir True if the result path suppose to be a folder
    */
    static QString findUniquePath( const QString &path, bool isPathDir = true );

    //! Creates a unique project directory for given project name (used for initial download of a project)
    static QString createUniqueProjectDirectory( const QString &baseDataDir, const QString &projectName );

  signals:
    Q_INVOKABLE void showNotificationRequested( const QString &message );

  public slots:
    void onQgsLogMessageReceived( const QString &message, const QString &tag, Qgis::MessageLevel level );

  private:

    // on iOS the names from gallery pickers are like
    // file:assets-library://asset/asset.PNG%3Fid=A53AB989-6354-433A-9CB9-958179B7C14D&ext=PNG
    // we need to change it to something more readable
    QString sanitizeName( const QString &path );
    static QString sLogFile;
    static void appendLog( const QByteArray &data, const QString &path );
    std::unique_ptr<AndroidUtils> mAndroidUtils;
};

#endif // INPUTUTILS_H
