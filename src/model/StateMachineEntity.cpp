#include "StateMachineEntity.hpp"

namespace model {

StateMachineEntity::StateMachineEntity(Type type)
    : mElementType(type) {
    static EntityID_t sNextId = 1;
    mId = sNextId;
    ++sNextId;
}

StateMachineEntity::Type StateMachineEntity::type() const {
    return mElementType;
}

EntityID_t StateMachineEntity::id() const {
    return mId;
}

void StateMachineEntity::deleteChild(const EntityID_t id) {
    // do nothing
}

void StateMachineEntity::deleteDirectChild(const QSharedPointer<StateMachineEntity> child) {
    // do nothing
}

QSharedPointer<StateMachineEntity> StateMachineEntity::findParentState(const EntityID_t childId) {
    // do nothing
    return nullptr;
}

QSharedPointer<StateMachineEntity> StateMachineEntity::findChild(const EntityID_t id,
                                                                 const StateMachineEntity::Type type) const {
    // do nothing
    return nullptr;
}

void StateMachineEntity::registerNewChild(const QSharedPointer<StateMachineEntity>& child) {
    // Propagate subscription: connect child's modelEntityAdded to this
    QObject::connect(child.get(), &StateMachineEntity::childAdded, this, &StateMachineEntity::childAdded);
    QObject::connect(child.get(), &StateMachineEntity::childRemoved, this, &StateMachineEntity::childRemoved);
    QObject::connect(child.get(), &StateMachineEntity::modelDataChanged, this, &StateMachineEntity::modelDataChanged);

    emit childAdded(child.toWeakRef());
}

void StateMachineEntity::unregisterChild(const QSharedPointer<StateMachineEntity>& child) {
    emit childRemoved(child.toWeakRef());
}

};  // namespace model
