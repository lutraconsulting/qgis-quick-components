/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "androidutils.h"

#ifdef ANDROID
#include <QtAndroid>
#include <QAndroidJniObject>
#include <QAndroidJniEnvironment>
#include <QDebug>
#endif

AndroidUtils::AndroidUtils( QObject *parent ): QObject( parent )
{
}

void AndroidUtils::showToast( QString message )
{
#ifdef ANDROID
  QtAndroid::runOnAndroidThread( [message]
  {
    QAndroidJniObject javaString = QAndroidJniObject::fromString( message );
    QAndroidJniObject toast = QAndroidJniObject::callStaticObjectMethod( "android/widget/Toast", "makeText",
        "(Landroid/content/Context;Ljava/lang/CharSequence;I)Landroid/widget/Toast;",
        QtAndroid::androidActivity().object(),
        javaString.object(),
        jint( 1 ) );
    toast.callMethod<void>( "show" );
  } );
#else
  Q_UNUSED( message )
#endif
}

bool AndroidUtils::isAndroid() const
{
#ifdef ANDROID
  return true;
#else
  return false;
#endif
}

//! https://stackoverflow.com/questions/35973235/android-permission-denial-starting-intent-with-revoked-permission-android-perms
void AndroidUtils::requirePermissions()
{
  checkAndAcquirePermissions( "android.permission.WRITE_EXTERNAL_STORAGE" );
}

bool AndroidUtils::checkAndAcquirePermissions( const QString &permissionString )
{
#ifdef ANDROID

  QtAndroid::PermissionResult r = QtAndroid::checkPermission( permissionString );
  if ( r == QtAndroid::PermissionResult::Denied )
  {
    QtAndroid::requestPermissionsSync( QStringList() << permissionString );
    r = QtAndroid::checkPermission( permissionString );
    if ( r == QtAndroid::PermissionResult::Denied )
    {
      return false;
    }
  }
#else
  Q_UNUSED( permissionString )
#endif
  return true;
}

QString AndroidUtils::getExifInfo( const QString &filePath )
{
  QAndroidJniObject jFilePath = QAndroidJniObject::fromString( filePath );
  QAndroidJniObject jTag = QAndroidJniObject::fromString( "TODO" );
  QAndroidJniObject attribute = QAndroidJniObject::callStaticObjectMethod( "uk.co.lutraconsulting.CameraActivity",
                                "getEXIFAttribute",
                                "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",
                                jFilePath.object<jstring>(),
                                jTag.object<jstring>() );
  return attribute.toString();
}

bool AndroidUtils::checkPermission( const QString &permissionString )
{
#ifdef ANDROID
  return QtAndroid::checkPermission( permissionString ) == QtAndroid::PermissionResult::Granted;
#else
  return true;
#endif
}

bool AndroidUtils::requestStoragePermission()
{
#ifdef ANDROID

  if ( !checkAndAcquirePermissions( "android.permission.WRITE_EXTERNAL_STORAGE" ) )
  {
    if ( !QtAndroid::shouldShowRequestPermissionRationale( "android.permission.WRITE_EXTERNAL_STORAGE" ) )
    {
      // permanently denied permission, user needs to go to settings to allow permission
      showToast( tr( "Storage permission is permanently denied, please allow it in settings" ) );
    }
    else
    {
      showToast( tr( "Input needs a storage permission in order to manipulate or download a project" ) );
    }
    return false;
  }

#endif
  return true;
}

void AndroidUtils::callImagePicker()
{
#ifdef ANDROID
  QAndroidJniObject ACTION_PICK = QAndroidJniObject::getStaticObjectField( "android/content/Intent", "ACTION_PICK", "Ljava/lang/String;" );
  QAndroidJniObject EXTERNAL_CONTENT_URI = QAndroidJniObject::getStaticObjectField( "android/provider/MediaStore$Images$Media", "EXTERNAL_CONTENT_URI", "Landroid/net/Uri;" );

  QAndroidJniObject intent = QAndroidJniObject( "android/content/Intent", "(Ljava/lang/String;Landroid/net/Uri;)V", ACTION_PICK.object<jstring>(), EXTERNAL_CONTENT_URI.object<jobject>() );

  if ( ACTION_PICK.isValid() && intent.isValid() )
  {
    intent.callObjectMethod( "setType", "(Ljava/lang/String;)Landroid/content/Intent;", QAndroidJniObject::fromString( "image/*" ).object<jstring>() );
    QtAndroid::startActivity( intent.object<jobject>(), MEDIA_CODE, this ); // this as receiver
  }
#endif
}

