#include "RegularState.hpp"

#include <QDebug>

namespace model {

RegularState::RegularState(const QString& name)
    : State(name, StateType::Regular) {}

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
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void RegularState::setOnEnteringCallback(const QString& callback) {
    mOnEnteringCallback = callback;
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void RegularState::setOnExitingCallback(const QString& callback) {
    mOnExitingCallback = callback;
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void RegularState::addChild(const QSharedPointer<StateMachineEntity>& child) {
    if (child) {
        mChildren.push_back(child);
        registerNewChild(child);
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
    QSharedPointer<StateMachineEntity> parent = findParentState(id);

    if (parent) {
        if (parent.get() == this) {
            deleteDirectChild(findChild(id));
        } else {
            parent->deleteChild(id);
        }
    }
}

void RegularState::deleteDirectChild(const QSharedPointer<StateMachineEntity> child) {
    if (child) {
        mChildren.removeAll(child);
        unregisterChild(child);
    }
}

QSharedPointer<StateMachineEntity> RegularState::findParentState(const EntityID_t childId) {
    QSharedPointer<StateMachineEntity> res;

    if (INVALID_MODEL_ID != childId) {
        for (const auto& element : mChildren) {
            if (element->id() == childId) {
                res = sharedFromThis();
                break;
            }
        }

        if (!res) {
            for (const auto& element : mChildren) {
                res = element->findParentState(childId);

                if (res) {
                    break;
                }
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
                break;
            }
        }

        if (!res) {
            for (const auto& element : mChildren) {
                res = element->findChild(id, type);

                if (res) {
                    break;
                }
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

QStringList RegularState::properties() const {
    return {"name", "onStateChangedCallback", "onEnteringCallback", "onExitingCallback"};
}

bool RegularState::setProperty(const QString& key, const QVariant& value) {
    bool handled = true;

    if (key == "onStateChangedCallback") {
        setOnStateChangedCallback(value.toString());
    } else if (key == "onEnteringCallback") {
        setOnEnteringCallback(value.toString());
    } else if (key == "onExitingCallback") {
        setOnExitingCallback(value.toString());
    } else if (key == "name") {
        setName(value.toString());
    } else {
        handled = State::setProperty(key, value);
    }

    return handled;
}

QVariant RegularState::getProperty(const QString& key) const {
    if (key == "onStateChangedCallback") {
        return mOnStateChangedCallback;
    } else if (key == "onEnteringCallback") {
        return mOnEnteringCallback;
    } else if (key == "onExitingCallback") {
        return mOnExitingCallback;
    } else if (key == "name") {
        return mName;
    }
    return State::getProperty(key);
}

};  // namespace model
