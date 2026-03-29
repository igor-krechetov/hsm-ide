#include "ModelActionFactory.hpp"

#include <QObject>

#include "CallbackAction.hpp"
#include "NoneAction.hpp"
#include "SendEventAction.hpp"
#include "TimerRestartAction.hpp"
#include "TimerStartAction.hpp"
#include "TimerStopAction.hpp"

namespace model {

QSharedPointer<IModelAction> ModelActionFactory::createModelAction(ModelAction type) {
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
    } else {
        res = QSharedPointer<NoneAction>::create();
    }

    return res;
}

QSharedPointer<IModelAction> ModelActionFactory::createModelActionFromData(const QString& data, ModelAction fallbackType) {
    QSharedPointer<IModelAction> res;
    const QString trimmed = data.trimmed();

    if (trimmed.isEmpty()) {
        res = createModelAction(ModelAction::NONE);
    } else if (trimmed.startsWith("start_timer(")) {
        res = createModelAction(ModelAction::TIMER_START);
    } else if (trimmed.startsWith("stop_timer(")) {
        res = createModelAction(ModelAction::TIMER_STOP);
    } else if (trimmed.startsWith("restart_timer(")) {
        res = createModelAction(ModelAction::TIMER_RESTART);
    } else if (trimmed.startsWith("transition(")) {
        res = createModelAction(ModelAction::SEND_EVENT);
    } else if (trimmed.startsWith("callback(")) {
        res = createModelAction(ModelAction::CALLBACK);
    } else {
        res = createModelAction(fallbackType);
    }

    if (res) {
        if (false == res->deserialize(trimmed)) {
            res = createModelAction(ModelAction::NONE);
        }
    }

    return res;
}

QString ModelActionFactory::actionName(ModelAction type) {
    QString res = QObject::tr("none");

    if (type == ModelAction::CALLBACK) {
        res = QObject::tr("callback");
    } else if (type == ModelAction::TIMER_START) {
        res = QObject::tr("start timer");
    } else if (type == ModelAction::TIMER_STOP) {
        res = QObject::tr("stop timer");
    } else if (type == ModelAction::TIMER_RESTART) {
        res = QObject::tr("restart timer");
    } else if (type == ModelAction::SEND_EVENT) {
        res = QObject::tr("send event");
    }

    return res;
}

ModelAction ModelActionFactory::actionTypeByName(const QString& name) {
    ModelAction res = ModelAction::NONE;
    const QString normalized = name.trimmed().toLower().replace('-', ' ').replace('_', ' ');

    if (normalized == "callback") {
        res = ModelAction::CALLBACK;
    } else if (normalized == "start timer") {
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

QStringList ModelActionFactory::supportedActionNames() {
    return {actionName(ModelAction::NONE),
            actionName(ModelAction::CALLBACK),
            actionName(ModelAction::TIMER_START),
            actionName(ModelAction::TIMER_STOP),
            actionName(ModelAction::TIMER_RESTART),
            actionName(ModelAction::SEND_EVENT)};
}

}  // namespace model
