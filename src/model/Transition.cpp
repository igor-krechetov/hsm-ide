#include "Transition.hpp"

#include "State.hpp"
#include "ModelUtils.hpp"
#include "private/IModelVisitor.hpp"

namespace model {

Transition::Transition(QSharedPointer<State> source, QSharedPointer<State> target, const QString& event)
    : StateMachineEntity(StateMachineEntity::Type::Transition)
    , mSource(source)
    , mTarget(target)
    , mEvent(event) {}

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
        mSource = tOther->mSource;
        mTarget = tOther->mTarget;
        mEvent = tOther->mEvent;
        mTransitionCallback = tOther->mTransitionCallback;
        mConditionCallback = tOther->mConditionCallback;
        mExpectedConditionValue = tOther->mExpectedConditionValue;
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

const QString& Transition::transitionCallback() const {
    return mTransitionCallback.get();
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
    return {"event", "transitionCallback", "conditionCallback", cKeyExpectedConditionValue, cKeyTransitionType};
}

bool Transition::setProperty(const QString& key, const QVariant& value) {
    bool handled = true;

    if (key == "event") {
        setEvent(value.toString());
    } else if (key == "transitionCallback") {
        setTransitionCallback(value.toString());
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
    } else if (key == "transitionCallback") {
        return mTransitionCallback.get();
    } else if (key == "conditionCallback") {
        return mConditionCallback.get() ;
    } else if (key == cKeyExpectedConditionValue) {
        return mExpectedConditionValue;
    } else if (key == cKeyTransitionType) {
        return static_cast<int>(mTransitionType);
    }
    return StateMachineEntity::getProperty(key);
}

};  // namespace model
