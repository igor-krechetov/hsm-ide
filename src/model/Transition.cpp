#include "Transition.hpp"

#include "State.hpp"
#include "private/IModelVisitor.hpp"

namespace model {

Transition::Transition(QSharedPointer<State> source, QSharedPointer<State> target, const QString& event)
    : StateMachineEntity(StateMachineEntity::Type::Transition)
    , mSource(source)
    , mTarget(target)
    , mEvent(event) {}

void Transition::accept(class IModelVisitor* visitor) {
    if (visitor) {
        visitor->visitTransition(this);
    }
}

const QString& Transition::event() const {
    return mEvent;
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

const QString& Transition::transitionCallback() const {
    return mTransitionCallback;
}

const QString& Transition::conditionCallback() const {
    return mConditionCallback;
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

void Transition::setTransitionCallback(const QString& callback) {
    mTransitionCallback = callback;
    emit modelDataChanged(sharedFromThis().toWeakRef());
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
    return {"event", "transitionCallback", "conditionCallback", "expectedConditionValue", cKeyTransitionType};
}

bool Transition::setProperty(const QString& key, const QVariant& value) {
    bool handled = true;

    if (key == "event") {
        setEvent(value.toString());
    } else if (key == "transitionCallback") {
        setTransitionCallback(value.toString());
    } else if (key == "conditionCallback") {
        setConditionCallback(value.toString());
    } else if (key == "expectedConditionValue") {
        setExpectedConditionValue(value.toBool());
    } else if (key == "transitionType") {
        setTransitionType(transitionTypeFromInt(value.toInt()));
    } else {
        handled = StateMachineEntity::setProperty(key, value);
    }
    
    return handled;
}

QVariant Transition::getProperty(const QString& key) const {
    if (key == "event") {
        return mEvent;
    } else if (key == "transitionCallback") {
        return mTransitionCallback;
    } else if (key == "conditionCallback") {
        return mConditionCallback;
    } else if (key == "expectedConditionValue") {
        return mExpectedConditionValue;
    } else if (key == "transitionType") {
        return static_cast<int>(mTransitionType);
    }
    return StateMachineEntity::getProperty(key);
}

};  // namespace model
