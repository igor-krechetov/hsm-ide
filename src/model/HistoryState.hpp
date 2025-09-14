#ifndef HISTORYSTATE_HPP
#define HISTORYSTATE_HPP

#include "State.hpp"

namespace model {

class HistoryState : public State {
public:
    enum class HistoryType { Shallow, Deep };

    explicit HistoryState(const QString& name, const HistoryType historyType);
    virtual ~HistoryState() = default;

    // Getters
    HistoryType historyType() const {
        return mHistoryType;
    }

    // Setters
    void setHistoryType(HistoryType historyType);

private:
    HistoryType mHistoryType;
};

};  // namespace model

#endif  // HISTORYSTATE_HPP
