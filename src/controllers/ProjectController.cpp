#include "ProjectController.hpp"

#include <QDebug>
#include <QFile>
#include <QLoggingCategory>
#include <QMap>
#include <QMimeData>
#include <QMultiMap>
#include <QPolygonF>
#include <QSet>
#include <QTextStream>

#include "ObjectUtils.hpp"
#include "controllers/ModificationHistoryController.hpp"
#include "model/EntryPoint.hpp"
#include "model/ExitPoint.hpp"
#include "model/InitialState.hpp"
#include "model/ModelElementsFactory.hpp"
#include "model/ModelRootState.hpp"
#include "model/RegularState.hpp"
#include "model/StateHierarchyRules.hpp"
#include "model/StateMachineModel.hpp"
#include "model/StateMachineSerializer.hpp"
#include "model/Transition.hpp"
#include "view/elements/HsmTransition.hpp"
#include "view/elements/private/HsmElement.hpp"  // TODO: move out from private
#include "view/models/StateMachineEntityViewModel.hpp"
#include "view/models/StateMachineTreeModel.hpp"
#include "view/widgets/HsmGraphicsView.hpp"

Q_DECLARE_LOGGING_CATEGORY(ProjectController)

namespace {

QSharedPointer<model::State> cloneStateTree(const QSharedPointer<model::State>& sourceState,
                                            const QSharedPointer<model::State>& targetParent,
                                            QMap<model::EntityID_t, QSharedPointer<model::State>>& clonedStates) {
    QSharedPointer<model::State> clonedState;

    if (sourceState) {
        clonedState = model::ModelElementsFactory::cloneStateEntity(sourceState);

        if (clonedState) {
            const model::EntityID_t sourceStateId = sourceState->id();
            clonedState->copyEntityData(*sourceState);
            targetParent->addChild(clonedState);
            clonedStates.insert(sourceState->id(), clonedState);

            sourceState->forEachChildElement(
                [&clonedStates, &clonedState, &sourceStateId](QSharedPointer<model::StateMachineEntity> parent,
                                                              QSharedPointer<model::StateMachineEntity> child) {
                    bool continueTraversal = true;

                    if (parent && (parent->id() == sourceStateId) && child &&
                        (child->type() == model::StateMachineEntity::Type::State)) {
                        cloneStateTree(child.dynamicCast<model::State>(), clonedState, clonedStates);
                    }

                    return continueTraversal;
                },
                1,
                false);
        }
    }

    return clonedState;
}

bool hasSelectedAncestor(const QSharedPointer<model::ModelRootState>& rootState,
                         const QSharedPointer<model::State>& state,
                         const QSet<model::EntityID_t>& selectedStateIds) {
    bool hasAncestor = false;
    QSharedPointer<model::StateMachineEntity> currentParent;

    if (rootState && state) {
        currentParent = rootState->findParentState(state->id());

        while (currentParent && (hasAncestor == false)) {
            hasAncestor = selectedStateIds.contains(currentParent->id());
            currentParent = rootState->findParentState(currentParent->id());
        }
    }

    return hasAncestor;
}

}  // namespace

