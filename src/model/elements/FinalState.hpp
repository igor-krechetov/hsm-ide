#ifndef FINALSTATE_HPP
#define FINALSTATE_HPP

#include "actions/IModelAction.hpp"
#include "private/StateWithActions.hpp"

namespace model {

class FinalState : public StateWithActions {
public:
    explicit FinalState(const QString& name);
    virtual ~FinalState() = default;

    void accept(class IModelVisitor* visitor) override;

    bool setProperty(const QString& key, const QVariant& value) override;
    QVariant getProperty(const QString& key) const override;
    QStringList properties() const override;

    // void copyEntityData(const StateMachineEntity& other) override;
};

};  // namespace model

#endif  // FINALSTATE_HPP
