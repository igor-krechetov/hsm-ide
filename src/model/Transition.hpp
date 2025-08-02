#ifndef TRANSITION_HPP
#define TRANSITION_HPP

#include <QSharedPointer>
#include <QString>

#include "StateMachineEntity.hpp"

// Forward declaration to break circular dependency
class State;

namespace model {

class Transition : public StateMachineEntity {
public:
    Transition(QSharedPointer<State> source, QSharedPointer<State> target, const QString& event);
    virtual ~Transition() = default;

    QSharedPointer<State> source() const;
    QSharedPointer<State> target() const;
    const QString& event() const;

private:
    // TODO: weak ptr?
    QSharedPointer<State> mSource;
    QSharedPointer<State> mTarget;
    QString mEvent;
};

} // namespace model

#endif  // TRANSITION_HPP
