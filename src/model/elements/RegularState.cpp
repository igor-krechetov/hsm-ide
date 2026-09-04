#include "RegularState.hpp"

#include <QDebug>

#include "actions/ModelActionFactory.hpp"
#include "actions/ModelActionUtils.hpp"
#include "private/IModelVisitor.hpp"

namespace model {

RegularState::RegularState(const QString& name)
    : State(name, StateType::REGULAR)
    , mOnStateChangedAction(ModelActionFactory::createModelAction(ModelAction::NONE)) {}

RegularState::~RegularState() {
    qDebug() << "DELETE RegularState:" << this;
    deleteAllChildren();
}

RegularState& RegularState::operator=(const RegularState& other) {
    if (this != &other) {
        State::operator=(other);
        mOnStateChangedAction =
            ModelActionFactory::createModelActionFromData(other.onStateChangedAction()->serialize(), ModelAction::NONE);
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

QSharedPointer<IModelAction> RegularState::onStateChangedAction() const {
    return mOnStateChangedAction;
}

const ModelActionList& RegularState::onEnteringActions() const {
    return mOnEnteringActions;
}

const ModelActionList& RegularState::onExitingActions() const {
    return mOnExitingActions;
}

QSharedPointer<IModelAction> RegularState::onEnteringAction() const {
    return (mOnEnteringActions.isEmpty() ? ModelActionFactory::createModelAction(ModelAction::NONE)
                                         : mOnEnteringActions.first());
}

QSharedPointer<IModelAction> RegularState::onExitingAction() const {
    return (mOnExitingActions.isEmpty() ? ModelActionFactory::createModelAction(ModelAction::NONE) : mOnExitingActions.first());
}

void RegularState::setOnStateChangedAction(const QSharedPointer<IModelAction>& action) {
    mOnStateChangedAction = (action ? action : ModelActionFactory::createModelAction(ModelAction::NONE));

    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void RegularState::setOnEnteringActions(const ModelActionList& actions) {
    setActions(mOnEnteringActions, actions);
}

void RegularState::setOnExitingActions(const ModelActionList& actions) {
    setActions(mOnExitingActions, actions);
}

void RegularState::addOnEnteringAction(const QSharedPointer<IModelAction>& action) {
    addAction(mOnEnteringActions, action);
}

void RegularState::addOnExitingAction(const QSharedPointer<IModelAction>& action) {
    addAction(mOnExitingActions, action);
}

void RegularState::insertOnEnteringAction(const int index, const QSharedPointer<IModelAction>& action) {
    insertAction(mOnEnteringActions, index, action);
}

void RegularState::insertOnExitingAction(const int index, const QSharedPointer<IModelAction>& action) {
    insertAction(mOnExitingActions, index, action);
}

void RegularState::removeOnEnteringAction(const int index) {
    removeAction(mOnEnteringActions, index);
}

void RegularState::removeOnExitingAction(const int index) {
    removeAction(mOnExitingActions, index);
}

void RegularState::moveOnEnteringAction(const int from, const int to) {
    moveAction(mOnEnteringActions, from, to);
}

void RegularState::moveOnExitingAction(const int from, const int to) {
    moveAction(mOnExitingActions, from, to);
}

bool RegularState::hasOnStateChangedAction() const {
    return (mOnStateChangedAction && mOnStateChangedAction->type() != ModelAction::NONE);
}

bool RegularState::hasOnEnteringAction() const {
    return (mOnEnteringActions.isEmpty() == false);
}

bool RegularState::hasOnExitingAction() const {
    return (mOnExitingActions.isEmpty() == false);
}

void RegularState::setOnStateChangedAction(const QString& actionData) {
    setOnStateChangedAction(ModelActionFactory::createModelActionFromData(actionData, ModelAction::CALLBACK));
}

void RegularState::setOnEnteringAction(const QSharedPointer<IModelAction>& action) {
    setActions(mOnEnteringActions, ModelActionUtils::singleActionList(action));
}

void RegularState::setOnExitingAction(const QSharedPointer<IModelAction>& action) {
    setActions(mOnExitingActions, ModelActionUtils::singleActionList(action));
}

void RegularState::setOnEnteringAction(const QString& actionData) {
    setOnEnteringAction(ModelActionFactory::createModelActionFromData(actionData, ModelAction::CALLBACK));
}

void RegularState::setOnExitingAction(const QString& actionData) {
    setOnExitingAction(ModelActionFactory::createModelActionFromData(actionData, ModelAction::CALLBACK));
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

QSharedPointer<State> RegularState::findChildStateByName(const QString& name) {
    QSharedPointer<State> res;

    forEachChildElement([&](QSharedPointer<StateMachineEntity> parent, QSharedPointer<StateMachineEntity> element) {
        bool continueSearch = true;

        if (element->type() == StateMachineEntity::Type::State) {
            QSharedPointer<State> statePtr = element.dynamicCast<State>();
            if (statePtr && statePtr->name() == name) {
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

QStringList RegularState::properties() const {
    return State::properties() + QStringList{"onStateChangedAction", "onEnteringAction", "onExitingAction"};
}

bool RegularState::setProperty(const QString& key, const QVariant& value) {
    bool handled = true;

    if (key == "onStateChangedAction") {
        if (value.canConvert<QSharedPointer<IModelAction>>()) {
            setOnStateChangedAction(value.value<QSharedPointer<IModelAction>>());
        } else {
            setOnStateChangedAction(value.toString());
        }
    } else if (key == "onEnteringAction") {
        if (value.canConvert<ModelActionList>()) {
            setOnEnteringActions(value.value<ModelActionList>());
        } else if (value.canConvert<QSharedPointer<IModelAction>>()) {
            setOnEnteringAction(value.value<QSharedPointer<IModelAction>>());
        } else {
            setOnEnteringAction(value.toString());
        }
    } else if (key == "onExitingAction") {
        if (value.canConvert<ModelActionList>()) {
            setOnExitingActions(value.value<ModelActionList>());
        } else if (value.canConvert<QSharedPointer<IModelAction>>()) {
            setOnExitingAction(value.value<QSharedPointer<IModelAction>>());
        } else {
            setOnExitingAction(value.toString());
        }
    } else {
        handled = State::setProperty(key, value);
    }

    return handled;
}

QVariant RegularState::getProperty(const QString& key) const {
    QVariant res;

    if (key == "onStateChangedAction") {
        res = QVariant::fromValue(mOnStateChangedAction);
    } else if (key == "onEnteringAction") {
        res = QVariant::fromValue(mOnEnteringActions);
    } else if (key == "onExitingAction") {
        res = QVariant::fromValue(mOnExitingActions);
    } else {
        res = State::getProperty(key);
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
    State::copyEntityData(other);

    if (const RegularState* rOther = dynamic_cast<const RegularState*>(&other)) {
        mOnStateChangedAction =
            ModelActionFactory::createModelActionFromData(rOther->onStateChangedAction()->serialize(), ModelAction::NONE);
        mOnEnteringActions = ModelActionUtils::cloneActionList(rOther->onEnteringActions());
        mOnExitingActions = ModelActionUtils::cloneActionList(rOther->onExitingActions());

        // mChildren is not copied as it represents owned children, not shallow data
    }
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
