#include "State.hpp"

#include "Transition.hpp"

State::State(const std::string& id, const Type type)
    : StateMachineElement(StateMachineElement::Type::State)
    , mId(id)
    , mStateType(type) {}

const std::string& State::id() const {
    return mId;
}

State::Type State::type() const {
    return mStateType;
}

void State::addChildState(QSharedPointer<State> child) {
    mChildren.push_back(child);
}

void State::addTransition(QSharedPointer<Transition> child) {
    mChildren.push_back(child);
}

const std::vector<QSharedPointer<StateMachineElement>>& State::children() const {
    return mChildren;
}
