#include "mainwindow.h"
#include "./ui/ui_main.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    scene = QGraphicsScene()
    self.window.mainView.setScene(scene)
    self.window.mainView.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOn)
    self.window.mainView.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOn)

    // initialize list of HSM elements
    items = HsmElementsFactory.createElementsList()
    for i in items:
        self.window.listHsmElements.addItem(i)

    // ---------------------
    // TODO: debug
    e1 = HsmStateElement()
    e1.setPos(10, 10)
    scene.addItem(e1)
    e2 = HsmStateElement()
    e2.setPos(175, 120)
    scene.addItem(e2)
    e3 = HsmStateElement()
    e3.setPos(-100, 120)
    scene.addItem(e3)

    t = HsmTransition()
    # scene.addItem(t)
    # t.connectElements(e1, e2)
    e1.addTransition(t, e2)
    // ---------------------
}

MainWindow::~MainWindow()
{
    delete ui;
}