ProjectController::ProjectController(const QString& id, QObject* parent)
    : QObject(parent)
    , mId(id)
    , mModel(new model::StateMachineModel("Untitled"))
    , mHistoryController(new ModificationHistoryController(mModel)) {
    QObject::connect(mModel.get(), &model::StateMachineModel::modelEntityAdded, this, &ProjectController::modelEntityAdded);
    QObject::connect(mModel.get(), &model::StateMachineModel::modelEntityDeleted, this, &ProjectController::modelEntityDeleted);
    QObject::connect(mModel.get(), &model::StateMachineModel::modelDataChanged, this, &ProjectController::modelDataChanged);

    mHsmStructureViewModel = new view::StateMachineTreeModel(mModel, this);
    mHsmEntityViewModel = new view::StateMachineEntityViewModel(mModel, this);
    mHsmEntityViewModel->setHistoryTransactionCallbacks([this](const QString& label) { beginHistoryTransaction(label); },
                                                        [this]() { commitHistoryTransaction(); });
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

        refreshViewFromModel();
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

void ProjectController::updateModelPath(const QString& newPath) {
    if (newPath.isEmpty() == false) {
        mModelPath = newPath;
        emit projectModelChanged(this);
    }
}

void ProjectController::handleViewDropEvent(const QString& elementTypeId,
                                            const QPointF& parentPos,
                                            const model::EntityID_t targetElementId) {
    qDebug() << Q_FUNC_INFO << elementTypeId << targetElementId << parentPos;

    beginHistoryTransaction("Create element");
    createElement(elementTypeId, parentPos, targetElementId);
    commitHistoryTransaction();
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

    beginHistoryTransaction("Delete elements");

    for (model::EntityID_t id : elementIDs) {
        mModel->root()->deleteChild(id);
        // NOTE: model will send signal which will trigger view update
    }

    commitHistoryTransaction();
}

QString ProjectController::serializeElementsToScxml(const QList<model::EntityID_t>& elementIDs) const {
    qDebug() << "ProjectController::serializeElementsToScxml: itemsCount=" << elementIDs.size();
    QString serializedData;

    if (mModel && mModel->root()) {
        QSharedPointer<model::StateMachineModel> tempModel = QSharedPointer<model::StateMachineModel>::create("Clipboard");
        QMap<model::EntityID_t, QSharedPointer<model::State>> stateCopies;
        QList<QSharedPointer<model::State>> topLevelSelectedStates;
        QSet<model::EntityID_t> selectedStateIds;

        for (const model::EntityID_t elementId : elementIDs) {
            const auto sourceEntity = mModel->root()->findChild(elementId);

            if (sourceEntity && sourceEntity->type() == model::StateMachineEntity::Type::State) {
                const auto sourceState = sourceEntity.dynamicCast<model::State>();

                if (sourceState) {
                    selectedStateIds.insert(sourceState->id());
                }
            }
        }

        for (const model::EntityID_t stateId : selectedStateIds) {
            const auto stateEntity = mModel->root()->findChild(stateId);

            if (stateEntity) {
                const auto state = stateEntity.dynamicCast<model::State>();

                if (state && (hasSelectedAncestor(mModel->root(), state, selectedStateIds) == false)) {
                    topLevelSelectedStates.push_back(state);
                }
            }
        }

        for (const auto& sourceState : topLevelSelectedStates) {
            cloneStateTree(sourceState, tempModel->root(), stateCopies);
        }

        for (auto stateIt = stateCopies.constBegin(); stateIt != stateCopies.constEnd(); ++stateIt) {
            const model::EntityID_t sourceStateId = stateIt.key();
            const auto sourceStateEntity = mModel->root()->findChild(sourceStateId);
            const auto sourceState = sourceStateEntity.dynamicCast<model::State>();
            const auto sourceStateCopy = stateIt.value();

            if (sourceState && sourceStateCopy) {
                sourceState->forEachChildElement(
                    [&sourceStateId, &sourceStateCopy, &stateCopies](QSharedPointer<model::StateMachineEntity> parent,
                                                                     QSharedPointer<model::StateMachineEntity> entity) {
                        bool continueTraversal = true;

                        if (parent && entity && (parent->id() == sourceStateId) &&
                            (entity->type() == model::StateMachineEntity::Type::Transition)) {
                            const auto sourceTransition = entity.dynamicCast<model::Transition>();
                            const auto sourceTransitionTarget =
                                (sourceTransition ? sourceTransition->target() : QSharedPointer<model::State>());
                            const auto targetStateCopy =
                                (sourceTransitionTarget ? stateCopies.value(sourceTransitionTarget->id())
                                                        : QSharedPointer<model::State>());

                            if (sourceTransition && targetStateCopy) {
                                const auto transitionCopy =
                                    QSharedPointer<model::Transition>::create(sourceStateCopy,
                                                                              targetStateCopy,
                                                                              sourceTransition->event());
                                transitionCopy->copyEntityData(*sourceTransition);
                                transitionCopy->setSource(sourceStateCopy);
                                transitionCopy->setTarget(targetStateCopy);
                                sourceStateCopy->addChild(transitionCopy);
                            }
                        }

                        return continueTraversal;
                    },
                    1,
                    false);
            }
        }

        model::StateMachineSerializer serializer;
        serializedData = serializer.serializeToScxml(tempModel, false).trimmed();
    }

    return serializedData;
}

bool ProjectController::pasteScxmlElements(const QString& scxmlContent,
                                           const QList<model::EntityID_t>& selectedElementIDs,
                                           const QPointF& cursorScenePos,
                                           const bool useCursorPosition) {
    bool pasted = false;

    if (mModel && (scxmlContent.isEmpty() == false)) {
        QSharedPointer<model::StateMachineModel> importModel =
            QSharedPointer<model::StateMachineModel>::create("ClipboardImport");
        model::StateMachineSerializer serializer;
        const QString wrapperId = "__hsmide_clipboard_wrapper__";

        if (serializer.deserializeFromUnwrapperScxml(scxmlContent, wrapperId, importModel)) {
            importModel->dump();  // TODO: debug

            const auto targetParent = resolvePasteTargetParent(selectedElementIDs);
            QList<QSharedPointer<model::State>> importedStates;
            // used to update references to ExitPoint/Final states
            QMultiMap<model::EntityID_t, QSharedPointer<model::Transition>> importedTransitions;
            QSharedPointer<model::State> rootState = importModel->root()->findChildStateByName(wrapperId);
            QPointF importedTopLeft;
            bool importedTopLeftInitialized = false;

            if (!rootState) {
                rootState = importModel->root();
            }

            // iterate over top level elements to:
            // - collect states that needs to be pasted
            // - colelct direct transitions (for reference updates if needed)
            // - calculate top-left position of imported elements
            rootState->forEachChildElement(
                [&](QSharedPointer<model::StateMachineEntity> parent, QSharedPointer<model::StateMachineEntity> entity) {
                    if (parent && entity && (parent == rootState) &&
                        (entity->type() == model::StateMachineEntity::Type::State)) {
                        const auto importedState = entity.dynamicCast<model::State>();

                        if (importedState) {
                            const QPointF importedPos = importedState->getPos();

                            // check if this is state is the most top-left one
                            if (false == importedTopLeftInitialized) {
                                importedTopLeft = importedPos;
                                importedTopLeftInitialized = true;
                            } else {
                                importedTopLeft.setX(qMin(importedTopLeft.x(), importedPos.x()));
                                importedTopLeft.setY(qMin(importedTopLeft.y(), importedPos.y()));
                            }

                            importedStates.push_back(importedState);
                        }

                        // collect direct transitions incase we'll need to update references
                        entity->forEachChildElement(
                            [&importedTransitions](QSharedPointer<model::StateMachineEntity> transitionParent,
                                                   QSharedPointer<model::StateMachineEntity> transitionEntity) {
                                if (transitionEntity &&
                                    (transitionEntity->type() == model::StateMachineEntity::Type::Transition)) {
                                    const auto transition = transitionEntity.dynamicCast<model::Transition>();

                                    if (transition) {
                                        importedTransitions.insert(transition->targetId(), transition);
                                    }
                                }

                                return true;
                            },
                            1,
                            false);
                    }

                    return true;
                },
                1,
                false);

            QPointF pasteAnchorPos = QPointF(0.0, 0.0);

            if (targetParent && (useCursorPosition == true)) {
                if (targetParent == mModel->root()) {
                    // NOTE: If pasting on the top level - just use cursor position as an anchor
                    pasteAnchorPos = cursorScenePos;
                } else {
                    // NOTE: if cursor is within selected target parent element, calculate paste anchor position
                    // based on cursor position. Otherwise, use default anchor (0,0) relative to target parent
                    const QPointF targetParentScenePos = calculateStateScenePos(targetParent);
                    const QPointF localCursorPos = cursorScenePos - targetParentScenePos;
                    const QSizeF parentSize = targetParent->getSize();
                    const bool cursorInsideParent = (localCursorPos.x() >= 0.0) && (localCursorPos.y() >= 0.0) &&
                                                    (localCursorPos.x() <= parentSize.width()) &&
                                                    (localCursorPos.y() <= parentSize.height());

                    if (cursorInsideParent) {
                        pasteAnchorPos = localCursorPos;
                    }
                }
            }

            beginHistoryTransaction("Paste elements");
            // NOTE: prevent view from being updated. Due to transitions depending on elements order, we'll just update the
            // model
            //       and then refresh the view.
            mIgnoreAddedModelEntities = true;

            const QPointF pasteOffset = pasteAnchorPos - importedTopLeft;

            for (const auto& state : importedStates) {
                QSharedPointer<model::State> newState;

                // NOTE: because <initial> and <final> are treated differently depending if they are in top-level or
                //       a substate, we ony know how to handle them right before adding them to the sceene
                if (state->stateType() == model::StateType::ENTRYPOINT && targetParent == mModel->root()) {
                    newState = model::ModelElementsFactory::createInitialFrom(state.dynamicCast<model::EntryPoint>());
                } else if (state->stateType() == model::StateType::EXITPOINT && targetParent == mModel->root()) {
                    // TODO: how do we update references to transitions pointing to this element?
                    newState = model::ModelElementsFactory::createFinalFrom(state.dynamicCast<model::ExitPoint>());

                    // iterate over importedTransitions for state-id()
                    auto range = importedTransitions.equal_range(state->id());
                    for (auto it = range.first; it != range.second; ++it) {
                        const auto& transition = it.value();

                        // Re-link the transition to point to new state
                        transition->setTarget(newState);
                    }
                } else {
                    newState = state;
                }

                if (newState && model::StateHierarchyRules::canAddEntityToParent(targetParent, newState)) {
                    // NOTE: transition geometry stores grip points in parent coordinate system.
                    //       For top-level pasted states we need to shift direct transition geometry
                    //       by the same offset as states so transition shape is preserved.
                    state->forEachChildElement(
                        [&pasteOffset](QSharedPointer<model::StateMachineEntity> transitionParent,
                                       QSharedPointer<model::StateMachineEntity> transitionEntity) {
                            if (transitionParent && transitionEntity &&
                                (transitionEntity->type() == model::StateMachineEntity::Type::Transition)) {
                                QVariant geometryVar =
                                    transitionEntity->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY);

                                if (geometryVar.isValid()) {
                                    QPolygonF geometry = geometryVar.value<QPolygonF>();

                                    for (auto& point : geometry) {
                                        point += pasteOffset;
                                    }

                                    transitionEntity->setMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY, geometry);
                                }
                            }

                            return true;
                        },
                        1,
                        false);

                    const QPointF sourcePos = state->getPos();
                    const QPointF normalizedPos = sourcePos - importedTopLeft;
                    newState->setPos(pasteAnchorPos + normalizedPos);
                    targetParent->addChild(newState);
                } else {
                    qWarning() << "Cannot paste state " << newState->id() << " to target parent " << targetParent->id();
                }
            }

            commitHistoryTransaction();
            mIgnoreAddedModelEntities = false;

            qDebug() << "---- model after paste: " << mModel->name();
            mModel->dump();  // TODO: debug

            refreshViewFromModel();

            pasted = true;
        }
    }

    return pasted;
}

