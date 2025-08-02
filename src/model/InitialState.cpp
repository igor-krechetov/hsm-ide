#include "InitialState.hpp"

namespace model {

InitialState::InitialState(const QString& id)
    : State(id, State::Type::Initial) {}

} // namespace model