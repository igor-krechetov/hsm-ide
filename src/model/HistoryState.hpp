#ifndef HISTORYSTATE_HPP
#define HISTORYSTATE_HPP

#include "State.hpp"

class HistoryState : public State {
public:
    enum class HistoryType { Shallow, Deep };

    explicit HistoryState(const std::string& id, const HistoryType historyType);

    HistoryType historyType() const {
        return mHistoryType;
    }

    // Initial states can't have children in SCXML
    void addChildState(std::shared_ptr<State> child) = delete;

private:
    HistoryType mHistoryType;
};

#endif  // HISTORYSTATE_HPP
