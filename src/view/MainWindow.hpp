#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QList>
#include <QMainWindow>
#include <QMenu>
#include <QPointer>
#include <QSharedPointer>
#include <QStringList>
#include <functional>
#include <memory>

#include "controllers/UnsavedChangesChoice.hpp"
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

protected:
    void closeEvent(QCloseEvent* event) override;

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
    void handleEditSelected();
    void handleDuplicateSelected();
    void handleZoomIn();
    void handleZoomOut();
    void handleResetZoom();
    void handleFitToView();
    void handleToggleGrid(const bool enabled);
    void handleToggleSnapToGrid(const bool enabled);

    void handleAbout();
    void handleKeyboardShortcuts();

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

#ifdef DEBUG_RENDERING
private slots:
    void onViewMouseMoved(const QPointF& scenePos);
#endif

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

    // Runs the controller's unsaved-changes guard, supplying this window's prompt and save
    // callbacks. Returns true if the close may proceed.
    bool confirmDiscardUnsaved(const QList<ProjectControllerPtr>& projects, const bool singleProjectContext);

    // Presents the unsaved-changes prompt and returns the user's choice. Passed to the
    // controller as its prompt callback; `singleProjectContext` tweaks the Save vs Save
    // All wording.
    UnsavedChangesChoice promptForUnsavedChanges(const QStringList& affectedProjectNames, const bool singleProjectContext);

    // Saves a project, falling back to a "Save As" dialog when it has no backing file.
    // Returns false if writing failed or the user cancelled the path dialog.
    bool saveProject(const ProjectControllerPtr& project);

    // Prompts for a destination path and writes the project there, updating the recent
    // files list on success. Returns false if writing failed or the dialog was cancelled.
    bool saveProjectAs(const ProjectControllerPtr& project);

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
