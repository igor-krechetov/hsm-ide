#ifndef MODELELEMENTSFACTORY_HPP
#define MODELELEMENTSFACTORY_HPP

#include <QSharedPointer>

#include "State.hpp"
#include "Transition.hpp"

namespace model {

class ModelElementsFactory {
public:
    static QSharedPointer<State> createUniqueState(const StateType type);
    static QSharedPointer<Transition> createUniqueTransition(const QSharedPointer<State>& source,
                                                             const QSharedPointer<State>& target);

private:
    static quint32 s_stateCounter;
};

}  // namespace model

#endif  // MODELELEMENTSFACTORY_HPP
