#ifndef MAINEDITORCONTROLLER_HPP
#define MAINEDITORCONTROLLER_HPP

#include <QObject>
#include <QSharedPointer>

#include "view/MainWindow.hpp"

class ProjectController;

class MainEditorController : public QObject {
    Q_OBJECT
public:
    MainEditorController();
    virtual ~MainEditorController() = default;

    int start();

private:
    // QSharedPointer<MainWindow> mMainWindow;
    MainWindow mMainWindow;
    // QSharedPointer<ProjectController> mProjectController;
    ProjectController* mProjectController = nullptr;
};

#endif  // MAINEDITORCONTROLLER_HPP