QPointF ProjectController::calculateStateScenePos(const QSharedPointer<model::State>& state) const {
    auto elemPtr = mView->findHsmElement(state->id());

    return (elemPtr != nullptr ? elemPtr->mapToScene(QPointF(0, 0)) : QPointF());
}

QSharedPointer<model::State> ProjectController::resolvePasteTargetParent(
    const QList<model::EntityID_t>& selectedElementIDs) const {
    QSharedPointer<model::State> targetParent;
    QList<QSharedPointer<model::State>> selectedStates;

    if (mModel && mModel->root()) {
        auto isAncestor = [this](const QSharedPointer<model::State>& possibleAncestor,
                                 const QSharedPointer<model::State>& possibleDescendant) {
            bool ancestorFound = false;
            QSharedPointer<model::StateMachineEntity> current = possibleDescendant;

            while (current) {
                if (current == possibleAncestor) {
                    ancestorFound = true;
                }

                current = mModel->root()->findParentState(current->id());
            }

            return ancestorFound;
        };

        for (const model::EntityID_t elementId : selectedElementIDs) {
            const auto entity = mModel->root()->findChild(elementId);

            if (entity && entity->type() == model::StateMachineEntity::Type::State) {
                selectedStates.push_back(entity.dynamicCast<model::State>());
            }
        }

        if (selectedStates.isEmpty() == false) {
            targetParent = selectedStates.first();

            for (const auto& candidate : selectedStates) {
                if (candidate && targetParent && isAncestor(candidate, targetParent) && (candidate != targetParent)) {
                    targetParent = candidate;
                }
            }

            // Resolve paste parent correctly when multiple sibling states are selected.
            // If there is no selected ancestor that contains all selected states, paste targets their common direct
            // parent (or root if parents differ).
            bool targetIsAncestorForAll = true;

            for (const auto& candidate : selectedStates) {
                if (candidate && targetParent && (isAncestor(targetParent, candidate) == false)) {
                    targetIsAncestorForAll = false;
                    break;
                }
            }

            if (false == targetIsAncestorForAll) {
                QSharedPointer<model::State> firstParent;
                bool allHaveSameParent = true;

                for (const auto& candidate : selectedStates) {
                    QSharedPointer<model::State> candidateParent;
                    const auto parentEntity = (candidate ? mModel->root()->findParentState(candidate->id()) : nullptr);

                    if (parentEntity) {
                        candidateParent = parentEntity.dynamicCast<model::State>();
                    }

                    if (false == firstParent.isNull()) {
                        if (candidateParent != firstParent) {
                            allHaveSameParent = false;
                            break;
                        }
                    } else {
                        firstParent = candidateParent;
                    }
                }

                if (allHaveSameParent && firstParent) {
                    targetParent = firstParent;
                } else {
                    targetParent = mModel->root();
                }
            }
        }

        if (targetParent.isNull()) {
            targetParent = mModel->root();
        }
    }

    return targetParent;
}

