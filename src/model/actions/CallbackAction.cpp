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
    bool res = false;
    const QString value = data.trimmed();
    QRegularExpression rgx(R"(^callback\((.*)\)$)");
    auto match = rgx.match(value);

    if (match.hasMatch()) {
        mFunction = match.captured(1).trimmed();
        res = (mFunction.isEmpty() == false);
    } else if (value.contains('(') == false && value.contains(')') == false && value.contains(' ') == false &&
               value.contains('\t') == false && value.isEmpty() == false) {
        mFunction = value;
        res = true;
    }

    return res;
}

}  // namespace model
