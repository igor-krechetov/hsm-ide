#include "StateMachineModel.hpp"
#include "StateMachineEntity.hpp"
#include "State.hpp"
#include "Transition.hpp"

namespace model {

StateMachineModel::StateMachineModel(const QString& name, QObject* parent)
    : QObject(parent)
    , mName(name) {}

StateMachineModel::~StateMachineModel() = default;

const QString& StateMachineModel::name() const {
    return mName;
}

QSharedPointer<State> StateMachineModel::createUniqueState(const State::Type type) {
    // TODO: generate unique ID
    return QSharedPointer<State>::create(QStringLiteral("new_element"), type);
}

QSharedPointer<Transition> StateMachineModel::createUniqueTransition(const EntityID_t source, const EntityID_t target) {
    auto from = findChild(source);
    auto to = findChild(target);
    // TODO: check for errors
    return createUniqueTransition(from.dynamicCast<State>(), to.dynamicCast<State>());
}

QSharedPointer<Transition> StateMachineModel::createUniqueTransition(const QSharedPointer<State>& source, const QSharedPointer<State>& target) {
    return QSharedPointer<Transition>::create(source, target, QStringLiteral("new_event"));
}

void StateMachineModel::addChild(const QSharedPointer<StateMachineEntity>& child) {
    mChildren.push_back(child);
}

void StateMachineModel::addChild(const QSharedPointer<Transition>& child) {
    addChild(child.dynamicCast<StateMachineEntity>());
}

void StateMachineModel::addChild(const QSharedPointer<State>& child) {
    addChild(child.dynamicCast<StateMachineEntity>());
}

void StateMachineModel::deleteChild(const EntityID_t id) {
    auto ptr = findChild(id);

    if (ptr) {
        mChildren.removeAll(ptr);
    }
}

const QList<QSharedPointer<StateMachineEntity>>& StateMachineModel::children() const {
    return mChildren;
}

QSharedPointer<StateMachineEntity> StateMachineModel::findChild(const EntityID_t id) const {
    QSharedPointer<StateMachineEntity> res;

    // TODO: need to support substates

    for (const auto& element: mChildren) {
        if (element->id() == id) {
            res = element;
            break;
        }
    }

    return res;
}

QSharedPointer<State> StateMachineModel::findState(const EntityID_t id) const {
    QSharedPointer<State> res;
    QSharedPointer<StateMachineEntity> childPtr = findChild(id);

    if (childPtr && childPtr->type() == StateMachineEntity::Type::State) {
        res = childPtr.dynamicCast<State>();
    }

    return res;
}

QSharedPointer<Transition> StateMachineModel::findTransition(const EntityID_t id) const {
    QSharedPointer<Transition> res;
    QSharedPointer<StateMachineEntity> childPtr = findChild(id);

    if (childPtr && childPtr->type() == StateMachineEntity::Type::Transition) {
        res = childPtr.dynamicCast<Transition>();
    }

    return res;
}

}; // namespace model
