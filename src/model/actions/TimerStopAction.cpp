#include "TimerStopAction.hpp"

#include <QRegularExpression>

namespace model {

ModelAction TimerStopAction::type() const {
    return ModelAction::TIMER_STOP;
}

QStringList TimerStopAction::properties() const {
    return {"timer_id"};
}

bool TimerStopAction::setProperty(const QString& key, const QVariant& value) {
    bool res = false;

    if (key == "timer_id") {
        mTimerId = value.toString();
        res = true;
    }

    return res;
}

QVariant TimerStopAction::getProperty(const QString& key) const {
    QVariant res;

    if (key == "timer_id") {
        res = mTimerId;
    }

    return res;
}

QString TimerStopAction::serialize() const {
    return QString("stop_timer(%1)").arg(mTimerId);
}

bool TimerStopAction::deserialize(const QString& data) {
    bool res = false;
    QRegularExpression rgx(R"(^stop_timer\((.*)\)$)");
    auto match = rgx.match(data.trimmed());

    if (match.hasMatch()) {
        mTimerId = match.captured(1).trimmed();
        res = true;
    }

    return res;
}

}  // namespace model
