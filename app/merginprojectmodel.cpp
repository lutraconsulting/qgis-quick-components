/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "merginprojectmodel.h"
#include "inpututils.h"

#include <QString>

MerginProjectModel::MerginProjectModel( LocalProjectsManager &localProjects, QObject *parent )
  : QAbstractListModel( parent )
  , mLocalProjects( localProjects )
{
  QObject::connect( &mLocalProjects, &LocalProjectsManager::projectMetadataChanged, this, &MerginProjectModel::projectMetadataChanged );
  QObject::connect( &mLocalProjects, &LocalProjectsManager::localProjectAdded, this, &MerginProjectModel::onLocalProjectAdded );
  QObject::connect( &mLocalProjects, &LocalProjectsManager::localProjectRemoved, this, &MerginProjectModel::onLocalProjectRemoved );

  mAdditionalItem->status = NonProjectItem;
}

QVariant MerginProjectModel::data( const QModelIndex &index, int role ) const
{
  int row = index.row();
  if ( row < 0 || row >= mMerginProjects.count() )
    return QVariant();

  const MerginProject *project = mMerginProjects.at( row ).get();

  switch ( role )
  {
    case ProjectName:
      return QVariant( project->projectName );
    case ProjectNamespace: return QVariant( project->projectNamespace );
    case ProjectInfo:
    {
      // TODO: better project info
      // TODO: clientUpdated currently not being set
      if ( !project->clientUpdated.isValid() )
      {
        return project->serverUpdated.toLocalTime().toString();
      }
      else
      {
        return project->clientUpdated.toLocalTime().toString();
      }
    }

    case Status: return QVariant( InputUtils::statusToString( project->status ) );
    case Pending: return QVariant( project->pending );
    case PassesFilter: return mSearchExpression.isEmpty() || project->projectName.contains( mSearchExpression, Qt::CaseInsensitive )
                                || project->projectNamespace.contains( mSearchExpression, Qt::CaseInsensitive );
    case SyncProgress: return QVariant( project->progress );
  }

  return QVariant();
}


QHash<int, QByteArray> MerginProjectModel::roleNames() const
{
  QHash<int, QByteArray> roleNames = QAbstractListModel::roleNames();
  roleNames[ProjectName] = "projectName";
  roleNames[ProjectNamespace] = "projectNamespace";
  roleNames[ProjectInfo] = "projectInfo";
  roleNames[Status] = "status";
  roleNames[Pending] = "pendingProject";
  roleNames[PassesFilter] = "passesFilter";
  roleNames[SyncProgress] = "syncProgress";
  return roleNames;
}

ProjectList MerginProjectModel::projects()
{
  return mMerginProjects;
}

int MerginProjectModel::rowCount( const QModelIndex &parent ) const
{
  Q_UNUSED( parent )
  return mMerginProjects.count();
}

void MerginProjectModel::updateModel( const MerginProjectList &merginProjects, QHash<QString, TransactionStatus> pendingProjects, int expectedProjectCount, int page )
{
  beginResetModel();
  mMerginProjects.removeOne( mAdditionalItem );

  if ( page == 1 )
  {
    mMerginProjects.clear();
  }
  setLastPage( page );

  for ( MerginProjectListEntry entry : merginProjects )
  {
    QString fullProjectName = MerginApi::getFullProjectName( entry.projectNamespace, entry.projectName );
    std::shared_ptr<MerginProject> project = std::make_shared<MerginProject>();
    project->projectNamespace = entry.projectNamespace;
    project->projectName = entry.projectName;
    project->serverUpdated = entry.serverUpdated;

    // figure out info from local projects (projectDir etc)
    LocalProjectInfo localProject = mLocalProjects.projectFromMerginName( entry.projectNamespace, entry.projectName );
    if ( localProject.isValid() )
    {
      project->projectDir = localProject.projectDir;
      project->status = localProject.status;
      // TODO: what else to copy?
    }

    if ( pendingProjects.contains( fullProjectName ) )
    {

      TransactionStatus projectTransaction = pendingProjects.value( fullProjectName );
      project->progress = projectTransaction.transferedSize / projectTransaction.totalSize;
      project->pending = true;
    }

    mMerginProjects << project;
  }

  if ( mMerginProjects.count() < expectedProjectCount )
  {
    mMerginProjects << mAdditionalItem;
  }

  endResetModel();
}

int MerginProjectModel::findProjectIndex( const QString &projectFullName )
{
  int row = 0;
  for ( std::shared_ptr<MerginProject> project : mMerginProjects )
  {
    if ( MerginApi::getFullProjectName( project->projectNamespace, project->projectName ) == projectFullName )
      return row;
    row++;
  }
  return -1;
}

void MerginProjectModel::setLastPage( int lastPage )
{
  mLastPage = lastPage;
  emit lastPageChanged();
}

int MerginProjectModel::lastPage() const
{
  return mLastPage;
}

QString MerginProjectModel::searchExpression() const
{
  return mSearchExpression;
}

void MerginProjectModel::setSearchExpression( const QString &searchExpression )
{
  if ( searchExpression != mSearchExpression )
  {
    mSearchExpression = searchExpression;
    // Hack to model changed signal
    beginResetModel();
    endResetModel();
  }
}

void MerginProjectModel::syncProjectStatusChanged( const QString &projectFullName, qreal progress )
{
  int row = findProjectIndex( projectFullName );
  if ( row == -1 )
    return;

  std::shared_ptr<MerginProject> project = mMerginProjects[row];
  project->pending = progress >= 0;
  project->progress = progress >= 0 ? progress : 0;

  QModelIndex ix = index( row );
  emit dataChanged( ix, ix );
}


void MerginProjectModel::projectMetadataChanged( const QString &projectDir )
{
  int row = 0;
  for ( std::shared_ptr<MerginProject> project : mMerginProjects )
  {
    if ( project->projectDir == projectDir )
    {
      LocalProjectInfo localProject = mLocalProjects.projectFromDirectory( projectDir );
      if ( !localProject.isValid() )
        return;

      // update cached information
      project->status = localProject.status;

      QModelIndex ix = index( row );
      emit dataChanged( ix, ix );
      return;
    }
    row++;
  }
}

void MerginProjectModel::onLocalProjectAdded( const QString &projectDir )
{
  LocalProjectInfo localProject = mLocalProjects.projectFromDirectory( projectDir );
  if ( !localProject.isValid() )
    return;

  QString projectFullName = MerginApi::getFullProjectName( localProject.projectNamespace, localProject.projectName );
  int i = findProjectIndex( projectFullName );
  if ( i == -1 )
    return;

  std::shared_ptr<MerginProject> project = mMerginProjects[i];

  // store project dir
  project->projectDir = localProject.projectDir;

  // update metadata and emit dataChanged() signal
  projectMetadataChanged( projectDir );
}

void MerginProjectModel::onLocalProjectRemoved( const QString &projectDir )
{
  // TODO: implement
  // (at this point this is not needed because after removal we need to switch tab
  // and that will re-fetch the list of projects)
  Q_UNUSED( projectDir );
}

