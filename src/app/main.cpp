/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#include <common/mayo_config.h>
#include <common/mayo_version.h>
#include <cstdlib>
#include <gsl/util>
#include <memory>

#include <QtCore/QCommandLineParser>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtCore/QTranslator>
#include <QtCore/QVersionNumber>
#include <QtCore/QtDebug>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QPixmap>
#include <QtWidgets/QApplication>

#include <fmt/format.h>

#include "base/io_system.h"
#include "base/settings.h"
#include "graphics/graphics_object_driver_mesh.h"
#include "graphics/graphics_object_driver_point_cloud.h"
#include "graphics/graphics_object_driver_shape.h"
#include "graphics/graphics_utils.h"
#include "gui/gui_application.h"
#include "io_assimp/io_assimp.h"
#include "io_dxf/io_dxf.h"
#include "io_occ/io_occ.h"
#include "io_off/io_off_reader.h"
#include "io_off/io_off_writer.h"
#include "io_ply/io_ply_reader.h"
#include "io_ply/io_ply_writer.h"
#include "qtbackend/qsettings_storage.h"
#include "qtbackend/qt_app_translator.h"
#include "qtbackend/qt_signal_thread_helper.h"
#include "qtcommon/filepath_conv.h"
#include "qtcommon/log_message_handler.h"

#include "app_module.h"
#include "commands_help.h"
#include "document_tree_node_properties_providers.h"
#include "io_gmio/io_gmio.h"
#include "io_image/io_image.h"
#include "mainwindow.h"
#include "qtgui_utils.h"
#include "theme.h"
#include "widget_model_tree.h"
#include "widget_model_tree_builder_mesh.h"
#include "widget_model_tree_builder_xde.h"
#include "widget_occ_view.h"
#include "widget_occ_view_gl.h"
#include "widget_occ_view_i.h"

