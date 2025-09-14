#include "EntryPoint.hpp"

#include "Transition.hpp"

namespace model {

EntryPoint::EntryPoint(const QString& name)
    : State(name, State::StateType::EntryPoint) {}

void EntryPoint::addTransition(const QSharedPointer<Transition>& transition) {
    mTransitions.append(transition);
}

const QList<QSharedPointer<Transition>>& EntryPoint::transitions() const {
    return mTransitions;
}

};  // namespace model