#ifndef ENTRYPOINT_HPP
#define ENTRYPOINT_HPP

#include "State.hpp"

namespace model {

class Transition;

class EntryPoint : public State {
public:
    explicit EntryPoint();
    virtual ~EntryPoint() = default;

    void accept(class IModelVisitor* visitor) override;

    QStringList properties() const override;

    inline const QList<QSharedPointer<Transition>>& transitions() const;

    void addTransition(const QSharedPointer<Transition>& transition);

    bool addChild(const QSharedPointer<StateMachineEntity>& child) override;
    void deleteChild(const EntityID_t id) override;
    void deleteDirectChild(const QSharedPointer<StateMachineEntity>& child) override;

    QSharedPointer<StateMachineEntity> findParentState(const EntityID_t childId) override;
    QSharedPointer<StateMachineEntity> findChild(
        const EntityID_t id,
        const StateMachineEntity::Type type = StateMachineEntity::Type::Invalid) const override;

    // NOTE: implement if element specific data will be added in future
    // void copyEntityData(const StateMachineEntity& other) override;

protected:
    bool forEachChildElement(
        std::function<bool(QSharedPointer<StateMachineEntity>, QSharedPointer<StateMachineEntity>)> callback,
        const int depth = DEPTH_INFINITE,
        const bool postOrderTraversal = true) override;

private:
    QList<QSharedPointer<Transition>> mTransitions;
};

inline const QList<QSharedPointer<Transition>>& EntryPoint::transitions() const {
    return mTransitions;
}

};  // namespace model

#endif  // ENTRYPOINT_HPP
