#ifndef TRANSITION_HPP
#define TRANSITION_HPP

#include <QSharedPointer>
#include <QString>
#include <QWeakPointer>

#include "StateMachineEntity.hpp"

namespace model {
class State;

class Transition : public StateMachineEntity {
public:
    enum class Type { EXTERNAL, INTERNAL };

public:
    Transition(QSharedPointer<State> source, QSharedPointer<State> target, const QString& event);
    virtual ~Transition() = default;

    QSharedPointer<State> source() const;
    QSharedPointer<State> target() const;

    void setSource(QSharedPointer<State> source);
    void setTarget(QSharedPointer<State> target);

    EntityID_t sourceId() const;
    EntityID_t targetId() const;

    // Getters
    const QString& event() const;
    Type transitionType() const;
    const QString& conditionCallback() const;
    bool expectedConditionValue() const;

    // Setters
    void setEvent(const QString& event);
    void setTransitionType(Type type);
    void setConditionCallback(const QString& callback);
    void setExpectedConditionValue(bool value);

private:
    Type mTransitionType = Type::EXTERNAL;
    QWeakPointer<State> mSource;
    QWeakPointer<State> mTarget;
    QString mEvent;
    QString mConditionCallback;
    bool mExpectedConditionValue = false;
};

};  // namespace model

#endif  // TRANSITION_HPP
