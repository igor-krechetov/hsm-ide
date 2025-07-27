#include "StateMachineElement.hpp"

StateMachineElement::StateMachineElement(Type type)
    : mElementType(type) {}

StateMachineElement::Type StateMachineElement::type() const {
    return mElementType;
}
