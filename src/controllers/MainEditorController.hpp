#ifndef MAINEDITORCONTROLLER_HPP
#define MAINEDITORCONTROLLER_HPP

#include <QMap>
#include <QObject>
#include <QSharedPointer>
#include <QUuid>
#include <QVector>

#include "view/MainWindow.hpp"

class ProjectController;
using ProjectControllerPtr = QSharedPointer<ProjectController>;

namespace model {
    class StateMachineEntity;
};

class MainEditorController : public QObject {
    Q_OBJECT
public:
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

    ProjectControllerPtr getProjectByPath(const QString& projectPath) const;

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
