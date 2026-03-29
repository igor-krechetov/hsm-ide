#include <QApplication>
#include <QCoreApplication>
#include <QLocale>
#include <QTranslator>

#include "controllers/MainEditorController.hpp"
#include "model/ModelTypes.hpp"
#include "model/actions/IModelAction.hpp"

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("hsmcpp");
    QCoreApplication::setApplicationName("hsm-ide");

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
