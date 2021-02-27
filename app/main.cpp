/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QtDebug>
#include <QQmlError>
#include <QDesktopWidget>
#include <QWindow>
#include <QtGlobal>
#include <QQmlContext>
#include <QQuickWindow>
#include <QLocale>
#ifdef INPUT_TEST
#include <QTest>
#endif
#include <qqml.h>
#include <qgsmessagelog.h>
#include "qgsconfig.h"
#include "qgsproviderregistry.h"
#include "qgsmaplayerproxymodel.h"

#include "androidutils.h"
#include "ios/iosutils.h"
#include "inpututils.h"
#include "positiondirection.h"
#include "projectsmodel.h"
#include "mapthemesmodel.h"
#include "digitizingcontroller.h"
#include "merginapi.h"
#include "merginapistatus.h"
#include "merginsubscriptionstatus.h"
#include "merginsubscriptiontype.h"
#include "merginprojectmodel.h"
#include "merginprojectstatusmodel.h"
#include "layersproxymodel.h"
#include "layersmodel.h"
#include "activelayer.h"
#include "purchasing.h"
#include "merginuserauth.h"
#include "merginuserinfo.h"
#include "variablesmanager.h"
#include "inputhelp.h"
#include "inputprojutils.h"
#include "fieldsmodel.h"
#include "projectwizard.h"

#ifdef INPUT_TEST
#include "test/testmerginapi.h"
#include "test/testlinks.h"
#if not defined APPLE_PURCHASING
#include "test/testpurchasing.h"
#endif
#endif

#include "qgsquickutils.h"
#include "qgsproject.h"

#ifndef NDEBUG
// #include <QQmlDebuggingEnabler>
#endif

#ifdef MOBILE_OS
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#endif

#ifdef ANDROID
#include <QtAndroidExtras>
#endif

#ifdef DESKTOP_OS
#include <QCommandLineParser>
#include <qgis.h>
#endif

#include "qgsapplication.h"
#include "loader.h"
#include "appsettings.h"

#ifdef Q_OS_IOS
#include "qgsquickplugin.h"
#endif

static QString getDataDir()
{
#ifdef QGIS_QUICK_DATA_PATH
  QString dataPathRaw( STR( QGIS_QUICK_DATA_PATH ) );

#ifdef ANDROID
  QFileInfo extDir( "/sdcard/" );
  if ( extDir.isDir() && extDir.isWritable() )
  {
    // seems that this directory transposes to the latter one in case there is no sdcard attached
    dataPathRaw = extDir.path() + "/" + dataPathRaw;
  }
  else
  {
    qDebug() << "extDir: " << extDir.path() << " not writable";

    QStringList split = QDir::homePath().split( "/" ); // something like /data/user/0/uk.co.lutraconsulting/files
    // TODO support active user from QDir::homePath()
    QFileInfo usrDir( "/storage/emulated/" + split[2] + "/" );
    dataPathRaw = usrDir.path() + "/" + dataPathRaw;
    if ( !( usrDir.isDir() && usrDir.isWritable() ) )
    {
      qDebug() << "usrDir: " << usrDir.path() << " not writable";
    }
  }
#endif

#ifdef Q_OS_IOS
  QString docsLocation = QStandardPaths::standardLocations( QStandardPaths::DocumentsLocation ).value( 0 );

  QDir myDir( docsLocation );
  if ( !myDir.exists() )
  {
    myDir.mkpath( docsLocation );
  }
  dataPathRaw = docsLocation + "/" + dataPathRaw;
#endif

#ifdef Q_OS_WIN32
  QString appLocation = QStandardPaths::writableLocation( QStandardPaths::AppDataLocation );

  QDir myDir( appLocation );
  if ( !myDir.exists() )
  {
    myDir.mkpath( appLocation );
  }
  dataPathRaw = appLocation + "/" + dataPathRaw;
#endif

  qputenv( "QGIS_QUICK_DATA_PATH", dataPathRaw.toUtf8().constData() );
#else
  qDebug( "== Must set QGIS_QUICK_DATA_PATH in order to get QGIS Quick running! ==" );
#endif
  QString dataDir = QString::fromLocal8Bit( qgetenv( "QGIS_QUICK_DATA_PATH" ) ) ;
  qDebug() << "QGIS_QUICK_DATA_PATH: " << dataDir;
  return dataDir;
}

