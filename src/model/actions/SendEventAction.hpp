#ifndef SENDEVENTACTION_HPP
#define SENDEVENTACTION_HPP

#include "IModelAction.hpp"

namespace model {

class SendEventAction : public IModelAction {
public:
    ModelAction type() const override;
    QStringList properties() const override;
    bool setProperty(const QString& key, const QVariant& value) override;
    QVariant getProperty(const QString& key) const override;
    QString serialize() const override;
    bool deserialize(const QString& data) override;

private:
    QString mEventId;
    QString mArguments;
};

}  // namespace model

#endif
