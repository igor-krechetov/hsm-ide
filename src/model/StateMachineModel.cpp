#include "StateMachineModel.hpp"

#include <QDebug>

#include "RegularState.hpp"
#include "State.hpp"
#include "Transition.hpp"
#include "ModelElementsFactory.hpp"
#include "StateMachineEntity.hpp"

namespace model {

StateMachineModel::StateMachineModel(const QString& name, QObject* parent)
    : QObject(parent)
    , mName(name)
    , mModelRoot(ModelElementsFactory::createUniqueState(State::StateType::Regular).dynamicCast<RegularState>()) {
    mModelRoot->setName(name);
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

bool StateMachineModel::moveElement(const EntityID_t elementId, const EntityID_t newParentId) {
    bool moved = false;
    QSharedPointer<RegularState> currentParent = mModelRoot->findParentState(elementId);

    if (currentParent && currentParent->id() != newParentId) {
        QSharedPointer<StateMachineEntity> element = currentParent->findChild(elementId);
        QSharedPointer<RegularState> newParent =
            (mModelRoot->id() == newParentId ? mModelRoot : mModelRoot->findRegularState(newParentId));
        if (element && newParent) {
            newParent->addChild(element);
            currentParent->deleteChild(element);
            moved = true;
        }
    }
    return moved;
}

};  // namespace model
