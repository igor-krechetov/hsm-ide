#ifndef INITIALSTATE_HPP
#define INITIALSTATE_HPP

#include "State.hpp"
#include "Transition.hpp"

class InitialState : public State {
public:
    explicit InitialState(const std::string& id);

    virtual ~InitialState() = default;

    // Initial states can't have children in SCXML
    void addChildState(std::shared_ptr<State> child) = delete;
};

#endif  // INITIALSTATE_HPP
