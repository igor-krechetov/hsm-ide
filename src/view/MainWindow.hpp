#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QPointer>

#include "model/ModelTypes.hpp"

QT_BEGIN_NAMESPACE
class Ui_hsm_ide;
QT_END_NAMESPACE

class HsmGraphicsView;
class MainEditorController;
class ProjectController;

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

    void projectTabSelected(int index);
    void projectTabCloseRequested(int index);

    void deleteSelectedItems();
    void onGraphicsViewSelectionChanged();
    void onModelTreeSelectionChanged(const QModelIndex &current, const QModelIndex &previous);

// MainController
public slots:
    void projectOpened(QPointer<ProjectController> project);
    void projectSelected(QPointer<ProjectController> project);
    void projectClosed(const QString& projectId);

private:
    void selectModelEntityById(const model::EntityID_t id);

private:
    Ui_hsm_ide* ui = nullptr;
    MainEditorController* mController = nullptr;
    QString mLastDirectory;
    QString mAppTitle;
    QString mConfigPath;
    QPointer<ProjectController> mActiveProject;
    QString mCurrentFilePath; // Tracks the current file path for Save/SaveAs

private slots:
    void onSidebarActionTriggered(bool checked);
};

#endif  // MAINWINDOW_HPP
