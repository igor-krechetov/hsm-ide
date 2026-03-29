#include "ExitPoint.hpp"

#include "actions/ModelActionFactory.hpp"
#include "private/IModelVisitor.hpp"

namespace model {

ExitPoint::ExitPoint(const QString& name)
    : State(name, StateType::EXITPOINT)
    , mOnStateChangedAction(ModelActionFactory::createModelAction(ModelAction::NONE))
    , mOnEnteringAction(ModelActionFactory::createModelAction(ModelAction::NONE))
    , mOnExitingAction(ModelActionFactory::createModelAction(ModelAction::NONE)) {}

void ExitPoint::accept(class IModelVisitor* visitor) {
    if (visitor) {
        visitor->visitExitPoint(this);
    }
}

const QString& ExitPoint::event() const {
    return mEvent;
}

QSharedPointer<IModelAction> ExitPoint::onStateChangedAction() const {
    return mOnStateChangedAction;
}

QSharedPointer<IModelAction> ExitPoint::onEnteringAction() const {
    return mOnEnteringAction;
}

QSharedPointer<IModelAction> ExitPoint::onExitingAction() const {
    return mOnExitingAction;
}

bool ExitPoint::hasOnStateChangedAction() const {
    return (mOnStateChangedAction && mOnStateChangedAction->type() != ModelAction::NONE);
}

bool ExitPoint::hasOnEnteringAction() const {
    return (mOnEnteringAction && mOnEnteringAction->type() != ModelAction::NONE);
}

bool ExitPoint::hasOnExitingAction() const {
    return (mOnExitingAction && mOnExitingAction->type() != ModelAction::NONE);
}

void ExitPoint::setEvent(const QString& event) {
    mEvent = event;
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void ExitPoint::setOnStateChangedAction(const QSharedPointer<IModelAction>& action) {
    mOnStateChangedAction = (action ? action : ModelActionFactory::createModelAction(ModelAction::NONE));
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void ExitPoint::setOnEnteringAction(const QSharedPointer<IModelAction>& action) {
    mOnEnteringAction = (action ? action : ModelActionFactory::createModelAction(ModelAction::NONE));
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void ExitPoint::setOnExitingAction(const QSharedPointer<IModelAction>& action) {
    mOnExitingAction = (action ? action : ModelActionFactory::createModelAction(ModelAction::NONE));
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void ExitPoint::setOnStateChangedAction(const QString& actionData) {
    setOnStateChangedAction(ModelActionFactory::createModelActionFromData(actionData, ModelAction::CALLBACK));
}

void ExitPoint::setOnEnteringAction(const QString& actionData) {
    setOnEnteringAction(ModelActionFactory::createModelActionFromData(actionData, ModelAction::CALLBACK));
}

void ExitPoint::setOnExitingAction(const QString& actionData) {
    setOnExitingAction(ModelActionFactory::createModelActionFromData(actionData, ModelAction::CALLBACK));
}

QStringList ExitPoint::properties() const {
    return State::properties() + QStringList{"event", "onStateChangedAction", "onEnteringAction", "onExitingAction"};
}

bool ExitPoint::setProperty(const QString& key, const QVariant& value) {
    bool handled = true;

    if (key == "event") {
        setEvent(value.toString());
    } else if (key == "onStateChangedAction") {
        if (value.canConvert<QSharedPointer<IModelAction>>()) {
            setOnStateChangedAction(value.value<QSharedPointer<IModelAction>>());
        } else {
            setOnStateChangedAction(value.toString());
        }
    } else if (key == "onEnteringAction") {
        if (value.canConvert<QSharedPointer<IModelAction>>()) {
            setOnEnteringAction(value.value<QSharedPointer<IModelAction>>());
        } else {
            setOnEnteringAction(value.toString());
        }
    } else if (key == "onExitingAction") {
        if (value.canConvert<QSharedPointer<IModelAction>>()) {
            setOnExitingAction(value.value<QSharedPointer<IModelAction>>());
        } else {
            setOnExitingAction(value.toString());
        }
    } else {
        handled = State::setProperty(key, value);
    }

    return handled;
}

QVariant ExitPoint::getProperty(const QString& key) const {
    if (key == "event") {
        return mEvent;
    } else if (key == "onStateChangedAction") {
        return QVariant::fromValue(mOnStateChangedAction);
    } else if (key == "onEnteringAction") {
        return QVariant::fromValue(mOnEnteringAction);
    } else if (key == "onExitingAction") {
        return QVariant::fromValue(mOnExitingAction);
    }

    return State::getProperty(key);
}

void ExitPoint::copyEntityData(const StateMachineEntity& other) {
    State::copyEntityData(other);

    if (const ExitPoint* eOther = dynamic_cast<const ExitPoint*>(&other)) {
        mEvent = eOther->mEvent;
        mOnStateChangedAction =
            ModelActionFactory::createModelActionFromData(eOther->onStateChangedAction()->serialize(), ModelAction::NONE);
        mOnEnteringAction =
            ModelActionFactory::createModelActionFromData(eOther->onEnteringAction()->serialize(), ModelAction::NONE);
        mOnExitingAction =
            ModelActionFactory::createModelActionFromData(eOther->onExitingAction()->serialize(), ModelAction::NONE);
    }
}

};  // namespace model
