#include "Transition.hpp"

#include "ModelUtils.hpp"
#include "State.hpp"
#include "actions/ModelActionFactory.hpp"
#include "private/IModelVisitor.hpp"

namespace model {

Transition::Transition(QSharedPointer<State> source, QSharedPointer<State> target, const QString& event)
    : StateMachineEntity(StateMachineEntity::Type::Transition)
    , mSource(source)
    , mTarget(target)
    , mEvent(event)
    , mTransitionAction(ModelActionFactory::createModelAction(ModelAction::NONE)) {}

Transition::~Transition() {
    qDebug() << "Transition::DELETE id:" << id() << " event:" << mEvent.get();
}

void Transition::accept(class IModelVisitor* visitor) {
    if (visitor) {
        visitor->visitTransition(this);
    }
}

Transition& Transition::operator=(const Transition& other) {
    if (this != &other) {
        copyEntityData(other);
    }

    return *this;
}

void Transition::copyEntityData(const StateMachineEntity& other) {
    StateMachineEntity::copyEntityData(other);

    if (const Transition* tOther = dynamic_cast<const Transition*>(&other)) {
        mTransitionType = tOther->mTransitionType;
        mEvent = tOther->mEvent;
        mTransitionAction =
            ModelActionFactory::createModelActionFromData(tOther->transitionAction()->serialize(), ModelAction::NONE);
        mConditionCallback = tOther->mConditionCallback;
        mExpectedConditionValue = tOther->mExpectedConditionValue;

        // do not copy source and target here, because they are not properties
        // of transition but linked
    }
}

const QString& Transition::event() const {
    return mEvent.get();
}

QSharedPointer<State> Transition::source() const {
    return mSource.lock();
}

QSharedPointer<State> Transition::target() const {
    return mTarget.lock();
}

void Transition::setSource(QSharedPointer<State> source) {
    if (mSource != source) {
        mSource = source;
        emit modelDataChanged(sharedFromThis().toWeakRef());
    }
}

void Transition::setTarget(QSharedPointer<State> target) {
    if (mTarget != target) {
        mTarget = target;
        emit modelDataChanged(sharedFromThis().toWeakRef());
    }
}

EntityID_t Transition::sourceId() const {
    auto ptr = mSource.lock();
    return (ptr ? ptr->id() : INVALID_MODEL_ID);
}

EntityID_t Transition::targetId() const {
    auto ptr = mTarget.lock();
    return (ptr ? ptr->id() : INVALID_MODEL_ID);
}

// Getters
TransitionType Transition::transitionType() const {
    return mTransitionType;
}

QSharedPointer<IModelAction> Transition::transitionAction() const {
    return mTransitionAction;
}

const QString& Transition::conditionCallback() const {
    return mConditionCallback.get();
}

bool Transition::expectedConditionValue() const {
    return mExpectedConditionValue;
}

// Setters
void Transition::setEvent(const QString& event) {
    mEvent = event;
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void Transition::setTransitionType(TransitionType type) {
    mTransitionType = type;
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void Transition::setTransitionAction(const QSharedPointer<IModelAction>& action) {
    mTransitionAction = (action ? action : ModelActionFactory::createModelAction(ModelAction::NONE));
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

bool Transition::hasTransitionAction() const {
    return (mTransitionAction && mTransitionAction->type() != ModelAction::NONE);
}

void Transition::setTransitionAction(const QString& actionData) {
    setTransitionAction(ModelActionFactory::createModelActionFromData(actionData, ModelAction::CALLBACK));
}

void Transition::setConditionCallback(const QString& callback) {
    mConditionCallback = callback;
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void Transition::setExpectedConditionValue(bool value) {
    mExpectedConditionValue = value;
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

QStringList Transition::properties() const {
    return {"event", "transitionAction", "conditionCallback", cKeyExpectedConditionValue, cKeyTransitionType};
}

bool Transition::setProperty(const QString& key, const QVariant& value) {
    bool handled = true;

    if (key == "event") {
        setEvent(value.toString());
    } else if (key == "transitionAction") {
        if (value.canConvert<QSharedPointer<IModelAction>>()) {
            setTransitionAction(value.value<QSharedPointer<IModelAction>>());
        } else {
            setTransitionAction(value.toString());
        }
    } else if (key == "conditionCallback") {
        setConditionCallback(value.toString());
    } else if (key == cKeyExpectedConditionValue) {
        setExpectedConditionValue(value.toBool());
    } else if (key == cKeyTransitionType) {
        setTransitionType(transitionTypeFromInt(value.toInt()));
    } else {
        handled = StateMachineEntity::setProperty(key, value);
    }

    return handled;
}

QVariant Transition::getProperty(const QString& key) const {
    if (key == "event") {
        return mEvent.get();
    } else if (key == "transitionAction") {
        return QVariant::fromValue(mTransitionAction);
    } else if (key == "conditionCallback") {
        return mConditionCallback.get();
    } else if (key == cKeyExpectedConditionValue) {
        return mExpectedConditionValue;
    } else if (key == cKeyTransitionType) {
        return static_cast<int>(mTransitionType);
    }
    return StateMachineEntity::getProperty(key);
}

};  // namespace model
