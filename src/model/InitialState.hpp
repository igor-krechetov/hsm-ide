#ifndef INITIALSTATE_HPP
#define INITIALSTATE_HPP

#include "State.hpp"
#include "Transition.hpp"

namespace model {

class InitialState : public State {
public:
    explicit InitialState(const QString& id);

    virtual ~InitialState() = default;

    // Initial states can't have children in SCXML
    void addChildState(std::shared_ptr<State> child) = delete;
};

} // namespace model

#endif  // INITIALSTATE_HPP
