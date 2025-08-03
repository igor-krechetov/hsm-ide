#include "ProjectController.hpp"

#include <QDebug>
#include <QDropEvent>
#include <QLoggingCategory>
#include <QMimeData>

#include "ObjectUtils.hpp"
#include "model/StateMachineModel.hpp"
#include "model/Transition.hpp"
#include "view/MainWindow.hpp"
#include "view/widgets/HsmGraphicsView.hpp"
// TODO: move out from private
#include "view/elements/private/HsmElement.hpp"
// #include "Transition.hpp"

Q_DECLARE_LOGGING_CATEGORY(ProjectController)

ProjectController::ProjectController(QPointer<MainWindow> mainWindow, QObject* parent)
    : QObject(parent)
    , mMainWindow(mainWindow)
    , mModel(new model::StateMachineModel("Default Model", this)) {
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
    qDebug() << Q_FUNC_INFO << event->position();
    event->setDropAction(Qt::CopyAction);
    event->accept();

    createElement(event->mimeData()->data("hsm/element").data(), event->position().toPoint());
}

void ProjectController::connectElements(const model::EntityID_t fromElementId, const model::EntityID_t toElementId) {
    qDebug() << Q_FUNC_INFO << fromElementId << " -> " << toElementId;
    // TODO

    // if (scene()->items().contains(transition.get()) == false) {
    //     scene()->addItem(transition.get());
    // }

    // transition->connectElements(this, target);
    // mTransitions.append(transition);
    // auto from = mModel->findChild(fromElementId);
    // auto to = mModel->findChild(toElementId);

    // if (from && to) {
    // TODO: substate support
    auto newTransition = mModel->createUniqueTransition(fromElementId, toElementId);

    if (newTransition) {
        qDebug() << "transition created: " << newTransition;
        mModel->addChild(newTransition);
        mMainWindow->view()->createHsmTransition(newTransition->id(), fromElementId, toElementId);
    } else {
        qCritical() << "Failed to create new transition";
    }
    // }
}

void ProjectController::createElement(const QString& elementTypeId, const QPoint& pos) {
    static std::map<QString, model::State::Type> sElementTypes = {{"start", model::State::Type::Initial},
                                                                  {"final", model::State::Type::Final},
                                                                  {"state", model::State::Type::Regular},
                                                                  {"entrypoint", model::State::Type::Initial},
                                                                  {"exitpoint", model::State::Type::Final},
                                                                  {"history", model::State::Type::History}};

    auto it = sElementTypes.find(elementTypeId);

    if (sElementTypes.end() != it) {
        auto newModelElement = mModel->createUniqueState(it->second);
        view::HsmElement* newViewElement = mMainWindow->view()->createHsmElement(newModelElement->id(), elementTypeId, pos);

        tryConnectSignal(newViewElement,
                         "elementConnected(model::EntityID_t,model::EntityID_t)",
                         this,
                         "connectElements(model::EntityID_t,model::EntityID_t)");
    } else {
        qCritical() << Q_FUNC_INFO << "Unsupported element type:" << elementTypeId;
    }
}

void ProjectController::createTransition(const QSharedPointer<model::State>& fromElement,
                                         const QSharedPointer<model::State>& toElement) {}
