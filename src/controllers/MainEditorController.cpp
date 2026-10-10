#include "MainEditorController.hpp"

#include <QApplication>
#include <QMap>
#include <QString>
#include <QUuid>
#include <algorithm>

#include "ProjectController.hpp"
#include "controllers/private/UnsavedChangesGuard.hpp"
#include "model/ModelUtils.hpp"
#include "model/elements/IncludeEntity.hpp"
#include "view/MainWindow.hpp"
#include "view/widgets/HsmGraphicsView.hpp"

namespace {

// Translates the public prompt outcome into the private guard's choice vocabulary.
UnsavedChangesGuard::Choice toGuardChoice(const UnsavedChangesChoice choice) {
    UnsavedChangesGuard::Choice guardChoice = UnsavedChangesGuard::Choice::Cancel;

    switch (choice) {
        case UnsavedChangesChoice::SaveAll:
            guardChoice = UnsavedChangesGuard::Choice::SaveAll;
            break;

        case UnsavedChangesChoice::Discard:
            guardChoice = UnsavedChangesGuard::Choice::Discard;
            break;

        case UnsavedChangesChoice::Cancel:
            guardChoice = UnsavedChangesGuard::Choice::Cancel;
            break;
    }

    return guardChoice;
}

}  // namespace

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
    mMainWindow.setWindowIcon(QIcon(":/icons/hsm_ide.png"));
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

bool MainEditorController::confirmDiscardUnsaved(const QList<ProjectControllerPtr>& projects,
                                                 const bool singleProjectContext,
                                                 const UnsavedChangesPromptCallback& promptCallback,
                                                 const SaveProjectCallback& saveCallback) {
    bool proceed = true;
    QList<UnsavedChangesGuard::GuardedProject> guardedProjects;

    for (const ProjectControllerPtr& project : projects) {
        if (project) {
            guardedProjects.append({project->name(), project->isModified(), project.data()});
        }
    }

    if (UnsavedChangesGuard::promptRequired(guardedProjects) && promptCallback) {
        const QStringList affected = UnsavedChangesGuard::affectedProjectNames(guardedProjects);
        const UnsavedChangesChoice choice = promptCallback(affected, singleProjectContext);
        const UnsavedChangesGuard::Decision decision = UnsavedChangesGuard::resolve(guardedProjects, toGuardChoice(choice));

        proceed = decision.proceedWithClose;

        for (void* handle : decision.projectsToSave) {
            const ProjectController* raw = static_cast<const ProjectController*>(handle);
            ProjectControllerPtr project;

            for (const ProjectControllerPtr& candidate : projects) {
                if (candidate.data() == raw) {
                    project = candidate;
                    break;
                }
            }

            if (!saveCallback || !saveCallback(project)) {
                // Abort the whole close: a save failed or was cancelled.
                proceed = false;
                break;
            }
        }
    }

    return proceed;
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

void MainEditorController::handleWorkspacePathRenamed(const QString& oldPath, const QString& newPath) {
    if ((oldPath.isEmpty() == false) && (newPath.isEmpty() == false)) {
        for (const auto& project : mProjectControllers) {
            if (project && (project->modelPath() == oldPath)) {
                project->updateModelPath(newPath);
                break;
            }
        }
    }
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
