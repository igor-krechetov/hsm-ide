#include "HistoryState.hpp"

namespace model {

HistoryState::HistoryState(const QString& name, const HistoryType historyType)
    : State(name, StateType::HISTORY)
    , mHistoryType(historyType) {}

void HistoryState::setHistoryType(HistoryType historyType) {
    mHistoryType = historyType;
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

QStringList HistoryState::properties() const {
    return State::properties() + QStringList{cKeyHistoryType};
}

bool HistoryState::setProperty(const QString& key, const QVariant& value) {
    bool handled = true;

    if (key == cKeyHistoryType) {
        setHistoryType(historyTypeFromInt(value.toInt()));
    } else {
        handled = State::setProperty(key, value);
    }

    return handled;
}

QVariant HistoryState::getProperty(const QString& key) const {
    if (key == cKeyHistoryType) {
        return static_cast<int>(mHistoryType);
    }

    return State::getProperty(key);
}


};  // namespace model
