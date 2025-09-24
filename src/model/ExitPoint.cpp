#include "ExitPoint.hpp"

namespace model {

ExitPoint::ExitPoint(const QString& name)
    : State(name, State::StateType::ExitPoint) {}

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
    emit modelDataChanged();
}

void ExitPoint::setOnStateChangedCallback(const QString& callback) {
    mOnStateChangedCallback = callback;
    emit modelDataChanged();
}

void ExitPoint::setOnEnteringCallback(const QString& callback) {
    mOnEnteringCallback = callback;
    emit modelDataChanged();
}

void ExitPoint::setOnExitingCallback(const QString& callback) {
    mOnExitingCallback = callback;
    emit modelDataChanged();
}

};  // namespace model
