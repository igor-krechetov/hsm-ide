#include "RegularState.hpp"

#include <QDebug>

#include "actions/ModelActionFactory.hpp"
#include "private/IModelVisitor.hpp"

namespace model {

RegularState::RegularState(const QString& name)
    : State(name, StateType::REGULAR) {}

RegularState::~RegularState() {
    qDebug() << "DELETE RegularState:" << this;
    deleteAllChildren();
}

RegularState& RegularState::operator=(const RegularState& other) {
    if (this != &other) {
        State::operator=(other);
        mOnStateChangedAction = createModelActionFromData(other.onStateChangedCallback(), ModelAction::CALLBACK);
        mOnEnteringAction = createModelActionFromData(other.onEnteringCallback(), ModelAction::CALLBACK);
        mOnExitingAction = createModelActionFromData(other.onExitingCallback(), ModelAction::CALLBACK);
    }

    return *this;
}

void RegularState::accept(class IModelVisitor* visitor) {
    if (visitor) {
        visitor->visitRegularState(this);
    }
}

QSharedPointer<IModelAction> RegularState::onStateChangedAction() const {
    return mOnStateChangedAction;
}

QSharedPointer<IModelAction> RegularState::onEnteringAction() const {
    return mOnEnteringAction;
}

QSharedPointer<IModelAction> RegularState::onExitingAction() const {
    return mOnExitingAction;
}

void RegularState::setOnStateChangedAction(const QSharedPointer<IModelAction>& action) {
    mOnStateChangedAction = action;

    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void RegularState::setOnEnteringAction(const QSharedPointer<IModelAction>& action) {
    mOnEnteringAction = action;

    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void RegularState::setOnExitingAction(const QSharedPointer<IModelAction>& action) {
    mOnExitingAction = action;

    emit modelDataChanged(sharedFromThis().toWeakRef());
}

QString RegularState::onStateChangedCallback() const {
    return (mOnStateChangedAction ? mOnStateChangedAction->serialize() : QString());
}

QString RegularState::onEnteringCallback() const {
    return (mOnEnteringAction ? mOnEnteringAction->serialize() : QString());
}

QString RegularState::onExitingCallback() const {
    return (mOnExitingAction ? mOnExitingAction->serialize() : QString());
}

void RegularState::setOnStateChangedCallback(const QString& callback) {
    setOnStateChangedAction(callback.isEmpty() ? QSharedPointer<IModelAction>()
                                               : createModelActionFromData(callback, ModelAction::CALLBACK));
}

void RegularState::setOnEnteringCallback(const QString& callback) {
    setOnEnteringAction(callback.isEmpty() ? QSharedPointer<IModelAction>()
                                           : createModelActionFromData(callback, ModelAction::CALLBACK));
}

void RegularState::setOnExitingCallback(const QString& callback) {
    setOnExitingAction(callback.isEmpty() ? QSharedPointer<IModelAction>()
                                          : createModelActionFromData(callback, ModelAction::CALLBACK));
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
        // child->forEachChildElement(
        //     [this](QSharedPointer<StateMachineEntity> parent, QSharedPointer<StateMachineEntity> element) {
        //         unregisterChild(element);
        //         return true;
        //     });
        mChildren.removeAll(child);
        unregisterChild(child);
    }
}

void RegularState::deleteAllChildren() {
    for (const auto& child : mChildren) {
        // child->forEachChildElement(
        //     [this](QSharedPointer<StateMachineEntity> parent, QSharedPointer<StateMachineEntity> element) {
        //         unregisterChild(element);
        //         return true;
        //     });
        unregisterChild(child);
    }

    mChildren.clear();
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
        return (mOnStateChangedAction ? QVariant(mOnStateChangedAction->serialize()) : QVariant(QString()));
    } else if (key == "onEnteringCallback") {
        return (mOnEnteringAction ? QVariant(mOnEnteringAction->serialize()) : QVariant(QString()));
    } else if (key == "onExitingCallback") {
        return (mOnExitingAction ? QVariant(mOnExitingAction->serialize()) : QVariant(QString()));
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

void RegularState::copyEntityData(const StateMachineEntity& other) {
    State::copyEntityData(other);

    if (const RegularState* rOther = dynamic_cast<const RegularState*>(&other)) {
        mOnStateChangedAction = createModelActionFromData(rOther->onStateChangedCallback(), ModelAction::CALLBACK);
        mOnEnteringAction = createModelActionFromData(rOther->onEnteringCallback(), ModelAction::CALLBACK);
        mOnExitingAction = createModelActionFromData(rOther->onExitingCallback(), ModelAction::CALLBACK);

        // mChildren is not copied as it represents owned children, not shallow data
    }
}

};  // namespace model
