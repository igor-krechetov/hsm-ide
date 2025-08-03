#include "Transition.hpp"

namespace model {

Transition::Transition(QSharedPointer<State> source, QSharedPointer<State> target, const QString& event)
    : StateMachineEntity(StateMachineEntity::Type::Transition)
    , mSource(source)
    , mTarget(target)
    , mEvent(event) {}

QSharedPointer<State> Transition::source() const {
    return mSource;
}

QSharedPointer<State> Transition::target() const {
    return mTarget;
}

const QString& Transition::event() const {
    return mEvent;
}

}; // namespace model
