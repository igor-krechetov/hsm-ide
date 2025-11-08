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

    QString name() const;
    void setName(const QString& name);

    QSharedPointer<RegularState>& root();

    void clearModel();

    QSharedPointer<Transition> createUniqueTransition(const EntityID_t source, const EntityID_t target);

    bool moveElement(const EntityID_t elementId, const EntityID_t newParentId);

    bool reconnectElements(const EntityID_t transitionId, const EntityID_t newFromElementId, const EntityID_t newToElementId);

    void dump();

signals:
    void modelChanged();
    void modelEntityAdded(QWeakPointer<StateMachineEntity> parent, QWeakPointer<StateMachineEntity> entity);
    void modelEntityDeleted(QWeakPointer<StateMachineEntity> parent, QWeakPointer<StateMachineEntity> entity);
    void modelDataChanged(QWeakPointer<StateMachineEntity> entity);

private:
    QSharedPointer<RegularState> mModelRoot;
};

};  // namespace model

#endif  // STATEMACHINEMODEL_HPP
