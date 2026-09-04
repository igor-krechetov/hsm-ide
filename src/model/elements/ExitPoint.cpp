#include "ExitPoint.hpp"

#include "actions/ModelActionFactory.hpp"
#include "actions/ModelActionUtils.hpp"
#include "private/IModelVisitor.hpp"

namespace model {

ExitPoint::ExitPoint(const QString& name)
    : State(name, StateType::EXITPOINT)
    , mOnStateChangedAction(ModelActionFactory::createModelAction(ModelAction::NONE)) {}

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

const ModelActionList& ExitPoint::onEnteringActions() const {
    return mOnEnteringActions;
}

const ModelActionList& ExitPoint::onExitingActions() const {
    return mOnExitingActions;
}

QSharedPointer<IModelAction> ExitPoint::onEnteringAction() const {
    return (mOnEnteringActions.isEmpty() ? ModelActionFactory::createModelAction(ModelAction::NONE)
                                         : mOnEnteringActions.first());
}

QSharedPointer<IModelAction> ExitPoint::onExitingAction() const {
    return (mOnExitingActions.isEmpty() ? ModelActionFactory::createModelAction(ModelAction::NONE) : mOnExitingActions.first());
}

bool ExitPoint::hasOnStateChangedAction() const {
    return (mOnStateChangedAction && mOnStateChangedAction->type() != ModelAction::NONE);
}

bool ExitPoint::hasOnEnteringAction() const {
    return (mOnEnteringActions.isEmpty() == false);
}

bool ExitPoint::hasOnExitingAction() const {
    return (mOnExitingActions.isEmpty() == false);
}

void ExitPoint::setEvent(const QString& event) {
    mEvent = event;
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void ExitPoint::setOnStateChangedAction(const QSharedPointer<IModelAction>& action) {
    mOnStateChangedAction = (action ? action : ModelActionFactory::createModelAction(ModelAction::NONE));
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void ExitPoint::setOnEnteringActions(const ModelActionList& actions) {
    setActions(mOnEnteringActions, actions);
}

void ExitPoint::setOnExitingActions(const ModelActionList& actions) {
    setActions(mOnExitingActions, actions);
}

void ExitPoint::addOnEnteringAction(const QSharedPointer<IModelAction>& action) {
    addAction(mOnEnteringActions, action);
}

void ExitPoint::addOnExitingAction(const QSharedPointer<IModelAction>& action) {
    addAction(mOnExitingActions, action);
}

void ExitPoint::insertOnEnteringAction(const int index, const QSharedPointer<IModelAction>& action) {
    insertAction(mOnEnteringActions, index, action);
}

void ExitPoint::insertOnExitingAction(const int index, const QSharedPointer<IModelAction>& action) {
    insertAction(mOnExitingActions, index, action);
}

void ExitPoint::removeOnEnteringAction(const int index) {
    removeAction(mOnEnteringActions, index);
}

void ExitPoint::removeOnExitingAction(const int index) {
    removeAction(mOnExitingActions, index);
}

void ExitPoint::moveOnEnteringAction(const int from, const int to) {
    moveAction(mOnEnteringActions, from, to);
}

void ExitPoint::moveOnExitingAction(const int from, const int to) {
    moveAction(mOnExitingActions, from, to);
}

void ExitPoint::setOnStateChangedAction(const QString& actionData) {
    setOnStateChangedAction(ModelActionFactory::createModelActionFromData(actionData, ModelAction::CALLBACK));
}

void ExitPoint::setOnEnteringAction(const QSharedPointer<IModelAction>& action) {
    setActions(mOnEnteringActions, ModelActionUtils::singleActionList(action));
}

void ExitPoint::setOnExitingAction(const QSharedPointer<IModelAction>& action) {
    setActions(mOnExitingActions, ModelActionUtils::singleActionList(action));
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
        if (value.canConvert<ModelActionList>()) {
            setOnEnteringActions(value.value<ModelActionList>());
        } else if (value.canConvert<QSharedPointer<IModelAction>>()) {
            setOnEnteringAction(value.value<QSharedPointer<IModelAction>>());
        } else {
            setOnEnteringAction(value.toString());
        }
    } else if (key == "onExitingAction") {
        if (value.canConvert<ModelActionList>()) {
            setOnExitingActions(value.value<ModelActionList>());
        } else if (value.canConvert<QSharedPointer<IModelAction>>()) {
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
    QVariant res;

    if (key == "event") {
        res = mEvent;
    } else if (key == "onStateChangedAction") {
        res = QVariant::fromValue(mOnStateChangedAction);
    } else if (key == "onEnteringAction") {
        res = QVariant::fromValue(mOnEnteringActions);
    } else if (key == "onExitingAction") {
        res = QVariant::fromValue(mOnExitingActions);
    } else {
        res = State::getProperty(key);
    }

    return res;
}

void ExitPoint::copyEntityData(const StateMachineEntity& other) {
    State::copyEntityData(other);

    if (const ExitPoint* eOther = dynamic_cast<const ExitPoint*>(&other)) {
        mEvent = eOther->mEvent;
        mOnStateChangedAction =
            ModelActionFactory::createModelActionFromData(eOther->onStateChangedAction()->serialize(), ModelAction::NONE);
        mOnEnteringActions = ModelActionUtils::cloneActionList(eOther->onEnteringActions());
        mOnExitingActions = ModelActionUtils::cloneActionList(eOther->onExitingActions());
    }
}

};  // namespace model
