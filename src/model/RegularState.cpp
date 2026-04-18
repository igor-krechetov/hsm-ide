#include "RegularState.hpp"

#include <QDebug>

#include "actions/ModelActionFactory.hpp"
#include "private/IModelVisitor.hpp"

namespace model {

RegularState::RegularState(const QString& name)
    : State(name, StateType::REGULAR)
    , mOnStateChangedAction(ModelActionFactory::createModelAction(ModelAction::NONE))
    , mOnEnteringAction(ModelActionFactory::createModelAction(ModelAction::NONE))
    , mOnExitingAction(ModelActionFactory::createModelAction(ModelAction::NONE)) {}

RegularState::~RegularState() {
    qDebug() << "DELETE RegularState:" << this;
    deleteAllChildren();
}

RegularState& RegularState::operator=(const RegularState& other) {
    if (this != &other) {
        State::operator=(other);
        mOnStateChangedAction =
            ModelActionFactory::createModelActionFromData(other.onStateChangedAction()->serialize(), ModelAction::NONE);
        mOnEnteringAction =
            ModelActionFactory::createModelActionFromData(other.onEnteringAction()->serialize(), ModelAction::NONE);
        mOnExitingAction =
            ModelActionFactory::createModelActionFromData(other.onExitingAction()->serialize(), ModelAction::NONE);
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
    mOnStateChangedAction = (action ? action : ModelActionFactory::createModelAction(ModelAction::NONE));

    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void RegularState::setOnEnteringAction(const QSharedPointer<IModelAction>& action) {
    mOnEnteringAction = (action ? action : ModelActionFactory::createModelAction(ModelAction::NONE));

    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void RegularState::setOnExitingAction(const QSharedPointer<IModelAction>& action) {
    mOnExitingAction = (action ? action : ModelActionFactory::createModelAction(ModelAction::NONE));

    emit modelDataChanged(sharedFromThis().toWeakRef());
}

bool RegularState::hasOnStateChangedAction() const {
    return (mOnStateChangedAction && mOnStateChangedAction->type() != ModelAction::NONE);
}

bool RegularState::hasOnEnteringAction() const {
    return (mOnEnteringAction && mOnEnteringAction->type() != ModelAction::NONE);
}

bool RegularState::hasOnExitingAction() const {
    return (mOnExitingAction && mOnExitingAction->type() != ModelAction::NONE);
}

void RegularState::setOnStateChangedAction(const QString& actionData) {
    setOnStateChangedAction(ModelActionFactory::createModelActionFromData(actionData, ModelAction::CALLBACK));
}

void RegularState::setOnEnteringAction(const QString& actionData) {
    setOnEnteringAction(ModelActionFactory::createModelActionFromData(actionData, ModelAction::CALLBACK));
}

void RegularState::setOnExitingAction(const QString& actionData) {
    setOnExitingAction(ModelActionFactory::createModelActionFromData(actionData, ModelAction::CALLBACK));
}

bool RegularState::addChild(const QSharedPointer<StateMachineEntity>& child) {
    bool res = false;

     // Prevent duplicates
    if (child && mChildren.contains(child) == false) {
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
    qDebug() << "StateMachineModel::deleteDirectChild" << child->id();

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
    return State::properties() + QStringList{"onStateChangedAction", "onEnteringAction", "onExitingAction"};
}

bool RegularState::setProperty(const QString& key, const QVariant& value) {
    bool handled = true;

    if (key == "onStateChangedAction") {
        if (value.canConvert<QSharedPointer<IModelAction>>()) {
            setOnStateChangedAction(value.value<QSharedPointer<IModelAction>>());
        } else {
            setOnStateChangedAction(value.toString());
        }
    } else if (key == "onEnteringAction") {
        if (value.canConvert<QSharedPointer<IModelAction>>()) {
            setOnEnteringAction(value.value<QSharedPointer<IModelAction>>());
        } else {
            setOnEnteringAction(value.toString());
        }
    } else if (key == "onExitingAction") {
        if (value.canConvert<QSharedPointer<IModelAction>>()) {
            setOnExitingAction(value.value<QSharedPointer<IModelAction>>());
        } else {
            setOnExitingAction(value.toString());
        }
    } else {
        handled = State::setProperty(key, value);
    }

    return handled;
}

QVariant RegularState::getProperty(const QString& key) const {
    if (key == "onStateChangedAction") {
        return QVariant::fromValue(mOnStateChangedAction);
    } else if (key == "onEnteringAction") {
        return QVariant::fromValue(mOnEnteringAction);
    } else if (key == "onExitingAction") {
        return QVariant::fromValue(mOnExitingAction);
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
        mOnStateChangedAction =
            ModelActionFactory::createModelActionFromData(rOther->onStateChangedAction()->serialize(), ModelAction::NONE);
        mOnEnteringAction =
            ModelActionFactory::createModelActionFromData(rOther->onEnteringAction()->serialize(), ModelAction::NONE);
        mOnExitingAction =
            ModelActionFactory::createModelActionFromData(rOther->onExitingAction()->serialize(), ModelAction::NONE);

        // mChildren is not copied as it represents owned children, not shallow data
    }
}

};  // namespace model
