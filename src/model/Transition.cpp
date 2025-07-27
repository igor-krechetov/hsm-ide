#include "Transition.hpp"

Transition::Transition(QSharedPointer<State> source, QSharedPointer<State> target, const std::string& event)
    : StateMachineElement(Type::Transition)
    , mSource(source)
    , mTarget(target)
    , mEvent(event) {}

QSharedPointer<State> Transition::source() const {
    return mSource;
}

QSharedPointer<State> Transition::target() const {
    return mTarget;
}

const std::string& Transition::event() const {
    return mEvent;
}
