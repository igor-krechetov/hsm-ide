#include "RegularState.hpp"

#include <QDebug>

namespace model {

RegularState::RegularState(const QString& name)
    : State(name, State::StateType::Regular) {}

// Getters
const QString& RegularState::onStateChangedCallback() const {
    return mOnStateChangedCallback;
}

const QString& RegularState::onEnteringCallback() const {
    return mOnEnteringCallback;
}

const QString& RegularState::onExitingCallback() const {
    return mOnExitingCallback;
}

// Setters
void RegularState::setOnStateChangedCallback(const QString& callback) {
    mOnStateChangedCallback = callback;
}

void RegularState::setOnEnteringCallback(const QString& callback) {
    mOnEnteringCallback = callback;
}

void RegularState::setOnExitingCallback(const QString& callback) {
    mOnExitingCallback = callback;
}

void RegularState::addChild(const QSharedPointer<StateMachineEntity>& child) {
    if (child) {
        mChildren.push_back(child);
    }
}

void RegularState::addChildState(const QSharedPointer<State>& child) {
    qDebug() << Q_FUNC_INFO << "parent=" << id() << mName;
    addChild(qSharedPointerCast<StateMachineEntity>(child));
}

void RegularState::addTransition(const QSharedPointer<Transition>& child) {
    qDebug() << Q_FUNC_INFO << "parent=" << id() << mName;
    addChild(qSharedPointerCast<StateMachineEntity>(child));
}

const QList<QSharedPointer<StateMachineEntity>>& RegularState::children() const {
    return mChildren;
}

void RegularState::deleteChild(const EntityID_t id) {
    deleteChild(findChild(id));
}

void RegularState::deleteChild(const QSharedPointer<StateMachineEntity> child) {
    if (child) {
        mChildren.removeAll(child);
    }
}

QSharedPointer<RegularState> RegularState::findParentState(const EntityID_t childId) {
    QSharedPointer<RegularState> res;

    if (INVALID_MODEL_ID != childId) {
        for (const auto& element : mChildren) {
            if (element->id() == childId) {
                res = sharedFromThis().dynamicCast<RegularState>();
            } else if (element->type() == StateMachineEntity::Type::State) {
                switch(element.dynamicCast<State>()->stateType()) {
                    case State::StateType::Regular: {
                        auto ptr = element.dynamicCast<RegularState>();
                        if (nullptr != ptr) {
                            res = ptr->findParentState(childId);
                        } else {
                            // TODO: error
                        }
                        break;
                    }
                    case State::StateType::EntryPoint:
                        // TODO: implement
                        break;
                    default:
                        // can't have children
                        break;
                }
            }

            if (res) {
                break;
            }
        }
    }

    return res;
}

QSharedPointer<StateMachineEntity> RegularState::findChild(const EntityID_t id, const StateMachineEntity::Type type) const {
    QSharedPointer<StateMachineEntity> res;

    if (INVALID_MODEL_ID != id) {
        for (const auto& element : mChildren) {
            if ((element->id() == id) && (type == StateMachineEntity::Type::Invalid || element->type() == type)) {
                res = element;
            } else if (element->type() == StateMachineEntity::Type::State) {
                switch(element.dynamicCast<State>()->stateType()) {
                    case State::StateType::Regular:
                        // TODO: check dynamicCast result
                        res = element.dynamicCast<RegularState>()->findChild(id, type);
                        break;
                    case State::StateType::EntryPoint:
                        // TODO:
                        break;
                    default:
                        // can't have children
                        break;
                }
            }

            if (res) {
                break;
            }
        }
    }

    return res;
}

QSharedPointer<State> RegularState::findState(const EntityID_t id) const {
    QSharedPointer<State> res;
    QSharedPointer<StateMachineEntity> childPtr = findChild(id, StateMachineEntity::Type::State);

    if (childPtr) {
        res = childPtr.dynamicCast<State>();
    }

    return res;
}

QSharedPointer<RegularState> RegularState::findRegularState(const EntityID_t id) const {
    QSharedPointer<RegularState> res;
    QSharedPointer<StateMachineEntity> childPtr = findChild(id, StateMachineEntity::Type::State);

    if (childPtr) {
        res = childPtr.dynamicCast<RegularState>();
    }

    return res;
}

QSharedPointer<Transition> RegularState::findTransition(const EntityID_t id) const {
    QSharedPointer<Transition> res;
    QSharedPointer<StateMachineEntity> childPtr = findChild(id, StateMachineEntity::Type::Transition);

    if (childPtr) {
        res = childPtr.dynamicCast<Transition>();
    }

    return res;
}

};  // namespace model
