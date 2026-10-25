#include "StateWithActions.hpp"

#include "actions/IModelAction.hpp"
#include "actions/ModelActionFactory.hpp"
#include "actions/ModelActionUtils.hpp"

namespace model {

StateWithActions::StateWithActions(const QString& name, const StateType type)
    : State(name, type)
    , mOnStateChangedAction(ModelActionFactory::createModelAction(ModelAction::NONE)) {}

QSharedPointer<IModelAction> StateWithActions::onStateChangedAction() const {
    return mOnStateChangedAction;
}

const ModelActionList& StateWithActions::onEnteringActions() const {
    return mOnEnteringActions;
}

const ModelActionList& StateWithActions::onExitingActions() const {
    return mOnExitingActions;
}

QSharedPointer<IModelAction> StateWithActions::onEnteringAction() const {
    return (mOnEnteringActions.isEmpty() ? ModelActionFactory::createModelAction(ModelAction::NONE)
                                         : mOnEnteringActions.first());
}

QSharedPointer<IModelAction> StateWithActions::onExitingAction() const {
    return (mOnExitingActions.isEmpty() ? ModelActionFactory::createModelAction(ModelAction::NONE) : mOnExitingActions.first());
}

bool StateWithActions::hasOnStateChangedAction() const {
    return (mOnStateChangedAction && mOnStateChangedAction->type() != ModelAction::NONE);
}

bool StateWithActions::hasOnEnteringAction() const {
    return (mOnEnteringActions.isEmpty() == false);
}

bool StateWithActions::hasOnExitingAction() const {
    return (mOnExitingActions.isEmpty() == false);
}

void StateWithActions::setOnStateChangedAction(const QSharedPointer<IModelAction>& action) {
    mOnStateChangedAction = (action ? action : ModelActionFactory::createModelAction(ModelAction::NONE));

    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void StateWithActions::setOnStateChangedAction(const QString& actionData) {
    setOnStateChangedAction(ModelActionFactory::createModelActionFromData(actionData, ModelAction::CALLBACK));
}

void StateWithActions::setOnEnteringActions(const ModelActionList& actions) {
    setActions(mOnEnteringActions, actions);
}

void StateWithActions::setOnExitingActions(const ModelActionList& actions) {
    setActions(mOnExitingActions, actions);
}

void StateWithActions::addOnEnteringAction(const QSharedPointer<IModelAction>& action) {
    addAction(mOnEnteringActions, action);
}

void StateWithActions::addOnExitingAction(const QSharedPointer<IModelAction>& action) {
    addAction(mOnExitingActions, action);
}

void StateWithActions::insertOnEnteringAction(const int index, const QSharedPointer<IModelAction>& action) {
    insertAction(mOnEnteringActions, index, action);
}

void StateWithActions::insertOnExitingAction(const int index, const QSharedPointer<IModelAction>& action) {
    insertAction(mOnExitingActions, index, action);
}

void StateWithActions::removeOnEnteringAction(const int index) {
    removeAction(mOnEnteringActions, index);
}

void StateWithActions::removeOnExitingAction(const int index) {
    removeAction(mOnExitingActions, index);
}

void StateWithActions::moveOnEnteringAction(const int from, const int to) {
    moveAction(mOnEnteringActions, from, to);
}

void StateWithActions::moveOnExitingAction(const int from, const int to) {
    moveAction(mOnExitingActions, from, to);
}

void StateWithActions::setOnEnteringAction(const QSharedPointer<IModelAction>& action) {
    setActions(mOnEnteringActions, ModelActionUtils::singleActionList(action));
}

void StateWithActions::setOnExitingAction(const QSharedPointer<IModelAction>& action) {
    setActions(mOnExitingActions, ModelActionUtils::singleActionList(action));
}

void StateWithActions::setOnEnteringAction(const QString& actionData) {
    setOnEnteringAction(ModelActionFactory::createModelActionFromData(actionData, ModelAction::CALLBACK));
}

void StateWithActions::setOnExitingAction(const QString& actionData) {
    setOnExitingAction(ModelActionFactory::createModelActionFromData(actionData, ModelAction::CALLBACK));
}

QStringList StateWithActions::properties() const {
    return State::properties() + QStringList{"onStateChangedAction", "onEnteringAction", "onExitingAction"};
}

bool StateWithActions::setProperty(const QString& key, const QVariant& value) {
    bool handled = true;

    if (key == "onStateChangedAction") {
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

QVariant StateWithActions::getProperty(const QString& key) const {
    QVariant res;

    if (key == "onStateChangedAction") {
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

void StateWithActions::copyEntityData(const StateMachineEntity& other) {
    State::copyEntityData(other);

    if (const StateWithActions* o = dynamic_cast<const StateWithActions*>(&other)) {
        mOnStateChangedAction =
            ModelActionFactory::createModelActionFromData(o->onStateChangedAction()->serialize(), ModelAction::CALLBACK);
        mOnEnteringActions = ModelActionUtils::cloneActionList(o->onEnteringActions());
        mOnExitingActions = ModelActionUtils::cloneActionList(o->onExitingActions());
    }
}

};  // namespace model
