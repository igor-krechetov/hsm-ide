#ifndef STATE_HPP
#define STATE_HPP

#include <QList>
#include <QSharedPointer>
#include <QString>

#include "private/StateMachineEntity.hpp"

namespace model {

class Transition;

class State : public StateMachineEntity {
public:
    enum class StateType { Invalid, Initial, Regular, EntryPoint, ExitPoint, Final, History };
    // Root - single instance
    // Initial - outgoing state (only one instance per model)
    //         - can only be added to top level
    //         - only one can exist in the model
    //         - only one outgoing state
    //         - is optional (for sub state machines)
    //      -> EntryPoint - event, condition, state
    // Regular - name, 3 callbacks
    //      -> ExitPoint - name, 3 callbacks, auto event
    // Final - onState callback
    // History - type

    explicit State(const QString& name, const StateType type = StateType::Regular);
    virtual ~State() = default;

    StateType stateType() const;

    const QString& name() const;
    void setName(const QString& name);

protected:
    StateType mStateType = StateType::Invalid;
    QString mName;
};

};  // namespace model

#endif  // STATE_HPP
