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

    // Getters
    QSharedPointer<IModelAction> onStateChangedAction() const;

    // Setters
    void setOnStateChangedAction(const QSharedPointer<IModelAction>& action);
    QString onStateChangedCallback() const;
    void setOnStateChangedCallback(const QString& callback);

    bool setProperty(const QString& key, const QVariant& value) override;
    QVariant getProperty(const QString& key) const override;
    QStringList properties() const override;

    void copyEntityData(const StateMachineEntity& other) override;

private:
    QSharedPointer<IModelAction> mOnStateChangedAction;
};

};  // namespace model

#endif  // FINALSTATE_HPP
