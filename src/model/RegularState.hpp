#ifndef REGULARSTATE_HPP
#define REGULARSTATE_HPP

#include "State.hpp"
#include "Transition.hpp"

namespace model {

class RegularState : public State {
public:
    explicit RegularState(const QString& id);

    virtual ~RegularState() = default;

    // Getters
    const QString& onStateChangedCallback() const;
    const QString& onEnteringCallback() const;
    const QString& onExitingCallback() const;

    // Setters
    void setOnStateChangedCallback(const QString& callback);
    void setOnEnteringCallback(const QString& callback);
    void setOnExitingCallback(const QString& callback);

    void addChild(const QSharedPointer<StateMachineEntity>& child);
    void addChildState(const QSharedPointer<State>& child);
    void addTransition(const QSharedPointer<Transition>& child);
    const QList<QSharedPointer<StateMachineEntity>>& children() const;

    void deleteChild(const EntityID_t id);
    void deleteChild(const QSharedPointer<StateMachineEntity> child);

    QSharedPointer<RegularState> findParentState(const EntityID_t childId);
    QSharedPointer<StateMachineEntity> findChild(const EntityID_t id,
                                                 const StateMachineEntity::Type type = StateMachineEntity::Type::Invalid) const;
    QSharedPointer<State> findState(const EntityID_t id) const;
    QSharedPointer<RegularState> findRegularState(const EntityID_t id) const;
    QSharedPointer<Transition> findTransition(const EntityID_t id) const;

private:
    QString mOnStateChangedCallback;
    QString mOnEnteringCallback;
    QString mOnExitingCallback;

    QList<QSharedPointer<StateMachineEntity>> mChildren;
};

};  // namespace model

#endif  // REGULARSTATE_HPP
