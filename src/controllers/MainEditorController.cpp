#include "MainEditorController.hpp"

#include <QApplication>
#include <QMap>
#include <QString>
#include <QUuid>

#include "ProjectController.hpp"
#include "view/MainWindow.hpp"
#include "view/widgets/HsmGraphicsView.hpp"

MainEditorController::MainEditorController()
    : QObject(nullptr)
    , mMainWindow(this) {
    // No default project controller; projects are opened via openProject()
    // connect signals with MainWindow
    connect(this, &MainEditorController::projectOpened, &mMainWindow, &MainWindow::projectOpened);
    connect(this, &MainEditorController::projectSelected, &mMainWindow, &MainWindow::projectSelected);
    connect(this, &MainEditorController::projectClosed, &mMainWindow, &MainWindow::projectClosed);
}

MainEditorController::~MainEditorController() {
    // Clean up all project controllers. Actual deletion of ProjectController instances
    // will be handled with QObject parent-child mechanism.
    // TODO: validate
    mProjectControllers.clear();
}

int MainEditorController::start() {
    mMainWindow.show();

    // Create initial empty project
    createProject();

    return QApplication::exec();
}

QString MainEditorController::createProject() {
    qDebug() << Q_FUNC_INFO << __LINE__;
    QString projectId = QUuid::createUuid().toString();
    ProjectController* controller = new ProjectController(projectId, this);

    mProjectControllers.insert(projectId, controller);
    // mMainWindow.setProjectController(controller);
    mCurrentProjectId = projectId;
    emit projectOpened(controller);

    switchToProject(projectId);

    return projectId;
}

QString MainEditorController::openProject(const QString& projectPath) {
    QString projectId = getProjectIdByPath(projectPath);

    if (projectId.isEmpty() == true) {
        projectId = createProject();
        auto controller = mProjectControllers.value(projectId);

        if (controller) {
            controller->importModel(projectPath);
        }
    } else {
        switchToProject(projectId);
    }

    return projectId;
}

void MainEditorController::closeProject(const QString& projectId) {
    if (mProjectControllers.contains(projectId)) {
        ProjectController* project = mProjectControllers.take(projectId);

        if (mCurrentProjectId == projectId) {
            mCurrentProjectId.clear();
        }

        delete project;
    }
}

void MainEditorController::switchToProject(const QString& projectId) {
    qDebug() << Q_FUNC_INFO << __LINE__;
    if (mProjectControllers.contains(projectId)) {
        mCurrentProjectId = projectId;
        emit projectSelected(mProjectControllers.value(projectId));
    }
}

QList<QString> MainEditorController::openedProjects() const {
    return mProjectControllers.keys();
}

QString MainEditorController::getProjectIdByPath(const QString& projectPath) const {
    // TODO: implement
    // for (auto it = mProjectControllers.constBegin(); it != mProjectControllers.constEnd(); ++it) {
    //     ProjectController* controller = it.value();
    //     if (controller && controller->projectPath() == projectPath) {
    //         return it.key();
    //     }
    // }
    return QString();
}

QPointer<ProjectController> MainEditorController::getProjectController(const QString& projectId) const {
    return mProjectControllers.value(projectId, nullptr);
}
