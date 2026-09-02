#ifndef EXITPOINT_HPP
#define EXITPOINT_HPP

#include <QString>

#include "State.hpp"
#include "actions/IModelAction.hpp"

namespace model {

class ExitPoint : public State {
public:
    explicit ExitPoint(const QString& name);
    virtual ~ExitPoint() = default;

    void accept(class IModelVisitor* visitor) override;

    const QString& event() const;
    QSharedPointer<IModelAction> onStateChangedAction() const;
    QSharedPointer<IModelAction> onEnteringAction() const;
    QSharedPointer<IModelAction> onExitingAction() const;

    bool hasOnStateChangedAction() const;
    bool hasOnEnteringAction() const;
    bool hasOnExitingAction() const;

    void setEvent(const QString& event);
    void setOnStateChangedAction(const QSharedPointer<IModelAction>& action);
    void setOnEnteringAction(const QSharedPointer<IModelAction>& action);
    void setOnExitingAction(const QSharedPointer<IModelAction>& action);
    void setOnStateChangedAction(const QString& actionData);
    void setOnEnteringAction(const QString& actionData);
    void setOnExitingAction(const QString& actionData);

    bool setProperty(const QString& key, const QVariant& value) override;
    QVariant getProperty(const QString& key) const override;
    QStringList properties() const override;
    void copyEntityData(const StateMachineEntity& other) override;

private:
    QString mEvent;
    QSharedPointer<IModelAction> mOnStateChangedAction;
    QSharedPointer<IModelAction> mOnEnteringAction;
    QSharedPointer<IModelAction> mOnExitingAction;
};

};  // namespace model

#endif  // EXITPOINT_HPP
