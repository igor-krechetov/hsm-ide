#include "MainWindow.hpp"

#include <QAction>
#include <QClipboard>
#include <QCursor>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMessageBox>
#include <QSignalBlocker>

#include "./ui/ui_main.h"
#include "AboutDialog.hpp"
#include "controllers/MainEditorController.hpp"
#include "controllers/ProjectController.hpp"
#include "controllers/SettingsController.hpp"
#include "view/models/StateMachineEntityViewModel.hpp"
#include "view/models/StateMachineTreeModel.hpp"
#include "view/widgets/WorkspaceView.hpp"

MainWindow::MainWindow(MainEditorController* parent)
    : QMainWindow(nullptr)
    , ui(new Ui_hsm_ide)
    , mController(parent) {
    ui->setupUi(this);
    connect(ui->actionDuplicate, &QAction::triggered, this, &MainWindow::handleClipboardDuplicate);

    // Conect events for HsmTreeView
    connect(ui->modelTree, &HsmTreeView::elementDoubleClickEvent, this, &MainWindow::onHsmElementDoubleClickEvent);

    // Connect sidebar actions to generic slot using toggled(bool)
    QList<QAction*> sidebarActions = ui->leftSideBar->actions();
    for (QAction* action : sidebarActions) {
        connect(action, &QAction::toggled, this, &MainWindow::onSidebarActionTriggered);
    }

    // Delete dummy tab
    ui->projectTabs->removeTab(0);

    mSettingsController = std::make_unique<SettingsController>();
    updateRecentHsmMenu();
    updateRecentWorkspacesMenu();

    connect(mController, &MainEditorController::hsmProjectOpened, this, [this](const QString& path) {
        if (mSettingsController) {
            mSettingsController->addRecentHsm(path);
            updateRecentHsmMenu();
        }
    });

    connect(ui->btnShowEmptyFolders, &QToolButton::toggled, ui->workspaceTree, &WorkspaceView::setShowEmptyFolders);
    connect(ui->btnCreateWorkspaceFile,
            &QToolButton::clicked,
            ui->workspaceTree,
            &WorkspaceView::createWorkspaceFileInSelection);
    connect(ui->btnCreateWorkspaceFolder,
            &QToolButton::clicked,
            ui->workspaceTree,
            &WorkspaceView::createWorkspaceFolderInSelection);
    connect(ui->btnCollapseWorkspace, &QToolButton::clicked, ui->workspaceTree, &WorkspaceView::collapseWorkspaceTree);
    connect(ui->workspaceTree, &WorkspaceView::workspaceFileActivated, this, [this](const QString& path) {
        if (path.isEmpty() == false) {
            mController->openProject(path);
        }
    });
    connect(
        ui->workspaceTree,
        &WorkspaceView::workspacePathRenamed,
        this,
        [this](const QString& oldPath, const QString& newPath) { mController->handleWorkspacePathRenamed(oldPath, newPath); });

    // Select default side menu
    ui->actionShowTabHsmElements->setChecked(true);
    ui->actionUndo->setEnabled(false);
    ui->actionRedo->setEnabled(false);
}

MainWindow::~MainWindow() {
    delete ui;
}

QPointer<HsmGraphicsView> MainWindow::currentView() {
    if ((nullptr != ui->projectTabs) && (ui->projectTabs->count() > 0)) {
        return qobject_cast<HsmGraphicsView*>(ui->projectTabs->currentWidget());
    }

    return nullptr;
}

QPointer<HsmGraphicsView> MainWindow::getViewByIndex(const int index) {
    QPointer<HsmGraphicsView> res;

    if (nullptr != ui->projectTabs) {
        res = qobject_cast<HsmGraphicsView*>(ui->projectTabs->widget(index));
    }

    return res;
}

void MainWindow::handleOpenWorkspace() {
    QString rootDir = QFileDialog::getExistingDirectory(this, tr("Select Directory"));

    if (rootDir.isEmpty() == false) {
        openWorkspace(rootDir);

        if (mSettingsController) {
            mSettingsController->addRecentWorkspace(rootDir);
            updateRecentWorkspacesMenu();
        }
    }
}

