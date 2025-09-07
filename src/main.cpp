#include <QApplication>
#include <QLocale>
#include <QTranslator>

#include "controllers/MainEditorController.hpp"
#include "model/ModelTypes.hpp"

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);

    qRegisterMetaType<model::EntityID_t>("model::EntityID_t");

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
