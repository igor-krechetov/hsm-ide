#ifndef TRANSITION_HPP
#define TRANSITION_HPP

#include <QSharedPointer>
#include <QWeakPointer>
#include <QString>

#include "StateMachineEntity.hpp"

namespace model {

// Forward declaration to break circular dependency
class State;

class Transition : public StateMachineEntity {
public:
    Transition(QSharedPointer<State> source, QSharedPointer<State> target, const QString& event);
    virtual ~Transition() = default;

    const QString& event() const;

    QSharedPointer<State> source() const;
    QSharedPointer<State> target() const;

    void setSource(QSharedPointer<State> source);
    void setTarget(QSharedPointer<State> target);

    EntityID_t sourceId() const;
    EntityID_t targetId() const;

private:
    // TODO: weak ptr?
    QWeakPointer<State> mSource;
    QWeakPointer<State> mTarget;
    QString mEvent;
};

}; // namespace model

#endif  // TRANSITION_HPP
