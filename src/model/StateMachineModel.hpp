#ifndef STATEMACHINEMODEL_HPP
#define STATEMACHINEMODEL_HPP

#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <vector>

#include "State.hpp"
// #include "Transition.hpp"

namespace model {
class StateMachineEntity;
class Transition;

class StateMachineModel : public QObject {
    Q_OBJECT
public:
    explicit StateMachineModel(const QString& name, QObject* parent = nullptr);
    virtual ~StateMachineModel();

    const QString& name() const;

    // Creates a new State element with unique ID
    QSharedPointer<State> createUniqueState(const State::Type type);
    QSharedPointer<Transition> createUniqueTransition(const EntityID_t source, const EntityID_t target);
    QSharedPointer<Transition> createUniqueTransition(const QSharedPointer<State>& source, const QSharedPointer<State>& target);

    void addChild(const QSharedPointer<StateMachineEntity>& child);
    void addChild(const QSharedPointer<Transition>& child);
    void addChild(const QSharedPointer<State>& child);

    const std::vector<QSharedPointer<StateMachineEntity>>& children() const;
    QSharedPointer<StateMachineEntity> findChild(const EntityID_t id) const;

private:
    QString mName;
    std::vector<QSharedPointer<StateMachineEntity>> mChildren;
};

}; // namespace model

#endif  // STATEMACHINEMODEL_HPP
