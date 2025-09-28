#include "SelfTransition.hpp"

#include "State.hpp"

namespace model {

SelfTransition::SelfTransition(QSharedPointer<State> state, const QString& event)
    : Transition(state, state, event) {}

// Getters
Transition::TransitionType SelfTransition::type() const {
    return mType;
}

// Setters
void SelfTransition::setType(Transition::TransitionType type) {
    mType = type;
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

};  // namespace model
