#include "EntityIdGenerator.hpp"

namespace model {

EntityIdGenerator::EntityIdGenerator()
    : mNextId(1)
    , mMaxRestoredId(0) {}

EntityID_t EntityIdGenerator::generateNextId() {
    EntityID_t result = INVALID_MODEL_ID;

    if (mNextId <= 0xFFFFFFFE) {
        result = mNextId;
        mAssignedIds.insert(result);
        ++mNextId;
    }

    return result;
}

bool EntityIdGenerator::registerRestoredId(EntityID_t id) {
    bool success = false;

    if ((id >= 1) && (id <= 0xFFFFFFFE) && (!mAssignedIds.contains(id))) {
        mAssignedIds.insert(id);

        if (id > mMaxRestoredId) {
            mMaxRestoredId = id;
        }

        // Keep counter ahead of all registered IDs so that any subsequent
        // generateNextId() call produces a value > all previously registered IDs.
        if (id >= mNextId) {
            mNextId = id + 1;
        }

        success = true;
    }

    return success;
}

void EntityIdGenerator::synchronizeCounter() {
    EntityID_t maxId = mMaxRestoredId;

    for (const EntityID_t id : mAssignedIds) {
        if (id > maxId) {
            maxId = id;
        }
    }

    if (maxId < 0xFFFFFFFE) {
        mNextId = maxId + 1;
    } else {
        mNextId = 0xFFFFFFFE;
    }
}

void EntityIdGenerator::reset() {
    mNextId = 1;
    mMaxRestoredId = 0;
    mAssignedIds.clear();
}

bool EntityIdGenerator::isAssigned(EntityID_t id) const {
    return mAssignedIds.contains(id);
}

EntityID_t EntityIdGenerator::currentCounter() const {
    return mNextId;
}

};  // namespace model
