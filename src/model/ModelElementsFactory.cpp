#include "ModelElementsFactory.hpp"

#include <QString>
#include <QDebug>

#include "InitialState.hpp"
#include "EntryPoint.hpp"
#include "ExitPoint.hpp"
#include "FinalState.hpp"
#include "HistoryState.hpp"
#include "RegularState.hpp"
#include "Transition.hpp"

namespace model {

// Static member definition
quint32 ModelElementsFactory::s_stateCounter = 0;

QSharedPointer<State> ModelElementsFactory::createUniqueState(const StateType type) {
    qDebug() << "------ createUniqueState" << (int)type;
    QSharedPointer<State> res;

    s_stateCounter++;
    QString uniqueName = QString("State_%1").arg(s_stateCounter);

    switch (type) {
        case StateType::INITIAL:
            res = QSharedPointer<State>(new InitialState(uniqueName));
            break;
        case StateType::REGULAR:
            res = QSharedPointer<State>(new RegularState(uniqueName));
            break;
        case StateType::ENTRYPOINT:
            res = QSharedPointer<State>(new EntryPoint(uniqueName));
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
        default:
            qCritical() << "Unexpected element type:" << static_cast<int>(type);
            break;
    }

    return res;
}

QSharedPointer<Transition> ModelElementsFactory::createUniqueTransition(const QSharedPointer<State>& source,
                                                                        const QSharedPointer<State>& target) {
    QSharedPointer<Transition> newTransition(new Transition(source, target, "new_event"));

    switch (source->stateType()) {
        case model::StateType::REGULAR:
        case model::StateType::ENTRYPOINT:
        case model::StateType::INITIAL:
            source->addChild(newTransition);
            break;
        default:
            qCritical() << "trying to add transition to unsupported state type=" << static_cast<int>(source->stateType());
            newTransition.reset();
            break;
    }

    return newTransition;
}

}  // namespace model
