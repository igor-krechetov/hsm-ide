#ifndef FINALSTATE_HPP
#define FINALSTATE_HPP

#include "State.hpp"

namespace model {

class FinalState : public State {
public:
    explicit FinalState(const QString& id);

    virtual ~FinalState() = default;

    // Final states can't have children or transitions in SCXML
    void addChildState(std::shared_ptr<State> child) = delete;
    void addTransition(std::shared_ptr<Transition> child) = delete;

    const std::vector<std::shared_ptr<StateMachineEntity>>& children() const = delete;
};

} // namespace model

#endif  // FINALSTATE_HPP
