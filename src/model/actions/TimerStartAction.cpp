#include "TimerStartAction.hpp"

#include <QRegularExpression>

namespace model {

ModelAction TimerStartAction::type() const {
    return ModelAction::TIMER_START;
}

QStringList TimerStartAction::properties() const {
    return {"timer_id", "interval", "singleshot"};
}

bool TimerStartAction::setProperty(const QString& key, const QVariant& value) {
    bool res = false;

    if (key == "timer_id") {
        mTimerId = value.toString();
        res = true;
    } else if (key == "interval") {
        mInterval = value.toUInt();
        res = true;
    } else if (key == "singleshot") {
        mSingleshot = value.toBool();
        res = true;
    }

    return res;
}

QVariant TimerStartAction::getProperty(const QString& key) const {
    QVariant res;

    if (key == "timer_id") {
        res = mTimerId;
    } else if (key == "interval") {
        res = mInterval;
    } else if (key == "singleshot") {
        res = mSingleshot;
    }

    return res;
}

QString TimerStartAction::serialize() const {
    return QString("start_timer(%1, %2, %3)").arg(mTimerId).arg(mInterval).arg(mSingleshot ? "true" : "false");
}

bool TimerStartAction::deserialize(const QString& data) {
    bool res = false;
    QRegularExpression rgx(R"(^start_timer\((.*)\)$)");
    auto match = rgx.match(data.trimmed());

    if (match.hasMatch()) {
        QStringList parts = match.captured(1).split(',', Qt::KeepEmptyParts);
        while (parts.size() < 3) {
            parts.push_back(QString());
        }

        mTimerId = parts.at(0).trimmed();
        mInterval = parts.at(1).trimmed().toUInt();
        mSingleshot = parts.at(2).trimmed().toLower() == "true";
        res = true;
    }

    return res;
}

}  // namespace model