static void setEnvironmentQgisPrefixPath()
{
#ifdef DESKTOP_OS
#ifdef QGIS_PREFIX_PATH
  qputenv( "QGIS_PREFIX_PATH", STR( QGIS_PREFIX_PATH ) );
#endif
  if ( QString::fromLocal8Bit( qgetenv( "QGIS_PREFIX_PATH" ) ).isEmpty() )
  {
    // if not on Android, QGIS_PREFIX_PATH env variable should have been set already or defined as C++ define
    qDebug( "== Must set QGIS_PREFIX_PATH in order to get QGIS Quick module running! ==" );
  }
#endif

#if defined (ANDROID) || defined (Q_OS_IOS)
  QDir myDir( QDir::homePath() );
  myDir.cdUp();
  QString prefixPath = myDir.absolutePath();  // something like: /data/data/org.qgis.quick
  qputenv( "QGIS_PREFIX_PATH", prefixPath.toUtf8().constData() );
#elif defined (Q_OS_WIN32)
  QString prefixPath = QCoreApplication::applicationDirPath();
  qputenv( "QGIS_PREFIX_PATH", prefixPath.toUtf8().constData() );
#endif

  qDebug() << "QGIS_PREFIX_PATH: " << QString::fromLocal8Bit( qgetenv( "QGIS_PREFIX_PATH" ) );
}


static void copy_demo_projects( const QString &demoDir, const QString &projectDir )
{
  if ( !demoDir.isEmpty() )
    InputUtils::cpDir( demoDir, projectDir );

  QFile demoFile( projectDir + "/Start here!/qgis-project.qgz" );
  if ( demoFile.exists() )
    qDebug() << "DEMO projects initialized";
  else
    InputUtils::log( QStringLiteral( "DEMO" ), QStringLiteral( "The Input has failed to initialize demo projects" ) );
}

static void init_qgis( const QString &pkgPath )
{
  QgsApplication::init();

#ifdef MOBILE_OS
  // QGIS plugins on Android are in the same path as other libraries
  QgsApplication::setPluginPath( QApplication::applicationDirPath() );
  QgsApplication::setPkgDataPath( pkgPath );
#else
  Q_UNUSED( pkgPath )
#endif

  QgsApplication::initQgis();

  // make sure the DB exists - otherwise custom projections will be failing
  if ( !QgsApplication::createDatabase() )
    qDebug( "Can't create qgis user DB!!!" );

  qDebug( "qgis providers:\n%s", QgsProviderRegistry::instance()->pluginList().toLatin1().data() );
}

