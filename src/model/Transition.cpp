#include "Transition.hpp"

#include "State.hpp"

namespace model {

Transition::Transition(QSharedPointer<State> source, QSharedPointer<State> target, const QString& event)
    : StateMachineEntity(StateMachineEntity::Type::Transition)
    , mSource(source)
    , mTarget(target)
    , mEvent(event) {}

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
    mSource = source;
}

void Transition::setTarget(QSharedPointer<State> target) {
    mTarget = target;
}

EntityID_t Transition::sourceId() const {
    auto ptr = mSource.lock();
    return (ptr ? ptr->id() : INVALID_MODEL_ID);
}

EntityID_t Transition::targetId() const {
    auto ptr = mTarget.lock();
    return (ptr ? ptr->id() : INVALID_MODEL_ID);
}

};  // namespace model
