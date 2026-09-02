#include "ModelElementsFactory.hpp"

#include <QDebug>
#include <QString>

#include "elements/EntryPoint.hpp"
#include "elements/ExitPoint.hpp"
#include "elements/FinalState.hpp"
#include "elements/HistoryState.hpp"
#include "elements/IncludeEntity.hpp"
#include "elements/InitialState.hpp"
#include "elements/ModelRootState.hpp"
#include "elements/RegularState.hpp"
#include "elements/Transition.hpp"

namespace model {

// Static member definition
quint32 ModelElementsFactory::s_stateCounter = 0;

QSharedPointer<State> ModelElementsFactory::createStateInstance(const StateType type, const QString& name) {
    QSharedPointer<State> res;

    switch (type) {
        case StateType::MODEL_ROOT:
            res = QSharedPointer<State>(new ModelRootState(name));
            break;
        case StateType::INITIAL:
            res = QSharedPointer<State>(new InitialState());
            break;
        case StateType::REGULAR:
            res = QSharedPointer<State>(new RegularState(name));
            break;
        case StateType::ENTRYPOINT:
            res = QSharedPointer<State>(new EntryPoint());
            break;
        case StateType::EXITPOINT:
            res = QSharedPointer<State>(new ExitPoint(name));
            break;
        case StateType::FINAL:
            res = QSharedPointer<State>(new FinalState(name));
            break;
        case StateType::HISTORY:
            res = QSharedPointer<State>(new HistoryState(name, HistoryType::SHALLOW));
            break;
        case StateType::INCLUDE:
            res = QSharedPointer<State>(new IncludeEntity(name));
            break;
        default:
            qFatal("ModelElementsFactory::createStateInstance: Unexpected element type: %d", static_cast<int>(type));
            break;
    }

    return res;
}

QSharedPointer<State> ModelElementsFactory::createUniqueState(const StateType type, EntityIdGenerator& generator) {
    qDebug() << "------ createUniqueState" << (int)type;
    QString uniqueName;

    if ((StateType::INITIAL != type) && (StateType::ENTRYPOINT != type)) {
        s_stateCounter++;
        uniqueName = QString("State_%1").arg(s_stateCounter);
    }

    QSharedPointer<State> res = createStateInstance(type, uniqueName);

    if (res) {
        res->setId(generator.generateNextId());
    }

    return res;
}

QSharedPointer<State> ModelElementsFactory::createStateWithId(const StateType type, EntityID_t id) {
    QString uniqueName;

    if ((StateType::INITIAL != type) && (StateType::ENTRYPOINT != type)) {
        s_stateCounter++;
        uniqueName = QString("State_%1").arg(s_stateCounter);
    }

    QSharedPointer<State> res = createStateInstance(type, uniqueName);

    if (res) {
        res->setId(id);
    }

    return res;
}

QSharedPointer<State> ModelElementsFactory::cloneStateEntity(const QSharedPointer<State>& source,
                                                             EntityIdGenerator& generator) {
    if (!source) {
        return nullptr;
    }

    QSharedPointer<State> res = createStateInstance(source->stateType(), source->name());

    if (res) {
        res->setId(generator.generateNextId());
    }

    return res;
}

QSharedPointer<Transition> ModelElementsFactory::createUniqueTransition(const QSharedPointer<State>& source,
                                                                        const QSharedPointer<State>& target,
                                                                        EntityIdGenerator& generator) {
    QString defaultEventName = "NEW_EVENT";
    QSharedPointer<Transition> newTransition;

    if (source && ((source->stateType() == StateType::INITIAL) || (source->stateType() == StateType::ENTRYPOINT))) {
        defaultEventName = "";
    }

    newTransition = QSharedPointer<Transition>(new Transition(source, target, defaultEventName));
    newTransition->setId(generator.generateNextId());

    if (false == source->addChild(newTransition)) {
        qCritical() << "trying to add transition to unsupported state type=" << static_cast<int>(source->stateType());
        newTransition.reset();
    }

    return newTransition;
}

QSharedPointer<Transition> ModelElementsFactory::createTransitionWithId(const QSharedPointer<State>& source,
                                                                        const QSharedPointer<State>& target,
                                                                        const QString& event,
                                                                        EntityID_t id) {
    QSharedPointer<Transition> res(new Transition(source, target, event));

    res->setId(id);

    return res;
}

QSharedPointer<State> ModelElementsFactory::createInitialFrom(const QSharedPointer<EntryPoint>& entryPoint,
                                                              EntityIdGenerator& generator) {
    QSharedPointer<InitialState> initial;

    if (entryPoint) {
        initial = QSharedPointer<InitialState>::create();
        initial->setId(generator.generateNextId());

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

QSharedPointer<State> ModelElementsFactory::createFinalFrom(const QSharedPointer<ExitPoint>& exitPoint,
                                                            EntityIdGenerator& generator) {
    QSharedPointer<FinalState> state;

    if (exitPoint) {
        state = QSharedPointer<FinalState>::create(exitPoint->name());
        state->setId(generator.generateNextId());
        state->setOnStateChangedAction(exitPoint->onStateChangedAction()->serialize());
    }

    return state;
}

}  // namespace model
