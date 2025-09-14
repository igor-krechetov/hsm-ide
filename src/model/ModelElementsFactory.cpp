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

QSharedPointer<State> ModelElementsFactory::createUniqueState(const State::StateType type) {
    qDebug() << "------ createUniqueState" << (int)type;
    QSharedPointer<State> res;

    s_stateCounter++;
    QString uniqueName = QString("State_%1").arg(s_stateCounter);

    switch (type) {
        case State::StateType::Initial:
            res = QSharedPointer<State>(new InitialState(uniqueName));
            break;
        case State::StateType::Regular:
            res = QSharedPointer<State>(new RegularState(uniqueName));
            break;
        case State::StateType::EntryPoint:
            res = QSharedPointer<State>(new EntryPoint(uniqueName));
            break;
        case State::StateType::ExitPoint:
            res = QSharedPointer<State>(new ExitPoint(uniqueName));
            break;
        case State::StateType::Final:
            res = QSharedPointer<State>(new FinalState(uniqueName));
            break;
        case State::StateType::History:
            res = QSharedPointer<State>(new HistoryState(uniqueName, HistoryState::HistoryType::Shallow));
            break;
        default:
            qCritical() << "Unexpected element type:" << static_cast<int>(type);
            break;
    }

    return res;
}

QSharedPointer<Transition> ModelElementsFactory::createUniqueTransition(const QSharedPointer<State>& source,
                                                                        const QSharedPointer<State>& target) {
    return QSharedPointer<Transition>(new Transition(source, target, "new_event"));
}

}  // namespace model
