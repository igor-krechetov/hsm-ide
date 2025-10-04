#ifndef HISTORYSTATE_HPP
#define HISTORYSTATE_HPP

#include "State.hpp"

namespace model {

class HistoryState : public State {
public:
    explicit HistoryState(const QString& name, const HistoryType historyType);
    virtual ~HistoryState() = default;

    // Getters
    HistoryType historyType() const {
        return mHistoryType;
    }

    // Setters
    void setHistoryType(HistoryType historyType);

    bool setProperty(const QString& key, const QVariant& value) override;
    QVariant getProperty(const QString& key) const override;
    QStringList properties() const override;

public:
    static constexpr char cKeyHistoryType[] = "historyType";

private:
    HistoryType mHistoryType;
};

};  // namespace model

#endif  // HISTORYSTATE_HPP
