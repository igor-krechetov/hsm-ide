#include "InitialState.hpp"

#include "Transition.hpp"
#include "private/IModelVisitor.hpp"

namespace model {

// TODO: does it make sence to generate an ID here?
InitialState::InitialState()
    : State("Initial", StateType::INITIAL) {}

void InitialState::accept(class IModelVisitor* visitor) {
    if (visitor) {
        visitor->visitInitialState(this);
    }
}

void InitialState::setTransition(const QSharedPointer<Transition>& transition) {
    if (mTransition) {
        unregisterChild(mTransition);
    }

    mTransition = transition;

    if (mTransition) {
        mTransition->setSource(sharedFromThis().dynamicCast<State>());
        registerNewChild(mTransition);
    }
}

QSharedPointer<Transition> InitialState::transition() const {
    return mTransition;
}

QStringList InitialState::properties() const {
    return {};
}

bool InitialState::addChild(const QSharedPointer<StateMachineEntity>& child) {
    if (child && (child->type() == StateMachineEntity::Type::Transition)) {
        setTransition(child.dynamicCast<Transition>());
    } else {
        setTransition(nullptr);
    }

    return true;
}

void InitialState::deleteChild(const EntityID_t id) {
    if (mTransition && mTransition->id() == id) {
        unregisterChild(mTransition);
        mTransition.reset();
    }
}

void InitialState::deleteDirectChild(const QSharedPointer<StateMachineEntity>& child) {
    if (child == mTransition) {
        unregisterChild(mTransition);
        mTransition.reset();
    }
}

QSharedPointer<StateMachineEntity> InitialState::findParentState(const EntityID_t childId) {
    return (mTransition && mTransition->id() == childId ? sharedFromThis() : nullptr);
}

QSharedPointer<StateMachineEntity> InitialState::findChild(const EntityID_t id, const StateMachineEntity::Type type) const {
    return ((type == StateMachineEntity::Type::Invalid || type == StateMachineEntity::Type::Transition) && mTransition &&
                    (mTransition->id() == id)
                ? mTransition
                : nullptr);
}

bool InitialState::forEachChildElement(
    std::function<bool(QSharedPointer<StateMachineEntity>, QSharedPointer<StateMachineEntity>)> callback,
    const int depth,
    const bool postOrderTraversal) {
    Q_UNUSED(depth);
    bool processedAllChildren = true;

    if (mTransition) {
        processedAllChildren = callback(sharedFromThis(), mTransition.dynamicCast<StateMachineEntity>());
    }

    return processedAllChildren;
}

// void InitialState::copyEntityData(const StateMachineEntity& other) {
//     State::copyEntityData(other);

//     if (const InitialState* iOther = dynamic_cast<const InitialState*>(&other)) {
//     }
// }

};  // namespace model