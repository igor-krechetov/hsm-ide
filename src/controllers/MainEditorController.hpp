#ifndef MAINEDITORCONTROLLER_HPP
#define MAINEDITORCONTROLLER_HPP

#include <QObject>
#include <QSharedPointer>
#include <QVector>
#include <QMap>
#include <QUuid>

#include "view/MainWindow.hpp"

class ProjectController;
using ProjectControllerPtr = QSharedPointer<ProjectController>;

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

signals:
    void projectOpened(ProjectControllerPtr project);
    void projectSelected(ProjectControllerPtr project);
    void projectClosed(ProjectControllerPtr project);

private:
    MainWindow mMainWindow;
    QList<ProjectControllerPtr> mProjectControllers;
    ProjectControllerPtr mCurrentProject;
};

#endif  // MAINEDITORCONTROLLER_HPP
