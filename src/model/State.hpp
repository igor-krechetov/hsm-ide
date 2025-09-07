#ifndef STATE_HPP
#define STATE_HPP

#include <QList>
#include <QSharedPointer>
#include <QString>

#include "StateMachineEntity.hpp"

namespace model {

class Transition;

class State : public StateMachineEntity {
public:
    enum class Type { Root, Initial, Regular, Final, History };

    explicit State(const QString& mName, const Type type = Type::Regular);
    virtual ~State() = default;

    Type stateType() const;

    const QString& name() const;
    void setName(const QString& name);

    void addChild(const QSharedPointer<StateMachineEntity>& child);
    void addChildState(const QSharedPointer<State>& child);
    void addTransition(const QSharedPointer<Transition>& child);
    const QList<QSharedPointer<StateMachineEntity>>& children() const;

    void deleteChild(const EntityID_t id);
    void deleteChild(const QSharedPointer<StateMachineEntity> child);

    QSharedPointer<State> findParentState(const EntityID_t childId);
    QSharedPointer<StateMachineEntity> findChild(const EntityID_t id,
                                                 const StateMachineEntity::Type type = StateMachineEntity::Type::Invalid) const;
    QSharedPointer<State> findState(const EntityID_t id) const;
    QSharedPointer<Transition> findTransition(const EntityID_t id) const;

protected:
    QString mName;
    Type mStateType;
    QList<QSharedPointer<StateMachineEntity>> mChildren;
};

};  // namespace model

#endif  // STATE_HPP
