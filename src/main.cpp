#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QLocale>
#include <QMessageLogContext>
#include <QMutex>
#include <QTextStream>
#include <QTranslator>

#include "controllers/MainEditorController.hpp"
#include "model/ModelTypes.hpp"
#include "model/actions/IModelAction.hpp"

#if defined(DEBUG_OUTPUT_FILE) || defined(DEBUG_OUTPUT_CONSOLE)

static QFile g_logFile;

static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    static QMutex mutex;
    QMutexLocker locker(&mutex);

    QString prefix;
    switch (type) {
        case QtDebugMsg:
            prefix = "[DEBUG]   ";
            break;
        case QtInfoMsg:
            prefix = "[INFO]    ";
            break;
        case QtWarningMsg:
            prefix = "[WARNING] ";
            break;
        case QtCriticalMsg:
            prefix = "[CRITICAL]";
            break;
        case QtFatalMsg:
            prefix = "[FATAL]   ";
            break;
    }

    QString location;
    if (context.file != nullptr) {
        location = QString(" (%1:%2)").arg(context.file).arg(context.line);
    }

  #ifdef DEBUG_OUTPUT_FILE
    if (g_logFile.isOpen()) {
        QTextStream out(&g_logFile);
        out << prefix << " " << msg << location << "\n";
        out.flush();
    }
  #endif

  #ifdef DEBUG_OUTPUT_CONSOLE
    QTextStream cerr(stderr);
    cerr << prefix << " " << msg << location << "\n";
    cerr.flush();
  #endif
}

#endif  // DEBUG_OUTPUT_FILE || DEBUG_OUTPUT_CONSOLE

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("hsmcpp");
    QCoreApplication::setApplicationName("hsm-ide");

#if defined(DEBUG_OUTPUT_FILE) || defined(DEBUG_OUTPUT_CONSOLE)
  #ifdef DEBUG_OUTPUT_FILE
    const QString logPath = QCoreApplication::applicationDirPath() + "/" + QCoreApplication::applicationName() + ".log";
    g_logFile.setFileName(logPath);
    g_logFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
  #endif
    qInstallMessageHandler(messageHandler);
#endif
    QGuiApplication::setApplicationDisplayName("HSM IDE");
    QGuiApplication::setDesktopFileName("hsm_ide");
    QGuiApplication::setWindowIcon(QIcon(":/icons/hsm_ide.png"));

    qDebug() << QGuiApplication::windowIcon().isNull();

    qRegisterMetaType<model::EntityID_t>("model::EntityID_t");
    qRegisterMetaType<QSharedPointer<model::IModelAction>>("QSharedPointer<model::IModelAction>");

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString& locale : uiLanguages) {
        const QString baseName = "hsm_ide_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    MainEditorController controller;
    return controller.start();
}
