#include "MainWindow.hpp"

#include "./ui/ui_main.h"
#include "view/elements/HsmElementsFactory.hpp"
#include "view/elements/HsmStateElement.hpp"
#include "view/elements/private/ElementGripItem.hpp"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui_hsm_ide) {
    ui->setupUi(this);

    if (QString::compare(QSysInfo::productType(), "win32", Qt::CaseInsensitive) != 0) {
        mLastDirectory = "~/";
    }

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

    // std::shared_ptr<ElementGripItem> g1 = std::make_shared<ElementGripItem>(nullptr);
    // g1->setPos(10, 20);
    // scene->addItem(g1.get());
    // ---------------------
}

MainWindow::~MainWindow() {
    delete ui;
}

QPointer<HsmGraphicsView> MainWindow::view() {
    return ui->mainView;
}

void MainWindow::deleteSelectedItems() {
    if (!ui || !ui->mainView || !ui->mainView->scene()) {
        return;
    }

    ui->mainView->deleteSelectedItems();
}
