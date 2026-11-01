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

namespace view {
class DropOverlayWidget;
}  // namespace view

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(MainEditorController* parent);
    virtual ~MainWindow();

    void openWorkspace(const QString& rootDir);

protected:
    void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

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

    // Installs the single central-area drop filter on every drop target in the
    // central subtree that could sit under the cursor during a file drag, and
    // ensures each accepts drops. Idempotent; safe to call again when a new view
    // is added so its viewport joins the filtered set.
    void installCentralAreaDropFilter();

    // Shared handler for the central-area file drop-to-open. Returns true iff it
    // consumed the event (uri-list drags only); false for everything else so
    // hsm/element drags continue to reach HsmGraphicsView unchanged.
    bool handleCentralAreaDragDrop(QEvent* event);

    // True iff watched is a central-subtree drop target: the central widget, the
    // tab widget, its tab bar, or one of the current HsmGraphicsView viewports.
    bool isCentralAreaDropTarget(QObject* watched) const;

    // Lazily creates the single overlay, sizes it to the central widget, raises
    // it above the tabs/editor, and shows it.
    void showDropOverlay();

    // Hides the overlay if it exists.
    void hideDropOverlay();

private:
    Ui_hsm_ide* ui = nullptr;
    MainEditorController* mController = nullptr;
    std::unique_ptr<SettingsController> mSettingsController;
    ProjectControllerPtr mActiveProject;
    QString mLastDirectory;
    QString mAppTitle;
    QString mConfigPath;
    QPointer<view::DropOverlayWidget> mDropOverlay;

private slots:
    void onSidebarActionTriggered(bool checked);
};

#endif  // MAINWINDOW_HPP
