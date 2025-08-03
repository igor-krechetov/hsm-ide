#include "HistoryState.hpp"

namespace model {

HistoryState::HistoryState(const QString& id, const HistoryState::HistoryType historyType)
    : State(id, State::Type::History)
    , mHistoryType(historyType) {}

}; // namespace model