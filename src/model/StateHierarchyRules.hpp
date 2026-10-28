#ifndef STATEHIERARCHYRULES_HPP
#define STATEHIERARCHYRULES_HPP

#include <QSharedPointer>

#include "elements/State.hpp"
#include "elements/StateMachineEntity.hpp"
namespace model {

class StateHierarchyRules {
public:
    static bool canBeTopLevel(const StateType elementType);
    static bool canStateBeChildOf(const StateType parentType, const StateType childType);
    static bool canTransitionBeChildOf(const StateType parentType);
    static bool canAddEntityToParent(const QSharedPointer<StateMachineEntity>& parent,
                                     const QSharedPointer<StateMachineEntity>& child);

    // Returns true when a transition between the two given endpoints is structurally valid.
    //
    // A transition is valid only when its source and target live under the SAME parent
    // (i.e. they are siblings on the same hierarchy level). A transition that crosses
    // between sub-elements of different parents, or jumps between levels, is invalid.
    //
    // History states are the sole exception and invert the rule: a transition INTO a
    // history state is expected to come from a higher level (a different parent), so for a
    // history target the transition is valid only when the endpoints do NOT share a parent.
    // A transition to a history state from within its own parent is therefore invalid.
    //
    // sourceParent / targetParent are the direct parent entities of the transition's source
    // and target elements (the model root for top-level elements). Identity is compared by
    // the underlying entity, so two different QSharedPointer instances wrapping the same
    // parent count as the same parent. A null parent matches only another null parent.
    // The target entity is inspected here (cast to a State) to detect a history target, so
    // callers don't have to resolve the type themselves.
    static bool isTransitionValid(const QSharedPointer<StateMachineEntity>& sourceParent,
                                  const QSharedPointer<StateMachineEntity>& targetParent,
                                  const QSharedPointer<StateMachineEntity>& target);
};

};  // namespace model

#endif  // STATEHIERARCHYRULES_HPP
