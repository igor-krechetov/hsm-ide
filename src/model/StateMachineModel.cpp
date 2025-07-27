#include "StateMachineModel.hpp"

#include "StateMachineElement.hpp"

StateMachineModel::StateMachineModel(const std::string& name, QObject* parent)
    : QObject(parent)
    , mName(name) {}

StateMachineModel::~StateMachineModel() = default;

const std::string& StateMachineModel::name() const {
    return mName;
}

void StateMachineModel::addChild(QSharedPointer<StateMachineElement> child) {
    mChildren.push_back(child);
}

const std::vector<QSharedPointer<StateMachineElement>>& StateMachineModel::children() const {
    return mChildren;
}
