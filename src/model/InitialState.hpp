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

    void addChild(const QSharedPointer<StateMachineEntity>& child) override;
    void deleteChild(const EntityID_t id) override;
    void deleteDirectChild(const QSharedPointer<StateMachineEntity>& child) override;

    QSharedPointer<StateMachineEntity> findParentState(const EntityID_t childId) override;
    QSharedPointer<StateMachineEntity> findChild(
        const EntityID_t id,
        const StateMachineEntity::Type type = StateMachineEntity::Type::Invalid) const override;

protected:
    void forEachChildElement(std::function<void(QSharedPointer<StateMachineEntity>)> callback,
                             const int depth = DEPTH_INFINITE) override;

private:
    QSharedPointer<Transition> mTransition;
};

};  // namespace model

#endif  // INITIALSTATE_HPP
