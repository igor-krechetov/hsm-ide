#ifndef FINALSTATE_HPP
#define FINALSTATE_HPP

#include "State.hpp"

namespace model {

class FinalState : public State {
public:
    explicit FinalState(const QString& id);
    virtual ~FinalState() = default;

    // Getters
    const QString& onStateChangedCallback() const;

    // Setters
    void setOnStateChangedCallback(const QString& callback);

private:
    QString mOnStateChangedCallback;
};

};  // namespace model

#endif  // FINALSTATE_HPP
