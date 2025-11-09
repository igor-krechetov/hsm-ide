#include "MainWindow.hpp"

#include <QSignalBlocker>
#include <QFileDialog>

#include "./ui/ui_main.h"
#include "controllers/ProjectController.hpp"

#include "model/StateMachineModel.hpp"
#include "view/elements/HsmElementsFactory.hpp"
#include "view/elements/HsmStateElement.hpp"
#include "view/elements/private/ElementGripItem.hpp"
#include "view/models/StateMachineEntityViewModel.hpp"
#include "view/models/StateMachineTreeModel.hpp"
#include "widgets/HsmEntityPropertyDelegate.hpp"


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui_hsm_ide) {
    ui->setupUi(this);

    QGraphicsScene* scene = new QGraphicsScene();
    ui->mainView->setScene(scene);
    // connect(ui->mainView, &HsmGraphicsView::deleteItemsRequested, this, &MainWindow::deleteSelectedItems);
    ui->mainView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->mainView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    // initialize list of HSM elements
    auto items = view::HsmElementsFactory::createElementsList();

    for (QListWidgetItem* newItem : items) {
        ui->listHsmElements->addItem(newItem);
    }

    // Connect selection change in graphics view
    connect(scene, &QGraphicsScene::selectionChanged, this, &MainWindow::onGraphicsViewSelectionChanged);
    // Connect selection change in model tree (after model is set)
    // (see setModel)
}

MainWindow::~MainWindow() {
    delete ui;
}

QPointer<HsmGraphicsView> MainWindow::view() {
    return ui->mainView;
}

void MainWindow::handleOpen() {
    QString initialDir = mCurrentFilePath.isEmpty() ? QString("~") : QFileInfo(mCurrentFilePath).absolutePath();
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open SCXML File"), initialDir, tr("SCXML Files (*.scxml);;All Files (*)"));
    if (!fileName.isEmpty() && mActiveProject) {
        mCurrentFilePath = fileName;
        mActiveProject->importModel(fileName);
        // Optionally update the UI/model here
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


void MainWindow::deleteSelectedItems() {
    if ((nullptr != ui) && (nullptr != ui->mainView) && (nullptr != ui->mainView->scene())) {
        ui->mainView->deleteSelectedItems();
    }
}

void MainWindow::setProjectController(QPointer<ProjectController> controller) {
    mActiveProject = controller;
    ui->mainView->setProjectController(mActiveProject);
}

void MainWindow::setModel(const QSharedPointer<model::StateMachineModel>& model) {
    // Connect StateMachineTreeModel to modelTree
    auto* treeModel = new view::StateMachineTreeModel(model, this);
    ui->modelTree->setModel(treeModel);

    // Connect StateMachineTreeModel to entityProperties
    auto* entityModel = new view::StateMachineEntityViewModel(model, this);

    ui->entityProperties->setModel(entityModel);
    ui->entityProperties->setColumnWidth(0, 140);
    // Set custom delegate for values
    ui->entityProperties->setItemDelegateForColumn(1, new view::HsmEntityPropertyDelegate(this));

    // Connect selection change in modelTree
    connect(ui->modelTree->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this,
            &MainWindow::onModelTreeSelectionChanged);
}

void MainWindow::onGraphicsViewSelectionChanged() {
    qDebug() << "---------- onGraphicsViewSelectionChanged";
    // Get selected elements from graphics view
    auto selectedIds = ui->mainView->getSelectedElements();
    if (!selectedIds.isEmpty()) {
        selectModelEntityById(selectedIds.first());
    }
}

void MainWindow::onModelTreeSelectionChanged(const QModelIndex& current, const QModelIndex& /*previous*/) {
    selectModelEntityById(current.data(Qt::UserRole).toUInt());
}

void MainWindow::selectModelEntityById(model::EntityID_t id) {
    qDebug() << Q_FUNC_INFO << "Selecting entity id=" << id;
    // Select in entityProperties
    auto* entityModel = qobject_cast<view::StateMachineEntityViewModel*>(ui->entityProperties->model());
    if (entityModel) {
        entityModel->selectEntityById(id);
    }

    // Select in graphics view
    QSignalBlocker blockerView(ui->mainView->scene());  // block signals from graphics scene
    ui->mainView->selectHsmElement(id);

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