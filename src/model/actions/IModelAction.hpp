#ifndef IMODELACTION_HPP
#define IMODELACTION_HPP

#include <QMetaType>
#include <QSharedPointer>
#include <QStringList>
#include <QVariant>

namespace model {

enum class ModelAction { NONE, CALLBACK, TIMER_START, TIMER_STOP, TIMER_RESTART, SEND_EVENT };

class IModelAction {
public:
    virtual ~IModelAction() = default;

    virtual ModelAction type() const = 0;

    virtual QStringList properties() const = 0;
    virtual bool setProperty(const QString& key, const QVariant& value) = 0;
    virtual QVariant getProperty(const QString& key) const = 0;

    virtual QString serialize() const = 0;
    virtual bool deserialize(const QString& data) = 0;
};

}  // namespace model

Q_DECLARE_METATYPE(QSharedPointer<model::IModelAction>)
Q_DECLARE_METATYPE(model::ModelAction)

#endif  // IMODELACTION_HPP
