#include "InitialState.hpp"

#include "Transition.hpp"

namespace model {

// TODO: does it make sence to generate an ID here?
InitialState::InitialState(const QString& name)
    : State(name, StateType::Initial) {}

void InitialState::setTransition(const QSharedPointer<Transition>& transition) {
    mTransition = transition;
    registerNewChild(transition);
}

QSharedPointer<Transition> InitialState::transition() const {
    return mTransition;
}

void InitialState::deleteChild(const EntityID_t id) {
    if (mTransition && mTransition->id() == id) {
        unregisterChild(mTransition);
        mTransition.reset();
    }
}

void InitialState::deleteDirectChild(const QSharedPointer<StateMachineEntity> child) {
    if (child == mTransition) {
        unregisterChild(mTransition);
        mTransition.reset();
    }
}

QSharedPointer<StateMachineEntity> InitialState::findParentState(const EntityID_t childId) {
    return (mTransition && mTransition->id() == childId ? sharedFromThis() : nullptr);
}

QSharedPointer<StateMachineEntity> InitialState::findChild(const EntityID_t id, const StateMachineEntity::Type type) const {
    return (type == StateMachineEntity::Type::Transition && mTransition && mTransition->id() == id ? mTransition : nullptr);
}

QStringList InitialState::properties() const {
    return {"name"};
}

bool InitialState::setProperty(const QString& key, const QVariant& value) {
    bool handled = true;

    if (key == "name") {
        setName(value.toString());
    } else {
        handled = State::setProperty(key, value);
    }

    return handled;
}

QVariant InitialState::getProperty(const QString& key) const {
    if (key == "name") {
        return mName;
    }
    return State::getProperty(key);
}

};  // namespace model