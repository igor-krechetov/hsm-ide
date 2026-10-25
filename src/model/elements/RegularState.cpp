#include "RegularState.hpp"

#include <QDebug>

#include "actions/ModelActionFactory.hpp"
#include "actions/ModelActionUtils.hpp"
#include "private/IModelVisitor.hpp"

namespace model {

RegularState::RegularState(const QString& name)
    : StateWithActions(name, StateType::REGULAR) {}

RegularState::~RegularState() {
    qDebug() << "DELETE RegularState:" << this;
    deleteAllChildren();
}

RegularState& RegularState::operator=(const RegularState& other) {
    if (this != &other) {
        State::operator=(other);
        mOnStateChangedAction =
            ModelActionFactory::createModelActionFromData(other.onStateChangedAction()->serialize(), ModelAction::CALLBACK);
        mOnEnteringActions = ModelActionUtils::cloneActionList(other.onEnteringActions());
        mOnExitingActions = ModelActionUtils::cloneActionList(other.onExitingActions());
    }

    return *this;
}

void RegularState::accept(class IModelVisitor* visitor) {
    if (visitor) {
        visitor->visitRegularState(this);
    }
}

bool RegularState::addChild(const QSharedPointer<StateMachineEntity>& child) {
    bool res = false;

    // Prevent duplicates
    if (child && mChildren.contains(child) == false) {
        mChildren.push_back(child);
        registerNewChild(child);
        res = true;
    }

    return res;
}

void RegularState::addChildState(const QSharedPointer<State>& child) {
    qDebug() << Q_FUNC_INFO << "parent=" << id() << mName;
    addChild(qSharedPointerCast<StateMachineEntity>(child));
}

void RegularState::addTransition(const QSharedPointer<Transition>& child) {
    qDebug() << Q_FUNC_INFO << "parent=" << id() << mName;
    addChild(qSharedPointerCast<StateMachineEntity>(child));
}

const QList<QSharedPointer<StateMachineEntity>>& RegularState::childrenEntities() const {
    return mChildren;
}

void RegularState::deleteChild(const EntityID_t id) {
    QSharedPointer<StateMachineEntity> parent = findParentState(id);

    if (parent) {
        QSharedPointer<StateMachineEntity> targetChild = parent->findChild(id);

        if (targetChild && (targetChild->type() == StateMachineEntity::Type::State)) {
            // NOTE: in general, connecting transitions to substates is not allowed. But there is at least one exception -
            // history states
            //       so need to collect a list of substates to search for all transitions that needs to be removed
            const QSet<EntityID_t> deletedStateIds = collectStateIdsForDeletion(targetChild);
            const QSet<EntityID_t> transitionIdsToDelete =
                collectLinkedTransitionIds(sharedFromThis().dynamicCast<RegularState>(), deletedStateIds);

            for (const EntityID_t transitionId : transitionIdsToDelete) {
                deleteChild(transitionId);
            }
        }

        if (parent.get() == this) {
            deleteDirectChild(targetChild);
        } else {
            parent->deleteDirectChild(targetChild);
        }
    }
}

void RegularState::deleteDirectChild(const QSharedPointer<StateMachineEntity>& child) {
    qDebug() << "StateMachineModel::deleteDirectChild" << child->id();

    if (child) {
        // child->forEachChildElement(
        //     [this](QSharedPointer<StateMachineEntity> parent, QSharedPointer<StateMachineEntity> element) {
        //         unregisterChild(element);
        //         return true;
        //     });
        mChildren.removeAll(child);
        unregisterChild(child);
    }
}

void RegularState::deleteAllChildren() {
    for (const auto& child : mChildren) {
        // child->forEachChildElement(
        //     [this](QSharedPointer<StateMachineEntity> parent, QSharedPointer<StateMachineEntity> element) {
        //         unregisterChild(element);
        //         return true;
        //     });
        unregisterChild(child);
    }

    mChildren.clear();
}

QSharedPointer<StateMachineEntity> RegularState::findParentState(const EntityID_t childId) {
    QSharedPointer<StateMachineEntity> res;

    if (INVALID_MODEL_ID != childId) {
        for (const auto& element : mChildren) {
            if (element->id() == childId) {
                res = sharedFromThis();
                break;
            }
        }

        if (!res) {
            for (const auto& element : mChildren) {
                res = element->findParentState(childId);

                if (res) {
                    break;
                }
            }
        }
    }

    return res;
}

QSharedPointer<StateMachineEntity> RegularState::findChild(const EntityID_t id, const StateMachineEntity::Type type) const {
    QSharedPointer<StateMachineEntity> res;

    if (INVALID_MODEL_ID != id) {
        for (const auto& element : mChildren) {
            if ((element->id() == id) && (type == StateMachineEntity::Type::Invalid || element->type() == type)) {
                res = element;
                break;
            }
        }

        if (!res) {
            for (const auto& element : mChildren) {
                res = element->findChild(id, type);

                if (res) {
                    break;
                }
            }
        }
    }

    return res;
}

QSharedPointer<State> RegularState::findState(const EntityID_t id) const {
    QSharedPointer<State> res;
    QSharedPointer<StateMachineEntity> childPtr = findChild(id, StateMachineEntity::Type::State);

    if (childPtr) {
        res = childPtr.dynamicCast<State>();
    }

    return res;
}

QSharedPointer<State> RegularState::findChildStateByName(const QString& name, const EntityID_t excludeId) {
    QSharedPointer<State> res;

    forEachChildElement([&](QSharedPointer<StateMachineEntity> parent, QSharedPointer<StateMachineEntity> element) {
        bool continueSearch = true;

        if (element->type() == StateMachineEntity::Type::State) {
            QSharedPointer<State> statePtr = element.dynamicCast<State>();
            if (statePtr && (statePtr->name() == name) && (element->id() != excludeId)) {
                res = statePtr;
                continueSearch = false;
            }
        }

        return continueSearch;
    });

    return res;
}

QSharedPointer<RegularState> RegularState::findRegularState(const EntityID_t id) const {
    QSharedPointer<RegularState> res;
    QSharedPointer<StateMachineEntity> childPtr = findChild(id, StateMachineEntity::Type::State);

    if (childPtr) {
        res = childPtr.dynamicCast<RegularState>();
    }

    return res;
}

QSharedPointer<Transition> RegularState::findTransition(const EntityID_t id) const {
    QSharedPointer<Transition> res;
    QSharedPointer<StateMachineEntity> childPtr = findChild(id, StateMachineEntity::Type::Transition);

    if (childPtr) {
        res = childPtr.dynamicCast<Transition>();
    }

    return res;
}

bool RegularState::forEachChildElement(
    std::function<bool(QSharedPointer<StateMachineEntity>, QSharedPointer<StateMachineEntity>)> callback,
    const int depth,
    const bool postOrderTraversal) {
    bool processedAllChildren = true;

    for (QSharedPointer<StateMachineEntity>& child : mChildren) {
        if (false == postOrderTraversal) {
            if ((processedAllChildren = callback(sharedFromThis(), child)) == false) {
                break;
            }
        }

        if (depth == DEPTH_INFINITE || depth > 1) {
            processedAllChildren =
                child->forEachChildElement(callback, (depth != DEPTH_INFINITE ? depth - 1 : depth), postOrderTraversal);

            if (false == processedAllChildren) {
                break;
            }
        }

        if (true == postOrderTraversal) {
            if ((processedAllChildren = callback(sharedFromThis(), child)) == false) {
                break;
            }
        }
    }

    return processedAllChildren;
}

void RegularState::copyEntityData(const StateMachineEntity& other) {
    StateWithActions::copyEntityData(other);

    // mChildren is not copied as it represents owned children, not shallow data
}

// =================================================================================================================
// Private
QSet<EntityID_t> RegularState::collectStateIdsForDeletion(const QSharedPointer<StateMachineEntity>& rootEntity) {
    QSet<EntityID_t> stateIds;

    if (rootEntity && (rootEntity->type() == StateMachineEntity::Type::State)) {
        stateIds.insert(rootEntity->id());
        rootEntity->forEachChildElement(
            [&stateIds](QSharedPointer<StateMachineEntity> parent, QSharedPointer<StateMachineEntity> child) {
                bool continueTraversal = true;
                Q_UNUSED(parent);

                if (child && (child->type() == StateMachineEntity::Type::State)) {
                    stateIds.insert(child->id());
                }

                return continueTraversal;
            },
            StateMachineEntity::DEPTH_INFINITE,
            false);
    }

    return stateIds;
}

QSet<EntityID_t> RegularState::collectLinkedTransitionIds(const QSharedPointer<RegularState>& searchRoot,
                                                          const QSet<EntityID_t>& stateIds) {
    QSet<EntityID_t> transitionIds;

    if (searchRoot && (stateIds.isEmpty() == false)) {
        searchRoot->forEachChildElement(
            [&transitionIds, &stateIds](QSharedPointer<StateMachineEntity> parent, QSharedPointer<StateMachineEntity> child) {
                bool continueTraversal = true;
                Q_UNUSED(parent);

                if (child && (child->type() == StateMachineEntity::Type::Transition)) {
                    const QSharedPointer<Transition> transition = child.dynamicCast<Transition>();

                    if (transition &&
                        (stateIds.contains(transition->sourceId()) || stateIds.contains(transition->targetId()))) {
                        transitionIds.insert(transition->id());
                    }
                }

                return continueTraversal;
            },
            StateMachineEntity::DEPTH_INFINITE,
            false);
    }

    return transitionIds;
}

};  // namespace model
