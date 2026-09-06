#include "StateHierarchyRules.hpp"

#include "elements/ModelRootState.hpp"

namespace model {

bool StateHierarchyRules::canBeTopLevel(const StateType elementType) {
    bool allowed = false;

    switch (elementType) {
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
            case StateType::ENTRYPOINT:
            case StateType::EXITPOINT:
            case StateType::HISTORY:
                allowed = true;
                break;
            case StateType::INITIAL:
            case StateType::FINAL:
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
        case StateType::ENTRYPOINT:
        case StateType::HISTORY:
            allowed = true;
            break;
        case StateType::EXITPOINT:
        case StateType::FINAL:
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

            qDebug() << "---- parentType" << (int)parentState->stateType() << " - childType" << (int)childState->stateType();

            if (parentState && childState) {
                allowed = canStateBeChildOf(parentState->stateType(), childState->stateType());

                // REQ-103f6: only one initial state is allowed at the model root.
                if (allowed && (parentState->stateType() == StateType::MODEL_ROOT) &&
                    (childState->stateType() == StateType::INITIAL)) {
                    const QSharedPointer<ModelRootState> rootState = parentState.dynamicCast<ModelRootState>();

                    if (rootState && rootState->hasInitialState()) {
                        allowed = false;
                    }
                }
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
