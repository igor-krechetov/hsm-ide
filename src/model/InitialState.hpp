#ifndef INITIALSTATE_HPP
#define INITIALSTATE_HPP

#include "State.hpp"

namespace model {

class Transition;

class InitialState : public State {
public:
    explicit InitialState(const QString& name);
    virtual ~InitialState() = default;

    void setTransition(const QSharedPointer<Transition>& transition);
    QSharedPointer<Transition> transition() const;

private:
    QSharedPointer<Transition> mTransition;
};

};  // namespace model

#endif  // INITIALSTATE_HPP
