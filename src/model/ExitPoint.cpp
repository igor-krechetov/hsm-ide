#include "ExitPoint.hpp"

namespace model {

ExitPoint::ExitPoint(const QString& id)
    : State(id, State::StateType::Regular) {}

// Getters
const QString& ExitPoint::event() const {
    return mEvent;
}

const QString& ExitPoint::onStateChangedCallback() const {
    return mOnStateChangedCallback;
}

const QString& ExitPoint::onEnteringCallback() const {
    return mOnEnteringCallback;
}

const QString& ExitPoint::onExitingCallback() const {
    return mOnExitingCallback;
}

// Setters
void ExitPoint::setEvent(const QString& event) {
    mEvent = event;
}

void ExitPoint::setOnStateChangedCallback(const QString& callback) {
    mOnStateChangedCallback = callback;
}

void ExitPoint::setOnEnteringCallback(const QString& callback) {
    mOnEnteringCallback = callback;
}

void ExitPoint::setOnExitingCallback(const QString& callback) {
    mOnExitingCallback = callback;
}

};  // namespace model
