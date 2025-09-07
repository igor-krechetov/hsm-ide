#include "ProjectController.hpp"

#include <QDebug>
#include <QLoggingCategory>
#include <QMimeData>

#include "ObjectUtils.hpp"
#include "model/StateMachineModel.hpp"
#include "model/Transition.hpp"
#include "view/MainWindow.hpp"
#include "view/widgets/HsmGraphicsView.hpp"
// TODO: move out from private
#include "view/elements/HsmTransition.hpp"
#include "view/elements/private/HsmElement.hpp"

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

void ProjectController::handleViewDropEvent(const QString &elementTypeId, const QPoint &pos, const model::EntityID_t targetElementId) {
    qDebug() << Q_FUNC_INFO << elementTypeId << targetElementId << pos;

    createElement(elementTypeId, pos, targetElementId);
}

void ProjectController::handleViewMoveEvent(const model::EntityID_t draggedElementId, const model::EntityID_t targetElementId) {
    qDebug() << Q_FUNC_INFO << draggedElementId << targetElementId;
    bool updateUi = false;

    // TODO: handle top level moves
    if (model::INVALID_MODEL_ID != targetElementId) {
        // change parent in model
        updateUi = mModel->moveElement(draggedElementId, targetElementId);
    } else {
        updateUi = mModel->moveElement(draggedElementId, mModel->root()->id());
    }

    qDebug() << "UPDATE UI: " << updateUi;

    if (true == updateUi) {
        // change parent in view
        mMainWindow->view()->moveHsmElement(draggedElementId, targetElementId);
    }
}

void ProjectController::handleDeleteElements(const QList<model::EntityID_t>& elementIDs) {
    qDebug() << Q_FUNC_INFO << elementIDs;

    for (model::EntityID_t id : elementIDs) {
        mMainWindow->view()->deleteHsmElement(id);
        mModel->root()->deleteChild(id);
    }
}

void ProjectController::connectElements(const model::EntityID_t fromElementId, const model::EntityID_t toElementId) {
    qDebug() << Q_FUNC_INFO << fromElementId << " -> " << toElementId;

    // TODO: substate support
    auto newTransition = mModel->createUniqueTransition(fromElementId, toElementId);

    if (newTransition) {
        qDebug() << "transition created: " << newTransition;
        view::HsmTransition* newViewTransition =
            mMainWindow->view()->createHsmTransition(newTransition->id(), fromElementId, toElementId);

        mModel->root()->addTransition(newTransition);
        tryConnectSignal(newViewTransition,
                         "transitionReconnected(model::EntityID_t,model::EntityID_t,model::EntityID_t)",
                         this,
                         "reconnectElements(model::EntityID_t,model::EntityID_t,model::EntityID_t)");
    } else {
        qCritical() << "Failed to create new transition";
    }
    // }
}

void ProjectController::reconnectElements(const model::EntityID_t transitionId,
                                          const model::EntityID_t newFromElementId,
                                          const model::EntityID_t newToElementId) {
    qDebug() << Q_FUNC_INFO << transitionId << ": " << newFromElementId << " -> " << newToElementId;

    auto transitionPtr = mModel->root()->findTransition(transitionId);

    if (transitionPtr) {
        transitionPtr->setSource(mModel->root()->findState(newFromElementId));
        transitionPtr->setTarget(mModel->root()->findState(newToElementId));

        mMainWindow->view()->reconnectHsmTransition(transitionId, newFromElementId, newToElementId);
    } else {
        qCritical() << "Failed to find transition with id " << transitionId;
    }
}

void ProjectController::createElement(const QString& elementTypeId,
                                      const QPoint& pos,
                                      const model::EntityID_t parentElementId) {
    qDebug() << Q_FUNC_INFO << elementTypeId << parentElementId << pos;
    static std::map<QString, model::State::Type> sElementTypes = {{"start", model::State::Type::Initial},
                                                                  {"final", model::State::Type::Final},
                                                                  {"state", model::State::Type::Regular},
                                                                  {"entrypoint", model::State::Type::Initial},
                                                                  {"exitpoint", model::State::Type::Final},
                                                                  {"history", model::State::Type::History}};

    auto it = sElementTypes.find(elementTypeId);

    if (sElementTypes.end() != it) {
        auto newModelElement = mModel->createUniqueState(it->second);
        auto parentState = mModel->root()->findState(parentElementId);
        view::HsmElement* newViewElement = mMainWindow->view()->createHsmElement(newModelElement->id(), elementTypeId, pos, parentElementId);

        qDebug() << Q_FUNC_INFO << parentState;

        if (parentState.isNull()) {
            parentState = mModel->root();
        }

        parentState->addChildState(newModelElement);

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
