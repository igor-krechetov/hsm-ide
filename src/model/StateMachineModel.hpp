#ifndef STATEMACHINEMODEL_HPP
#define STATEMACHINEMODEL_HPP

#include <QList>
#include <QObject>
#include <QSharedPointer>
#include <QString>

#include "elements/State.hpp"
#include "private/EntityIdGenerator.hpp"

namespace model {
class ModelRootState;
class Transition;

class StateMachineModel : public QObject {
    Q_OBJECT
public:
    explicit StateMachineModel(const QString& name, QObject* parent = nullptr);
    virtual ~StateMachineModel();
    StateMachineModel& operator=(const StateMachineModel& other);

    QString name() const;
    void setName(const QString& name);

    QSharedPointer<ModelRootState>& root();
    const QSharedPointer<ModelRootState>& root() const;

    // REQ-103f7: returns base unchanged if the name is free, otherwise appends "_copy"
    // until unique across the whole model. Used to auto-uniquify generated names on
    // create/drop (duplicate-name detection for display is handled in the view layer).
    QString generateUniqueName(const QString& base) const;

    void clearModel();

    QSharedPointer<Transition> createUniqueTransition(const EntityID_t source, const EntityID_t target);

    bool moveElement(const EntityID_t elementId, const EntityID_t newParentId);

    bool reconnectElements(const EntityID_t transitionId, const EntityID_t newFromElementId, const EntityID_t newToElementId);

    EntityIdGenerator& idGenerator();
    const EntityIdGenerator& idGenerator() const;

    void dump() const;

signals:
    void modelChanged();
    void modelEntityAdded(QWeakPointer<StateMachineEntity> parent, QWeakPointer<StateMachineEntity> entity);
    void modelEntityDeleted(QWeakPointer<StateMachineEntity> parent, QWeakPointer<StateMachineEntity> entity);
    void modelDataChanged(QWeakPointer<StateMachineEntity> entity);

private:
    // Internal helper for generateUniqueName: true if any state (other than excludeId)
    // in the whole model already has the given name.
    bool hasStateWithName(const QString& name, const EntityID_t excludeId = INVALID_MODEL_ID) const;

    QSharedPointer<ModelRootState> mModelRoot;
    EntityIdGenerator mIdGenerator;
};

};  // namespace model

#endif  // STATEMACHINEMODEL_HPP
