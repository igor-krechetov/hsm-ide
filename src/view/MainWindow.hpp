#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QPointer>
#include <QSharedPointer>

#include "model/ModelTypes.hpp"

QT_BEGIN_NAMESPACE
class Ui_hsm_ide;
QT_END_NAMESPACE

class HsmGraphicsView;
class MainEditorController;
class ProjectController;
using ProjectControllerPtr = QSharedPointer<ProjectController>;

namespace model {
class StateMachineModel;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(MainEditorController* parent);
    virtual ~MainWindow();

    QPointer<HsmGraphicsView> currentView();

    // UI actions and events
public slots:
    void handleNewProject();
    void handleOpen();
    void handleSave();
    void handleSaveAs();
    void handleCloseCurrentProject();
    void handleCloseAllProjects();

    void projectTabSelected(int index);
    void projectTabCloseRequested(int index);

    void deleteSelectedItems();
    void onGraphicsViewSelectionChanged();
    void onModelTreeSelectionChanged(const QModelIndex& current, const QModelIndex& previous);

    // MainController
public slots:
    void projectOpened(ProjectControllerPtr project);
    void projectSelected(ProjectControllerPtr project);
    void projectClosed(ProjectControllerPtr project);

private:
    void selectModelEntityById(const model::EntityID_t id);

private:
    Ui_hsm_ide* ui = nullptr;
    MainEditorController* mController = nullptr;
    ProjectControllerPtr mActiveProject;
    QString mLastDirectory;
    QString mAppTitle;
    QString mConfigPath;
    QString mCurrentFilePath;  // Tracks the current file path for Save/SaveAs

private slots:
    void onSidebarActionTriggered(bool checked);
};

#endif  // MAINWINDOW_HPP
