#include "mainwindow.h"
#include "./ui/ui_main.h"

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

    ui->mainView.setScene(scene)
    ui->mainView.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn)
    ui->mainView.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn)

    // initialize list of HSM elements
    auto items = HsmElementsFactory.createElementsList();

    for (const HsmElement* newItem: items) {
        ui->listHsmElements.addItem(i);
    }

    // ---------------------
    // TODO: debug
    HsmStateElement* e1 = new HsmStateElement();
    e1->setPos(10, 10);
    scene->addItem(e1);

    HsmStateElement* e2 = new HsmStateElement();
    e2->setPos(175, 120);
    scene->addItem(e2);

    HsmStateElement* e3 = new HsmStateElement();
    e3->setPos(-100, 120);
    scene->addItem(e3);

    HsmTransition* t = new HsmTransition();
    e1->addTransition(t, e2);
    // ---------------------
}

MainWindow::~MainWindow()
{
    delete ui;
}

