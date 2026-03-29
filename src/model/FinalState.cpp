#include "FinalState.hpp"

#include "actions/ModelActionFactory.hpp"
#include "private/IModelVisitor.hpp"

namespace model {

FinalState::FinalState(const QString& name)
    : State(name, StateType::FINAL)
    , mOnStateChangedAction(ModelActionFactory::createModelAction(ModelAction::NONE)) {}

void FinalState::accept(class IModelVisitor* visitor) {
    if (visitor) {
        visitor->visitFinalState(this);
    }
}

QSharedPointer<IModelAction> FinalState::onStateChangedAction() const {
    return mOnStateChangedAction;
}

bool FinalState::hasOnStateChangedAction() const {
    return (mOnStateChangedAction && mOnStateChangedAction->type() != ModelAction::NONE);
}

void FinalState::setOnStateChangedAction(const QSharedPointer<IModelAction>& action) {
    mOnStateChangedAction = (action ? action : ModelActionFactory::createModelAction(ModelAction::NONE));
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void FinalState::setOnStateChangedAction(const QString& actionData) {
    setOnStateChangedAction(ModelActionFactory::createModelActionFromData(actionData, ModelAction::CALLBACK));
}

QStringList FinalState::properties() const {
    return State::properties() + QStringList{"onStateChangedAction"};
}

bool FinalState::setProperty(const QString& key, const QVariant& value) {
    bool handled = true;

    if (key == "onStateChangedAction") {
        if (value.canConvert<QSharedPointer<IModelAction>>()) {
            setOnStateChangedAction(value.value<QSharedPointer<IModelAction>>());
        } else {
            setOnStateChangedAction(value.toString());
        }
    } else {
        handled = State::setProperty(key, value);
    }

    return handled;
}

QVariant FinalState::getProperty(const QString& key) const {
    if (key == "onStateChangedAction") {
        return QVariant::fromValue(mOnStateChangedAction);
    }

    return State::getProperty(key);
}

void FinalState::copyEntityData(const StateMachineEntity& other) {
    State::copyEntityData(other);
    if (const FinalState* fOther = dynamic_cast<const FinalState*>(&other)) {
        mOnStateChangedAction =
            ModelActionFactory::createModelActionFromData(fOther->onStateChangedAction()->serialize(), ModelAction::NONE);
    }
}

};  // namespace model
