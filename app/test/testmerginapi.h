/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef TESTMERGINAPI_H
#define TESTMERGINAPI_H

#include <QObject>

#include <qgsvectorlayer.h>
#include <merginapi.h>
#include <projectsmodel.h>
#include <merginprojectmodel.h>

#include <qgsapplication.h>

class TestMerginApi: public QObject
{
    Q_OBJECT
  public:
    explicit TestMerginApi( MerginApi *api, MerginProjectModel *mpm, ProjectModel *pm );
    ~TestMerginApi() = default;

    static const QString TEST_PROJECT_NAME;
    static const QString TEST_EMPTY_FILE_NAME;

  private slots:

    // global init + cleanup functions
    void initTestCase();
    void cleanupTestCase();

    void testListProject();
    void testDownloadProject();
    void testCancelDownloadProject();
    void testCreateProjectTwice();
    void testDeleteNonExistingProject();
    void testCreateDeleteProject();
    void testUploadProject();
    void testMultiChunkUploadDownload();
    void testEmptyFileUploadDownload();
    void testPushAddedFile();
    void testPushRemovedFile();
    void testPushModifiedFile();
    void testPushNoChanges();
    void testUpdateAddedFile();
    void testUpdateRemovedFiles();
    void testUpdateRemovedVsModifiedFiles();
    void testConflictRemoteUpdateLocalUpdate();
    void testConflictRemoteAddLocalAdd();
    void testUploadWithUpdate();
    void testDiffUpload();
    void testDiffSubdirsUpload();
    void testDiffUpdateBasic();
    void testDiffUpdateWithRebase();
    void testDiffUpdateWithRebaseFailed();
    void testUpdateWithDiffs();
    void testUpdateWithMissedVersion();
    void testMigrateProject();
    void testMigrateDetachProject();

    void testRegister();

  private:
    MerginApi *mApi;
    MerginProjectModel *mMerginProjectModel;
    ProjectModel *mProjectModel;
    QString mUsername;
    QString mTestDataPath;
    //! extra API to do requests we are not testing (as if some other user did those)
    MerginApi *mApiExtra = nullptr;
    LocalProjectsManager *mLocalProjectsExtra = nullptr;

    MerginProjectList getProjectList();

    //! Creates a project on the server and pushes an initial version and removes the local copy.
    void createRemoteProject( MerginApi *api, const QString &projectNamespace, const QString &projectName, const QString &sourcePath );
    //! Deletes a project on the server
    void deleteRemoteProject( MerginApi *api, const QString &projectNamespace, const QString &projectName );

    //! Downloads a remote project to the local drive
    void downloadRemoteProject( MerginApi *api, const QString &projectNamespace, const QString &projectName );

    //! Uploads any local changes in the local project to the remote project
    void uploadRemoteProject( MerginApi *api, const QString &projectNamespace, const QString &projectName );

    //! Deletes a project from the local drive
    void deleteLocalProject( MerginApi *api, const QString &projectNamespace, const QString &projectName );

    //! Write all of "data" as the content to the given filename
    void writeFileContent( const QString &filename, const QByteArray &data );
    //! Read content of the given filename
    QByteArray readFileContent( const QString &filename );

    //! Creates local project in given project directory
    void createLocalProject( const QString projectDir );
};

# endif // TESTMERGINAPI_H
