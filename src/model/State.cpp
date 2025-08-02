#include "State.hpp"
#include "Transition.hpp"

namespace model {

State::State(const QString& id, const Type type)
    : StateMachineEntity(StateMachineEntity::Type::State)
    , mName(id)
    , mStateType(type) {}

// const QString& State::id() const {
//     return mId;
// }

State::Type State::stateType() const {
    return mStateType;
}

void State::addChildState(QSharedPointer<State> child) {
    mChildren.push_back(qSharedPointerCast<StateMachineEntity>(child));
}

void State::addTransition(QSharedPointer<Transition> child) {
    mChildren.push_back(qSharedPointerCast<StateMachineEntity>(child));
}

const std::vector<QSharedPointer<StateMachineEntity>>& State::children() const {
    return mChildren;
}

} // namespace model
