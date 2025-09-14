#include "InitialState.hpp"

#include "Transition.hpp"

namespace model {

// TODO: does it make sence to generate an ID here?
InitialState::InitialState(const QString& name)
    : State(name, State::StateType::Initial) {}

void InitialState::setTransition(const QSharedPointer<Transition>& transition) {
    mTransition = transition;
}

QSharedPointer<Transition> InitialState::transition() const {
    return mTransition;
}

};  // namespace model