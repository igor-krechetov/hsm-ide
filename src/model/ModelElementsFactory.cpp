#include "ModelElementsFactory.hpp"

#include <QDebug>
#include <QString>

#include "EntryPoint.hpp"
#include "ExitPoint.hpp"
#include "FinalState.hpp"
#include "HistoryState.hpp"
#include "IncludeEntity.hpp"
#include "InitialState.hpp"
#include "ModelRootState.hpp"
#include "RegularState.hpp"
#include "Transition.hpp"

namespace model {

// Static member definition
quint32 ModelElementsFactory::s_stateCounter = 0;

QSharedPointer<State> ModelElementsFactory::createUniqueState(const StateType type) {
    qDebug() << "------ createUniqueState" << (int)type;
    QSharedPointer<State> res;
    QString uniqueName;

    if ((StateType::INITIAL != type) && (StateType::ENTRYPOINT != type)) {
        s_stateCounter++;
        uniqueName = QString("State_%1").arg(s_stateCounter);
    }

    switch (type) {
        case StateType::MODEL_ROOT:
            res = QSharedPointer<State>(new ModelRootState(uniqueName));
            break;
        case StateType::INITIAL:
            res = QSharedPointer<State>(new InitialState());
            break;
        case StateType::REGULAR:
            res = QSharedPointer<State>(new RegularState(uniqueName));
            break;
        case StateType::ENTRYPOINT:
            res = QSharedPointer<State>(new EntryPoint());
            break;
        case StateType::EXITPOINT:
            res = QSharedPointer<State>(new ExitPoint(uniqueName));
            break;
        case StateType::FINAL:
            res = QSharedPointer<State>(new FinalState(uniqueName));
            break;
        case StateType::HISTORY:
            res = QSharedPointer<State>(new HistoryState(uniqueName, HistoryType::SHALLOW));
            break;
        case StateType::INCLUDE:
            res = QSharedPointer<State>(new IncludeEntity(uniqueName));
            break;
        default:
            qFatal("ModelElementsFactory::createUniqueState: Unexpected element type: %d", static_cast<int>(type));
            break;
    }

    return res;
}

QSharedPointer<State> ModelElementsFactory::cloneStateEntity(const QSharedPointer<State>& source) {
    if (!source) {
        return nullptr;
    }

    QSharedPointer<State> res;

    switch (source->stateType()) {
        case StateType::MODEL_ROOT:
            res = QSharedPointer<State>(new ModelRootState(source->name()));
            break;
        case StateType::INITIAL:
            res = QSharedPointer<State>(new InitialState());
            break;
        case StateType::REGULAR:
            res = QSharedPointer<State>(new RegularState(source->name()));
            break;
        case StateType::ENTRYPOINT:
            res = QSharedPointer<State>(new EntryPoint());
            break;
        case StateType::EXITPOINT:
            res = QSharedPointer<State>(new ExitPoint(source->name()));
            break;
        case StateType::FINAL:
            res = QSharedPointer<State>(new FinalState(source->name()));
            break;
        case StateType::HISTORY:
            res = QSharedPointer<State>(new HistoryState(source->name(), HistoryType::SHALLOW));
            break;
        case StateType::INCLUDE:
            res = QSharedPointer<State>(new IncludeEntity(source->name()));
            break;
        default:
            qFatal("ModelElementsFactory::cloneStateEntity: Unexpected element type: %d",
                   static_cast<int>(source->stateType()));
            break;
    }

    return res;
}

QSharedPointer<Transition> ModelElementsFactory::createUniqueTransition(const QSharedPointer<State>& source,
                                                                        const QSharedPointer<State>& target) {
    QSharedPointer<Transition> newTransition(new Transition(source, target, "NEW_EVENT"));

    if (false == source->addChild(newTransition)) {
        qCritical() << "trying to add transition to unsupported state type=" << static_cast<int>(source->stateType());
        newTransition.reset();
    }

    return newTransition;
}

QSharedPointer<State> ModelElementsFactory::createInitialFrom(const QSharedPointer<EntryPoint>& entryPoint) {
    QSharedPointer<InitialState> initial;

    if (entryPoint) {
        initial = QSharedPointer<InitialState>::create();

        // Copy the first transition if exists
        if (!entryPoint->transitions().isEmpty()) {
            qDebug() << "---- createInitialFrom: entryPoint=" << entryPoint->id() << ", initial=" << initial->id();
            qDebug() << "--------- sourceId=" << entryPoint->transitions().first()->sourceId();
            qDebug() << "--------- targetId=" << entryPoint->transitions().first()->targetId();
            initial->setTransition(entryPoint->transitions().first());
            qDebug() << "--------- new sourceId=" << initial->transition()->sourceId();
            qDebug() << "--------- new targetId=" << initial->transition()->targetId();
        }
    }

    return initial;
}

QSharedPointer<State> ModelElementsFactory::createFinalFrom(const QSharedPointer<ExitPoint>& exitPoint) {
    QSharedPointer<FinalState> state;

    if (exitPoint) {
        state = QSharedPointer<FinalState>::create(exitPoint->name());
        state->setOnStateChangedAction(exitPoint->onStateChangedAction()->serialize());
    }

    return state;
}

}  // namespace model
