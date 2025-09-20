#include "HistoryState.hpp"

namespace model {

HistoryState::HistoryState(const QString& id, const HistoryState::HistoryType historyType)
    : State(id, State::StateType::History)
    , mHistoryType(historyType) {}

void HistoryState::setHistoryType(HistoryState::HistoryType historyType) {
    mHistoryType = historyType;
}

};  // namespace model
