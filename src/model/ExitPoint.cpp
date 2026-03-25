#include "ExitPoint.hpp"

#include "actions/ModelActionFactory.hpp"
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

QSharedPointer<IModelAction> ExitPoint::onStateChangedAction() const {
    return mOnStateChangedAction;
}

QSharedPointer<IModelAction> ExitPoint::onEnteringAction() const {
    return mOnEnteringAction;
}

QSharedPointer<IModelAction> ExitPoint::onExitingAction() const {
    return mOnExitingAction;
}

// Setters
void ExitPoint::setEvent(const QString& event) {
    mEvent = event;
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void ExitPoint::setOnStateChangedAction(const QSharedPointer<IModelAction>& action) {
    mOnStateChangedAction = action;
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void ExitPoint::setOnEnteringAction(const QSharedPointer<IModelAction>& action) {
    mOnEnteringAction = action;
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void ExitPoint::setOnExitingAction(const QSharedPointer<IModelAction>& action) {
    mOnExitingAction = action;
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

QString ExitPoint::onStateChangedCallback() const {
    return (mOnStateChangedAction ? mOnStateChangedAction->serialize() : QString());
}

QString ExitPoint::onEnteringCallback() const {
    return (mOnEnteringAction ? mOnEnteringAction->serialize() : QString());
}

QString ExitPoint::onExitingCallback() const {
    return (mOnExitingAction ? mOnExitingAction->serialize() : QString());
}

void ExitPoint::setOnStateChangedCallback(const QString& callback) {
    setOnStateChangedAction(callback.isEmpty() ? QSharedPointer<IModelAction>()
                                               : createModelActionFromData(callback, ModelAction::CALLBACK));
}

void ExitPoint::setOnEnteringCallback(const QString& callback) {
    setOnEnteringAction(callback.isEmpty() ? QSharedPointer<IModelAction>()
                                           : createModelActionFromData(callback, ModelAction::CALLBACK));
}

void ExitPoint::setOnExitingCallback(const QString& callback) {
    setOnExitingAction(callback.isEmpty() ? QSharedPointer<IModelAction>()
                                          : createModelActionFromData(callback, ModelAction::CALLBACK));
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
        return (mOnStateChangedAction ? QVariant(mOnStateChangedAction->serialize()) : QVariant(QString()));
    } else if (key == "onEnteringCallback") {
        return (mOnEnteringAction ? QVariant(mOnEnteringAction->serialize()) : QVariant(QString()));
    } else if (key == "onExitingCallback") {
        return (mOnExitingAction ? QVariant(mOnExitingAction->serialize()) : QVariant(QString()));
    }

    return State::getProperty(key);
}

void ExitPoint::copyEntityData(const StateMachineEntity& other) {
    State::copyEntityData(other);

    if (const ExitPoint* eOther = dynamic_cast<const ExitPoint*>(&other)) {
        mEvent = eOther->mEvent;
        mOnStateChangedAction = createModelActionFromData(eOther->onStateChangedCallback(), ModelAction::CALLBACK);
        mOnEnteringAction = createModelActionFromData(eOther->onEnteringCallback(), ModelAction::CALLBACK);
        mOnExitingAction = createModelActionFromData(eOther->onExitingCallback(), ModelAction::CALLBACK);
    }
}

};  // namespace model
