#ifndef MODELELEMENTSFACTORY_HPP
#define MODELELEMENTSFACTORY_HPP

#include <QSharedPointer>

#include "State.hpp"
#include "Transition.hpp"

namespace model {

class EntryPoint;
class ExitPoint;

class ModelElementsFactory {
public:
    static QSharedPointer<State> createUniqueState(const StateType type);
    static QSharedPointer<State> cloneStateEntity(const QSharedPointer<State>& source);
    static QSharedPointer<Transition> createUniqueTransition(const QSharedPointer<State>& source,
                                                             const QSharedPointer<State>& target);
    static QSharedPointer<State> createInitialFrom(const QSharedPointer<EntryPoint>& entryPoint);
    static QSharedPointer<State> createFinalFrom(const QSharedPointer<ExitPoint>& exitPoint);

private:
    static quint32 s_stateCounter;
};

}  // namespace model

#endif  // MODELELEMENTSFACTORY_HPP
