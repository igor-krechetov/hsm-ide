#ifndef INITIALSTATE_HPP
#define INITIALSTATE_HPP

#include "State.hpp"

namespace model {

class Transition;

class InitialState : public State {
public:
    // TODO: remove name. Initial states can't have names
    explicit InitialState();
    virtual ~InitialState() = default;

    void setName(const QString& name) = delete;

    void accept(class IModelVisitor* visitor) override;

    void setTransition(const QSharedPointer<Transition>& transition);
    QSharedPointer<Transition> transition() const;

    QStringList properties() const override;

    bool addChild(const QSharedPointer<StateMachineEntity>& child) override;
    void deleteChild(const EntityID_t id) override;
    void deleteDirectChild(const QSharedPointer<StateMachineEntity>& child) override;

    QSharedPointer<StateMachineEntity> findParentState(const EntityID_t childId) override;
    QSharedPointer<StateMachineEntity> findChild(
        const EntityID_t id,
        const StateMachineEntity::Type type = StateMachineEntity::Type::Invalid) const override;

protected:
    bool forEachChildElement(std::function<bool(QSharedPointer<StateMachineEntity>,QSharedPointer<StateMachineEntity>)> callback,
                             const int depth = DEPTH_INFINITE,
                             const bool postOrderTraversal = true) override;

private:
    QSharedPointer<Transition> mTransition;
};

};  // namespace model

#endif  // INITIALSTATE_HPP