namespace Mayo
{
// Provides an i18n context for the current file(main.cpp)
class Main
{
    MAYO_DECLARE_TEXT_ID_FUNCTIONS(Mayo::Main)
    Q_DECLARE_TR_FUNCTIONS(Mayo::Main)
};

namespace
{

// Stores arguments(options) passed at command line
struct CommandLineArguments
{
    QString themeName;
    FilePath filepathSettings;
    FilePath filepathLog;
    bool includeDebugLogs = true;
    std::vector<FilePath> listFilepathToOpen;
    bool showSystemInformation = false;
};

} // namespace

// Parses command line and process Qt builtin options(basically --version and
// --help)
static CommandLineArguments processCommandLine()
{
    CommandLineArguments args;

    // Configure command-line parser
    QCommandLineParser cmdParser;
    cmdParser.setApplicationDescription(
        Main::tr("Mayo the opensource 3D CAD viewer and converter"));
    cmdParser.addHelpOption();
    cmdParser.addVersionOption();

    const QCommandLineOption cmdOptionTheme(
        QStringList{"t", "theme"}, Main::tr("Theme for the UI(classic|dark)"), Main::tr("name"));
    cmdParser.addOption(cmdOptionTheme);

    const QCommandLineOption cmdFileSettings(
        QStringList{"s", "settings"}, Main::tr("Settings file(INI format) to load at startup"),
        Main::tr("filepath"));
    cmdParser.addOption(cmdFileSettings);

    const QCommandLineOption cmdFileLog(QStringList{"log-file"},
                                        Main::tr("Writes log messages into output file"),
                                        Main::tr("filepath"));
    cmdParser.addOption(cmdFileLog);

    const QCommandLineOption cmdDebugLogs(
        QStringList{"debug-logs"},
        Main::tr("Don't filter out debug log messages in release build"));
    cmdParser.addOption(cmdDebugLogs);

    const QCommandLineOption cmdSysInfo(QStringList{"system-info"},
                                        Main::tr("Show detailed system information and quit"));
    cmdParser.addOption(cmdSysInfo);

    cmdParser.addPositionalArgument(Main::tr("files"),
                                    Main::tr("Files to open at startup, optionally"),
                                    Main::tr("[files...]"));

    cmdParser.process(QCoreApplication::arguments());

    // Retrieve arguments
    args.themeName = "dark";
    if (cmdParser.isSet(cmdOptionTheme))
        args.themeName = cmdParser.value(cmdOptionTheme);

    if (cmdParser.isSet(cmdFileSettings))
        args.filepathSettings = filepathFrom(cmdParser.value(cmdFileSettings));

    if (cmdParser.isSet(cmdFileLog))
        args.filepathLog = filepathFrom(cmdParser.value(cmdFileLog));

    for (const QString &posArg : cmdParser.positionalArguments())
        args.listFilepathToOpen.push_back(filepathFrom(posArg));

    args.showSystemInformation = cmdParser.isSet(cmdSysInfo);

    return args;
}

static std::unique_ptr<Theme> globalTheme;

// Declared in theme.h
Theme *mayoTheme()
{
    return globalTheme.get();
}

// Helper to query the OpenGL version string
[[maybe_unused]] static std::string queryGlVersionString()
{
    QOpenGLContext glContext;
    if (!glContext.create())
        return {};

    QOffscreenSurface surface;
    surface.create();
    if (!glContext.makeCurrent(&surface))
        return {};

    auto glVersion = glContext.functions()->glGetString(GL_VERSION);
    if (!glVersion)
        return {};

    return reinterpret_cast<const char *>(glVersion);
}

static Thumbnail createGuiDocumentThumbnail(GuiDocument *guiDoc, QSize size)
{
    Thumbnail thumbnail;

    IO::ImageWriter::Parameters params;
    params.width = size.width();
    params.height = size.height();
    params.backgroundColor =
        QtGuiUtils::toPreferredColorSpace(mayoTheme()->color(Theme::Color::Palette_Window));
    OccHandle<Image_AlienPixMap> pixmap = IO::ImageWriter::createImage(guiDoc, params);
    if (!pixmap)
    {
        qDebug() << "Empty pixmap returned by IO::ImageWriter::createImage()";
        return thumbnail;
    }

    GraphicsUtils::ImagePixmap_flipY(*pixmap);
    Image_PixMap::SwapRgbaBgra(*pixmap);
    const QPixmap qPixmap = QtGuiUtils::toQPixmap(*pixmap);
    thumbnail.imageData = QtGuiUtils::toQByteArray(qPixmap);
    thumbnail.imageCacheKey = qPixmap.cacheKey();
    return thumbnail;
}

// Initializes and runs Mayo application
static int runApp(QCoreApplication *qtApp)
{
    const CommandLineArguments args = processCommandLine();

    // Helper function: print critical message and exit application with code
    // failure
    auto fnCriticalExit = [](const QString &msg)
    {
        qCritical().noquote() << msg;
        std::exit(EXIT_FAILURE);
    };

    // Helper function: load application settings from INI file(if provided)
    // otherwise use the application regular storage(eg registry on Windows)
    auto fnLoadAppSettings = [&](Settings *appSettings)
    {
        if (args.filepathSettings.empty())
        {
            appSettings->load();
        }
        else
        {
            const QString strFilepathSettings = filepathTo<QString>(args.filepathSettings);
            if (!filepathIsRegularFile(args.filepathSettings))
                fnCriticalExit(Main::tr("Failed to load application settings file [path=%1]")
                                   .arg(strFilepathSettings));

            QSettingsStorage fileSettings(strFilepathSettings, QSettings::IniFormat);
            std::cout << "Loading settings from " << strFilepathSettings.toStdString() << std::endl;
            appSettings->loadFrom(fileSettings, &AppModule::excludeSettingPredicate);
        }
    };

    // Signals
    setGlobalSignalThreadHelper(std::make_unique<QtSignalThreadHelper>());

    // Message logging
    LogMessageHandler::instance().enableDebugLogs(args.includeDebugLogs);
    LogMessageHandler::instance().setOutputFilePath(args.filepathLog);

    // Initialize AppModule
    auto appModule = AppModule::get();
    appModule->settings()->setStorage(std::make_unique<QSettingsStorage>());
    appModule->setRecentFileThumbnailRecorder(&createGuiDocumentThumbnail);
    appModule->addLibraryInfo(IO::AssimpLib::strName(), IO::AssimpLib::strVersion(),
                              IO::AssimpLib::strVersionDetails());
    appModule->addLibraryInfo(IO::GmioLib::strName(), IO::GmioLib::strVersion(),
                              IO::GmioLib::strVersionDetails());

    {
        // Load translation files
        auto fnLoadQmFile = [=](const QString &qmFilePath)
        {
            auto translator = new QTranslator(qtApp);
            if (translator->load(qmFilePath))
                qtApp->installTranslator(translator);
            else
                qWarning() << Main::tr("Failed to load translation file [path=%1]").arg(qmFilePath);
        };
        const QString appLangCode = appModule->languageCode();
        fnLoadQmFile(QString(":/i18n/mayo_%1.qm").arg(appLangCode));
        fnLoadQmFile(QString(":/i18n/qtbase_%1.qm").arg(appLangCode));
    }

    // Initialize Base application
    auto app = appModule->application();
    TextId::addTranslatorFunction(&qtAppTranslate); // Set Qt i18n backend

    // Initialize Gui application
    auto guiApp = new GuiApplication(app);
    auto _ = gsl::finally([=] { delete guiApp; });

    /* init gl */
    constexpr bool use_glWidget = true;
    if (use_glWidget)
    {
        // Use QOpenGLWidget if possible
        GraphicsScene::setFunctionCreateGraphicsDriver(
            &QOpenGLWidgetOccView::createCompatibleGraphicsDriver);
        IWidgetOccView::setCreator(&QOpenGLWidgetOccView::create);
    }
    else
    {
        // Use QWidgetOccView
        GraphicsScene::setFunctionCreateGraphicsDriver(
            &QWidgetOccView::createCompatibleGraphicsDriver);
        IWidgetOccView::setCreator(&QWidgetOccView::create);
    }

    // Register Graphics entity drivers
    guiApp->addGraphicsObjectDriver(std::make_unique<GraphicsShapeObjectDriver>());
    guiApp->addGraphicsObjectDriver(std::make_unique<GraphicsMeshObjectDriver>());
    guiApp->addGraphicsObjectDriver(std::make_unique<GraphicsPointCloudObjectDriver>());

    // Register providers to query document tree node properties
    appModule->addPropertiesProvider(std::make_unique<XCaf_DocumentTreeNodePropertiesProvider>());
    appModule->addPropertiesProvider(std::make_unique<Mesh_DocumentTreeNodePropertiesProvider>());
    appModule->addPropertiesProvider(
        std::make_unique<PointCloud_DocumentTreeNodePropertiesProvider>());

    // Register I/O objects
    IO::System *ioSystem = appModule->ioSystem();
    ioSystem->addFactoryReader(std::make_unique<IO::DxfFactoryReader>());
    ioSystem->addFactoryReader(std::make_unique<IO::OccFactoryReader>());
    ioSystem->addFactoryReader(std::make_unique<IO::OffFactoryReader>());
    ioSystem->addFactoryReader(std::make_unique<IO::PlyFactoryReader>());
    ioSystem->addFactoryReader(IO::AssimpFactoryReader::create());
    ioSystem->addFactoryWriter(std::make_unique<IO::OccFactoryWriter>());
    ioSystem->addFactoryWriter(std::make_unique<IO::OffFactoryWriter>());
    ioSystem->addFactoryWriter(std::make_unique<IO::PlyFactoryWriter>());
    ioSystem->addFactoryWriter(IO::GmioFactoryWriter::create());
    ioSystem->addFactoryWriter(std::make_unique<IO::ImageFactoryWriter>(guiApp));
    IO::addPredefinedFormatProbes(ioSystem);
    appModule->properties()->IO_bindParameters(ioSystem);
    appModule->properties()->retranslate();

    // Process CLI
    if (args.showSystemInformation)
    {
        CommandSystemInformation cmdSysInfo(nullptr);
        cmdSysInfo.execute();
        return qtApp->exec();
    }

    // Record recent files when documents are closed
    guiApp->signalGuiDocumentErased.connectSlot(&AppModule::recordRecentFile, AppModule::get());

    // Register WidgetModelTreeBuilter prototypes
    WidgetModelTree::addPrototypeBuilder(std::make_unique<WidgetModelTreeBuilder_Mesh>());
    WidgetModelTree::addPrototypeBuilder(std::make_unique<WidgetModelTreeBuilder_Xde>());

    // Create theme
    globalTheme.reset(createTheme(args.themeName));
    if (!globalTheme)
        fnCriticalExit(Main::tr("Failed to load theme '%1'").arg(args.themeName));

    mayoTheme()->setup();

    // Create MainWindow
    MainWindow mainWindow(guiApp);
    mainWindow.setWindowTitle(QCoreApplication::applicationName());
    appModule->settings()->loadProperty(&appModule->properties()->appUiState);
    mainWindow.show();
    if (!args.listFilepathToOpen.empty())
    {
        QTimer::singleShot(0, qtApp,
                           [&] { mainWindow.openDocumentsFromList(args.listFilepathToOpen); });
    }

    appModule->settings()->resetAll();
    fnLoadAppSettings(appModule->settings());
    const int code = qtApp->exec();
    appModule->recordRecentFiles(guiApp);
    appModule->settings()->save();
    return code;
}
} // namespace Mayo

int main(int argc, char *argv[])
{
    qInstallMessageHandler(&Mayo::LogMessageHandler::qtHandler);

    // OpenCascade TKOpenGl depends on XLib for Linux(excepting Android) and BSD
    // systems(excepting macOS) See for example implementation of
    // Aspect_DisplayConnection where XLib is explicitly used On systems running
    // eg Wayland this would cause problems(see
    // https://github.com/fougue/mayo/issues/178) As a workaround the Qt platform
    // is forced to xcb
#if (defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)) || (defined(Q_OS_BSD4) && !defined(Q_OS_MACOS))
    if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "xcb");
#elif defined(Q_OS_HAIKU)
    if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "haiku");
#endif

    // Configure and create Qt application object
#if defined(Q_OS_WIN)
    // Never use ANGLE on Windows, since OCCT 3D Viewer does not expect this
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
#endif
    QCoreApplication::setOrganizationName("Fougue Ltd");
    QCoreApplication::setOrganizationDomain("www.fougue.pro");
    QCoreApplication::setApplicationName("Mayo");
    QCoreApplication::setApplicationVersion(QString::fromUtf8(Mayo::strVersion));
    QApplication app(argc, argv);

    // Run Mayo application GUI
    return Mayo::runApp(&app);
}
