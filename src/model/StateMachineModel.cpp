#include "StateMachineModel.hpp"

#include <QDebug>

#include "ModelElementsFactory.hpp"
#include "RegularState.hpp"
#include "State.hpp"
#include "StateMachineEntity.hpp"
#include "Transition.hpp"

namespace model {

StateMachineModel::StateMachineModel(const QString& name, QObject* parent)
    : QObject(parent)
    , mName(name)
    , mModelRoot(ModelElementsFactory::createUniqueState(State::StateType::Regular).dynamicCast<RegularState>()) {
    mModelRoot->setName(name);
    // Subscribe to modelEntityAdded for mModelRoot
    QObject::connect(mModelRoot.data(), &StateMachineEntity::childAdded, this, &StateMachineModel::modelChanged);
    QObject::connect(mModelRoot.data(), &StateMachineEntity::childRemoved, this, &StateMachineModel::modelChanged);
    QObject::connect(mModelRoot.data(), &StateMachineEntity::modelDataChanged, this, &StateMachineModel::modelDataChanged);
}

StateMachineModel::~StateMachineModel() = default;

const QString& StateMachineModel::name() const {
    return mName;
}

QSharedPointer<RegularState>& StateMachineModel::root() {
    return mModelRoot;
}

QSharedPointer<Transition> StateMachineModel::createUniqueTransition(const EntityID_t source, const EntityID_t target) {
    auto from = mModelRoot->findChild(source);
    auto to = mModelRoot->findChild(target);
    // TODO: check for errors
    return ModelElementsFactory::createUniqueTransition(from.dynamicCast<State>(), to.dynamicCast<State>());
}

// TODO: this probably will not work with transitions. decide how transitions will be moved
bool StateMachineModel::moveElement(const EntityID_t elementId, const EntityID_t newParentId) {
    bool moved = false;
    QSharedPointer<StateMachineEntity> currentParent = mModelRoot->findParentState(elementId);

    if (currentParent && currentParent->id() != newParentId) {
        QSharedPointer<StateMachineEntity> element = currentParent->findChild(elementId);
        QSharedPointer<RegularState> newParent =
            (mModelRoot->id() == newParentId ? mModelRoot : mModelRoot->findRegularState(newParentId));

        if (element && newParent) {
            newParent->addChild(element);
            currentParent->deleteDirectChild(element);
            moved = true;
        }
    }

    return moved;
}

};  // namespace model
