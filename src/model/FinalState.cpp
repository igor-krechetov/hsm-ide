#include "FinalState.hpp"

namespace model {

FinalState::FinalState(const QString& id)
    : State(id, State::Type::Final) {}

} // namespace model