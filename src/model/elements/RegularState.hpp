#ifndef REGULARSTATE_HPP
#define REGULARSTATE_HPP

#include <QSet>

#include "State.hpp"
#include "Transition.hpp"
#include "actions/IModelAction.hpp"

namespace model {

class RegularState : public State {
public:
    explicit RegularState(const QString& name);
    virtual ~RegularState();
    RegularState& operator=(const RegularState& other);

    void accept(class IModelVisitor* visitor) override;

    // Getters
    QSharedPointer<IModelAction> onStateChangedAction() const;
    const ModelActionList& onEnteringActions() const;
    const ModelActionList& onExitingActions() const;

    // Backward-compatible single-action getters (first action, or a NONE action if empty)
    QSharedPointer<IModelAction> onEnteringAction() const;
    QSharedPointer<IModelAction> onExitingAction() const;

    // Setters
    void setOnStateChangedAction(const QSharedPointer<IModelAction>& action);

    // Multi-action list mutators (onEntering / onExiting)
    void setOnEnteringActions(const ModelActionList& actions);
    void setOnExitingActions(const ModelActionList& actions);
    void addOnEnteringAction(const QSharedPointer<IModelAction>& action);
    void addOnExitingAction(const QSharedPointer<IModelAction>& action);
    void insertOnEnteringAction(const int index, const QSharedPointer<IModelAction>& action);
    void insertOnExitingAction(const int index, const QSharedPointer<IModelAction>& action);
    void removeOnEnteringAction(const int index);
    void removeOnExitingAction(const int index);
    void moveOnEnteringAction(const int from, const int to);
    void moveOnExitingAction(const int from, const int to);

    bool hasOnStateChangedAction() const;
    bool hasOnEnteringAction() const;
    bool hasOnExitingAction() const;

    void setOnStateChangedAction(const QString& actionData);
    // Backward-compatible single-action setters (reset the list to one action)
    void setOnEnteringAction(const QSharedPointer<IModelAction>& action);
    void setOnExitingAction(const QSharedPointer<IModelAction>& action);
    void setOnEnteringAction(const QString& actionData);
    void setOnExitingAction(const QString& actionData);

    bool addChild(const QSharedPointer<StateMachineEntity>& child) override;
    void addChildState(const QSharedPointer<State>& child);
    void addTransition(const QSharedPointer<Transition>& child);
    const QList<QSharedPointer<StateMachineEntity>>& childrenEntities() const;

    void deleteChild(const EntityID_t id) override;
    void deleteDirectChild(const QSharedPointer<StateMachineEntity>& child) override;
    void deleteAllChildren() override;

    QSharedPointer<StateMachineEntity> findParentState(const EntityID_t childId) override;
    QSharedPointer<StateMachineEntity> findChild(
        const EntityID_t id,
        const StateMachineEntity::Type type = StateMachineEntity::Type::Invalid) const override;
    QSharedPointer<State> findState(const EntityID_t id) const;
    QSharedPointer<State> findChildStateByName(const QString& name) override;
    QSharedPointer<RegularState> findRegularState(const EntityID_t id) const;
    QSharedPointer<Transition> findTransition(const EntityID_t id) const;

    QStringList properties() const override;
    bool setProperty(const QString& key, const QVariant& value) override;
    QVariant getProperty(const QString& key) const override;

    bool forEachChildElement(
        std::function<bool(QSharedPointer<StateMachineEntity>, QSharedPointer<StateMachineEntity>)> callback,
        const int depth = DEPTH_INFINITE,
        const bool postOrderTraversal = true) override;

    void copyEntityData(const StateMachineEntity& other) override;

private:
    QSet<EntityID_t> collectStateIdsForDeletion(const QSharedPointer<StateMachineEntity>& rootEntity);
    QSet<EntityID_t> collectLinkedTransitionIds(const QSharedPointer<RegularState>& searchRoot,
                                                const QSet<EntityID_t>& stateIds);

protected:
    QSharedPointer<IModelAction> mOnStateChangedAction;
    ModelActionList mOnEnteringActions;
    ModelActionList mOnExitingActions;

    QList<QSharedPointer<StateMachineEntity>> mChildren;
};

};  // namespace model

#endif  // REGULARSTATE_HPP
