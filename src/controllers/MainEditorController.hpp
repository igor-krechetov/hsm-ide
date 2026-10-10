#ifndef MAINEDITORCONTROLLER_HPP
#define MAINEDITORCONTROLLER_HPP

#include <QList>
#include <QMap>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QUuid>
#include <QVector>
#include <functional>

#include "controllers/UnsavedChangesChoice.hpp"
#include "view/MainWindow.hpp"

class ProjectController;
using ProjectControllerPtr = QSharedPointer<ProjectController>;

namespace model {
class StateMachineEntity;
};

class MainEditorController : public QObject {
    Q_OBJECT
public:
    // Callback the caller supplies to present the unsaved-changes prompt. It receives the
    // names of the affected projects and whether this is a single-project context (which
    // tweaks the Save vs Save All wording), and returns the user's choice.
    using UnsavedChangesPromptCallback =
        std::function<UnsavedChangesChoice(const QStringList& affectedProjectNames, bool singleProjectContext)>;
    // Callback the caller supplies to save one project, prompting for a destination path if
    // the project is untitled. Returns the save result (false if it failed or was cancelled).
    using SaveProjectCallback = std::function<bool(const ProjectControllerPtr& project)>;

    MainEditorController();
    virtual ~MainEditorController();

    int start();

    // Methods to manage multiple projects
    ProjectControllerPtr createProject();
    ProjectControllerPtr openProject(const QString& projectPath);
    void closeProject(const ProjectControllerPtr& project);
    void switchToProject(const ProjectControllerPtr& project);
    void closeAllProjects();

    const QList<ProjectControllerPtr>& openedProjects() const;

    // Orchestrates the unsaved-changes guard for a set of projects about to be discarded.
    // The caller supplies the prompt and save callbacks (presentation stays in the view).
    // Returns true if the close may proceed; false if the user cancelled or a save failed.
    bool confirmDiscardUnsaved(const QList<ProjectControllerPtr>& projects,
                               const bool singleProjectContext,
                               const UnsavedChangesPromptCallback& promptCallback,
                               const SaveProjectCallback& saveCallback);

    ProjectControllerPtr getProjectByPath(const QString& projectPath) const;
    void handleWorkspacePathRenamed(const QString& oldPath, const QString& newPath);

    void handleHsmElementDoubleClick(QWeakPointer<model::StateMachineEntity> entity);

signals:
    void projectOpened(ProjectControllerPtr project);
    void projectSelected(ProjectControllerPtr project);
    void projectClosed(ProjectControllerPtr project);
    void hsmProjectOpened(const QString& path);

private:
    MainWindow mMainWindow;
    QList<ProjectControllerPtr> mProjectControllers;
    ProjectControllerPtr mCurrentProject;
};

#endif  // MAINEDITORCONTROLLER_HPP
