#ifndef STATEMACHINEMODEL_HPP
#define STATEMACHINEMODEL_HPP

#include <QList>
#include <QObject>
#include <QSharedPointer>
#include <QString>

#include "State.hpp"

namespace model {
class RegularState;
class Transition;

class StateMachineModel : public QObject {
    Q_OBJECT
public:
    explicit StateMachineModel(const QString& name, QObject* parent = nullptr);
    virtual ~StateMachineModel();

    const QString& name() const;

    QSharedPointer<RegularState>& root();

    // Creates a new State element with unique ID
    QSharedPointer<State> createUniqueState(const State::StateType type);
    QSharedPointer<Transition> createUniqueTransition(const EntityID_t source, const EntityID_t target);
    QSharedPointer<Transition> createUniqueTransition(const QSharedPointer<State>& source, const QSharedPointer<State>& target);

    bool moveElement(const EntityID_t elementId, const EntityID_t newParentId);

private:
    QString mName;
    QSharedPointer<RegularState> mModelRoot;
};

};  // namespace model

#endif  // STATEMACHINEMODEL_HPP
