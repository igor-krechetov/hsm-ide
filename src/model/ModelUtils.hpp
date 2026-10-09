#ifndef MODELUTILS_HPP
#define MODELUTILS_HPP

#include <QSharedPointer>

#include "ModelTypes.hpp"
#include "elements/State.hpp"
#include "elements/StateMachineEntity.hpp"
#include "private/EntityIdGenerator.hpp"

namespace model {

template <typename T>
QSharedPointer<T> hsmDynamicCast(QSharedPointer<StateMachineEntity> entity, const StateType type) {
    QSharedPointer<T> ptr;

    if (entity && (entity->type() == StateMachineEntity::Type::State)) {
        QSharedPointer<State> ptrState = entity.dynamicCast<State>();

        if (ptrState && (ptrState->stateType() == type)) {
            ptr = ptrState.dynamicCast<T>();
        }
    }

    return ptr;
}

template <typename T>
QSharedPointer<T> hsmDynamicCast(QSharedPointer<StateMachineEntity> entity) {
    QSharedPointer<T> ptr;

    if (entity && (entity->type() == StateMachineEntity::Type::Transition)) {
        ptr = entity.dynamicCast<T>();
    }

    return ptr;
}

// Normalize literal for usage in code generation
QString sanitiseIdentifier(const QString& input);

// Reassign a fresh id from the given id generator to every entity in the subtree
// rooted at `root` (the root state, its descendant states, and all transitions), so an
// imported/pasted subtree can be merged into a model without colliding with existing
// ids or with a previous paste. Transitions hold shared pointers to their source/target
// states, so re-iding the states automatically updates each transition's sourceId()/
// targetId(); only the transition's own id is minted here. No-op for a null root.
void reassignImportedIds(const QSharedPointer<State>& root, EntityIdGenerator& idGenerator);

};  // namespace model

#endif  // MODELUTILS_HPP