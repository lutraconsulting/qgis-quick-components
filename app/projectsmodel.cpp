/***************************************************************************
  qgsquicklayertreemodel.cpp
  --------------------------------------
  Date                 : Nov 2017
  Copyright            : (C) 2017 by Peter Petrik
  Email                : zilolv at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "projectsmodel.h"

#include <QString>
#include <QDirIterator>
#include <QDebug>
#include <QDateTime>

#include "inpututils.h"
#include "merginapi.h"

ProjectModel::ProjectModel( LocalProjectsManager &localProjects, QObject *parent )
  : QAbstractListModel( parent )
  , mLocalProjects( localProjects )
{
  findProjectFiles();
}

ProjectModel::~ProjectModel() {}

void ProjectModel::findProjectFiles()
{
  beginResetModel();
  // populate from mLocalProjects
  mProjectFiles.clear();
  const QList<LocalProjectInfo> projects = mLocalProjects.projects();
  for ( const LocalProjectInfo &project : projects )
  {
    QDir dir( project.projectDir );
    QFileInfo fi( project.qgisProjectFilePath );

    ProjectFile projectFile;
    projectFile.path = project.qgisProjectFilePath;
    projectFile.folderName = dir.dirName();
    projectFile.projectName = project.projectName;
    projectFile.projectNamespace = project.projectNamespace;
    projectFile.isValid = project.isShowable();
    QDateTime created = fi.created().toLocalTime();
    if ( projectFile.isValid )
    {
      projectFile.info = QString( created.toString() );
    }
    else
    {
      projectFile.info = project.qgisProjectError;
    }
    mProjectFiles << projectFile;
  }

  std::sort( mProjectFiles.begin(), mProjectFiles.end() );
  endResetModel();
}


QVariant ProjectModel::data( const QModelIndex &index, int role ) const
{
  int row = index.row();
  if ( row < 0 || row >= mProjectFiles.count() )
    return QVariant( "" );

  const ProjectFile &projectFile = mProjectFiles.at( row );

  switch ( role )
  {
    case ProjectName: return QVariant( projectFile.projectName );
    case ProjectNamespace: return QVariant( projectFile.projectNamespace );
    case FolderName: return QVariant( projectFile.folderName );
    case Path: return QVariant( projectFile.path );
    case ProjectInfo: return QVariant( projectFile.info );
    case IsValid: return QVariant( projectFile.isValid );
    case PassesFilter: return mSearchExpression.isEmpty() || projectFile.folderName.contains( mSearchExpression, Qt::CaseInsensitive );
  }

  return QVariant();
}

QHash<int, QByteArray> ProjectModel::roleNames() const
{
  QHash<int, QByteArray> roleNames = QAbstractListModel::roleNames();
  roleNames[ProjectName] = "projectName";
  roleNames[ProjectNamespace] = "projectNamespace";
  roleNames[FolderName] = "folderName";
  roleNames[Path] = "path";
  roleNames[ProjectInfo] = "projectInfo";
  roleNames[IsValid] = "isValid";
  roleNames[PassesFilter] = "passesFilter";
  return roleNames;
}

QModelIndex ProjectModel::index( int row, int column, const QModelIndex &parent ) const
{
  Q_UNUSED( column );
  Q_UNUSED( parent );
  return createIndex( row, 0, nullptr );
}

int ProjectModel::rowAccordingPath( QString path ) const
{
  int i = 0;
  for ( ProjectFile prj : mProjectFiles )
  {
    if ( prj.path == path )
    {
      return i;
    }
    i++;
  }
  return -1;
}

void ProjectModel::deleteProject( int row )
{
  if ( row < 0 || row >= mProjectFiles.length() )
  {
    InputUtils::log( "Deleting local project error", QStringLiteral( "Unable to delete local project, index out of bounds" ) );
    return;
  }

  ProjectFile project = mProjectFiles.at( row );

  mLocalProjects.deleteProjectDirectory( mLocalProjects.dataDir() + "/" + project.folderName );

  beginResetModel();
  mProjectFiles.removeAt( row );
  endResetModel();
}

QVariantList ProjectModel::allDataForRole()
{
  QVariantList data;

  for ( auto &proj : mProjectFiles )
  {
    data << MerginApi::getFullProjectName( proj.projectNamespace, proj.projectName );
  }

  return data;
}

int ProjectModel::rowCount( const QModelIndex &parent ) const
{
  Q_UNUSED( parent );
  return mProjectFiles.count();
}

QString ProjectModel::dataDir() const
{
  return mLocalProjects.dataDir();
}

QString ProjectModel::searchExpression() const
{
  return mSearchExpression;
}

void ProjectModel::setSearchExpression( const QString &searchExpression )
{
  if ( searchExpression != mSearchExpression )
  {
    mSearchExpression = searchExpression;
    // Hack to model changed signal
    beginResetModel();
    endResetModel();
  }
}

bool ProjectModel::containsProject( const QString &projectNamespace, const QString &projectName )
{
  return mLocalProjects.hasMerginProject( projectNamespace, projectName );
}

void ProjectModel::syncedProjectFinished( const QString &projectDir, const QString &projectFullName, bool successfully )
{

  // Do basic validity check
  if ( successfully )
  {
    QString errMsg;
    mLocalProjects.findQgisProjectFile( projectDir, errMsg );
    mLocalProjects.updateProjectErrors( projectDir, errMsg );
  }

  reloadProjectFiles( projectDir, projectFullName, successfully );
}

void ProjectModel::reloadProjectFiles( QString projectFolder, QString projectName, bool successful )
{
  if ( !successful ) return;

  if ( projectFolder.isEmpty() ) return;

  Q_UNUSED( projectName );
  beginResetModel();
  findProjectFiles();
  endResetModel();
}
