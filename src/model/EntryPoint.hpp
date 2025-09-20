#ifndef ENTRYPOINT_HPP
#define ENTRYPOINT_HPP

#include "State.hpp"

namespace model {

class Transition;

class EntryPoint : public State {
public:
    explicit EntryPoint(const QString& id);
    virtual ~EntryPoint() = default;

    void addTransition(const QSharedPointer<Transition>& child);
    const QList<QSharedPointer<Transition>>& transitions() const;

private:
    QList<QSharedPointer<Transition>> mTransitions;
};

};  // namespace model

#endif  // ENTRYPOINT_HPP
