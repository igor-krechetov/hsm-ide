#ifndef EXITPOINT_HPP
#define EXITPOINT_HPP

#include <QString>

#include "actions/IModelAction.hpp"
#include "private/StateWithActions.hpp"

namespace model {

class ExitPoint : public StateWithActions {
public:
    explicit ExitPoint(const QString& name);
    virtual ~ExitPoint() = default;

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

#endif  // EXITPOINT_HPP
