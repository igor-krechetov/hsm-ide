#include "MainWindow.hpp"

#include <QSignalBlocker>
#include <QFileDialog>

#include "./ui/ui_main.h"

#include "controllers/MainEditorController.hpp"
#include "controllers/ProjectController.hpp"

#include "model/StateMachineModel.hpp"
#include "view/elements/HsmElementsFactory.hpp"
#include "view/elements/HsmStateElement.hpp"
#include "view/elements/private/ElementGripItem.hpp"
#include "view/models/StateMachineEntityViewModel.hpp"
#include "view/models/StateMachineTreeModel.hpp"
#include "widgets/HsmEntityPropertyDelegate.hpp"


MainWindow::MainWindow(MainEditorController* parent)
    : QMainWindow(nullptr)
    , ui(new Ui_hsm_ide)
    , mController(parent) {
    ui->setupUi(this);

    ui->entityProperties->setItemDelegateForColumn(1, new view::HsmEntityPropertyDelegate(this));

    // initialize list of HSM elements
    auto items = view::HsmElementsFactory::createElementsList();

    for (QListWidgetItem* newItem : items) {
        ui->listHsmElements->addItem(newItem);
    }

    // Connect sidebar actions to generic slot using toggled(bool)
    QList<QAction*> sidebarActions = ui->leftSideBar->actions();
    for (QAction* action : sidebarActions) {
        connect(action, &QAction::toggled, this, &MainWindow::onSidebarActionTriggered);
    }

    // Delete dummy tab
    ui->projectTabs->removeTab(0);

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

void MainWindow::handleNewProject() {
    mController->createProject();
}

void MainWindow::handleOpen() {
    QString initialDir = mCurrentFilePath.isEmpty() ? QString("~") : QFileInfo(mCurrentFilePath).absolutePath();
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open SCXML File"), initialDir, tr("SCXML Files (*.scxml);;All Files (*)"));

    if (!fileName.isEmpty() && mActiveProject) {
        mController->openProject(fileName);
    }
}

void MainWindow::handleSave() {
    if (nullptr != mActiveProject) {
        if (mCurrentFilePath.isEmpty()) {
            handleSaveAs();
        } else {
            mActiveProject->exportModel(mCurrentFilePath);
        }
    }
}

void MainWindow::handleSaveAs() {
    QString initialDir = mCurrentFilePath.isEmpty() ? QString("~") : QFileInfo(mCurrentFilePath).absolutePath();
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save SCXML File As"), initialDir, tr("SCXML Files (*.scxml);;All Files (*)"));

    if (!fileName.isEmpty() && mActiveProject) {
        if (!fileName.endsWith(".scxml", Qt::CaseInsensitive)) {
            fileName += ".scxml";
        }

        mCurrentFilePath = fileName;
        mActiveProject->exportModel(fileName);
    }
}

void MainWindow::projectTabSelected(int index) {
    if (nullptr != mController) {
        QPointer<HsmGraphicsView> view = currentView();

        if (nullptr != view) {
            mController->switchToProject(view->projectController()->id());
        }
    }
}

void MainWindow::projectTabCloseRequested(int index) {
    // TODO
}

void MainWindow::deleteSelectedItems() {
    if ((nullptr != ui) && (nullptr != currentView()) && (nullptr != currentView()->scene())) {
        currentView()->deleteSelectedItems();
    }
}

void MainWindow::onGraphicsViewSelectionChanged() {
    qDebug() << "---------- onGraphicsViewSelectionChanged";
    // Get selected elements from graphics view
    auto selectedIds = currentView()->getSelectedElements();
    if (!selectedIds.isEmpty()) {
        selectModelEntityById(selectedIds.first());
    }
}

void MainWindow::onModelTreeSelectionChanged(const QModelIndex& current, const QModelIndex& /*previous*/) {
    selectModelEntityById(current.data(Qt::UserRole).toUInt());
}

// =================================================================================================================
void MainWindow::projectOpened(QPointer<ProjectController> project) {
    qDebug() << Q_FUNC_INFO;
    // create a new tab in the projectTabs QTabWidget with vertical layout and HsmGraphicsView inside
    HsmGraphicsView* newView = new HsmGraphicsView();
    QGraphicsScene* scene = new QGraphicsScene();

    // Connect selection change in graphics view
    connect(scene, &QGraphicsScene::selectionChanged, this, &MainWindow::onGraphicsViewSelectionChanged);
    // Connect selection change in model tree (after model is set)
    // (see setModel)

    newView->setScene(scene);
    newView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    newView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    project->registerView(newView);

    ui->projectTabs->addTab(newView, project->name());

    connect(project, &ProjectController::projectModelChanged, this, [this](QPointer<ProjectController> project) {
        if (nullptr != project) {
            const int projectIndex = ui->projectTabs->indexOf(project->view());

            if (projectIndex != -1) {
                ui->projectTabs->setTabText(projectIndex, (project->isModified() ? "*" + project->name() : project->name()));
            } else {
                qCritical() << "MainWindow::projectOpened: project view not found in tabs";
            }
        }
    });
}

void MainWindow::projectSelected(QPointer<ProjectController> project) {
    qDebug() << Q_FUNC_INFO << __LINE__;
    mActiveProject = project;

    if (nullptr != mActiveProject) {
        const int projectIndex = ui->projectTabs->indexOf(mActiveProject->view());

        if (projectIndex != -1) {
            ui->projectTabs->setCurrentIndex(projectIndex);

            // Connect StateMachineTreeModel to modelTree
            ui->modelTree->setModel(mActiveProject->hsmStructureModel());

            // Connect StateMachineTreeModel to entityProperties
            ui->entityProperties->setModel(mActiveProject->hsmEntityViewModel());
            ui->entityProperties->setColumnWidth(0, 140);

            // Connect selection change in modelTree
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

void MainWindow::projectClosed(const QString& projectId) {
    // TODO
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
                action->setChecked(action == senderAction);
            }
            ui->dockSidebarTabs->show();

            if (senderAction == ui->actionShowTabHsmElements) {
                ui->stackedWidget->setCurrentWidget(ui->pageElements);
            } else if (senderAction == ui->actionShowTabWorkspace) {
                ui->stackedWidget->setCurrentWidget(ui->pageWorkspace);
            } else if (senderAction == ui->actionShowTabDebug) {
                ui->stackedWidget->setCurrentWidget(ui->pageDebug);
            }
        }
    }
}