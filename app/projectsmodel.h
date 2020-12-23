/***************************************************************************
  qgsquicklayertreemodel.h
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


#ifndef PROJECTSMODEL_H
#define PROJECTSMODEL_H

#include <QAbstractListModel>
#include <QString>
#include <QModelIndex>

class LocalProjectsManager;

/*
 * Given data directory, find all QGIS projects (*.qgs or *.qgz) in the directory and subdirectories
 * and create list model from them. Available are full path to the file, name of the project
 * and short name of the project (clipped to N chars)
 */
class ProjectModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY( QString dataDir READ dataDir ) // never changes
    Q_PROPERTY( QString searchExpression READ searchExpression WRITE setSearchExpression )

  public:
    enum Roles
    {
      ProjectName = Qt::UserRole + 1, // name of a project file
      ProjectNamespace,
      FolderName,
      Path,
      ProjectInfo,
      Size,
      IsValid,
      PassesFilter
    };
    Q_ENUMS( Roles )

    explicit ProjectModel( LocalProjectsManager &localProjects, QObject *parent = nullptr );
    ~ProjectModel() override;

    Q_INVOKABLE QVariant data( const QModelIndex &index, int role ) const override;
    Q_INVOKABLE QModelIndex index( int row, int column = 0, const QModelIndex &parent = QModelIndex() ) const override;
    Q_INVOKABLE int rowAccordingPath( QString path ) const;
    Q_INVOKABLE void deleteProject( int row );

    Q_INVOKABLE QVariantList allDataForRole();

    QHash<int, QByteArray> roleNames() const override;

    int rowCount( const QModelIndex &parent = QModelIndex() ) const override;

    QString dataDir() const;

    QString searchExpression() const;
    void setSearchExpression( const QString &searchExpression );

    // Test function
    bool containsProject( const QString &projectNamespace, const QString &projectName );

  public slots:
    void syncedProjectFinished( const QString &projectDir, const QString &projectFullName, bool successfully );
    void findProjectFiles();

  private:
    void reloadProjectFiles( QString projectFolder, QString projectName, bool successful );

    struct ProjectFile
    {
      QString projectName;        //!< mergin project name (second part of "namespace/project"). empty for non-mergin project
      QString projectNamespace;   //!< mergin project namespace (first part of "namespace/project"). empty for non-mergin project
      QString folderName;         //!< name of the project folder (not the full path)
      QString path;               //!< path to the .qgs/.qgz project file
      QString info;
      bool isValid;

      /**
       * Ordering of local projects: first non-mergin projects (using folder name),
       * then mergin projects (sorted first by namespace, then project name)
       */
      bool operator < ( const ProjectFile &other ) const
      {
        if ( projectNamespace.isEmpty() && other.projectNamespace.isEmpty() )
        {
          return folderName.compare( other.folderName, Qt::CaseInsensitive ) < 0;
        }
        if ( !projectNamespace.isEmpty() && other.projectNamespace.isEmpty() )
        {
          return false;
        }
        if ( projectNamespace.isEmpty() && !other.projectNamespace.isEmpty() )
        {
          return true;
        }

        if ( projectNamespace.compare( other.projectNamespace, Qt::CaseInsensitive ) == 0 )
        {
          return projectName.compare( other.projectName, Qt::CaseInsensitive ) < 0;
        }
        if ( projectNamespace.compare( other.projectNamespace, Qt::CaseInsensitive ) < 0 )
        {
          return true;
        }
        else
          return false;
      }
    };
    LocalProjectsManager &mLocalProjects;
    QList<ProjectFile> mProjectFiles;
    QString mSearchExpression;

};

#endif // PROJECTSMODEL_H
