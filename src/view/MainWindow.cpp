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
    auto items = HsmElementsFactory::createElementsList();

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
    // TODO: need to re-think how to handle transition
    //       What to do when one of the states is deleted? Delete transition too?
    //       Do we want to allow dangling transitions?
    if (!ui || !ui->mainView || !ui->mainView->scene()) {
        return;
    }

    auto items = ui->mainView->scene()->selectedItems();
    QList<QGraphicsItem*> topLevelItems;

    for (QGraphicsItem* item : items) {
        QGraphicsItem* parent = item->parentItem();

        if (nullptr == parent) {
            topLevelItems.append(item);
        }
    }
    for (QGraphicsItem* item : topLevelItems) {
        delete item;
    }
}
