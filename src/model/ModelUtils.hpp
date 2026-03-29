#ifndef MODELUTILS_HPP
#define MODELUTILS_HPP

#include <QSharedPointer>

#include "ModelTypes.hpp"
#include "State.hpp"
#include "StateMachineEntity.hpp"

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

};  // namespace model

#endif  // MODELUTILS_HPP