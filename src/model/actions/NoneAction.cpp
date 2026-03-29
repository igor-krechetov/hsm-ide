#include "NoneAction.hpp"

namespace model {

ModelAction NoneAction::type() const {
    return ModelAction::NONE;
}

QStringList NoneAction::properties() const {
    return {};
}

bool NoneAction::setProperty(const QString& key, const QVariant& value) {
    Q_UNUSED(key);
    Q_UNUSED(value);
    return false;
}

QVariant NoneAction::getProperty(const QString& key) const {
    Q_UNUSED(key);
    return QVariant();
}

QString NoneAction::serialize() const {
    return QString();
}

bool NoneAction::deserialize(const QString& data) {
    return data.trimmed().isEmpty();
}

}  // namespace model
