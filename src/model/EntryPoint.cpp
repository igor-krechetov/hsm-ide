#include "EntryPoint.hpp"

#include "Transition.hpp"

namespace model {

EntryPoint::EntryPoint(const QString& id)
    : State(id, State::StateType::Regular) {}

void EntryPoint::addTransition(const QSharedPointer<Transition>& child) {
    mTransitions.append(child);
}

const QList<QSharedPointer<Transition>>& EntryPoint::transitions() const {
    return mTransitions;
}

};  // namespace model