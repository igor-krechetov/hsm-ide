#include "SendEventAction.hpp"

#include <QRegularExpression>

namespace model {

ModelAction SendEventAction::type() const {
    return ModelAction::SEND_EVENT;
}

QStringList SendEventAction::properties() const {
    return {"event_id", "arguments"};
}

bool SendEventAction::setProperty(const QString& key, const QVariant& value) {
    bool res = false;

    if (key == "event_id") {
        mEventId = value.toString();
        res = true;
    } else if (key == "arguments") {
        mArguments = value.toString();
        res = true;
    }

    return res;
}

QVariant SendEventAction::getProperty(const QString& key) const {
    QVariant res;

    if (key == "event_id") {
        res = mEventId;
    } else if (key == "arguments") {
        res = mArguments;
    }

    return res;
}

QString SendEventAction::serialize() const {
    return QString("transition(%1, %2)").arg(mEventId, mArguments);
}

bool SendEventAction::deserialize(const QString& data) {
    bool res = false;
    QRegularExpression rgx(R"(^transition\((.*)\)$)");
    auto match = rgx.match(data.trimmed());

    if (match.hasMatch()) {
        QStringList parts = match.captured(1).split(',', Qt::KeepEmptyParts);
        while (parts.size() < 2) {
            parts.push_back(QString());
        }

        mEventId = parts.at(0).trimmed();
        mArguments = parts.mid(1).join(",").trimmed();
        res = true;
    }

    return res;
}

}  // namespace model
