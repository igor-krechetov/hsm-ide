#include "ModificationHistoryController.hpp"

#include "model/StateMachineModel.hpp"

ModificationHistoryController::ModificationHistoryController(const QSharedPointer<model::StateMachineModel>& model)
    : mModel(model) {}

void ModificationHistoryController::beginTransaction(const QString& label) {
    Q_UNUSED(label);
    qDebug() << "HISTORY: beginTransaction:" << label;

    if (mModel && !mRestoring && !mTransactionActive) {
        if (mFrames.isEmpty()) {
            auto initialSnapshot = makeSnapshot();

            if (initialSnapshot) {
                mFrames.push_back(initialSnapshot);
                mFrameIndex = 0;
            }
        }

        mCurrentChangedEntities.clear();
        mTransactionDirty = false;
        mTransactionActive = true;
    } else {
        qDebug() << "HISTORY: skip begin. transaction is active or restoring";
    }
}

void ModificationHistoryController::commitTransaction() {
    qDebug() << "HISTORY: commitTransaction";

    if (mModel && mTransactionActive) {
        if (mTransactionDirty) {
            auto snapshot = makeSnapshot();

            if (snapshot) {
                if (mFrameIndex + 1 < mFrames.size()) {
                    mFrames.resize(mFrameIndex + 1);
                }

                mFrames.push_back(snapshot);
                mFrameIndex = mFrames.size() - 1;

                snapshot->dump(); // TODO: debug
            }
        } else {
            qDebug() << "HISTORY: skip. transaction is not dirty";
        }

        mCurrentChangedEntities.clear();
        mTransactionDirty = false;
        mTransactionActive = false;
    } else {
        qDebug() << "HISTORY: skip. no active transaction";
    }
}

void ModificationHistoryController::cancelTransaction() {
    if (mModel && mTransactionActive) {
        mCurrentChangedEntities.clear();
        mTransactionDirty = false;
        mTransactionActive = false;
    }
}

void ModificationHistoryController::onModelEntityAdded(const model::EntityID_t parentId, const model::EntityID_t entityId) {
    Q_UNUSED(parentId);
    markChanged(entityId);
}

void ModificationHistoryController::onModelEntityDeleted(const model::EntityID_t parentId, const model::EntityID_t entityId) {
    Q_UNUSED(parentId);
    markChanged(entityId);
}

void ModificationHistoryController::onModelDataChanged(const model::EntityID_t entityId) {
    markChanged(entityId);
}

bool ModificationHistoryController::undo() {
    bool res = false;

    if (canUndo()) {
        const int targetIndex = mFrameIndex - 1;

        if (restoreSnapshot(mFrames[targetIndex])) {
            mFrameIndex = targetIndex;
            res = true;
        }
    }

    return res;
}

bool ModificationHistoryController::redo() {
    bool res = false;

    if (canRedo()) {
        const int targetIndex = mFrameIndex + 1;

        if (restoreSnapshot(mFrames[targetIndex])) {
            mFrameIndex = targetIndex;
            res = true;
        }
    }

    return res;
}

bool ModificationHistoryController::canUndo() const {
    return (mFrameIndex > 0);
}

bool ModificationHistoryController::canRedo() const {
    return (mFrameIndex >= 0 && (mFrameIndex + 1 < mFrames.size()));
}

bool ModificationHistoryController::transactionActive() const {
    return mTransactionActive;
}

bool ModificationHistoryController::isRestoring() const {
    return mRestoring;
}

void ModificationHistoryController::markChanged(const model::EntityID_t entityId) {
    if (mTransactionActive && !mRestoring) {
        mTransactionDirty = true;

        if (entityId != model::INVALID_MODEL_ID) {
            mCurrentChangedEntities.insert(entityId);
        }
    }
}

void ModificationHistoryController::unmarkChanged(const model::EntityID_t entityId) {
    if (mTransactionActive && !mRestoring) {
        mCurrentChangedEntities.remove(entityId);
        mTransactionDirty = !mCurrentChangedEntities.isEmpty();
    }
}

bool ModificationHistoryController::restoreSnapshot(const QSharedPointer<model::StateMachineModel>& snapshot) {
    bool restored = false;

    if (mModel && snapshot) {
        mRestoring = true;
        *mModel = *snapshot;
        restored = true;
        mRestoring = false;
    }

    return restored;
}

QSharedPointer<model::StateMachineModel> ModificationHistoryController::makeSnapshot() const {
    QSharedPointer<model::StateMachineModel> snapshot;

    if (mModel) {
        snapshot = QSharedPointer<model::StateMachineModel>::create(mModel->name());
        *snapshot = *mModel;
    }

    return snapshot;
}
