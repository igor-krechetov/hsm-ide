#include "FinalState.hpp"

#include "private/IModelVisitor.hpp"

namespace model {

FinalState::FinalState(const QString& name)
    : StateWithActions(name, StateType::FINAL) {}

void FinalState::accept(class IModelVisitor* visitor) {
    if (visitor) {
        visitor->visitFinalState(this);
    }
}

const QString& FinalState::event() const {
    return mEvent;
}

void FinalState::setEvent(const QString& event) {
    if (mEvent != event) {
        mEvent = event;
        emit modelDataChanged(sharedFromThis().toWeakRef());
    }
}

QStringList FinalState::properties() const {
    // A top-level final state never exits, so onExiting is hidden from the property system.
    return State::properties() + QStringList{"event", "onStateChangedAction", "onEnteringAction"};
}

bool FinalState::setProperty(const QString& key, const QVariant& value) {
    bool handled = true;

    if (key == "event") {
        setEvent(value.toString());
    } else if (key == "onExitingAction") {
        // onExiting is hidden for a final state: treat it as an unknown key (no-op).
        handled = false;
    } else {
        handled = StateWithActions::setProperty(key, value);
    }

    return handled;
}

QVariant FinalState::getProperty(const QString& key) const {
    QVariant res;

    if (key == "event") {
        res = mEvent;
    } else if (key == "onExitingAction") {
        // onExiting is hidden for a final state: return the same default as an unknown key.
    } else {
        res = StateWithActions::getProperty(key);
    }

    return res;
}

void FinalState::copyEntityData(const StateMachineEntity& other) {
    StateWithActions::copyEntityData(other);

    if (const FinalState* fOther = dynamic_cast<const FinalState*>(&other)) {
        mEvent = fOther->mEvent;
    }
}

};  // namespace model
