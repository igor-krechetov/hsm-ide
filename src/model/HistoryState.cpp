#include "HistoryState.hpp"

namespace model {

HistoryState::HistoryState(const QString& name, const HistoryState::HistoryType historyType)
    : State(name, State::StateType::History)
    , mHistoryType(historyType) {}

void HistoryState::setHistoryType(HistoryState::HistoryType historyType) {
    mHistoryType = historyType;
}

};  // namespace model
