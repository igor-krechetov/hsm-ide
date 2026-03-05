#ifndef MODIFICATIONHISTORYCONTROLLER_HPP
#define MODIFICATIONHISTORYCONTROLLER_HPP

#include <QSet>
#include <QSharedPointer>
#include <QString>
#include <QVector>

#include "model/ModelTypes.hpp"

namespace model {
class StateMachineModel;
}

class ModificationHistoryController {
public:
    explicit ModificationHistoryController(const QSharedPointer<model::StateMachineModel>& model);

    void beginTransaction(const QString& label);
    void commitTransaction();

    void onModelEntityAdded(const model::EntityID_t parentId, const model::EntityID_t entityId);
    void onModelEntityDeleted(const model::EntityID_t parentId, const model::EntityID_t entityId);
    void onModelDataChanged(const model::EntityID_t entityId);

    bool undo();
    bool redo();

    bool canUndo() const;
    bool canRedo() const;
    bool transactionActive() const;
    bool isRestoring() const;

private:
    void markChanged(const model::EntityID_t entityId);
    bool restoreSnapshot(const QSharedPointer<model::StateMachineModel>& snapshot);
    QSharedPointer<model::StateMachineModel> makeSnapshot() const;

private:
    QSharedPointer<model::StateMachineModel> mModel;
    QVector<QSharedPointer<model::StateMachineModel>> mFrames;
    int mFrameIndex = -1;

    bool mTransactionActive = false;
    bool mTransactionDirty = false;
    bool mRestoring = false;

    QSet<model::EntityID_t> mCurrentChangedEntities;
};

#endif  // MODIFICATIONHISTORYCONTROLLER_HPP
