#ifndef TRANSITION_HPP
#define TRANSITION_HPP

#include <QSharedPointer>
#include <string>

#include "StateMachineElement.hpp"

// Forward declaration to break circular dependency
class State;

class Transition : public StateMachineElement {
public:
    Transition(QSharedPointer<State> source, QSharedPointer<State> target, const std::string& event);
    virtual ~Transition() = default;

    QSharedPointer<State> source() const;
    QSharedPointer<State> target() const;
    const std::string& event() const;

private:
    // TODO: weak ptr?
    QSharedPointer<State> mSource;
    QSharedPointer<State> mTarget;
    std::string mEvent;
};

#endif  // TRANSITION_HPP
