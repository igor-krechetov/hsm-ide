#include "mainwindow.hpp"
#include "./ui/ui_main.h"
#include "elements/HsmElementsFactory.hpp"
#include "elements/HsmStateElement.hpp"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    if (QString::compare(QSysInfo::productType(), "win32", Qt::CaseInsensitive) != 0)
    {
        mLastDirectory = "~/";
    }

    QGraphicsScene* scene = new QGraphicsScene();

    ui->mainView->setScene(scene);
    ui->mainView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->mainView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    // initialize list of HSM elements
    auto items = HsmElementsFactory::createElementsList();

    for (QListWidgetItem* newItem: items) {
        ui->listHsmElements->addItem(newItem);
    }

    // ---------------------
    // TODO: debug
    HsmStateElement* e1 = static_cast<HsmStateElement*>(HsmElementsFactory::createElement("state"));
    e1->setPos(10, 10);
    scene->addItem(e1);

    HsmStateElement* e2 = static_cast<HsmStateElement*>(HsmElementsFactory::createElement("state"));
    e2->setPos(175, 120);
    scene->addItem(e2);

    HsmStateElement* e3 = static_cast<HsmStateElement*>(HsmElementsFactory::createElement("state"));
    e3->setPos(-100, 120);
    scene->addItem(e3);


    HsmTransition* t = new HsmTransition();
    t->init();
    e1->addTransition(t, e2);
    // ---------------------
}

MainWindow::~MainWindow()
{
    delete ui;
}

