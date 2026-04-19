#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QMenu>
#include <QPointer>
#include <QSharedPointer>
#include <QStringList>
#include <functional>
#include <memory>

#include "model/ModelTypes.hpp"

QT_BEGIN_NAMESPACE
class Ui_hsm_ide;
QT_END_NAMESPACE

class HsmGraphicsView;
class MainEditorController;
class ProjectController;
class SettingsController;
class WorkspaceView;
using ProjectControllerPtr = QSharedPointer<ProjectController>;

namespace model {
class StateMachineModel;
class StateMachineEntity;
}  // namespace model

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(MainEditorController* parent);
    virtual ~MainWindow();

    QPointer<HsmGraphicsView> currentView();
    QPointer<HsmGraphicsView> getViewByIndex(const int index);

    // UI actions and events
public slots:
    void handleOpenWorkspace();
    void handleCloseWorkspace();

    void handleNewFile();
    void handleOpenFile();
    void handleSave();
    void handleSaveAs();
    void handleUndo();
    void handleRedo();
    void handleCloseCurrentProject();
    void handleCloseAllProjects();

    void handleClipboardCopy();
    void handleClipboardCut();
    void handleClipboardPaste();
    void handleSelectAll();
    void handleClipboardDuplicate();
    void handleToggleGrid(const bool enabled);
    void handleToggleSnapToGrid(const bool enabled);

    void handleAbout();

    void projectTabSelected(int index);
    void projectTabCloseRequested(int index);

    void deleteSelectedItems();
    // void cutSelectedItems();
    // void pasteClipboardItems();
    void onGraphicsViewSelectionChanged();
    void onModelTreeSelectionChanged(const QModelIndex& current, const QModelIndex& previous);

    // from HsmGraphicsView
    void onHsmElementDoubleClickEvent(QWeakPointer<model::StateMachineEntity> entity);

    // MainController
public slots:
    void projectOpened(ProjectControllerPtr project);
    void projectSelected(ProjectControllerPtr project);
    void projectClosed(ProjectControllerPtr project);

private:
    void openWorkspace(const QString& rootDir);
    void updateMenuItemsRecent(QMenu* menu,
                               const QStringList& items,
                               const std::function<void(const QString&)>& callbackOpen,
                               const std::function<void()>& callbackClear);
    void updateRecentHsmMenu();
    void updateRecentWorkspacesMenu();
    void selectModelEntityById(const model::EntityID_t id);

    bool copySelectedItems();

private:
    Ui_hsm_ide* ui = nullptr;
    MainEditorController* mController = nullptr;
    std::unique_ptr<SettingsController> mSettingsController;
    ProjectControllerPtr mActiveProject;
    QString mLastDirectory;
    QString mAppTitle;
    QString mConfigPath;

private slots:
    void onSidebarActionTriggered(bool checked);
};

#endif  // MAINWINDOW_HPP
