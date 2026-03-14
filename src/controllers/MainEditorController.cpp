#include "MainEditorController.hpp"

#include <QApplication>
#include <QMap>
#include <QString>
#include <QUuid>
#include <algorithm>

#include "ProjectController.hpp"
#include "view/MainWindow.hpp"
#include "view/widgets/HsmGraphicsView.hpp"
#include "model/ModelUtils.hpp"
#include "model/IncludeEntity.hpp"

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
    // Clean up all project controllers. Memory is managed by QSharedPointer
    mProjectControllers.clear();
    mCurrentProject.clear();
}

int MainEditorController::start() {
    mMainWindow.show();

    // Create initial empty project
    createProject();

    return QApplication::exec();
}

ProjectControllerPtr MainEditorController::createProject() {
    qDebug() << Q_FUNC_INFO << __LINE__;
    const QString projectId = QUuid::createUuid().toString();
    ProjectControllerPtr project = ProjectControllerPtr::create(projectId, this);

    mProjectControllers.push_back(project);
    emit projectOpened(project);

    switchToProject(project);

    return project;
}

ProjectControllerPtr MainEditorController::openProject(const QString& projectPath) {
    ProjectControllerPtr project = getProjectByPath(projectPath);

    if (project.isNull()) {
        if (projectPath.isEmpty() == false) {
            project = createProject();

            if (project) {
                if (false == project->importModel(projectPath)) {
                    // do not open project if we failed to load the model
                    project.reset();
                } else {
                    emit hsmProjectOpened(projectPath);
                }
            }
        }
    } else {
        switchToProject(project);
        emit hsmProjectOpened(projectPath);
    }

    return project;
}

void MainEditorController::closeProject(const ProjectControllerPtr& project) {
    qDebug() << "MainEditorController::closeProject" << project->name();
    auto it = std::find(mProjectControllers.begin(), mProjectControllers.end(), project);

    if (it != mProjectControllers.end()) {
        if (mCurrentProject == project) {
            mCurrentProject.clear();
        }

        emit projectClosed(project);
        it = mProjectControllers.erase(it);

        if (it != mProjectControllers.end()) {
            switchToProject(*it);
        }
    }
}

void MainEditorController::switchToProject(const ProjectControllerPtr& project) {
    auto it = std::find(mProjectControllers.begin(), mProjectControllers.end(), project);

    if (it != mProjectControllers.end()) {
        if (mCurrentProject != project) {
            mCurrentProject = *it;
            emit projectSelected(mCurrentProject);
        }
    }
}

void MainEditorController::closeAllProjects() {
    mCurrentProject.clear();

    for (const auto& project : mProjectControllers) {
        emit projectClosed(project);
    }

    mProjectControllers.clear();
}

const QList<ProjectControllerPtr>& MainEditorController::openedProjects() const {
    return mProjectControllers;
}

ProjectControllerPtr MainEditorController::getProjectByPath(const QString& projectPath) const {
    ProjectControllerPtr project;

    for (const auto& ptr : mProjectControllers) {
        if (ptr && (ptr->modelPath() == projectPath)) {
            project = ptr;
            break;
        }
    }

    return project;
}

void MainEditorController::handleHsmElementDoubleClick(QWeakPointer<model::StateMachineEntity> entity) {
    auto ptrInclude = model::hsmDynamicCast<model::IncludeEntity>(entity.toStrongRef(), model::StateType::INCLUDE);

    if (ptrInclude) {
        openProject(ptrInclude->path());
    }
}

// QPointer<ProjectController> MainEditorController::getProjectController(const QString& projectId) const {
//     return mProjectControllers.value(projectId, nullptr);
// }
