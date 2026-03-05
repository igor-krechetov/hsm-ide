#include "StateMachineModel.hpp"

#include <QDebug>
#include <QHash>
#include <QSignalBlocker>

#include "ModelElementsFactory.hpp"
#include "ModelRootState.hpp"
#include "RegularState.hpp"
#include "State.hpp"
#include "StateMachineEntity.hpp"
#include "Transition.hpp"

namespace model {

namespace {}

StateMachineModel::StateMachineModel(const QString& name, QObject* parent)
    : QObject(parent) {
    clearModel();
    setName(name);
}

StateMachineModel::~StateMachineModel() {
    qDebug() << "DELETE: " << this;
}

StateMachineModel& StateMachineModel::operator=(const StateMachineModel& other) {
    if (this == &other) {
        return *this;
    }

    clearModel();

    if (!other.root() || !mModelRoot) {
        return *this;
    }

    setName(other.name());
    mModelRoot->copyEntityData(*other.root());

    QHash<const State*, QSharedPointer<State>> copiedStates;
    copiedStates.insert(other.root().data(), mModelRoot);

    // pass 1: clone state entities (without parent links)
    other.root()->forEachChildElement(
        [&](QSharedPointer<StateMachineEntity> parent, QSharedPointer<StateMachineEntity> entity) {
            Q_UNUSED(parent);

            if (entity->type() == StateMachineEntity::Type::State) {
                auto sourceState = entity.dynamicCast<State>();
                auto newState = ModelElementsFactory::cloneStateEntity(sourceState);

                if (!newState) {
                    return false;
                }

                newState->copyEntityData(*sourceState);
                copiedStates.insert(sourceState.data(), newState);
            }

            return true;
        },
        StateMachineEntity::DEPTH_INFINITE,
        false);

    // pass 2: attach states to copied parents
    other.root()->forEachChildElement(
        [&](QSharedPointer<StateMachineEntity> parent, QSharedPointer<StateMachineEntity> entity) {
            if (entity->type() == StateMachineEntity::Type::State) {
                auto sourceParentState = parent.dynamicCast<State>();
                auto sourceChildState = entity.dynamicCast<State>();

                auto newParent = copiedStates.value(sourceParentState.data()).dynamicCast<RegularState>();
                auto newChild = copiedStates.value(sourceChildState.data());

                if (!newParent || !newChild) {
                    return false;
                }

                newParent->addChildState(newChild);
            }

            return true;
        },
        StateMachineEntity::DEPTH_INFINITE,
        false);

    // pass 3: clone transitions with remapped src/dst
    other.root()->forEachChildElement(
        [&](QSharedPointer<StateMachineEntity> parent, QSharedPointer<StateMachineEntity> entity) {
            Q_UNUSED(parent);

            if (entity->type() == StateMachineEntity::Type::Transition) {
                auto sourceTransition = entity.dynamicCast<Transition>();
                auto sourceSrc = sourceTransition->source();
                auto sourceDst = sourceTransition->target();

                auto newSrc = copiedStates.value(sourceSrc.data());
                auto newDst = copiedStates.value(sourceDst.data());
                auto newSrcRegular = newSrc.dynamicCast<RegularState>();

                if (!newSrcRegular) {
                    return false;
                }

                auto newTransition = QSharedPointer<Transition>::create(newSrc, newDst, sourceTransition->event());
                newTransition->copyEntityData(*sourceTransition);
                newSrcRegular->addTransition(newTransition);
            }

            return true;
        },
        StateMachineEntity::DEPTH_INFINITE,
        false);

    return *this;
}

QString StateMachineModel::name() const {
    return (mModelRoot ? mModelRoot->name() : QString());
}

void StateMachineModel::setName(const QString& name) {
    if (mModelRoot) {
        mModelRoot->setName(name);
    }
}

QSharedPointer<ModelRootState>& StateMachineModel::root() {
    return mModelRoot;
}

const QSharedPointer<ModelRootState>& StateMachineModel::root() const {
    return mModelRoot;
}

void StateMachineModel::clearModel() {
    QString oldName = name();

    mModelRoot = ModelElementsFactory::createUniqueState(StateType::MODEL_ROOT).dynamicCast<ModelRootState>();
    mModelRoot->setName(oldName);

    // Subscribe to modelEntityAdded for mModelRoot
    QObject::connect(mModelRoot.data(), &StateMachineEntity::childAdded, this, &StateMachineModel::modelChanged);
    QObject::connect(mModelRoot.data(), &StateMachineEntity::childRemoved, this, &StateMachineModel::modelChanged);
    QObject::connect(mModelRoot.data(), &StateMachineEntity::childAdded, this, &StateMachineModel::modelEntityAdded);
    QObject::connect(mModelRoot.data(), &StateMachineEntity::childRemoved, this, &StateMachineModel::modelEntityDeleted);
    QObject::connect(mModelRoot.data(), &StateMachineEntity::modelDataChanged, this, &StateMachineModel::modelDataChanged);
}

QSharedPointer<Transition> StateMachineModel::createUniqueTransition(const EntityID_t source, const EntityID_t target) {
    auto from = mModelRoot->findChild(source);
    auto to = mModelRoot->findChild(target);
    // TODO: check for errors
    return ModelElementsFactory::createUniqueTransition(from.dynamicCast<State>(), to.dynamicCast<State>());
}

// TODO: this probably will not work with transitions. decide how transitions will be moved
bool StateMachineModel::moveElement(const EntityID_t elementId, const EntityID_t newParentId) {
    bool moved = false;

    if (elementId != newParentId) {
        QSharedPointer<StateMachineEntity> currentParent = mModelRoot->findParentState(elementId);

        if (currentParent && currentParent->id() != newParentId) {
            QSharedPointer<StateMachineEntity> element = currentParent->findChild(elementId);
            QSharedPointer<RegularState> newParent =
                (mModelRoot->id() == newParentId ? mModelRoot : mModelRoot->findRegularState(newParentId));

            if (element && newParent) {
                // block model and parent from sending childAdded signals
                QSignalBlocker blockModel(this);
                QSignalBlocker blockParent(currentParent.get());

                newParent->addChild(element);
                currentParent->deleteDirectChild(element);
                moved = true;
            }

            if (true == moved) {
                emit modelChanged();
            }
        }
    }

    return moved;
}

bool StateMachineModel::reconnectElements(const EntityID_t transitionId,
                                          const EntityID_t newFromElementId,
                                          const EntityID_t newToElementId) {
    bool res = false;

    if (mModelRoot) {
        QSharedPointer<model::Transition> transitionPtr = mModelRoot->findTransition(transitionId);

        if (transitionPtr) {
            QSharedPointer<model::State> currentSourceElement = transitionPtr->source();
            QSharedPointer<model::State> newSourceElement = mModelRoot->findState(newFromElementId);

            if (currentSourceElement && (currentSourceElement->id() != newFromElementId)) {
                // Need to move transition to a new parent and change source
                qDebug() << "---- reset old source";
                QSignalBlocker block(currentSourceElement.get());

                currentSourceElement->deleteChild(transitionId);
            }

            if (currentSourceElement != newSourceElement) {
                qDebug() << "---- setSource";
                transitionPtr->setSource(newSourceElement);

                if (newSourceElement) {
                    newSourceElement->addChild(transitionPtr);
                }
            }
            transitionPtr->setTarget(mModelRoot->findState(newToElementId));
            qDebug() << transitionPtr->sourceId() << transitionPtr->targetId();

            res = true;
        } else {
            qCritical() << "Failed to find transition with id " << transitionId;
        }
    }

    return res;
}

void StateMachineModel::dump() {
    int indent = 0;

    mModelRoot->forEachChildElement(
        [&](QSharedPointer<StateMachineEntity> parent, QSharedPointer<StateMachineEntity> entity) {
            if (entity->type() == StateMachineEntity::Type::State) {
                QString stateType;
                QSharedPointer<State> state = entity.dynamicCast<State>();

                switch (state->stateType()) {
                    case StateType::MODEL_ROOT:
                        stateType = "MODEL_ROOT";
                        break;
                    case StateType::INITIAL:
                        stateType = "INITIAL";
                        break;
                    case StateType::REGULAR:
                        stateType = "REGULAR";
                        break;
                    case StateType::ENTRYPOINT:
                        stateType = "ENTRYPOINT";
                        break;
                    case StateType::EXITPOINT:
                        stateType = "EXITPOINT";
                        break;
                    case StateType::FINAL:
                        stateType = "FINAL";
                        break;
                    case StateType::HISTORY:
                        stateType = "HISTORY";
                        break;
                    case StateType::INCLUDE:
                        stateType = "INCLUDE";
                        break;
                    default:
                        stateType = "UNKNOWN";
                        break;
                }

                qDebug() << "[" << stateType << "] id:" << entity->id() << "name:" << entity->getProperty("name").toString();
            } else if (entity->type() == StateMachineEntity::Type::Transition) {
                QSharedPointer<Transition> transition = entity.dynamicCast<Transition>();
                auto sourceState = transition->source();
                auto targetState = transition->target();
                QString sourceName = "<null>";
                QString targetName = "<null>";

                if (sourceState) {
                    sourceName = sourceState->name();
                }
                if (targetState) {
                    targetName = targetState->name();
                }

                qDebug() << "[TRANSITION] id:" << entity->id() << "src:" << sourceName << " target:" << targetName
                         << "event:" << transition->event();
            }
            return true;
        },
        StateMachineEntity::DEPTH_INFINITE,
        false);
}

};  // namespace model
