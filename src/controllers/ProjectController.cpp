#include "ProjectController.hpp"

#include <QDebug>
#include <QFile>
#include <QLoggingCategory>
#include <QMimeData>
#include <QTextStream>

#include "ObjectUtils.hpp"
#include "model/EntryPoint.hpp"
#include "model/InitialState.hpp"
#include "model/ModelElementsFactory.hpp"
#include "model/RegularState.hpp"
#include "model/StateMachineModel.hpp"
#include "model/StateMachineSerializer.hpp"
#include "model/Transition.hpp"
#include "view/MainWindow.hpp"
#include "view/elements/HsmTransition.hpp"
#include "view/elements/private/HsmElement.hpp"  // TODO: move out from private
#include "view/widgets/HsmGraphicsView.hpp"

Q_DECLARE_LOGGING_CATEGORY(ProjectController)

ProjectController::ProjectController(QPointer<MainWindow> mainWindow, QObject* parent)
    : QObject(parent)
    , mMainWindow(mainWindow)
    , mModel(new model::StateMachineModel("Default Model", this)) {
    // ---------------------
    // TODO: debug

    // createElement("state", QPoint(10, 10));
    // createElement("state", QPoint(-150, 170));
    // createElement("state", QPoint(250, 170));

    // createElement("initial", QPoint(-150, -120));
    // createElement("entrypoint", QPoint(-100, -120));
    // createElement("exitpoint", QPoint(-20, -120));
    // createElement("final", QPoint(75, -120));
    // createElement("history", QPoint(175, -120));

    // createTransition("e1", "e2");

    QObject::connect(mModel.get(), &model::StateMachineModel::modelEntityAdded, this, &ProjectController::modelEntityAdded);
    QObject::connect(mModel.get(), &model::StateMachineModel::modelEntityDeleted, this, &ProjectController::modelEntityDeleted);
    QObject::connect(mModel.get(), &model::StateMachineModel::modelDataChanged, this, &ProjectController::modelDataChanged);

    // std::shared_ptr<HsmTransition> t = std::make_shared<HsmTransition>();
    // t->init();
    // e1->addTransition(t, e2);
    mMainWindow->setModel(mModel);

    importModel("./test.scxml");
}

bool ProjectController::importModel(const QString& path) {
    model::StateMachineSerializer serializer;
    QString scxmlContent;
    QFile file(path);

    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        scxmlContent = in.readAll();
        file.close();
    } else {
        qCritical() << "Failed to open file for reading: " << path;
        return false;
    }

    serializer.deserializeFromScxml(scxmlContent, mModel);

    return false;
}

bool ProjectController::exportModel(const QString& path) {
    bool res = false;
    model::StateMachineSerializer serializer;
    const QString scxmlContent = serializer.serializeToScxml(mModel);

    if (scxmlContent.isEmpty() == false) {
        QFile file(path);

        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QTextStream out(&file);

            out << scxmlContent;
            file.close();
            res = true;
        }
    }

    return res;
}

void ProjectController::handleViewDropEvent(const QString& elementTypeId,
                                            const QPointF& pos,
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
        mModel->root()->deleteChild(id);
        // NOTE: model will send signal which will trigger view update
    }
}

void ProjectController::handleModelEntityAdded(QSharedPointer<model::StateMachineEntity> parent,
                                               QSharedPointer<model::StateMachineEntity> entity) {
    qDebug() << "handleModelEntityAdded"
             << "parent=" << parent->id() << " child=" << entity->id();

    if (entity->type() == model::StateMachineEntity::Type::State) {
        static std::map<model::StateType, QString> sElementTypes = {{model::StateType::INITIAL, "initial"},
                                                                    {model::StateType::FINAL, "final"},
                                                                    {model::StateType::REGULAR, "state"},
                                                                    {model::StateType::ENTRYPOINT, "entrypoint"},
                                                                    {model::StateType::EXITPOINT, "exitpoint"},
                                                                    {model::StateType::HISTORY, "history"}};

        auto ptrState = entity.dynamicCast<model::State>();
        auto itType = sElementTypes.find(ptrState->stateType());

        if (itType != sElementTypes.end()) {
            view::HsmElement* newViewElement =
                mMainWindow->view()->createHsmElement(entity, itType->second, entity->getPos(), entity->getSize(), parent->id());

            tryConnectSignal(newViewElement,
                             "elementConnected(model::EntityID_t,model::EntityID_t)",
                             this,
                             "connectElements(model::EntityID_t,model::EntityID_t)");
        }
    } else if (entity->type() == model::StateMachineEntity::Type::Transition) {
        auto ptrTransition = entity.dynamicCast<model::Transition>();

        if (ptrTransition->source() && ptrTransition->target()) {
            view::HsmTransition* newViewTransition =
                mMainWindow->view()->createHsmTransition(ptrTransition, ptrTransition->sourceId(), ptrTransition->targetId());

            tryConnectSignal(newViewTransition,
                             "transitionReconnected(model::EntityID_t,model::EntityID_t,model::EntityID_t)",
                             this,
                             "reconnectElements(model::EntityID_t,model::EntityID_t,model::EntityID_t)");
        }
    }

    entity->forEachChildElement(
        [this](QSharedPointer<model::StateMachineEntity> parent, QSharedPointer<model::StateMachineEntity> element) {
            handleModelEntityAdded(parent, element);
            return true;
        });
}

