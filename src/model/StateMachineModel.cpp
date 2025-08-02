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

QSharedPointer<Transition> StateMachineModel::createUniqueTransition(const QSharedPointer<State>& source, const QSharedPointer<State>& target) {
    // TODO: implement
    return nullptr;
}

void StateMachineModel::addChild(QSharedPointer<StateMachineEntity>& child) {
    mChildren.push_back(child);
}

const std::vector<QSharedPointer<StateMachineEntity>>& StateMachineModel::children() const {
    return mChildren;
}

QSharedPointer<StateMachineEntity> StateMachineModel::findChild(const StateMachineEntity::ID_t id) const {
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

} // namespace model
