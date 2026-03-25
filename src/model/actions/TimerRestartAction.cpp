#include "TimerRestartAction.hpp"

#include <QRegularExpression>

namespace model {

ModelAction TimerRestartAction::type() const {
    return ModelAction::TIMER_RESTART;
}

QStringList TimerRestartAction::properties() const {
    return {"timer_id"};
}

bool TimerRestartAction::setProperty(const QString& key, const QVariant& value) {
    bool res = false;

    if (key == "timer_id") {
        mTimerId = value.toString();
        res = true;
    }

    return res;
}

QVariant TimerRestartAction::getProperty(const QString& key) const {
    QVariant res;

    if (key == "timer_id") {
        res = mTimerId;
    }

    return res;
}

QString TimerRestartAction::serialize() const {
    return QString("restart_timer(%1)").arg(mTimerId);
}

bool TimerRestartAction::deserialize(const QString& data) {
    bool res = false;
    QRegularExpression rgx(R"(^restart_timer\((.*)\)$)");
    auto match = rgx.match(data.trimmed());

    if (match.hasMatch()) {
        mTimerId = match.captured(1).trimmed();
        res = true;
    }

    return res;
}

}  // namespace model
