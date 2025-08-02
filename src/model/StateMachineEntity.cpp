#include "StateMachineEntity.hpp"

namespace model {

StateMachineEntity::StateMachineEntity(Type type)
    : mElementType(type) {
    static ID_t sNextId = 1;
    mId = sNextId;
    ++sNextId;
}

StateMachineEntity::Type StateMachineEntity::type() const {
    return mElementType;
}

StateMachineEntity::ID_t StateMachineEntity::id() const {
    return mId;
}

} // namespace model
