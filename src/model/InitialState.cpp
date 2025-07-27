#include "InitialState.hpp"

InitialState::InitialState(const std::string& id)
    : State(id, State::Type::Initial) {}