void initDeclarative()
{
  qmlRegisterUncreatableType<MerginUserAuth>( "lc", 1, 0, "MerginUserAuth", "" );
  qmlRegisterUncreatableType<MerginUserInfo>( "lc", 1, 0, "MerginUserInfo", "" );
  qmlRegisterUncreatableType<PurchasingPlan>( "lc", 1, 0, "MerginPlan", "" );
  qmlRegisterUncreatableType<ProjectModel>( "lc", 1, 0, "ProjectModel", "" );
  qmlRegisterUncreatableType<MapThemesModel>( "lc", 1, 0, "MapThemesModel", "" );
  qmlRegisterUncreatableType<Loader>( "lc", 1, 0, "Loader", "" );
  qmlRegisterUncreatableType<AppSettings>( "lc", 1, 0, "AppSettings", "" );
  qmlRegisterUncreatableType<MerginApiStatus>( "lc", 1, 0, "MerginApiStatus", "MerginApiStatus Enum" );
  qmlRegisterUncreatableType<MerginSubscriptionStatus>( "lc", 1, 0, "MerginSubscriptionStatus", "MerginSubscriptionStatus Enum" );
  qmlRegisterUncreatableType<MerginSubscriptionType>( "lc", 1, 0, "MerginSubscriptionType", "MerginSubscriptionType Enum" );
  qmlRegisterUncreatableType<MerginProjectStatusModel>( "lc", 1, 0, "MerginProjectStatusModel", "Enum" );
  qmlRegisterUncreatableType<LayersModel>( "lc", 1, 0, "LayersModel", "" );
  qmlRegisterUncreatableType<LayersProxyModel>( "lc", 1, 0, "LayersProxyModel", "" );
  qmlRegisterUncreatableType<ActiveLayer>( "lc", 1, 0, "ActiveLayer", "" );
  qmlRegisterType<DigitizingController>( "lc", 1, 0, "DigitizingController" );
  qmlRegisterType<PositionDirection>( "lc", 1, 0, "PositionDirection" );
  qmlRegisterType<FieldsModel>( "lc", 1, 0, "FieldsModel" );
}

#ifdef INPUT_TEST
void initTestDeclarative()
{
  qRegisterMetaType<MerginProjectList>( "MerginProjectList" );
}
#endif

void addQmlImportPath( QQmlEngine &engine )
{
  // This adds a runtime qml directory containing QgsQuick plugin
  // when Input is installed (e.g. Android/Win32)
  engine.addImportPath( QgsApplication::qmlImportPath() );
  qDebug() << "adding QML import Path: " << QgsApplication::qmlImportPath();

#ifdef QML_BUILD_IMPORT_DIR
  // Adds a runtime qml directory containing QgsQuick plugin
  // if we are using the developer mode (not installed Input)
  // e.g. Linux/MacOS
  QString qmlBuildImportPath( STR( QML_BUILD_IMPORT_DIR ) );
  engine.addImportPath( qmlBuildImportPath );
  qDebug() << "adding QML import Path: " << qmlBuildImportPath;
#endif

#ifdef Q_OS_IOS
  // REQUIRED FOR IOS - to load QgsQuick/*.qml files defined in qmldir
  engine.addImportPath( "qrc:///" );
  qDebug() << "adding QML import Path: " << "qrc:///";
#endif
}