void MainWindow::handleCloseWorkspace() {
    Q_ASSERT(ui->workspaceTree != nullptr);

    ui->workspaceTree->clearWorkspace();
}

void MainWindow::handleNewFile() {
    mController->createProject();
}

void MainWindow::handleOpenFile() {
    const QString initialDir = ((nullptr == mActiveProject || mActiveProject->modelPath().isEmpty())
                                    ? QString("~")
                                    : QFileInfo(mActiveProject->modelPath()).absolutePath());
    QString fileName =
        QFileDialog::getOpenFileName(this, tr("Open SCXML File"), initialDir, tr("SCXML Files (*.scxml);;All Files (*)"));

    if (fileName.isEmpty() == false) {
        mController->openProject(fileName);
    }
}

void MainWindow::handleSave() {
    if (nullptr != mActiveProject) {
        if (mActiveProject->modelPath().isEmpty()) {
            handleSaveAs();
        } else {
            mActiveProject->exportModel();
        }
    }
}

void MainWindow::handleSaveAs() {
    const QString initialDir = ((nullptr == mActiveProject || mActiveProject->modelPath().isEmpty())
                                    ? QString("~")
                                    : QFileInfo(mActiveProject->modelPath()).absolutePath());
    QString fileName =
        QFileDialog::getSaveFileName(this, tr("Save SCXML File As"), initialDir, tr("SCXML Files (*.scxml);;All Files (*)"));

    if (!fileName.isEmpty() && mActiveProject) {
        if (!fileName.endsWith(".scxml", Qt::CaseInsensitive)) {
            fileName += ".scxml";
        }

        mActiveProject->exportModel(fileName);
    }
}

void MainWindow::handleUndo() {
    if (mActiveProject) {
        mActiveProject->undo();
        ui->actionUndo->setEnabled(mActiveProject->canUndo());
        ui->actionRedo->setEnabled(mActiveProject->canRedo());
    }
}

void MainWindow::handleRedo() {
    if (mActiveProject) {
        mActiveProject->redo();
        ui->actionUndo->setEnabled(mActiveProject->canUndo());
        ui->actionRedo->setEnabled(mActiveProject->canRedo());
    }
}

void MainWindow::handleCloseCurrentProject() {
    if (mActiveProject) {
        mController->closeProject(mActiveProject);
    }
}

void MainWindow::handleCloseAllProjects() {
    // block signals from projectTabs to avoid processing tab selection events
    QSignalBlocker block(ui->projectTabs);

    mActiveProject.clear();
    mController->closeAllProjects();
}

// =================================================================================================================
// Edit menu items

void MainWindow::handleClipboardCopy() {
    copySelectedItems();
}

void MainWindow::handleClipboardCut() {
    if (copySelectedItems() == true) {
        deleteSelectedItems();
    }
}

void MainWindow::handleClipboardPaste() {
    if (mActiveProject) {
        const QString clipboardText = QGuiApplication::clipboard()->text();
        QPointer<HsmGraphicsView> viewPtr = currentView();

        if ((clipboardText.isEmpty() == false) && (nullptr != viewPtr)) {
            const QPoint cursorPosInView = viewPtr->viewport()->mapFromGlobal(QCursor::pos());
            const QPointF cursorScenePos = viewPtr->mapToScene(cursorPosInView);
            mActiveProject->pasteScxmlElements(clipboardText, viewPtr->getSelectedElements(), cursorScenePos, true);
        }
    }
}

