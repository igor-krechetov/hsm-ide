#ifndef FINALSTATE_HPP
#define FINALSTATE_HPP

#include "State.hpp"
#include "actions/IModelAction.hpp"

namespace model {

class FinalState : public State {
public:
    explicit FinalState(const QString& name);
    virtual ~FinalState() = default;

    void accept(class IModelVisitor* visitor) override;

    QSharedPointer<IModelAction> onStateChangedAction() const;
    bool hasOnStateChangedAction() const;

    void setOnStateChangedAction(const QSharedPointer<IModelAction>& action);
    void setOnStateChangedAction(const QString& actionData);

    bool setProperty(const QString& key, const QVariant& value) override;
    QVariant getProperty(const QString& key) const override;
    QStringList properties() const override;

    void copyEntityData(const StateMachineEntity& other) override;

private:
    QSharedPointer<IModelAction> mOnStateChangedAction;
};

};  // namespace model

#endif  // FINALSTATE_HPP