int main( int argc, char *argv[] )
{
  QgsApplication app( argc, argv, true );

  const QString version = InputUtils::appVersion();

  // Set up the QSettings environment must be done after qapp is created
  QCoreApplication::setOrganizationName( "Lutra Consulting" );
  QCoreApplication::setOrganizationDomain( "lutraconsulting.co.uk" );
  QCoreApplication::setApplicationName( "Input" );
  QCoreApplication::setApplicationVersion( version );

  // Initialize translations
  QLocale locale;
  QTranslator inputTranslator;
  if ( inputTranslator.load( locale, "input", "_", ":/" ) )
  {
    app.installTranslator( &inputTranslator );
    qDebug() <<  "Loaded input translation for " << locale;
  }
  else
  {
    qDebug() <<  "Error in loading input translation for " << locale;
  }

#ifdef INPUT_TEST
  bool IS_MERGIN_API_TEST = false;
  bool IS_PURCHASING_TEST = false;
  bool IS_LINKS_TEST = false;
  for ( int i = 0; i < argc; ++i )
  {
    if ( std::string( argv[i] ) == "--testMerginApi" ) IS_MERGIN_API_TEST = true;
    if ( std::string( argv[i] ) == "--testPurchasing" ) IS_PURCHASING_TEST = true;
    if ( std::string( argv[i] ) == "--testLinks" ) IS_LINKS_TEST = true;
  }
  Q_ASSERT( !( IS_MERGIN_API_TEST && IS_PURCHASING_TEST && IS_LINKS_TEST ) );
  bool IS_TEST = IS_PURCHASING_TEST || IS_MERGIN_API_TEST || IS_LINKS_TEST;
#endif
  qDebug() << "Built with QGIS version " << VERSION_INT;

  // Require permissions before accessing data folder
#ifdef ANDROID
  AndroidUtils::requirePermissions();
#endif
  // Set/Get enviroment
  QString dataDir = getDataDir();
  QString projectDir = dataDir + "/projects";

#ifdef INPUT_TEST
  if ( IS_TEST )
  {
    // override the path where local projects are stored
    // and wipe the temporary projects dir if it already exists
    QDir testDataDir( STR( INPUT_TEST_DATA_DIR ) );  // #defined in input.pro
    QDir testProjectsDir( testDataDir.path() + "/../temp_projects" );
    if ( testProjectsDir.exists() )
      testProjectsDir.removeRecursively();
    QDir( testDataDir.path() + "/.." ).mkpath( "temp_projects" );
    projectDir = testProjectsDir.canonicalPath();
  }
#endif

  InputUtils::setLogFilename( projectDir + "/.logs" );
  setEnvironmentQgisPrefixPath();

  QString appBundleDir;
  QString demoDir;
#ifdef ANDROID
  appBundleDir = dataDir + "/qgis-data";
  demoDir = "assets:/demo-projects";
#endif
#ifdef Q_OS_IOS
  appBundleDir = QCoreApplication::applicationDirPath() + "/qgis-data";
  demoDir = QCoreApplication::applicationDirPath() + "/demo-projects";
#endif
#ifdef Q_OS_WIN32
  appBundleDir = QCoreApplication::applicationDirPath() + "\\qgis-data";
  //TODO win32 package demo projects
#endif
  InputProjUtils inputProjUtils;
  inputProjUtils.initProjLib( appBundleDir, dataDir, projectDir );
  init_qgis( appBundleDir );

  // AppSettings has to be initialized after QGIS app init (because of correct reading/writing QSettings).
  AppSettings as;
  // copy demo projects when the app is launched for the first time
  if ( !as.demoProjectsCopied() )
  {
    copy_demo_projects( demoDir, projectDir );
    as.setDemoProjectsCopied( true );
  }

  // Create Input classes
  AndroidUtils au;
  IosUtils iosUtils;
  LocalProjectsManager localProjects( projectDir );
  ProjectModel pm( localProjects );
  MapThemesModel mtm;
  std::unique_ptr<MerginApi> ma =  std::unique_ptr<MerginApi>( new MerginApi( localProjects ) );
  InputUtils iu;
  MerginProjectModel mpm( localProjects );
  MerginProjectStatusModel mpsm( localProjects );
  InputHelp help( ma.get(), &iu );
  ProjectWizard pw( projectDir );

  // layer models
  LayersModel lm;
  LayersProxyModel browseLpm( &lm, ModelTypes::BrowseDataLayerSelection );
  LayersProxyModel recordingLpm( &lm, ModelTypes::ActiveLayerSelection );

  ActiveLayer al;
  Loader loader( mtm, as, al );
  std::unique_ptr<Purchasing> purchasing( new Purchasing( ma.get() ) );
  std::unique_ptr<VariablesManager> vm( new VariablesManager( ma.get() ) );

  // Connections
  QObject::connect( &app, &QGuiApplication::applicationStateChanged, &loader, &Loader::appStateChanged );
  QObject::connect( &app, &QCoreApplication::aboutToQuit, &loader, &Loader::appAboutToQuit );
  QObject::connect( ma.get(), &MerginApi::syncProjectFinished, &pm, &ProjectModel::syncedProjectFinished );
  QObject::connect( ma.get(), &MerginApi::projectDetached, &pm, &ProjectModel::findProjectFiles );
  QObject::connect( &pw, &ProjectWizard::projectCreated, &localProjects, &LocalProjectsManager::addLocalProject );
  QObject::connect( ma.get(), &MerginApi::listProjectsFinished, &mpm, &MerginProjectModel::updateModel );
  QObject::connect( ma.get(), &MerginApi::syncProjectStatusChanged, &mpm, &MerginProjectModel::syncProjectStatusChanged );
  QObject::connect( ma.get(), &MerginApi::reloadProject, &loader, &Loader::reloadProject );
  QObject::connect( &mtm, &MapThemesModel::mapThemeChanged, &recordingLpm, &LayersProxyModel::onMapThemeChanged );
  QObject::connect( &loader, &Loader::projectReloaded, vm.get(), &VariablesManager::merginProjectChanged );
  QObject::connect( &loader, &Loader::projectWillBeReloaded, &inputProjUtils, &InputProjUtils::resetHandlers );
  QObject::connect( &pw, &ProjectWizard::notify, &iu, &InputUtils::showNotificationRequested );
  QObject::connect( QgsApplication::messageLog(),
                    static_cast<void ( QgsMessageLog::* )( const QString &message, const QString &tag, Qgis::MessageLevel level )>( &QgsMessageLog::messageReceived ),
                    &iu,
                    &InputUtils::onQgsLogMessageReceived );

  QFile projectLoadingFile( Loader::LOADING_FLAG_FILE_PATH );
  if ( projectLoadingFile.exists() )
  {
    // Cleaning default project due to a project loading has crashed during the last run.
    as.setDefaultProject( QString() );
    projectLoadingFile.remove();
    InputUtils::log( QStringLiteral( "Loading project error" ), QStringLiteral( "The Input has been unexpectedly finished during the last run." ) );
  }

#ifdef INPUT_TEST
  if ( IS_TEST )
  {
    initTestDeclarative();
    // use command line args we got, but filter out "--test*" that's recognized by us but not by QTest framework
    // (command line args may be used to filter function names that should be executed)
    QVector<char *> args;
    for ( int i = 0; i < argc; ++i )
    {
      if ( !QString( argv[i] ).startsWith( "--test" ) )
        args << argv[i];
    }

    int nFailed = 0;
    if ( IS_MERGIN_API_TEST )
    {
      TestMerginApi merginApiTest( ma.get(), &mpm, &pm );
      nFailed = QTest::qExec( &merginApiTest, args.count(), args.data() );
    }
    else if ( IS_LINKS_TEST )
    {
      TestLinks linksTest( ma.get(), &iu );
      nFailed = QTest::qExec( &linksTest, args.count(), args.data() );
    }
#if not defined APPLE_PURCHASING
    else if ( IS_PURCHASING_TEST )
    {
      TestPurchasing purchasingTest( ma.get(), purchasing.get() );
      nFailed += QTest::qExec( &purchasingTest, args.count(), args.data() );
    }
#endif
    return nFailed;
  }
#endif

  // we ship our fonts because they do not need to be installed on the target platform
  QStringList fonts;
  fonts << ":/Lato-Regular.ttf"
        << ":/Lato-Bold.ttf";
  for ( QString font : fonts )
  {
    if ( QFontDatabase::addApplicationFont( font ) == -1 )
      qDebug() << "!! Failed to load font" << font;
    else
      qDebug() << "Loaded font" << font;
  }
  app.setFont( QFont( "Lato" ) );

#ifdef Q_OS_IOS
  // REQUIRED FOR IOS  - to load QgsQuick C++ classes
  QgsQuickPlugin plugin;
  plugin.registerTypes( "QgsQuick" );
#endif

  QQmlEngine engine;
  addQmlImportPath( engine );
  initDeclarative();
  // QGIS environment variables to set
  // OGR_SQLITE_JOURNAL is set to DELETE to avoid working with WAL files
  // and properly close connection after writting changes to gpkg.
  qputenv( "OGR_SQLITE_JOURNAL", "DELETE" );

  // Register to QQmlEngine
  engine.rootContext()->setContextProperty( "__androidUtils", &au );
  engine.rootContext()->setContextProperty( "__iosUtils", &iosUtils );
  engine.rootContext()->setContextProperty( "__inputUtils", &iu );
  engine.rootContext()->setContextProperty( "__inputProjUtils", &inputProjUtils );
  engine.rootContext()->setContextProperty( "__inputHelp", &help );
  engine.rootContext()->setContextProperty( "__projectsModel", &pm );
  engine.rootContext()->setContextProperty( "__loader", &loader );
  engine.rootContext()->setContextProperty( "__mapThemesModel", &mtm );
  engine.rootContext()->setContextProperty( "__appSettings", &as );
  engine.rootContext()->setContextProperty( "__merginApi", ma.get() );
  engine.rootContext()->setContextProperty( "__merginProjectsModel", &mpm );
  engine.rootContext()->setContextProperty( "__merginProjectStatusModel", &mpsm );
  engine.rootContext()->setContextProperty( "__recordingLayersModel", &recordingLpm );
  engine.rootContext()->setContextProperty( "__browseDataLayersModel", &browseLpm );
  engine.rootContext()->setContextProperty( "__activeLayer", &al );
  engine.rootContext()->setContextProperty( "__purchasing", purchasing.get() );
  engine.rootContext()->setContextProperty( "__projectWizard", &pw );

#ifdef MOBILE_OS
  engine.rootContext()->setContextProperty( "__appwindowvisibility", QWindow::Maximized );
  engine.rootContext()->setContextProperty( "__appwindowwidth", QVariant( 0 ) );
  engine.rootContext()->setContextProperty( "__appwindowheight", QVariant( 0 ) );
#else
  engine.rootContext()->setContextProperty( "__appwindowvisibility", QWindow::Windowed );
  engine.rootContext()->setContextProperty( "__appwindowwidth", 640 );
  engine.rootContext()->setContextProperty( "__appwindowheight", 1136 );
#endif
  engine.rootContext()->setContextProperty( "__version", version );

  // Set simulated position for desktop builds
#ifdef DESKTOP_OS
  bool use_simulated_position = true;
#else
  bool use_simulated_position = false;
#endif
  engine.rootContext()->setContextProperty( "__use_simulated_position", use_simulated_position );

  QQmlComponent component( &engine, QUrl( "qrc:/main.qml" ) );
  QObject *object = component.create();

  if ( !component.errors().isEmpty() )
  {
    qDebug( "%s", QgsApplication::showSettings().toLocal8Bit().data() );

    qDebug() << "****************************************";
    qDebug() << "*****        QML errors:           *****";
    qDebug() << "****************************************";
    for ( const QQmlError &error : component.errors() )
    {
      qDebug() << "  " << error;
    }
    qDebug() << "****************************************";
    qDebug() << "****************************************";
  }

  if ( object == nullptr )
  {
    qDebug() << "FATAL ERROR: unable to create main.qml";
    return EXIT_FAILURE;
  }

#ifdef Q_OS_IOS
  QString logoUrl = "qrc:logo.png";
#else
  QString logoUrl = ":/logo.png";
#endif
  if ( QQuickWindow *quickWindow = qobject_cast<QQuickWindow *>( object ) )
  {
    quickWindow->setIcon( QIcon( logoUrl ) );
  }

#ifdef DESKTOP_OS
  QCommandLineParser parser;
  parser.addVersionOption();
  parser.process( app );
#endif

  // Add some data for debugging
  qDebug() << QgsQuickUtils().dumpScreenInfo();
  qDebug() << "data directory: " << dataDir;
  qDebug() <<  "All up and running";

#ifdef ANDROID
  QtAndroid::hideSplashScreen();
#endif

  int ret = EXIT_FAILURE;
  try
  {
    ret = app.exec();
  }
  catch ( QgsException &e )
  {
    iu.log( "Error", QStringLiteral( "Caught unhandled QgsException %1" ).arg( e.what() ) );
  }
  catch ( std::exception &e )
  {
    iu.log( "Error", QStringLiteral( "Caught unhandled std::exception %1" ).arg( e.what() ) );
  }
  catch ( ... )
  {
    iu.log( "Error", QStringLiteral( "Caught unhandled unknown exception" ) );
  }
  return ret;
}


