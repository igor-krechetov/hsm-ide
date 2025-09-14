#include "FinalState.hpp"

namespace model {

FinalState::FinalState(const QString& name)
    : State(name, State::StateType::Final) {}

// Getters
const QString& FinalState::onStateChangedCallback() const {
    return mOnStateChangedCallback;
}

// Setters
void FinalState::setOnStateChangedCallback(const QString& callback) {
    mOnStateChangedCallback = callback;
}

};  // namespace model
