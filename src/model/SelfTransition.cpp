#include "SelfTransition.hpp"

#include "State.hpp"

namespace model {

SelfTransition::SelfTransition(QSharedPointer<State> state, const QString& event)
    : Transition(state, state, event) {}

// Getters
Transition::Type SelfTransition::type() const {
    return mType;
}

// Setters
void SelfTransition::setType(Transition::Type type) {
    mType = type;
}

};  // namespace model
