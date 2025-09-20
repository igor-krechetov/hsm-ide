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
    enum class StateType { Invalid, Regular, EntryPoint, ExitPoint, Final, History };
    // Root - single instance
    // Initial - only outgoing state (only one instance per model)
    //      -> EntryPoint - event, condition, state
    // Regular - name, 3 callbacks
    //      -> ExitPoint - name, 3 callbacks, auto event
    // Final - onState callback
    // History - type

    explicit State(const QString& mName, const StateType type = StateType::Regular);
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
