#ifndef STATEMACHINEENTITY_HPP
#define STATEMACHINEENTITY_HPP

#include <QSharedPointer>
#include <vector>

// TODO: move StateMachineElement::ID_t to a common header

namespace model {

class StateMachineEntity {
public:
    enum class Type { State, Transition }; // namespace model
    using ID_t = uint32_t;

public:
    explicit StateMachineEntity(const Type type);
    virtual ~StateMachineEntity() = default;

    ID_t id() const;
    Type type() const;

private:
    ID_t mId = 0;
    Type mElementType;
};

} // namespace model

#endif  // STATEMACHINEENTITY_HPP
