#ifndef STATEMACHINEENTITY_HPP
#define STATEMACHINEENTITY_HPP

#include <QObject>
#include <QSharedPointer>
#include <vector>

#include "model/ModelTypes.hpp"

// TODO: move StateMachineElement::ID_t to a common header

namespace model {

class StateMachineEntity : public QObject, public QEnableSharedFromThis<StateMachineEntity> {
    Q_OBJECT
public:
    enum class Type { Invalid, State, Transition };  // namespace model

public:
    explicit StateMachineEntity(const Type type);
    virtual ~StateMachineEntity() = default;

    EntityID_t id() const;
    Type type() const;

    virtual void deleteChild(const EntityID_t id);
    virtual void deleteDirectChild(const QSharedPointer<StateMachineEntity> child);

    virtual QSharedPointer<StateMachineEntity> findParentState(const EntityID_t childId);
    virtual QSharedPointer<StateMachineEntity> findChild(
        const EntityID_t id,
        const StateMachineEntity::Type type = StateMachineEntity::Type::Invalid) const;

protected:
    void registerNewChild(const QSharedPointer<StateMachineEntity>& child);
    void unregisterChild(const QSharedPointer<StateMachineEntity>& child);

signals:
    void childAdded(QWeakPointer<StateMachineEntity> child);
    // TODO: use id instead?
    void childRemoved(QWeakPointer<StateMachineEntity> child);
    void modelDataChanged();

private:
    EntityID_t mId = 0;
    Type mElementType = Type::Invalid;
};

};  // namespace model

#endif  // STATEMACHINEENTITY_HPP
