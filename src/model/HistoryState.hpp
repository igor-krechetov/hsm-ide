#ifndef HISTORYSTATE_HPP
#define HISTORYSTATE_HPP

#include "State.hpp"

namespace model {

class HistoryState : public State {
public:
    enum class HistoryType { Shallow, Deep };

    explicit HistoryState(const QString& id, const HistoryType historyType);

    HistoryType historyType() const {
        return mHistoryType;
    }

    // Initial states can't have children in SCXML
    void addChildState(std::shared_ptr<State> child) = delete;

private:
    HistoryType mHistoryType;
};

} // namespace model

#endif  // HISTORYSTATE_HPP
