#ifndef STATE_HPP
#define STATE_HPP

#include <QSharedPointer>
#include <string>
#include <vector>

#include "StateMachineElement.hpp"
#include "Transition.hpp"

class State : public StateMachineElement {
public:
    enum class Type { Initial, Regular, Final, History };

    explicit State(const std::string& id, const Type type = Type::Regular);
    virtual ~State() = default;

    const std::string& id() const;
    Type type() const;

    void addChildState(QSharedPointer<State> child);
    void addTransition(QSharedPointer<Transition> child);
    const std::vector<QSharedPointer<StateMachineElement>>& children() const;

protected:
    std::string mId;
    Type mStateType;
    std::vector<QSharedPointer<StateMachineElement>> mChildren;
};

#endif  // STATE_HPP
