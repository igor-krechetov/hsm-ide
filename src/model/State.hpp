#ifndef STATE_HPP
#define STATE_HPP

#include <QSharedPointer>
#include <QString>
#include <vector>

#include "StateMachineEntity.hpp"

namespace model {

class Transition;

class State : public StateMachineEntity {
public:
    enum class Type { Initial, Regular, Final, History };

    explicit State(const QString& mName, const Type type = Type::Regular);
    virtual ~State() = default;

    Type stateType() const;

    void addChildState(QSharedPointer<State> child);
    void addTransition(QSharedPointer<Transition> child);
    const std::vector<QSharedPointer<StateMachineEntity>>& children() const;

protected:
    QString mName;
    Type mStateType;
    std::vector<QSharedPointer<StateMachineEntity>> mChildren;
};

} // namespace model

#endif  // STATE_HPP
