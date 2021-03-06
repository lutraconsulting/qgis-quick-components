/***************************************************************************
 qgsquickattributemodel.cpp
  --------------------------------------
  Date                 : 10.12.2014
  Copyright            : (C) 2014 by Matthias Kuhn
  Email                : matthias@opengis.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgis.h"
#include "qgsmessagelog.h"
#include "qgsvectorlayer.h"

#include "qgsquickattributemodel.h"
#include "qgsvectorlayereditbuffer.h"

QgsQuickAttributeModel::QgsQuickAttributeModel( QObject *parent )
  : QAbstractListModel( parent )
{
  connect( this, &QgsQuickAttributeModel::modelReset, this, &QgsQuickAttributeModel::featureLayerPairChanged );
  connect( this, &QgsQuickAttributeModel::featureChanged, this, &QgsQuickAttributeModel::featureLayerPairChanged );
  connect( this, &QgsQuickAttributeModel::layerChanged, this, &QgsQuickAttributeModel::featureLayerPairChanged );
  connect( this, &QgsQuickAttributeModel::featureCreated, this, &QgsQuickAttributeModel::onFeatureCreated );
  connect( this, &QgsQuickAttributeModel::rememberValuesAllowChanged, this, &QgsQuickAttributeModel::onRememberValuesAllowChanged );
}

QgsQuickFeatureLayerPair QgsQuickAttributeModel::featureLayerPair() const
{
  return mFeatureLayerPair;
}

void QgsQuickAttributeModel::setFeatureLayerPair( const QgsQuickFeatureLayerPair &pair )
{
  setVectorLayer( pair.layer() );
  setFeature( pair.feature() );
}

void QgsQuickAttributeModel::forceClean()
{
  mRememberedValues.clear();
  mFeatureLayerPair = QgsQuickFeatureLayerPair();
}

void QgsQuickAttributeModel::onFeatureCreated( const QgsFeature &feature )
{
  if ( mRememberValuesAllowed )
  {
    QString layerName = mFeatureLayerPair.layer()->id();

    // save created feature to remember values
    if ( mRememberedValues.contains( layerName ) )
    {
      mRememberedValues[layerName].feature = feature;
    }
  }
}

void QgsQuickAttributeModel::onRememberValuesAllowChanged()
{
  if ( mRememberValuesAllowed ) // add current layer
  {
    if ( mFeatureLayerPair.layer() )
      mRememberedValues[mFeatureLayerPair.layer()->id()].attributeFilter.fill( false, mFeatureLayerPair.layer()->fields().size() );
  }
  else
    mRememberedValues.clear();
}

void QgsQuickAttributeModel::setVectorLayer( QgsVectorLayer *layer )
{
  if ( mFeatureLayerPair.layer() == layer )
    return;

  beginResetModel();
  mFeatureLayerPair = QgsQuickFeatureLayerPair( mFeatureLayerPair.feature(), layer );

  if ( mRememberValuesAllowed )
  {
    if ( layer && !mRememberedValues.contains( layer->id() ) )
    {
      mRememberedValues[layer->id()].attributeFilter.fill( false, layer->fields().size() );
    }
  }

  endResetModel();
  emit layerChanged();
}

void QgsQuickAttributeModel::prefillRememberedValues()
{
  RememberedValues from = mRememberedValues[mFeatureLayerPair.layer()->id()];
  if ( !from.feature.isValid() )
    return;

  QgsAttributes fromAttributes = from.feature.attributes();
  for ( int i = 0; i < fromAttributes.length(); i++ )
  {
    if ( from.attributeFilter.at( i ) )
      mFeatureLayerPair.featureRef().setAttribute( i, fromAttributes.at( i ) );
  }
}

void QgsQuickAttributeModel::setFeature( const QgsFeature &feature )
{
  if ( mFeatureLayerPair.feature() == feature )
    return;

  beginResetModel();
  mFeatureLayerPair = QgsQuickFeatureLayerPair( feature, mFeatureLayerPair.layer() );

  if ( mRememberValuesAllowed && FID_IS_NULL( mFeatureLayerPair.feature().id() ) ) // this is a new feature
    prefillRememberedValues();

  endResetModel();
  emit featureChanged();
}

QHash<int, QByteArray> QgsQuickAttributeModel::roleNames() const
{
  QHash<int, QByteArray> roles = QAbstractListModel::roleNames();
  roles[AttributeName]  = QByteArrayLiteral( "AttributeName" );
  roles[AttributeValue] = QByteArrayLiteral( "AttributeValue" );
  roles[Field] = QByteArrayLiteral( "Field" );
  roles[RememberAttribute] = QByteArrayLiteral( "RememberAttribute" );

  return roles;
}


int QgsQuickAttributeModel::rowCount( const QModelIndex &parent ) const
{
  if ( parent.isValid() )
    return 0;
  else
    return mFeatureLayerPair.feature().attributes().count();
}

QVariant QgsQuickAttributeModel::data( const QModelIndex &index, int role ) const
{
  switch ( role )
  {
    case AttributeName:
      return mFeatureLayerPair.layer()->attributeDisplayName( index.row() );
      break;

    case AttributeValue:
      return mFeatureLayerPair.feature().attribute( index.row() );
      break;

    case Field:
      return mFeatureLayerPair.layer()->fields().at( index.row() );
      break;

    case RememberAttribute:
      if ( mRememberValuesAllowed )
        return mRememberedValues[mFeatureLayerPair.layer()->id()].attributeFilter.at( index.row() );
      else
        return QVariant( false );
      break;
  }

  return QVariant();
}

bool QgsQuickAttributeModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
  if ( data( index, role ) == value )
    return true;

  switch ( role )
  {
    case AttributeValue:
    {
      QVariant val( value );
      QgsField fld = mFeatureLayerPair.feature().fields().at( index.row() );

      if ( !fld.convertCompatible( val ) )
      {
        QString msg( tr( "Value \"%1\" %4 could not be converted to a compatible value for field %2(%3)." ).arg( value.toString(), fld.name(), fld.typeName(), value.isNull() ? "NULL" : "NOT NULL" ) );
        QString userFriendlyMsg( tr( "Value %1 is not compatible with field type %2." ).arg( value.toString(), fld.typeName() ) );
        QgsMessageLog::logMessage( msg );
        emit dataChangedFailed( userFriendlyMsg );
        return false;
      }
      bool success = mFeatureLayerPair.featureRef().setAttribute( index.row(), val );
      if ( success )
        emit dataChanged( index, index, QVector<int>() << role );
      return success;
      break;
    }

    case RememberAttribute:
    {
      if ( mRememberValuesAllowed )
      {
        mRememberedValues[mFeatureLayerPair.layer()->id()].attributeFilter[index.row()] = value.toBool();
        emit dataChanged( index, index, QVector<int>() << role );
      }
      break;
    }
  }

  return false;
}

bool QgsQuickAttributeModel::save()
{
  if ( !mFeatureLayerPair.layer() )
    return false;

  bool rv = true;

  if ( !startEditing() )
  {
    rv = false;
  }

  QgsFeature feat = mFeatureLayerPair.feature();
  if ( !mFeatureLayerPair.layer()->updateFeature( feat ) )
    QgsMessageLog::logMessage( tr( "Cannot update feature" ),
                               QStringLiteral( "QgsQuick" ),
                               Qgis::Warning );

  // This calls lower-level I/O functions which shouldn't be used
  // in a Q_INVOKABLE because they can make the UI unresponsive.
  rv = commit();

  if ( rv )
  {
    QgsFeature feat;
    if ( mFeatureLayerPair.layer()->getFeatures( QgsFeatureRequest().setFilterFid( mFeatureLayerPair.feature().id() ) ).nextFeature( feat ) )
      setFeature( feat );
    else
      QgsMessageLog::logMessage( tr( "Feature %1 could not be fetched after commit" ).arg( mFeatureLayerPair.feature().id() ),
                                 QStringLiteral( "QgsQuick" ),
                                 Qgis::Warning );
  }
  return rv;
}

bool QgsQuickAttributeModel::deleteFeature()
{
  if ( !mFeatureLayerPair.layer() )
    return false;

  bool rv = true;

  if ( !startEditing() )
  {
    rv = false;
  }

  bool isDeleted = mFeatureLayerPair.layer()->deleteFeature( mFeatureLayerPair.feature().id() );
  rv = commit();

  if ( !isDeleted )
    QgsMessageLog::logMessage( tr( "Cannot delete feature" ),
                               QStringLiteral( "QgsQuick" ),
                               Qgis::Warning );
  else
  {
    mFeatureLayerPair = QgsQuickFeatureLayerPair();
    emit featureLayerPairChanged();
  }

  return rv;
}

void QgsQuickAttributeModel::reset()
{
  if ( !mFeatureLayerPair.layer() )
    return;

  mFeatureLayerPair.layer()->rollBack();
}

bool QgsQuickAttributeModel::suppressFeatureForm() const
{
  if ( !mFeatureLayerPair.layer() )
    return false;

  return mFeatureLayerPair.layer()->editFormConfig().suppress();
}

void QgsQuickAttributeModel::resetAttributes()
{
  if ( !mFeatureLayerPair.layer() )
    return;

  QgsExpressionContext expressionContext = mFeatureLayerPair.layer()->createExpressionContext();
  expressionContext.setFeature( mFeatureLayerPair.feature() );

  QgsFields fields = mFeatureLayerPair.layer()->fields();

  beginResetModel();
  for ( int i = 0; i < fields.count(); ++i )
  {
    if ( !mRememberValuesAllowed || !mRememberedValues[mFeatureLayerPair.layer()->id()].attributeFilter.at( i ) )
    {
      if ( !fields.at( i ).defaultValueDefinition().expression().isEmpty() )
      {
        QgsExpression exp( fields.at( i ).defaultValueDefinition().expression() );
        exp.prepare( &expressionContext );
        if ( exp.hasParserError() )
          QgsMessageLog::logMessage( tr( "Default value expression for %1:%2 has parser error: %3" ).arg(
                                       mFeatureLayerPair.layer()->name(),
                                       fields.at( i ).name(),
                                       exp.parserErrorString() ),
                                     QStringLiteral( "QgsQuick" ),
                                     Qgis::Warning );

        QVariant value = exp.evaluate( &expressionContext );

        if ( exp.hasEvalError() )
          QgsMessageLog::logMessage( tr( "Default value expression for %1:%2 has evaluation error: %3" ).arg(
                                       mFeatureLayerPair.layer()->name(),
                                       fields.at( i ).name(),
                                       exp.evalErrorString() ),
                                     QStringLiteral( "QgsQuick" ),
                                     Qgis::Warning );

        mFeatureLayerPair.feature().setAttribute( i, value );
      }
      else
      {
        mFeatureLayerPair.feature().setAttribute( i, QVariant() );
      }
    }
  }
  endResetModel();
}

void QgsQuickAttributeModel::create()
{
  if ( !mFeatureLayerPair.layer() )
    return;

  startEditing();
  QgsFeature feat = mFeatureLayerPair.feature();
  if ( !mFeatureLayerPair.layer()->addFeature( feat ) )
  {
    QgsMessageLog::logMessage( tr( "Feature could not be added" ),
                               QStringLiteral( "QgsQuick" ),
                               Qgis::Critical );
  }
  commit();

  emit featureCreated( mFeatureLayerPair.featureRef() );
}

bool QgsQuickAttributeModel::hasAnyChanges()
{
  return FID_IS_NULL( mFeatureLayerPair.feature().id() ) || mFeatureLayerPair.layer()->editBuffer()->isFeatureAttributesChanged( mFeatureLayerPair.feature().id() );
}

bool QgsQuickAttributeModel::commit()
{
  if ( !mFeatureLayerPair.layer()->commitChanges() )
  {
    QgsMessageLog::logMessage( tr( "Could not save changes. Rolling back." ),
                               QStringLiteral( "QgsQuick" ),
                               Qgis::Critical );
    mFeatureLayerPair.layer()->rollBack();
    return false;
  }
  else
  {
    return true;
  }
}

bool QgsQuickAttributeModel::startEditing()
{
  // Already an edit session active
  if ( mFeatureLayerPair.layer()->editBuffer() )
    return true;

  if ( !mFeatureLayerPair.layer()->startEditing() )
  {
    QgsMessageLog::logMessage( tr( "Cannot start editing" ),
                               QStringLiteral( "QgsQuick" ),
                               Qgis::Warning );
    return false;
  }
  else
  {
    return true;
  }
}

void QgsQuickAttributeModel::setRememberValuesAllowed( bool allowed )
{
  if ( allowed == mRememberValuesAllowed )
    return;

  mRememberValuesAllowed = allowed;
  emit rememberValuesAllowChanged();
}

bool QgsQuickAttributeModel::rememberValuesAllowed() const
{
  return mRememberValuesAllowed;
}

bool QgsQuickAttributeModel::isFieldRemembered( const int fieldIndex ) const
{
  if ( !mRememberValuesAllowed || !mFeatureLayerPair.layer() || !mRememberedValues.contains( mFeatureLayerPair.layer()->id() ) )
    return false;

  if ( fieldIndex < 0 || fieldIndex > mRememberedValues[mFeatureLayerPair.layer()->id()].attributeFilter.size() )
    return false;

  return mRememberedValues[mFeatureLayerPair.layer()->id()].attributeFilter.at( fieldIndex );
}
