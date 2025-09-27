#ifndef ENTRYPOINT_HPP
#define ENTRYPOINT_HPP

#include "State.hpp"

namespace model {

class Transition;

class EntryPoint : public State {
public:
    explicit EntryPoint(const QString& name);
    virtual ~EntryPoint() = default;

    inline const QList<QSharedPointer<Transition>>& transitions() const;

    void addTransition(const QSharedPointer<Transition>& transition);

    void deleteChild(const EntityID_t id) override;
    void deleteDirectChild(const QSharedPointer<StateMachineEntity> child) override;

    QSharedPointer<StateMachineEntity> findParentState(const EntityID_t childId) override;
    QSharedPointer<StateMachineEntity> findChild(const EntityID_t id,
                                                 const StateMachineEntity::Type type = StateMachineEntity::Type::Invalid) const override;

private:
    QList<QSharedPointer<Transition>> mTransitions;
};

inline const QList<QSharedPointer<Transition>>& EntryPoint::transitions() const {
    return mTransitions;
}

};  // namespace model

#endif  // ENTRYPOINT_HPP
