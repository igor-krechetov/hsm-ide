#ifndef MAINEDITORCONTROLLER_HPP
#define MAINEDITORCONTROLLER_HPP

#include <QObject>
#include <QSharedPointer>
#include <QVector>
#include <QMap>
#include <QUuid>

#include "view/MainWindow.hpp"

class ProjectController;

class MainEditorController : public QObject {
    Q_OBJECT
public:
    MainEditorController();
    virtual ~MainEditorController();

    int start();

    // Methods to manage multiple projects
    QString createProject(); // returns project ID
    QString openProject(const QString& projectPath); // returns project ID
    void closeProject(const QString& projectId);
    void switchToProject(const QString& projectId);
    QList<QString> openedProjects() const; // returns list of project IDs

    QString getProjectIdByPath(const QString& projectPath) const;
    QPointer<ProjectController> getProjectController(const QString& projectId) const;

signals:
    void projectOpened(QPointer<ProjectController> project);
    void projectSelected(QPointer<ProjectController> project);
    void projectClosed(const QString& projectId);

private:
    MainWindow mMainWindow;
    // Manage multiple ProjectControllers by unique ID
    QMap<QString, ProjectController*> mProjectControllers; // key: projectId
    QString mCurrentProjectId;// TODO: replace with ProjectController*
};

#endif  // MAINEDITORCONTROLLER_HPP
