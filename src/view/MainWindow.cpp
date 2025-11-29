#include "MainWindow.hpp"

#include <QFileDialog>
#include <QFileSystemModel>
#include <QSignalBlocker>

#include "./ui/ui_main.h"
#include "AboutDialog.hpp"
#include "controllers/MainEditorController.hpp"
#include "controllers/ProjectController.hpp"
#include "view/models/StateMachineEntityViewModel.hpp"
#include "view/models/StateMachineTreeModel.hpp"
#include "view/models/NonEmptyDirFileSystemProxyModel.hpp"

MainWindow::MainWindow(MainEditorController* parent)
    : QMainWindow(nullptr)
    , ui(new Ui_hsm_ide)
    , mController(parent) {
    ui->setupUi(this);

    // Conect events for HsmTreeView
    connect(ui->modelTree, &HsmTreeView::elementDoubleClickEvent, this, &MainWindow::onHsmElementDoubleClickEvent);

    // Connect sidebar actions to generic slot using toggled(bool)
    QList<QAction*> sidebarActions = ui->leftSideBar->actions();
    for (QAction* action : sidebarActions) {
        connect(action, &QAction::toggled, this, &MainWindow::onSidebarActionTriggered);
    }

    // Delete dummy tab
    ui->projectTabs->removeTab(0);

    // Select default side menu
    ui->actionShowTabHsmElements->setChecked(true);
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
    Q_ASSERT(ui->workspaceTree != nullptr);

    QString rootDir = QFileDialog::getExistingDirectory(this, tr("Select Directory"));

    if (rootDir.isEmpty() == false) {
        // Use QFileSystemModel as source, NonEmptyDirFileSystemProxyModel as proxy
        QFileSystemModel* fsModel = new QFileSystemModel(this);
        fsModel->setRootPath(rootDir);
        fsModel->setNameFilters({"*.scxml"});
        fsModel->setNameFilterDisables(false);   // Hide all files that don't match

        view::NonEmptyDirFileSystemProxyModel* proxyModel = new view::NonEmptyDirFileSystemProxyModel(this);
        proxyModel->setSourceModel(fsModel);

        ui->workspaceTree->setModel(proxyModel);
        ui->workspaceTree->setRootIndex(proxyModel->mapFromSource(fsModel->index(rootDir)));

        ui->workspaceTree->setColumnHidden(1, true); // Size
        ui->workspaceTree->setColumnHidden(2, true); // Type
        ui->workspaceTree->setColumnHidden(3, true); // Date Modified
        ui->workspaceTree->setHeaderHidden(true);
    }
}

void MainWindow::handleCloseWorkspace() {
    Q_ASSERT(ui->workspaceTree != nullptr);

    ui->workspaceTree->setModel(nullptr);
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

void MainWindow::onGraphicsViewSelectionChanged() {
    // Get selected elements from graphics view
    auto selectedIds = currentView()->getSelectedElements();

    if (!selectedIds.isEmpty()) {
        selectModelEntityById(selectedIds.first());
    }
}

void MainWindow::onModelTreeSelectionChanged(const QModelIndex& current, const QModelIndex& /*previous*/) {
    selectModelEntityById(current.data(Qt::UserRole).toUInt());
}

void MainWindow::onOpenWorkspaceFile(const QModelIndex& index) {
    qDebug() << "onOpenWorkspaceFile" << index;
    view::NonEmptyDirFileSystemProxyModel *model = qobject_cast<view::NonEmptyDirFileSystemProxyModel*>(ui->workspaceTree->model());

    Q_ASSERT(nullptr != model);
    const QString path = model->getFilePath(index);

    if (path.isEmpty() == false) {
        mController->openProject(path);
    }
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
            connect(ui->modelTree->selectionModel(),
                    &QItemSelectionModel::currentChanged,
                    this,
                    &MainWindow::onModelTreeSelectionChanged);
        } else {
            qCritical() << "MainWindow::projectSelected: project view not found in tabs";
        }
    } else {
        qCritical() << "MainWindow::projectSelected: project is nullptr";
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