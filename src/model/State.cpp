#include "State.hpp"

#include <QDebug>

#include "Transition.hpp"

namespace model {

State::State(const QString& id, const Type type)
    : StateMachineEntity(StateMachineEntity::Type::State)
    , mName(id)
    , mStateType(type) {}

// const QString& State::id() const {
//     return mId;
// }

State::Type State::stateType() const {
    return mStateType;
}

const QString& State::name() const {
    return mName;
}

void State::setName(const QString& name) {
    mName = name;
}

void State::addChild(const QSharedPointer<StateMachineEntity>& child) {
    if (child) {
        mChildren.push_back(child);
    }
}

void State::addChildState(const QSharedPointer<State>& child) {
    qDebug() << Q_FUNC_INFO << "parent=" << id() << mName;
    addChild(qSharedPointerCast<StateMachineEntity>(child));
}

void State::addTransition(const QSharedPointer<Transition>& child) {
    qDebug() << Q_FUNC_INFO << "parent=" << id() << mName;
    addChild(qSharedPointerCast<StateMachineEntity>(child));
}

const QList<QSharedPointer<StateMachineEntity>>& State::children() const {
    return mChildren;
}

void State::deleteChild(const EntityID_t id) {
    deleteChild(findChild(id));
}

void State::deleteChild(const QSharedPointer<StateMachineEntity> child) {
    if (child) {
        mChildren.removeAll(child);
    }
}

QSharedPointer<State> State::findParentState(const EntityID_t childId) {
    QSharedPointer<State> res;

    if (INVALID_MODEL_ID != childId) {
        for (const auto& element : mChildren) {
            if (element->id() == childId) {
                res = sharedFromThis().dynamicCast<State>();
            } else if (element->type() == StateMachineEntity::Type::State) {
                res = element.dynamicCast<State>()->findParentState(childId);
            }

            if (res) {
                break;
            }
        }
    }

    return res;
}

QSharedPointer<StateMachineEntity> State::findChild(const EntityID_t id, const StateMachineEntity::Type type) const {
    QSharedPointer<StateMachineEntity> res;

    if (INVALID_MODEL_ID != id) {
        for (const auto& element : mChildren) {
            if ((element->id() == id) && (type == StateMachineEntity::Type::Invalid || element->type() == type)) {
                res = element;
            } else if (element->type() == StateMachineEntity::Type::State) {
                res = element.dynamicCast<State>()->findChild(id, type);
            }

            if (res) {
                break;
            }
        }
    }

    return res;
}

QSharedPointer<State> State::findState(const EntityID_t id) const {
    QSharedPointer<State> res;
    QSharedPointer<StateMachineEntity> childPtr = findChild(id, StateMachineEntity::Type::State);

    if (childPtr) {
        res = childPtr.dynamicCast<State>();
    }

    return res;
}

QSharedPointer<Transition> State::findTransition(const EntityID_t id) const {
    QSharedPointer<Transition> res;
    QSharedPointer<StateMachineEntity> childPtr = findChild(id, StateMachineEntity::Type::Transition);

    if (childPtr) {
        res = childPtr.dynamicCast<Transition>();
    }

    return res;
}

};  // namespace model
