#include "ProjectController.hpp"

#include <QDebug>
#include <QLoggingCategory>
#include <QMimeData>

#include "ObjectUtils.hpp"
#include "model/EntryPoint.hpp"
#include "model/InitialState.hpp"
#include "model/ModelElementsFactory.hpp"
#include "model/RegularState.hpp"
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
    createElement("state", QPoint(-150, 170));
    createElement("state", QPoint(250, 170));

    createElement("initial", QPoint(-150, -120));
    createElement("entrypoint", QPoint(-100, -120));
    createElement("exitpoint", QPoint(-20, -120));
    createElement("final", QPoint(75, -120));
    createElement("history", QPoint(175, -120));

    // createTransition("e1", "e2");

    QObject::connect(mModel.get(), &model::StateMachineModel::modelEntityDeleted, this, &ProjectController::modelEntityDeleted);

    // std::shared_ptr<HsmTransition> t = std::make_shared<HsmTransition>();
    // t->init();
    // e1->addTransition(t, e2);
    mMainWindow->setModel(mModel);
}

void ProjectController::handleViewDropEvent(const QString& elementTypeId,
                                            const QPoint& pos,
                                            const model::EntityID_t targetElementId) {
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
    auto source = mModel->root()->findState(fromElementId);
    auto target = mModel->root()->findState(toElementId);
    auto newTransition = model::ModelElementsFactory::createUniqueTransition(source, target);

    if (newTransition) {
        switch (source->stateType()) {
            case model::StateType::REGULAR: {
                auto regularStatePtr = source.dynamicCast<model::RegularState>();

                if (regularStatePtr) {
                    regularStatePtr->addTransition(newTransition);
                }
                break;
            }
            case model::StateType::ENTRYPOINT: {
                auto entryPointPtr = source.dynamicCast<model::EntryPoint>();

                if (entryPointPtr) {
                    entryPointPtr->addTransition(newTransition);
                }
                break;
            }
            case model::StateType::INITIAL: {
                auto initialStatePtr = source.dynamicCast<model::InitialState>();

                if (initialStatePtr) {
                    auto oldTransition = initialStatePtr->transition();

                    if (oldTransition) {
                        // delete old transition from scene since initial state can only have one outgoing transition
                        mMainWindow->view()->deleteHsmElement(oldTransition->id());
                    }

                    source.dynamicCast<model::InitialState>()->setTransition(newTransition);
                }
                break;
            }
            default:
                // TODO: error (should not happen)
                qCritical() << "trying to add transition to unsupported state type=" << static_cast<int>(source->stateType());
                newTransition.reset();
                break;
        }

        if (newTransition) {
            view::HsmTransition* newViewTransition =
                mMainWindow->view()->createHsmTransition(newTransition, fromElementId, toElementId);

            tryConnectSignal(newViewTransition,
                             "transitionReconnected(model::EntityID_t,model::EntityID_t,model::EntityID_t)",
                             this,
                             "reconnectElements(model::EntityID_t,model::EntityID_t,model::EntityID_t)");
        }
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

void ProjectController::modelEntityDeleted(QWeakPointer<model::StateMachineEntity> entity) {
    qDebug() << Q_FUNC_INFO;
    auto ptr = entity.lock();

    if (ptr) {
        mMainWindow->view()->deleteHsmElement(ptr->id());
    } else {
        qCritical() << "StateMachineEntity doesnt exist!";
    }
}

void ProjectController::createElement(const QString& elementTypeId,
                                      const QPoint& pos,
                                      const model::EntityID_t parentElementId) {
    qDebug() << Q_FUNC_INFO << elementTypeId << parentElementId << pos;

    static std::map<QString, model::StateType> sElementTypes = {// TODO: decide what to do with start type
                                                                       {"initial", model::StateType::INITIAL},
                                                                       {"final", model::StateType::FINAL},
                                                                       {"state", model::StateType::REGULAR},
                                                                       {"entrypoint", model::StateType::ENTRYPOINT},
                                                                       {"exitpoint", model::StateType::EXITPOINT},
                                                                       {"history", model::StateType::HISTORY}};

    auto it = sElementTypes.find(elementTypeId);

    if (sElementTypes.end() != it) {
        auto newModelElement = model::ModelElementsFactory::createUniqueState(it->second);
        auto parentState = mModel->root()->findRegularState(parentElementId);
        view::HsmElement* newViewElement =
            mMainWindow->view()->createHsmElement(newModelElement, elementTypeId, pos, parentElementId);

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
