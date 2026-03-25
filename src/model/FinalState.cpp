#include "FinalState.hpp"

#include "actions/ModelActionFactory.hpp"
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
QSharedPointer<IModelAction> FinalState::onStateChangedAction() const {
    return mOnStateChangedAction;
}

// Setters
void FinalState::setOnStateChangedAction(const QSharedPointer<IModelAction>& action) {
    mOnStateChangedAction = action;
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

QString FinalState::onStateChangedCallback() const {
    return (mOnStateChangedAction ? mOnStateChangedAction->serialize() : QString());
}

void FinalState::setOnStateChangedCallback(const QString& callback) {
    setOnStateChangedAction(callback.isEmpty() ? QSharedPointer<IModelAction>()
                                               : createModelActionFromData(callback, ModelAction::CALLBACK));
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
        return (mOnStateChangedAction ? QVariant(mOnStateChangedAction->serialize()) : QVariant(QString()));
    }

    return State::getProperty(key);
}

void FinalState::copyEntityData(const StateMachineEntity& other) {
    State::copyEntityData(other);
    if (const FinalState* fOther = dynamic_cast<const FinalState*>(&other)) {
        mOnStateChangedAction = createModelActionFromData(fOther->onStateChangedCallback(), ModelAction::CALLBACK);
    }
}

};  // namespace model
