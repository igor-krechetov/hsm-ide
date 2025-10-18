#include "HistoryState.hpp"

#include "Transition.hpp"
#include "private/IModelVisitor.hpp"
namespace model {

HistoryState::HistoryState(const QString& name, const HistoryType historyType)
    : State(name, StateType::HISTORY)
    , mHistoryType(historyType) {}

void HistoryState::accept(class IModelVisitor* visitor) {
    if (visitor) {
        visitor->visitHistoryState(this);
    }
}

void HistoryState::setHistoryType(HistoryType historyType) {
    mHistoryType = historyType;
    emit modelDataChanged(sharedFromThis().toWeakRef());
}

void HistoryState::setDefaultTransition(const QSharedPointer<Transition>& transition) {
    if (mDefaultTransition) {
        unregisterChild(mDefaultTransition);
    }

    mDefaultTransition = transition;
    registerNewChild(transition);
}

QSharedPointer<Transition> HistoryState::defaultTransition() const {
    return mDefaultTransition;
}

QStringList HistoryState::properties() const {
    return State::properties() + QStringList{cKeyHistoryType};
}

bool HistoryState::setProperty(const QString& key, const QVariant& value) {
    bool handled = true;

    if (key == cKeyHistoryType) {
        setHistoryType(historyTypeFromInt(value.toInt()));
    } else {
        handled = State::setProperty(key, value);
    }

    return handled;
}

QVariant HistoryState::getProperty(const QString& key) const {
    if (key == cKeyHistoryType) {
        return static_cast<int>(mHistoryType);
    }

    return State::getProperty(key);
}

bool HistoryState::addChild(const QSharedPointer<StateMachineEntity>& child) {
    if (child && (child->type() == StateMachineEntity::Type::Transition)) {
        setDefaultTransition(child.dynamicCast<Transition>());
    } else {
        setDefaultTransition(nullptr);
    }

    return true;
}

void HistoryState::deleteChild(const EntityID_t id) {
    if (mDefaultTransition && mDefaultTransition->id() == id) {
        unregisterChild(mDefaultTransition);
        mDefaultTransition.reset();
    }
}

void HistoryState::deleteDirectChild(const QSharedPointer<StateMachineEntity>& child) {
    if (child == mDefaultTransition) {
        unregisterChild(mDefaultTransition);
        mDefaultTransition.reset();
    }
}

QSharedPointer<StateMachineEntity> HistoryState::findParentState(const EntityID_t childId) {
    return (mDefaultTransition && mDefaultTransition->id() == childId ? sharedFromThis() : nullptr);
}

QSharedPointer<StateMachineEntity> HistoryState::findChild(const EntityID_t id, const StateMachineEntity::Type type) const {
    return ((type == StateMachineEntity::Type::Invalid || type == StateMachineEntity::Type::Transition) && mDefaultTransition &&
                    (mDefaultTransition->id() == id)
                ? mDefaultTransition
                : nullptr);
}

void HistoryState::forEachChildElement(std::function<void(QSharedPointer<StateMachineEntity>)> callback, const int depth) {
    Q_UNUSED(depth);

    if (mDefaultTransition) {
        callback(mDefaultTransition.dynamicCast<StateMachineEntity>());
    }
}

};  // namespace model
