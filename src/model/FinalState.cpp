#include "FinalState.hpp"

#include "private/IModelVisitor.hpp"

namespace model {

FinalState::FinalState(const QString& name)
    : State(name, StateType::FINAL) {}

void FinalState::accept(class IModelVisitor* visitor) {
    if (visitor) {
        visitor->visitFinalState(this);
    }
}

// Getters
const QString& FinalState::onStateChangedCallback() const {
    return mOnStateChangedCallback;
}

// Setters
void FinalState::setOnStateChangedCallback(const QString& callback) {
    mOnStateChangedCallback = callback;
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

QStringList FinalState::properties() const {
    return State::properties() + QStringList{"onStateChangedCallback"};
}

bool FinalState::setProperty(const QString& key, const QVariant& value) {
    bool handled = true;

    if (key == "onStateChangedCallback") {
        setOnStateChangedCallback(value.toString());
    } else {
        handled = State::setProperty(key, value);
    }

    return handled;
}

QVariant FinalState::getProperty(const QString& key) const {
    if (key == "onStateChangedCallback") {
        return mOnStateChangedCallback;
    }

    return State::getProperty(key);
}

void FinalState::copyEntityData(const StateMachineEntity& other) {
    State::copyEntityData(other);
    if (const FinalState* fOther = dynamic_cast<const FinalState*>(&other)) {
        mOnStateChangedCallback = fOther->mOnStateChangedCallback;
    }
}

};  // namespace model