void ProjectController::handleModelEntityAdded(QSharedPointer<model::StateMachineEntity> parent,
                                               QSharedPointer<model::StateMachineEntity> entity,
                                               const bool addChildren) {
    if (false == mIgnoreAddedModelEntities) {
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

        if (true == addChildren) {
            entity->forEachChildElement(
                [this](QSharedPointer<model::StateMachineEntity> parent, QSharedPointer<model::StateMachineEntity> element) {
                    handleModelEntityAdded(parent, element, false);
                    return true;
                },
                -1,
                false);
        }
    }
}

void ProjectController::connectElements(const model::EntityID_t fromElementId, const model::EntityID_t toElementId) {
    qDebug() << Q_FUNC_INFO << fromElementId << " -> " << toElementId;

    beginHistoryTransaction("Create transition");
    auto source = mModel->root()->findState(fromElementId);
    auto target = mModel->root()->findState(toElementId);
    auto newTransition = model::ModelElementsFactory::createUniqueTransition(source, target);
    commitHistoryTransaction();
}

void ProjectController::reconnectElements(const model::EntityID_t transitionId,
                                          const model::EntityID_t newFromElementId,
                                          const model::EntityID_t newToElementId) {
    qDebug() << Q_FUNC_INFO << transitionId << ": " << newFromElementId << " -> " << newToElementId;

    beginHistoryTransaction("Reconnect transition");

    if (mModel->reconnectElements(transitionId, newFromElementId, newToElementId) == true) {
        mView->reconnectHsmTransition(transitionId, newFromElementId, newToElementId);
        commitHistoryTransaction();
    } else {
        qCritical() << "Failed to reconnect transition with id " << transitionId << newFromElementId << newToElementId;
    }

    cancelHistoryTransaction();
}

