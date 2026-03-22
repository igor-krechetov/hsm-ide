#include "ExitPoint.hpp"

#include "private/IModelVisitor.hpp"

namespace model {

ExitPoint::ExitPoint(const QString& name)
    : State(name, StateType::EXITPOINT) {}

void ExitPoint::accept(class IModelVisitor* visitor) {
    if (visitor) {
        visitor->visitExitPoint(this);
    }
}

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
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void ExitPoint::setOnStateChangedCallback(const QString& callback) {
    mOnStateChangedCallback = callback;
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void ExitPoint::setOnEnteringCallback(const QString& callback) {
    mOnEnteringCallback = callback;
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void ExitPoint::setOnExitingCallback(const QString& callback) {
    mOnExitingCallback = callback;
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

QStringList ExitPoint::properties() const {
    return State::properties() + QStringList{"event", "onStateChangedCallback", "onEnteringCallback", "onExitingCallback"};
}

bool ExitPoint::setProperty(const QString& key, const QVariant& value) {
    bool handled = true;

    if (key == "event") {
        setEvent(value.toString());
    } else if (key == "onStateChangedCallback") {
        setOnStateChangedCallback(value.toString());
    } else if (key == "onEnteringCallback") {
        setOnEnteringCallback(value.toString());
    } else if (key == "onExitingCallback") {
        setOnExitingCallback(value.toString());
    } else {
        handled = State::setProperty(key, value);
    }

    return handled;
}

QVariant ExitPoint::getProperty(const QString& key) const {
    if (key == "event") {
        return mEvent;
    } else if (key == "onStateChangedCallback") {
        return mOnStateChangedCallback;
    } else if (key == "onEnteringCallback") {
        return mOnEnteringCallback;
    } else if (key == "onExitingCallback") {
        return mOnExitingCallback;
    }

    return State::getProperty(key);
}

void ExitPoint::copyEntityData(const StateMachineEntity& other) {
    State::copyEntityData(other);
    if (const ExitPoint* eOther = dynamic_cast<const ExitPoint*>(&other)) {
        mEvent = eOther->mEvent;
        mOnStateChangedCallback = eOther->mOnStateChangedCallback;
        mOnEnteringCallback = eOther->mOnEnteringCallback;
        mOnExitingCallback = eOther->mOnExitingCallback;
    }
}

};  // namespace model
