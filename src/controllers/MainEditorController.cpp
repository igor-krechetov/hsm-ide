#include "MainEditorController.hpp"

#include <QApplication>

#include "ProjectController.hpp"
#include "view/MainWindow.hpp"
#include "view/widgets/HsmGraphicsView.hpp"

MainEditorController::MainEditorController()
    : QObject(nullptr)
    , mProjectController(new ProjectController(&mMainWindow, this)) {
    mMainWindow.view()->setProjectController(mProjectController);
}

int MainEditorController::start() {
    mMainWindow.show();
    return QApplication::exec();
}