void ProjectController::connectElements(const model::EntityID_t fromElementId, const model::EntityID_t toElementId) {
    qDebug() << Q_FUNC_INFO << fromElementId << " -> " << toElementId;
    auto source = mModel->root()->findState(fromElementId);
    auto target = mModel->root()->findState(toElementId);
    auto newTransition = model::ModelElementsFactory::createUniqueTransition(source, target);
}

void ProjectController::reconnectElements(const model::EntityID_t transitionId,
                                          const model::EntityID_t newFromElementId,
                                          const model::EntityID_t newToElementId) {
    qDebug() << Q_FUNC_INFO << transitionId << ": " << newFromElementId << " -> " << newToElementId;

    if (mModel->reconnectElements(transitionId, newFromElementId, newToElementId) == true) {
        mMainWindow->view()->reconnectHsmTransition(transitionId, newFromElementId, newToElementId);
    } else {
        qCritical() << "Failed to reconnect transition with id " << transitionId << newFromElementId << newToElementId;
    }
}

void ProjectController::modelEntityAdded(QWeakPointer<model::StateMachineEntity> parent,
                                         QWeakPointer<model::StateMachineEntity> entity) {
    qDebug() << Q_FUNC_INFO;
    auto ptrChild = entity.lock();
    auto ptrParent = parent.lock();

    if (ptrChild && ptrParent) {
        handleModelEntityAdded(ptrParent, ptrChild);
    } else {
        qCritical() << "StateMachineEntity doesnt exist!";
    }
}

void ProjectController::modelEntityDeleted(QWeakPointer<model::StateMachineEntity> parent,
                                           QWeakPointer<model::StateMachineEntity> entity) {
    qDebug() << Q_FUNC_INFO;
    auto ptrEntity = entity.lock();

    if (ptrEntity) {
        mMainWindow->view()->deleteHsmElement(ptrEntity->id());
    } else {
        qCritical() << "StateMachineEntity doesnt exist!";
    }
}

void ProjectController::modelDataChanged(QWeakPointer<model::StateMachineEntity> entity) {
    auto ptrEntity = entity.lock();

    if (ptrEntity && ptrEntity->type() == model::StateMachineEntity::Type::Transition) {
        auto ptrTransition = ptrEntity.dynamicCast<model::Transition>();

        // if transition has both source and destination and it doesnt exist yet - create it
        if (ptrTransition->source() && ptrTransition->target()) {
            if (mMainWindow->view()->findHsmTransition(ptrTransition->id()).isNull() == true) {
                view::HsmTransition* newViewTransition = mMainWindow->view()->createHsmTransition(ptrTransition,
                                                                                                  ptrTransition->sourceId(),
                                                                                                  ptrTransition->targetId());

                tryConnectSignal(newViewTransition,
                                 "transitionReconnected(model::EntityID_t,model::EntityID_t,model::EntityID_t)",
                                 this,
                                 "reconnectElements(model::EntityID_t,model::EntityID_t,model::EntityID_t)");
            }
        }

        // TODO: do we need a case when source or target changes directly in the model? maybe through the tree?
    }
}

void ProjectController::createElement(const QString& elementTypeId,
                                      const QPointF& pos,
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

        qDebug() << Q_FUNC_INFO << parentState;

        if (parentState.isNull()) {
            parentState = mModel->root();
        }

        newModelElement->setPos(pos);
        parentState->addChildState(newModelElement);
    } else {
        qCritical() << Q_FUNC_INFO << "Unsupported element type:" << elementTypeId;
    }
}

void ProjectController::createTransition(const QSharedPointer<model::State>& fromElement,
                                         const QSharedPointer<model::State>& toElement) {
    // TODO: implement
}
