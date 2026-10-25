#ifndef STATEWITHACTIONS_HPP
#define STATEWITHACTIONS_HPP

#include "../State.hpp"
#include "actions/IModelAction.hpp"

namespace model {

class StateWithActions : public State {
public:
    virtual ~StateWithActions() = default;

    // Getters
    QSharedPointer<IModelAction> onStateChangedAction() const;
    const ModelActionList& onEnteringActions() const;
    const ModelActionList& onExitingActions() const;

    // Backward-compatible single-action getters (first action, or a NONE action if empty)
    QSharedPointer<IModelAction> onEnteringAction() const;
    QSharedPointer<IModelAction> onExitingAction() const;

    bool hasOnStateChangedAction() const;
    bool hasOnEnteringAction() const;
    bool hasOnExitingAction() const;

    // Setters
    void setOnStateChangedAction(const QSharedPointer<IModelAction>& action);
    void setOnStateChangedAction(const QString& actionData);

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

    // Backward-compatible single-action setters (reset the list to one action)
    void setOnEnteringAction(const QSharedPointer<IModelAction>& action);
    void setOnExitingAction(const QSharedPointer<IModelAction>& action);
    void setOnEnteringAction(const QString& actionData);
    void setOnExitingAction(const QString& actionData);

    QStringList properties() const override;
    bool setProperty(const QString& key, const QVariant& value) override;
    QVariant getProperty(const QString& key) const override;

    void copyEntityData(const StateMachineEntity& other) override;

protected:
    explicit StateWithActions(const QString& name, const StateType type);

    QSharedPointer<IModelAction> mOnStateChangedAction;
    ModelActionList mOnEnteringActions;
    ModelActionList mOnExitingActions;
};

};  // namespace model

#endif  // STATEWITHACTIONS_HPP
