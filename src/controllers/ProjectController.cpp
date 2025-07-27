#include "ProjectController.hpp"

#include <QDebug>
#include <QDropEvent>
#include <QMimeData>

#include "model/StateMachineModel.hpp"
#include "view/MainWindow.hpp"
#include "view/widgets/HsmGraphicsView.hpp"

ProjectController::ProjectController(QSharedPointer<MainWindow> mainWindow, QObject* parent)
    : QObject(parent)
    , mMainWindow(mainWindow)
    , mModel(new StateMachineModel("Default Model", this)) {
    // ---------------------
    // TODO: debug
    createElement("state", QPoint(10, 10));
    createElement("state", QPoint(175, 120));
    createElement("state", QPoint(-100, 120));
    // createTransition("e1", "e2");

    // std::shared_ptr<HsmTransition> t = std::make_shared<HsmTransition>();
    // t->init();
    // e1->addTransition(t, e2);
}

void ProjectController::handleViewDropEvent(QDropEvent* event) {
    qDebug() << "ProjectController::handleViewDropEvent:" << event->position();
    event->setDropAction(Qt::CopyAction);
    event->accept();

    createElement(event->mimeData()->data("hsm/element").data(), event->position().toPoint());
}

void ProjectController::createElement(const QString& elementId, const QPoint& pos) {
    mMainWindow->view()->createHsmElement(elementId, pos);
}

void ProjectController::createTransition(const QSharedPointer<StateMachineElement>& fromElement,
                                         const QSharedPointer<StateMachineElement>& toElement) {}