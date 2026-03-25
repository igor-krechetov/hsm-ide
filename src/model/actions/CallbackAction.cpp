#include "CallbackAction.hpp"

#include <QRegularExpression>

namespace model {

ModelAction CallbackAction::type() const {
    return ModelAction::CALLBACK;
}

QStringList CallbackAction::properties() const {
    return {"function"};
}

bool CallbackAction::setProperty(const QString& key, const QVariant& value) {
    bool res = false;

    if (key == "function") {
        mFunction = value.toString();
        res = true;
    }

    return res;
}

QVariant CallbackAction::getProperty(const QString& key) const {
    QVariant res;

    if (key == "function") {
        res = mFunction;
    }

    return res;
}

QString CallbackAction::serialize() const {
    return mFunction;
}

bool CallbackAction::deserialize(const QString& data) {
    bool res = true;
    QString value = data.trimmed();
    QRegularExpression rgx(R"(^callback\((.*)\)$)");
    auto match = rgx.match(value);

    if (match.hasMatch()) {
        value = match.captured(1).trimmed();
    }

    mFunction = value;

    return res;
}

}  // namespace model