void ProjectController::modelEntityAdded(QWeakPointer<model::StateMachineEntity> parent,
                                         QWeakPointer<model::StateMachineEntity> entity) {
    qDebug() << Q_FUNC_INFO;
    auto ptrChild = entity.lock();
    auto ptrParent = parent.lock();

    if (ptrChild && ptrParent) {
        if (mHistoryController) {
            mHistoryController->onModelEntityAdded(ptrParent->id(), ptrChild->id());
        }
        handleModelEntityAdded(ptrParent, ptrChild, true);
        projectModified();
    } else {
        qCritical() << "StateMachineEntity doesnt exist!";
    }
}

void ProjectController::modelEntityDeleted(QWeakPointer<model::StateMachineEntity> parent,
                                           QWeakPointer<model::StateMachineEntity> entity) {
    auto ptrEntity = entity.lock();

    if (ptrEntity) {
        if (mHistoryController) {
            mHistoryController->onModelEntityDeleted(model::INVALID_MODEL_ID, ptrEntity->id());
        }
        mView->deleteHsmElement(ptrEntity->id());
        projectModified();
    } else {
        qCritical() << "StateMachineEntity doesnt exist!";
    }
}

void ProjectController::modelDataChanged(QWeakPointer<model::StateMachineEntity> entity) {
    auto ptrEntity = entity.lock();

    if (ptrEntity && mHistoryController) {
        mHistoryController->onModelDataChanged(ptrEntity->id());
    }

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

void ProjectController::beginHistoryTransaction(const QString& label) {
    if (mHistoryController) {
        mHistoryController->beginTransaction(label);
    }
}

void ProjectController::commitHistoryTransaction() {
    if (mHistoryController) {
        mHistoryController->commitTransaction();
        emit projectModelChanged(this);
    }
}

void ProjectController::cancelHistoryTransaction() {
    if (mHistoryController) {
        mHistoryController->cancelTransaction();
    }
}

void ProjectController::markHistoryElement(const model::EntityID_t elementId) {
    if (mHistoryController) {
        mHistoryController->markChanged(elementId);
    }
}

void ProjectController::unmarkHistoryElement(const model::EntityID_t elementId) {
    if (mHistoryController) {
        mHistoryController->unmarkChanged(elementId);
    }
}

bool ProjectController::undo() {
    if (mHistoryController && mHistoryController->undo()) {
        refreshViewFromModel();
        emit projectModelChanged(this);
        return true;
    }

    return false;
}

bool ProjectController::redo() {
    if (mHistoryController && mHistoryController->redo()) {
        refreshViewFromModel();
        emit projectModelChanged(this);
        return true;
    }

    return false;
}

bool ProjectController::canUndo() const {
    return (mHistoryController ? mHistoryController->canUndo() : false);
}

bool ProjectController::canRedo() const {
    return (mHistoryController ? mHistoryController->canRedo() : false);
}

void ProjectController::refreshViewFromModel() {
    qDebug() << Q_FUNC_INFO;
    if (mView && mModel) {
        auto root = mModel->root();

        // Clear all elements from the view
        mView->clearAllHsmElements();

        if (root) {
            // Recursively add states only
            root->forEachChildElement(
                [&](QSharedPointer<model::StateMachineEntity> parent, QSharedPointer<model::StateMachineEntity> child) {
                    if (child && child->type() == model::StateMachineEntity::Type::State) {
                        handleModelEntityAdded(parent, child, false);
                    }
                    return true;
                },
                -1,
                false);

            // Recursively add transitions. If we try doing it together with states,
            // we might end up not having a source/target when creatng transition (because we can't control states order in the
            // model)
            root->forEachChildElement(
                [&](QSharedPointer<model::StateMachineEntity> parent, QSharedPointer<model::StateMachineEntity> child) {
                    if (child && child->type() == model::StateMachineEntity::Type::Transition) {
                        handleModelEntityAdded(parent, child, false);
                    }
                    return true;
                },
                -1,
                false);
        }
    }
}
