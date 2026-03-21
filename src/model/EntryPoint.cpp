#include "EntryPoint.hpp"

#include "Transition.hpp"
#include "private/IModelVisitor.hpp"

namespace model {

EntryPoint::EntryPoint()
    : State("", StateType::ENTRYPOINT) {}

void EntryPoint::accept(class IModelVisitor* visitor) {
    if (visitor) {
        visitor->visitEntryPoint(this);
    }
}

QStringList EntryPoint::properties() const {
    return {};
}

void EntryPoint::addTransition(const QSharedPointer<Transition>& transition) {
    mTransitions.append(transition);
    registerNewChild(transition);
}

bool EntryPoint::addChild(const QSharedPointer<StateMachineEntity>& child) {
    bool res = false;

    if (child && (child->type() == StateMachineEntity::Type::Transition)) {
        addTransition(child.dynamicCast<Transition>());
        res = true;
    }

    return res;
}

void EntryPoint::deleteChild(const EntityID_t id) {
    deleteDirectChild(findChild(id));
}

void EntryPoint::deleteDirectChild(const QSharedPointer<StateMachineEntity>& child) {
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

bool EntryPoint::forEachChildElement(
    std::function<bool(QSharedPointer<StateMachineEntity>, QSharedPointer<StateMachineEntity>)> callback,
    const int depth,
    const bool postOrderTraversal) {
    Q_UNUSED(depth);
    bool processedAllChildren = true;

    for (QSharedPointer<Transition>& child : mTransitions) {
        processedAllChildren = callback(sharedFromThis(), child.dynamicCast<StateMachineEntity>());

        if (false == processedAllChildren) {
            break;
        }
    }

    return processedAllChildren;
}

};  // namespace model