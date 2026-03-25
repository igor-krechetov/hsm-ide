#include "ModelActionFactory.hpp"

#include "CallbackAction.hpp"
#include "SendEventAction.hpp"
#include "TimerRestartAction.hpp"
#include "TimerStartAction.hpp"
#include "TimerStopAction.hpp"

namespace model {

QSharedPointer<IModelAction> createModelAction(ModelAction type) {
    QSharedPointer<IModelAction> res;

    if (type == ModelAction::CALLBACK) {
        res = QSharedPointer<CallbackAction>::create();
    } else if (type == ModelAction::TIMER_START) {
        res = QSharedPointer<TimerStartAction>::create();
    } else if (type == ModelAction::TIMER_STOP) {
        res = QSharedPointer<TimerStopAction>::create();
    } else if (type == ModelAction::TIMER_RESTART) {
        res = QSharedPointer<TimerRestartAction>::create();
    } else if (type == ModelAction::SEND_EVENT) {
        res = QSharedPointer<SendEventAction>::create();
    }

    return res;
}

QSharedPointer<IModelAction> createModelActionFromData(const QString& data, ModelAction fallbackType) {
    QSharedPointer<IModelAction> res;
    const QString trimmed = data.trimmed();

    if (trimmed.startsWith("start_timer(")) {
        res = createModelAction(ModelAction::TIMER_START);
    } else if (trimmed.startsWith("stop_timer(")) {
        res = createModelAction(ModelAction::TIMER_STOP);
    } else if (trimmed.startsWith("restart_timer(")) {
        res = createModelAction(ModelAction::TIMER_RESTART);
    } else if (trimmed.startsWith("transition(")) {
        res = createModelAction(ModelAction::SEND_EVENT);
    } else {
        res = createModelAction(fallbackType);
    }

    if (res) {
        if (false == res->deserialize(trimmed)) {
            res = createModelAction(fallbackType);
            if (res) {
                res->deserialize(trimmed);
            }
        }
    }

    return res;
}

QString actionName(ModelAction type) {
    QString res = "callback";

    if (type == ModelAction::TIMER_START) {
        res = "start timer";
    } else if (type == ModelAction::TIMER_STOP) {
        res = "stop timer";
    } else if (type == ModelAction::TIMER_RESTART) {
        res = "restart timer";
    } else if (type == ModelAction::SEND_EVENT) {
        res = "send event";
    }

    return res;
}

ModelAction actionTypeByName(const QString& name) {
    ModelAction res = ModelAction::CALLBACK;
    const QString normalized = name.trimmed().toLower().replace('-', ' ').replace('_', ' ');

    if (normalized == "start timer") {
        res = ModelAction::TIMER_START;
    } else if (normalized == "stop timer") {
        res = ModelAction::TIMER_STOP;
    } else if (normalized == "restart timer") {
        res = ModelAction::TIMER_RESTART;
    } else if (normalized == "send event" || normalized == "transition") {
        res = ModelAction::SEND_EVENT;
    }

    return res;
}

QStringList supportedActionNames() {
    return {actionName(ModelAction::CALLBACK),
            actionName(ModelAction::TIMER_START),
            actionName(ModelAction::TIMER_STOP),
            actionName(ModelAction::TIMER_RESTART),
            actionName(ModelAction::SEND_EVENT)};
}

}  // namespace model
