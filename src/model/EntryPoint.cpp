#include "EntryPoint.hpp"

#include "Transition.hpp"

namespace model {

EntryPoint::EntryPoint(const QString& name)
    : State(name, StateType::ENTRYPOINT) {}

void EntryPoint::addTransition(const QSharedPointer<Transition>& transition) {
    mTransitions.append(transition);
    registerNewChild(transition);
}

void EntryPoint::deleteChild(const EntityID_t id) {
    deleteDirectChild(findChild(id));
}

void EntryPoint::deleteDirectChild(const QSharedPointer<StateMachineEntity> child) {
    if (child) {
        mTransitions.removeAll(child.dynamicCast<Transition>());
        unregisterChild(child);
    }
}

QSharedPointer<StateMachineEntity> EntryPoint::findParentState(const EntityID_t childId) {
    QSharedPointer<StateMachineEntity> res;

    if (INVALID_MODEL_ID != childId) {
        for (const auto& transition : mTransitions) {
            if (transition->id() == childId) {
                res = sharedFromThis();
                break;
            }
        }
    }

    return res;
}

QSharedPointer<StateMachineEntity> EntryPoint::findChild(const EntityID_t id, const StateMachineEntity::Type type) const {
    QSharedPointer<StateMachineEntity> res;

    if (INVALID_MODEL_ID != id) {
        for (const auto& transition : mTransitions) {
            if ((transition->id() == id) && (type == StateMachineEntity::Type::Invalid || transition->type() == type)) {
                res = transition;
                break;
            }
        }
    }

    return res;
}

};  // namespace model