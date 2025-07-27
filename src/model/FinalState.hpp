#ifndef FINALSTATE_HPP
#define FINALSTATE_HPP

#include "State.hpp"

class FinalState : public State {
public:
    explicit FinalState(const std::string& id);

    virtual ~FinalState() = default;

    // Final states can't have children or transitions in SCXML
    void addChildState(std::shared_ptr<State> child) = delete;
    void addTransition(std::shared_ptr<Transition> child) = delete;

    const std::vector<std::shared_ptr<StateMachineElement>>& children() const = delete;
};

#endif  // FINALSTATE_HPP
