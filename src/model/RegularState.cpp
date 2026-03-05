#include "RegularState.hpp"

#include <QDebug>

#include "private/IModelVisitor.hpp"

namespace model {

RegularState::RegularState(const QString& name)
    : State(name, StateType::REGULAR) {}

RegularState& RegularState::operator=(const RegularState& other) {
    if (this != &other) {
        State::operator=(other);
        mOnStateChangedCallback = other.mOnStateChangedCallback;
        mOnEnteringCallback = other.mOnEnteringCallback;
        mOnExitingCallback = other.mOnExitingCallback;
    }

    return *this;
}

void RegularState::accept(class IModelVisitor* visitor) {
    if (visitor) {
        visitor->visitRegularState(this);
    }
}

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

bool RegularState::addChild(const QSharedPointer<StateMachineEntity>& child) {
    bool res = false;

    if (child) {
        mChildren.push_back(child);
        registerNewChild(child);
        res = true;
    }

    return res;
}

void RegularState::addChildState(const QSharedPointer<State>& child) {
    qDebug() << Q_FUNC_INFO << "parent=" << id() << mName;
    addChild(qSharedPointerCast<StateMachineEntity>(child));
}

void RegularState::addTransition(const QSharedPointer<Transition>& child) {
    qDebug() << Q_FUNC_INFO << "parent=" << id() << mName;
    addChild(qSharedPointerCast<StateMachineEntity>(child));
}

const QList<QSharedPointer<StateMachineEntity>>& RegularState::childrenEntities() const {
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

void RegularState::deleteDirectChild(const QSharedPointer<StateMachineEntity>& child) {
    if (child) {
        child->forEachChildElement(
            [this](QSharedPointer<StateMachineEntity> parent, QSharedPointer<StateMachineEntity> element) {
                unregisterChild(element);
                return true;
            });
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

QSharedPointer<State> RegularState::findChildStateByName(const QString& name) {
    QSharedPointer<State> res;

    forEachChildElement([&](QSharedPointer<StateMachineEntity> parent, QSharedPointer<StateMachineEntity> element) {
        bool continueSearch = true;

        if (element->type() == StateMachineEntity::Type::State) {
            QSharedPointer<State> statePtr = element.dynamicCast<State>();
            if (statePtr && statePtr->name() == name) {
                res = statePtr;
                continueSearch = false;
            }
        }

        return continueSearch;
    });

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
    return State::properties() + QStringList{"onStateChangedCallback", "onEnteringCallback", "onExitingCallback"};
}

bool RegularState::setProperty(const QString& key, const QVariant& value) {
    bool handled = true;

    if (key == "onStateChangedCallback") {
        setOnStateChangedCallback(value.toString());
    } else if (key == "onEnteringCallback") {
        setOnEnteringCallback(value.toString());
    } else if (key == "onExitingCallback") {
        setOnExitingCallback(value.toString());
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
    }

    return State::getProperty(key);
}

bool RegularState::forEachChildElement(
    std::function<bool(QSharedPointer<StateMachineEntity>, QSharedPointer<StateMachineEntity>)> callback,
    const int depth,
    const bool postOrderTraversal) {
    bool processedAllChildren = true;

    for (QSharedPointer<StateMachineEntity>& child : mChildren) {
        if (false == postOrderTraversal) {
            if ((processedAllChildren = callback(sharedFromThis(), child)) == false) {
                break;
            }
        }

        if (depth == DEPTH_INFINITE || depth > 1) {
            processedAllChildren =
                child->forEachChildElement(callback, (depth != DEPTH_INFINITE ? depth - 1 : depth), postOrderTraversal);

            if (false == processedAllChildren) {
                break;
            }
        }

        if (true == postOrderTraversal) {
            if ((processedAllChildren = callback(sharedFromThis(), child)) == false) {
                break;
            }
        }
    }

    return processedAllChildren;
}

};  // namespace model
