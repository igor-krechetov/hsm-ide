#ifndef MODELELEMENTSFACTORY_HPP
#define MODELELEMENTSFACTORY_HPP

#include <QSharedPointer>

#include "elements/State.hpp"
#include "elements/Transition.hpp"
#include "private/EntityIdGenerator.hpp"

namespace model {

class EntryPoint;
class ExitPoint;

class ModelElementsFactory {
public:
    static QSharedPointer<State> createUniqueState(const StateType type, EntityIdGenerator& generator);
    static QSharedPointer<State> createStateWithId(const StateType type, EntityID_t id);
    static QSharedPointer<State> cloneStateEntity(const QSharedPointer<State>& source, EntityIdGenerator& generator);
    static QSharedPointer<Transition> createUniqueTransition(const QSharedPointer<State>& source,
                                                             const QSharedPointer<State>& target,
                                                             EntityIdGenerator& generator);
    static QSharedPointer<Transition> createTransitionWithId(const QSharedPointer<State>& source,
                                                             const QSharedPointer<State>& target,
                                                             const QString& event,
                                                             EntityID_t id);
    static QSharedPointer<State> createInitialFrom(const QSharedPointer<EntryPoint>& entryPoint, EntityIdGenerator& generator);
    static QSharedPointer<State> createFinalFrom(const QSharedPointer<ExitPoint>& exitPoint, EntityIdGenerator& generator);

private:
    static QSharedPointer<State> createStateInstance(const StateType type, const QString& name);

private:
    static quint32 s_stateCounter;
};

}  // namespace model

#endif  // MODELELEMENTSFACTORY_HPP
