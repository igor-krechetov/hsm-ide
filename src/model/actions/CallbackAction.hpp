#ifndef CALLBACKACTION_HPP
#define CALLBACKACTION_HPP

#include "IModelAction.hpp"

namespace model {

class CallbackAction : public IModelAction {
public:
    ModelAction type() const override;
    QStringList properties() const override;
    bool setProperty(const QString& key, const QVariant& value) override;
    QVariant getProperty(const QString& key) const override;
    QString serialize() const override;
    bool deserialize(const QString& data) override;

private:
    QString mFunction;
};

}  // namespace model

#endif  // CALLBACKACTION_HPP
