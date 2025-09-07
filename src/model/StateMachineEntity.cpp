#include "StateMachineEntity.hpp"

namespace model {

StateMachineEntity::StateMachineEntity(Type type)
    : mElementType(type) {
    static EntityID_t sNextId = 1;
    mId = sNextId;
    ++sNextId;
}

StateMachineEntity::Type StateMachineEntity::type() const {
    return mElementType;
}

EntityID_t StateMachineEntity::id() const {
    return mId;
}

};  // namespace model