void MainWindow::handleSelectAll() {
    QPointer<HsmGraphicsView> viewPtr = currentView();

    if (nullptr != viewPtr) {
        viewPtr->selectAllHsmElements();
}

void MainWindow::handleClipboardDuplicate() {
    if (copySelectedItems() == true) {
        QPointer<HsmGraphicsView> viewPtr = currentView();

        if (nullptr != viewPtr) {
            viewPtr->clearSelection();
            handleClipboardPaste();
        }
    }
}

// =================================================================================================================
void MainWindow::handleAbout() {
    AboutDialog aboutDlg(this);

    aboutDlg.exec();
}

void MainWindow::projectTabSelected(int index) {
    if (nullptr != mController) {
        QPointer<HsmGraphicsView> view = getViewByIndex(index);

        if (nullptr != view) {
            mController->switchToProject(view->projectController());
        }
    }
}

void MainWindow::projectTabCloseRequested(int index) {
    if (nullptr != mController) {
        QPointer<HsmGraphicsView> view = getViewByIndex(index);

        if (nullptr != view) {
            mController->closeProject(view->projectController());
        }
    }
}

void MainWindow::deleteSelectedItems() {
    QPointer<HsmGraphicsView> viewPtr = currentView();

    if ((nullptr != viewPtr) && (nullptr != viewPtr->scene())) {
        viewPtr->deleteSelectedItems();
    }
}

bool MainWindow::copySelectedItems() {
    bool res = false;
    QPointer<HsmGraphicsView> viewPtr = currentView();

    if ((nullptr != viewPtr) && (nullptr != viewPtr->scene()) && mActiveProject) {
        const QList<model::EntityID_t> selectedIds = viewPtr->getSelectedElements();
        const QString scxmlData = mActiveProject->serializeElementsToScxml(selectedIds);

        if (scxmlData.isEmpty() == false) {
            QGuiApplication::clipboard()->setText(scxmlData);
            res = true;
        }
    }

    return res;
}

// void MainWindow::cutSelectedItems() {
//     QPointer<HsmGraphicsView> viewPtr = currentView();

//     if ((nullptr != viewPtr) && (nullptr != viewPtr->scene()) && mActiveProject) {
//         const QList<model::EntityID_t> selectedIds = viewPtr->getSelectedElements();
//         const QString scxmlData = mActiveProject->serializeElementsToScxml(selectedIds);

//         if (scxmlData.isEmpty() == false) {
//             QGuiApplication::clipboard()->setText(scxmlData);
//             deleteSelectedItems();
//         }
//     }
// }

// void MainWindow::pasteClipboardItems() {
//     if (mActiveProject) {
//         const QString clipboardText = QGuiApplication::clipboard()->text();

//         if (clipboardText.isEmpty() == false) {
//             mActiveProject->pasteScxmlElements(clipboardText, currentView()->getSelectedElements());
//         }
//     }
// }

void MainWindow::onGraphicsViewSelectionChanged() {
    // Get selected elements from graphics view
    auto selectedIds = currentView()->getSelectedElements();

    if (selectedIds.size() == 1) {
        selectModelEntityById(selectedIds.first());
    }
    // TODO: decide what to do with multi selection
}

void MainWindow::onModelTreeSelectionChanged(const QModelIndex& current, const QModelIndex& /*previous*/) {
    selectModelEntityById(current.data(Qt::UserRole).toUInt());
}

void MainWindow::openWorkspace(const QString& rootDir) {
    Q_ASSERT(ui->workspaceTree != nullptr);
    ui->workspaceTree->setWorkspaceRoot(rootDir);
    ui->workspaceTree->setShowEmptyFolders(ui->btnShowEmptyFolders->isChecked());
}

void MainWindow::updateMenuItemsRecent(QMenu* menu,
                                       const QStringList& items,
                                       const std::function<void(const QString&)>& callbackOpen,
                                       const std::function<void()>& callbackClear) {
    Q_ASSERT(menu != nullptr);

    menu->clear();

    if (items.isEmpty()) {
        menu->setEnabled(false);
        return;
    }

    menu->setEnabled(true);

    for (const QString& curPath : items) {
        QAction* entryAction = menu->addAction(curPath);
        connect(entryAction, &QAction::triggered, this, [callbackOpen, curPath]() { callbackOpen(curPath); });
    }

    menu->addSeparator();
    QAction* clearAction = menu->addAction(tr("Clear Menu"));
    connect(clearAction, &QAction::triggered, this, callbackClear);
}

void MainWindow::updateRecentHsmMenu() {
    Q_ASSERT(mSettingsController != nullptr);

    updateMenuItemsRecent(
        ui->menuRecentFiles,
        mSettingsController->recentHsm(),
        [this](const QString& path) {
            const QFileInfo hsmInfo(path);

            if (!hsmInfo.exists() || !hsmInfo.isFile()) {
                mSettingsController->removeRecentHsm(path);
                updateRecentHsmMenu();
                QMessageBox::warning(this,
                                     tr("Recent File Not Found"),
                                     tr("Cannot open recent HSM because the file no longer exists:\n%1").arg(path));
                return;
            }

            mController->openProject(path);
        },
        [this]() {
            mSettingsController->clearRecentHsm();
            updateRecentHsmMenu();
        });
}

void MainWindow::updateRecentWorkspacesMenu() {
    Q_ASSERT(mSettingsController != nullptr);

    updateMenuItemsRecent(
        ui->menuRecentWorkspaces,
        mSettingsController->recentWorkspaces(),
        [this](const QString& path) {
            const QFileInfo workspaceInfo(path);

            if (!workspaceInfo.exists() || !workspaceInfo.isDir()) {
                mSettingsController->removeRecentWorkspace(path);
                updateRecentWorkspacesMenu();
                QMessageBox::warning(this,
                                     tr("Recent Workspace Not Found"),
                                     tr("Cannot open recent workspace because the directory no longer exists:\n%1").arg(path));
                return;
            }

            openWorkspace(path);
            mSettingsController->addRecentWorkspace(path);
            updateRecentWorkspacesMenu();
        },
        [this]() {
            mSettingsController->clearRecentWorkspaces();
            updateRecentWorkspacesMenu();
        });
}

void MainWindow::onHsmElementDoubleClickEvent(QWeakPointer<model::StateMachineEntity> entity) {
    Q_ASSERT(mController != nullptr);
    mController->handleHsmElementDoubleClick(entity);
}

// =================================================================================================================
void MainWindow::projectOpened(ProjectControllerPtr project) {
    qDebug() << Q_FUNC_INFO;
    HsmGraphicsView* newView = new HsmGraphicsView();
    QGraphicsScene* scene = new QGraphicsScene();

    connect(scene, &QGraphicsScene::selectionChanged, this, &MainWindow::onGraphicsViewSelectionChanged);
    connect(newView, &HsmGraphicsView::hsmElementDoubleClickEvent, this, &MainWindow::onHsmElementDoubleClickEvent);

    newView->setScene(scene);
    newView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    newView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    project->registerView(newView);
    ui->projectTabs->addTab(newView, project->name());

    connect(project.data(), &ProjectController::projectModelChanged, this, [this](QPointer<ProjectController> projectRaw) {
        if (projectRaw) {
            const int projectIndex = ui->projectTabs->indexOf(projectRaw->view());
            if (projectIndex != -1) {
                ui->projectTabs->setTabText(projectIndex,
                                            (projectRaw->isModified() ? "*" + projectRaw->name() : projectRaw->name()));
                if (mActiveProject && mActiveProject.data() == projectRaw.data()) {
                    ui->actionUndo->setEnabled(projectRaw->canUndo());
                    ui->actionRedo->setEnabled(projectRaw->canRedo());
                }
            } else {
                qCritical() << "MainWindow::projectOpened: project view not found in tabs";
            }
        }
    });
}

void MainWindow::projectSelected(ProjectControllerPtr project) {
    qDebug() << Q_FUNC_INFO << __LINE__ << project;
    mActiveProject = project;

    if (!mActiveProject.isNull()) {
        const int projectIndex = ui->projectTabs->indexOf(mActiveProject->view());

        if (projectIndex != -1) {
            ui->projectTabs->setCurrentIndex(projectIndex);
            ui->modelTree->setModel(mActiveProject->hsmStructureModel());
            ui->entityProperties->setModel(mActiveProject->hsmEntityViewModel());
            ui->entityProperties->setColumnWidth(0, 140);
            ui->actionUndo->setEnabled(mActiveProject->canUndo());
            ui->actionRedo->setEnabled(mActiveProject->canRedo());
            connect(ui->modelTree->selectionModel(),
                    &QItemSelectionModel::currentChanged,
                    this,
                    &MainWindow::onModelTreeSelectionChanged);
        } else {
            qCritical() << "MainWindow::projectSelected: project view not found in tabs";
        }
    } else {
        qCritical() << "MainWindow::projectSelected: project is nullptr";
        ui->actionUndo->setEnabled(false);
        ui->actionRedo->setEnabled(false);
    }
}

void MainWindow::projectClosed(ProjectControllerPtr project) {
    if (project) {
        const int projectIndex = ui->projectTabs->indexOf(project->view());

        if (projectIndex != -1) {
            ui->projectTabs->removeTab(projectIndex);
            project->view()->deleteLater();
        }

        if (project == mActiveProject) {
            ui->modelTree->setModel(nullptr);
            ui->entityProperties->setModel(nullptr);
            mActiveProject.clear();
            ui->actionUndo->setEnabled(false);
            ui->actionRedo->setEnabled(false);
        }
    }
}

// =================================================================================================================
void MainWindow::selectModelEntityById(model::EntityID_t id) {
    qDebug() << Q_FUNC_INFO << "Selecting entity id=" << id;
    // Select in entityProperties
    auto* entityModel = qobject_cast<view::StateMachineEntityViewModel*>(ui->entityProperties->model());

    if (entityModel) {
        entityModel->selectEntityById(id);
    }

    // Select in graphics view
    QSignalBlocker blockerView(currentView()->scene());  // block signals from graphics scene

    currentView()->selectHsmElement(id);

    // Select in modelTree
    auto* treeModel = qobject_cast<view::StateMachineTreeModel*>(ui->modelTree->model());
    if (treeModel) {
        QSignalBlocker blockerTree(ui->modelTree);  // block signals from tree view
        QModelIndex idx = treeModel->findModelEntity(id);

        if (idx.isValid()) {
            ui->modelTree->setCurrentIndex(idx);
        }
    }
}

// TODO: would be good to move to custom widget
void MainWindow::onSidebarActionTriggered(bool checked) {
    QAction* senderAction = qobject_cast<QAction*>(sender());

    if (nullptr != senderAction) {
        QList<QAction*> sidebarActions = ui->leftSideBar->actions();

        if (false == checked) {
            // If toggled off, hide dockSidebarTabs and uncheck all
            ui->dockSidebarTabs->hide();

            for (QAction* action : sidebarActions) {
                action->setChecked(false);
            }
        } else {
            // Otherwise, show dockSidebarTabs, set the correct page and uncheck other actions
            for (QAction* action : sidebarActions) {
                // block signals so that we don't tigger onSidebarActionTriggered
                QSignalBlocker block(action);

                action->setChecked(action == senderAction);
            }
            ui->dockSidebarTabs->show();

            // to store previous size set by user for each page
            static QMap<int, int> sLastPageSize = {{1, 260}, {2, 260}};
            QWidget* selectedPage = nullptr;

            if (senderAction == ui->actionShowTabHsmElements) {
                selectedPage = ui->pageElements;
            } else if (senderAction == ui->actionShowTabWorkspace) {
                selectedPage = ui->pageWorkspace;
            } else if (senderAction == ui->actionShowTabDebug) {
                selectedPage = ui->pageDebug;
            }

            if (nullptr != selectedPage) {
                // store size of previous page
                // qDebug() << "--- store last size" << ui->stackedWidget->currentIndex() << ui->dockSidebarTabs->width();
                sLastPageSize.insert(ui->stackedWidget->currentIndex(), ui->dockSidebarTabs->width());

                // change page
                ui->stackedWidget->setCurrentWidget(selectedPage);

                if (senderAction == ui->actionShowTabHsmElements) {
                    // Elements page should have a fixed, non-resizable size
                    ui->dockSidebarTabs->setMaximumWidth(ui->pageElements->minimumWidth());
                } else {
                    // restore size for the new page
                    const int lastSize = sLastPageSize.value(ui->stackedWidget->currentIndex(), 0);

                    // qDebug() << "--- lastSize" << ui->stackedWidget->currentIndex() << lastSize;

                    ui->dockSidebarTabs->setMinimumWidth(0);
                    ui->dockSidebarTabs->setMaximumWidth(1024);

                    if (lastSize > 0) {
                        QList<QDockWidget*> docks = {ui->dockSidebarTabs};
                        QList<int> sizes = {lastSize};

                        resizeDocks(docks, sizes, Qt::Horizontal);
                    }
                }
            }
        }
    }
}