void AndroidUtils::callCamera( const QString &targetPath )
{
#ifdef ANDROID

  if ( checkAndAcquirePermissions( "android.permission.CAMERA" ) == false )
  {
    if ( !QtAndroid::shouldShowRequestPermissionRationale( "android.permission.CAMERA" ) )
    {
      // permanently denied permission, user needs to go to settings to allow permission
      showToast( tr( "Camera permission is permanently denied, please allow it in settings" ) );
    }
    else
    {
      showToast( tr( "We need a camera permission in order to take a photo" ) );
    }
    return;
  }

  const QString IMAGE_CAPTURE_ACTION = QString( "android.media.action.IMAGE_CAPTURE" );

  QAndroidJniObject activity = QAndroidJniObject::fromString( QStringLiteral( "uk.co.lutraconsulting.CameraActivity" ) );
  QAndroidJniObject intent = QAndroidJniObject( "android/content/Intent", "(Ljava/lang/String;)V", activity.object<jstring>() );

  QAndroidJniObject packageName = QAndroidJniObject::fromString( QStringLiteral( "uk.co.lutraconsulting" ) );
  QAndroidJniObject className = QAndroidJniObject::fromString( QStringLiteral( "uk.co.lutraconsulting.CameraActivity" ) );

  intent.callObjectMethod( "setClassName", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;", packageName.object<jstring>(), className.object<jstring>() );

  QAndroidJniObject extra = QAndroidJniObject::fromString( "targetPath" );
  QAndroidJniObject my_prefix = QAndroidJniObject::fromString( targetPath );

  intent.callObjectMethod( "putExtra",
                           "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;",
                           extra.object<jstring>(),
                           my_prefix.object<jstring>() );

  if ( intent.isValid() )
  {
    QtAndroid::startActivity( intent.object<jobject>(), CAMERA_CODE, this );
  }
#else
  Q_UNUSED( targetPath )
#endif
}

#ifdef ANDROID
void AndroidUtils::handleActivityResult( int receiverRequestCode, int resultCode, const QAndroidJniObject &data )
{

  jint RESULT_OK = QAndroidJniObject::getStaticField<jint>( "android/app/Activity", "RESULT_OK" );
  jint RESULT_CANCELED = QAndroidJniObject::getStaticField<jint>( "android/app/Activity", "RESULT_CANCELED" );

  if ( resultCode == RESULT_CANCELED )
  {
    QAndroidJniObject RESULT_STRING = QAndroidJniObject::fromString( QStringLiteral( "__RESULT__" ) );
    // User has triggered cancel, result has no data.
    if ( !data.isValid() )
    {
      return;
    }

    QAndroidJniObject errorJNI = data.callObjectMethod( "getStringExtra", "(Ljava/lang/String;)Ljava/lang/String;", RESULT_STRING.object<jstring>() );
    // Internal cancelation due to an error
    if ( !errorJNI.isValid() )
    {
      QString errorMsg = errorJNI.toString();
      showToast( errorMsg );
    }
    return;
  }

  if ( receiverRequestCode == MEDIA_CODE && resultCode == RESULT_OK )
  {
    QAndroidJniObject uri = data.callObjectMethod( "getData", "()Landroid/net/Uri;" );
    QAndroidJniObject mediaStore = QAndroidJniObject::getStaticObjectField( "android/provider/MediaStore$MediaColumns", "DATA", "Ljava/lang/String;" );
    QAndroidJniEnvironment env;
    jobjectArray projection = ( jobjectArray )env->NewObjectArray( 1, env->FindClass( "java/lang/String" ), NULL );
    jobject projectionDataAndroid = env->NewStringUTF( mediaStore.toString().toStdString().c_str() );
    env->SetObjectArrayElement( projection, 0, projectionDataAndroid );
    QAndroidJniObject contentResolver = QtAndroid::androidActivity().callObjectMethod( "getContentResolver", "()Landroid/content/ContentResolver;" );
    QAndroidJniObject cursor = contentResolver.callObjectMethod( "query", "(Landroid/net/Uri;[Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;)Landroid/database/Cursor;", uri.object<jobject>(), projection, NULL, NULL, NULL );
    jint columnIndex = cursor.callMethod<jint>( "getColumnIndex", "(Ljava/lang/String;)I", mediaStore.object<jstring>() );
    cursor.callMethod<jboolean>( "moveToFirst", "()Z" );
    QAndroidJniObject result = cursor.callObjectMethod( "getString", "(I)Ljava/lang/String;", columnIndex );
    QString selectedImagePath = "file://" + result.toString();
    emit imageSelected( selectedImagePath );
  }
  else if ( receiverRequestCode == CAMERA_CODE && resultCode == RESULT_OK )
  {
    QAndroidJniObject RESULT_STRING = QAndroidJniObject::fromString( QStringLiteral( "__RESULT__" ) );
    QAndroidJniObject absolutePathJNI = data.callObjectMethod( "getStringExtra", "(Ljava/lang/String;)Ljava/lang/String;", RESULT_STRING.object<jstring>() );
    QString absolutePath = absolutePathJNI.toString();

    QString selectedImagePath = "file://" + absolutePath;
    getExifInfo( absolutePath );

    emit imageSelected( absolutePath );
  }
  else
  {
    QString msg( "Something went wrong with media store activity" );
    qDebug() << msg;
    showToast( msg );
  }

}
#endif
