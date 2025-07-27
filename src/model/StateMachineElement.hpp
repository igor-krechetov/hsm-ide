#ifndef STATEMACHINEELEMENT_HPP
#define STATEMACHINEELEMENT_HPP

#include <QSharedPointer>
#include <string>
#include <vector>

class StateMachineElement {
public:
    enum class Type { State, Transition };

    explicit StateMachineElement(const Type type);
    virtual ~StateMachineElement() = default;

    Type type() const;

private:
    Type mElementType;
};

#endif  // STATEMACHINEELEMENT_HPP
