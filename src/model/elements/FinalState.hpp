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

    const QString& event() const;
    void setEvent(const QString& event);

    bool setProperty(const QString& key, const QVariant& value) override;
    QVariant getProperty(const QString& key) const override;
    QStringList properties() const override;

    void copyEntityData(const StateMachineEntity& other) override;

private:
    QString mEvent;
};

};  // namespace model

#endif  // FINALSTATE_HPP
