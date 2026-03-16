#ifndef STATEHIERARCHYRULES_HPP
#define STATEHIERARCHYRULES_HPP

#include <QSharedPointer>

#include "State.hpp"
#include "StateMachineEntity.hpp"

namespace model {

class StateHierarchyRules {
public:
    static bool canBeTopLevel(const StateType childType);
    static bool canStateBeChildOf(const StateType parentType, const StateType childType);
    static bool canTransitionBeChildOf(const StateType parentType);
    static bool canAddEntityToParent(const QSharedPointer<StateMachineEntity>& parent,
                                     const QSharedPointer<StateMachineEntity>& child);
};

};  // namespace model

#endif  // STATEHIERARCHYRULES_HPP
