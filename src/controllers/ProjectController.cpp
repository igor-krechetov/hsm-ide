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
#include "model/ModelRootState.hpp"
#include "model/RegularState.hpp"
#include "model/StateMachineModel.hpp"
#include "model/StateMachineSerializer.hpp"
#include "model/Transition.hpp"
#include "view/elements/HsmTransition.hpp"
#include "view/elements/private/HsmElement.hpp"  // TODO: move out from private
#include "view/models/StateMachineEntityViewModel.hpp"
#include "view/models/StateMachineTreeModel.hpp"
#include "view/widgets/HsmGraphicsView.hpp"

Q_DECLARE_LOGGING_CATEGORY(ProjectController)

ProjectController::ProjectController(const QString& id, QObject* parent)
    : QObject(parent)
    , mId(id)
    , mModel(new model::StateMachineModel("Untitled")) {
    QObject::connect(mModel.get(), &model::StateMachineModel::modelEntityAdded, this, &ProjectController::modelEntityAdded);
    QObject::connect(mModel.get(), &model::StateMachineModel::modelEntityDeleted, this, &ProjectController::modelEntityDeleted);
    QObject::connect(mModel.get(), &model::StateMachineModel::modelDataChanged, this, &ProjectController::modelDataChanged);

    mHsmStrctureViewModel = new view::StateMachineTreeModel(mModel, this);
    mHsmEntityViewModel = new view::StateMachineEntityViewModel(mModel, this);
}

ProjectController::~ProjectController() {
    qDebug() << "DELETE ProjectController:" << this << mView;
}

void ProjectController::registerView(QPointer<HsmGraphicsView> view) {
    qDebug() << "---- ProjectController::registerView:" << view;
    mView = view;

    if (mView) {
        mView->setProjectController(sharedFromThis().toWeakRef());
    }
}

bool ProjectController::importModel(const QString& path) {
    bool res = false;
    QFile file(path);

    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        QString scxmlContent = in.readAll();

        file.close();

        {
            // block signals so we dont send unnecessary projectModelChanged updates
            QSignalBlocker blocker(this);
            model::StateMachineSerializer serializer;

            if (true == serializer.deserializeFromScxml(scxmlContent, mModel)) {
                mModelPath = path;
                mModified = false;
                res = true;
            }
        }

        emit projectModelChanged(this);
    } else {
        qCritical() << "Failed to open file for reading: " << path;
    }

    return res;
}

bool ProjectController::exportModel() {
    return exportModel(modelPath());
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

            mModified = false;
            mModelPath = path;
            res = true;

            emit projectModelChanged(this);
        }
    }

    return res;
}

