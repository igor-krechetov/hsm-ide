#include "State.hpp"

#include <QDebug>

#include "Transition.hpp"

namespace model {

State::State(const QString& name, const StateType type)
    : StateMachineEntity(StateMachineEntity::Type::State)
    , mName(name)
    , mStateType(type) {}

// const QString& State::id() const {
//     return mId;
// }

State::StateType State::stateType() const {
    return mStateType;
}

const QString& State::name() const {
    return mName;
}

void State::setName(const QString& name) {
    mName = name;
}

};  // namespace model
