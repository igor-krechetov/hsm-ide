#ifndef TRANSITION_HPP
#define TRANSITION_HPP

#include <QSharedPointer>
#include <QString>
#include <QWeakPointer>

#include "StateMachineEntity.hpp"
#include "actions/IModelAction.hpp"
#include "private/IdentifierString.hpp"

namespace model {
class State;

class Transition : public StateMachineEntity {
public:
    Transition(QSharedPointer<State> source, QSharedPointer<State> target, const QString& event);
    virtual ~Transition();
    Transition& operator=(const Transition& other);

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
    QSharedPointer<IModelAction> transitionAction() const;
    const QString& conditionCallback() const;
    bool expectedConditionValue() const;

    // Setters
    void setEvent(const QString& event);
    void setTransitionType(TransitionType type);
    void setTransitionAction(const QSharedPointer<IModelAction>& action);
    bool hasTransitionAction() const;
    void setTransitionAction(const QString& actionData);
    void setConditionCallback(const QString& callback);
    void setExpectedConditionValue(bool value);
    bool setProperty(const QString& key, const QVariant& value) override;
    QVariant getProperty(const QString& key) const override;

    void copyEntityData(const StateMachineEntity& other) override;

    QStringList properties() const override;

public:
    static constexpr char cKeyTransitionType[] = "transitionType";
    static constexpr char cKeyExpectedConditionValue[] = "expectedConditionValue";

private:
    TransitionType mTransitionType = TransitionType::EXTERNAL;
    QWeakPointer<State> mSource;
    QWeakPointer<State> mTarget;
    priv::IdentifierString mEvent;
    QSharedPointer<IModelAction> mTransitionAction;
    priv::IdentifierString mConditionCallback;
    bool mExpectedConditionValue = false;
};

};  // namespace model

#endif  // TRANSITION_HPP
