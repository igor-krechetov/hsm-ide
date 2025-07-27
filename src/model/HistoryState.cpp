#include "HistoryState.hpp"

HistoryState::HistoryState(const std::string& id, HistoryState::HistoryType historyType)
    : State(id, State::Type::History)
    , mHistoryType(historyType) {}
