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
    Transition(QSharedPointer<State> source, QSharedPointer<State> target, const QString& event);
    virtual ~Transition() = default;

    void accept(class IModelVisitor* visitor) override;

    QSharedPointer<State> source() const;
    QSharedPointer<State> target() const;

    void setSource(QSharedPointer<State> source);
    void setTarget(QSharedPointer<State> target);

    EntityID_t sourceId() const;
    EntityID_t targetId() const;

    // Getters
    const QString& event() const;
    TransitionType transitionType() const;
    const QString& transitionCallback() const;
    const QString& conditionCallback() const;
    bool expectedConditionValue() const;

    // Setters
    void setEvent(const QString& event);
    void setTransitionType(TransitionType type);
    void setTransitionCallback(const QString& callback);
    void setConditionCallback(const QString& callback);
    void setExpectedConditionValue(bool value);
    bool setProperty(const QString& key, const QVariant& value) override;
    QVariant getProperty(const QString& key) const override;

    QStringList properties() const override;

public:
    static constexpr char cKeyTransitionType[] = "transitionType";

private:
    TransitionType mTransitionType = TransitionType::EXTERNAL;
    QWeakPointer<State> mSource;
    QWeakPointer<State> mTarget;
    QString mEvent;
    QString mTransitionCallback;
    QString mConditionCallback;
    bool mExpectedConditionValue = false;
};

};  // namespace model

#endif  // TRANSITION_HPP