void ProjectController::handleViewDropEvent(const QString& elementTypeId,
                                            const QPointF& parentPos,
                                            const model::EntityID_t targetElementId) {
    qDebug() << Q_FUNC_INFO << elementTypeId << targetElementId << parentPos;

    createElement(elementTypeId, parentPos, targetElementId);
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
        mView->moveHsmElement(draggedElementId, targetElementId);
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
             << "parent=" << parent->id() << " child=" << entity->id() << "childType=" << (int)entity->type();

    if (entity->type() == model::StateMachineEntity::Type::State) {
        static std::map<model::StateType, QString> sElementTypes = {{model::StateType::INITIAL, "initial"},
                                                                    {model::StateType::FINAL, "final"},
                                                                    {model::StateType::REGULAR, "state"},
                                                                    {model::StateType::ENTRYPOINT, "entrypoint"},
                                                                    {model::StateType::EXITPOINT, "exitpoint"},
                                                                    {model::StateType::HISTORY, "history"},
                                                                    {model::StateType::INCLUDE, "include"}};

        auto ptrState = entity.dynamicCast<model::State>();
        auto itType = sElementTypes.find(ptrState->stateType());

        if (itType != sElementTypes.end()) {
            view::HsmElement* newViewElement =
                mView->createHsmElement(entity, itType->second, entity->getPos(), entity->getSize(), parent->id());

            tryConnectSignal(newViewElement,
                             "elementConnected(model::EntityID_t,model::EntityID_t)",
                             this,
                             "connectElements(model::EntityID_t,model::EntityID_t)");
        } else {
            qFatal("ProjectController::handleModelEntityAdded: unexpected element type: %d",
                   static_cast<int>(ptrState->stateType()));
        }
    } else if (entity->type() == model::StateMachineEntity::Type::Transition) {
        auto ptrTransition = entity.dynamicCast<model::Transition>();

        if (ptrTransition) {
            qDebug() << ptrTransition->source() << ptrTransition->target();

            if (ptrTransition->source() && ptrTransition->target()) {
                view::HsmTransition* newViewTransition =
                    mView->createHsmTransition(ptrTransition, ptrTransition->sourceId(), ptrTransition->targetId());

                tryConnectSignal(newViewTransition,
                                 "transitionReconnected(model::EntityID_t,model::EntityID_t,model::EntityID_t)",
                                 this,
                                 "reconnectElements(model::EntityID_t,model::EntityID_t,model::EntityID_t)");
            }
        } else {
            qCritical() << "expected Transition entity, but couldnt cast to model::Transition";
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
        mView->reconnectHsmTransition(transitionId, newFromElementId, newToElementId);
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
        projectModified();
    } else {
        qCritical() << "StateMachineEntity doesnt exist!";
    }
}

void ProjectController::modelEntityDeleted(QWeakPointer<model::StateMachineEntity> parent,
                                           QWeakPointer<model::StateMachineEntity> entity) {
    qDebug() << Q_FUNC_INFO;
    auto ptrEntity = entity.lock();

    if (ptrEntity) {
        mView->deleteHsmElement(ptrEntity->id());
        projectModified();
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
            if (mView->findHsmTransition(ptrTransition->id()).isNull() == true) {
                view::HsmTransition* newViewTransition =
                    mView->createHsmTransition(ptrTransition, ptrTransition->sourceId(), ptrTransition->targetId());

                tryConnectSignal(newViewTransition,
                                 "transitionReconnected(model::EntityID_t,model::EntityID_t,model::EntityID_t)",
                                 this,
                                 "reconnectElements(model::EntityID_t,model::EntityID_t,model::EntityID_t)");
            }
        }

        // TODO: do we need a case when source or target changes directly in the model? maybe through the tree?
    }

    projectModified();
}

void ProjectController::createElement(const QString& elementTypeId,
                                      const QPointF& posParent,
                                      const model::EntityID_t parentElementId) {
    qDebug() << Q_FUNC_INFO << elementTypeId << parentElementId << posParent;

    static std::map<QString, model::StateType> sElementTypes = {// TODO: decide what to do with start type
                                                                {"initial", model::StateType::INITIAL},
                                                                {"final", model::StateType::FINAL},
                                                                {"state", model::StateType::REGULAR},
                                                                {"entrypoint", model::StateType::ENTRYPOINT},
                                                                {"exitpoint", model::StateType::EXITPOINT},
                                                                {"history", model::StateType::HISTORY},
                                                                {"include", model::StateType::INCLUDE}};

    auto it = sElementTypes.find(elementTypeId);

    if (sElementTypes.end() != it) {
        auto newModelElement = model::ModelElementsFactory::createUniqueState(it->second);
        auto parentState = mModel->root()->findRegularState(parentElementId);

        qDebug() << Q_FUNC_INFO << parentState;

        if (parentState.isNull()) {
            parentState = mModel->root();
        }

        newModelElement->setPos(posParent);
        parentState->addChildState(newModelElement);
    } else {
        qCritical() << Q_FUNC_INFO << "Unsupported element type:" << elementTypeId;
    }
}

void ProjectController::createTransition(const QSharedPointer<model::State>& fromElement,
                                         const QSharedPointer<model::State>& toElement) {
    // TODO: implement
}

void ProjectController::projectModified() {
    mModified = true;
    emit projectModelChanged(this);
}