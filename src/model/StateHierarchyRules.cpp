#include "StateHierarchyRules.hpp"

namespace model {

bool StateHierarchyRules::canBeTopLevel(const StateType childType) {
    bool allowed = false;

    switch (childType) {
        case StateType::INITIAL:
        case StateType::FINAL:
        case StateType::REGULAR:
        case StateType::INCLUDE:
            allowed = true;
            break;
        default:
            allowed = false;
            break;
    }

    return allowed;
}

bool StateHierarchyRules::canStateBeChildOf(const StateType parentType, const StateType childType) {
    bool allowed = false;

    if (parentType == StateType::MODEL_ROOT) {
        allowed = canBeTopLevel(childType);
    } else if (parentType == StateType::REGULAR) {
        switch (childType) {
            case StateType::REGULAR:
            case StateType::INCLUDE:
            case StateType::INITIAL:
            case StateType::ENTRYPOINT:
            case StateType::EXITPOINT:
            case StateType::HISTORY:
                allowed = true;
                break;
            default:
                allowed = false;
                break;
        }
    }

    return allowed;
}

bool StateHierarchyRules::canTransitionBeChildOf(const StateType parentType) {
    bool allowed = false;

    switch (parentType) {
        case StateType::REGULAR:
        case StateType::INCLUDE:
        case StateType::INITIAL:
        case StateType::HISTORY:
            allowed = true;
            break;
        default:
            allowed = false;
            break;
    }

    return allowed;
}

bool StateHierarchyRules::canAddEntityToParent(const QSharedPointer<StateMachineEntity>& parent,
                                               const QSharedPointer<StateMachineEntity>& child) {
    bool allowed = false;

    if (parent && child && (parent->type() == StateMachineEntity::Type::State)) {
        const QSharedPointer<State> parentState = parent.dynamicCast<State>();

        if (child->type() == StateMachineEntity::Type::State) {
            const QSharedPointer<State> childState = child.dynamicCast<State>();

            if (parentState && childState) {
                allowed = canStateBeChildOf(parentState->stateType(), childState->stateType());
            }
        } else if (child->type() == StateMachineEntity::Type::Transition) {
            if (parentState) {
                allowed = canTransitionBeChildOf(parentState->stateType());
            }
        }
    }

    return allowed;
}

};  // namespace model
