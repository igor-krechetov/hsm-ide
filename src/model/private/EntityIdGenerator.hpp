#ifndef ENTITYIDGENERATOR_HPP
#define ENTITYIDGENERATOR_HPP

#include <QSet>

#include "model/ModelTypes.hpp"

namespace model {

/**
 * @brief Manages unique entity ID generation and tracking for a single StateMachineModel.
 *
 * Replaces the former static counter in StateMachineEntity with a per-model instance that
 * supports deterministic UID assignment during deserialization and monotonically increasing
 * generation for new entities.
 *
 * Valid ID range: [1, 0xFFFFFFFE]. Value 0 is uninitialized, 0xFFFFFFFF is the sentinel
 * (INVALID_MODEL_ID).
 *
 * Typical lifecycle:
 *  - New model: generateNextId() for each entity created in the editor.
 *  - Load file: registerRestoredId() for each parsed UID, then synchronizeCounter().
 *  - Clear model: reset().
 */
class EntityIdGenerator {
public:
    EntityIdGenerator();
    ~EntityIdGenerator() = default;

    /**
     * @brief Generates the next available unique ID.
     *
     * Returns a monotonically increasing value starting from 1. Each returned ID is
     * automatically tracked as assigned.
     *
     * @return A valid EntityID_t in [1, 0xFFFFFFFE], or INVALID_MODEL_ID if the UID space
     *         is exhausted.
     */
    EntityID_t generateNextId();

    /**
     * @brief Registers a UID restored from a persisted file.
     *
     * Validates that the ID is within the allowed range and not already assigned.
     * On success the internal counter is advanced past the registered value so that
     * subsequent generateNextId() calls never collide.
     *
     * @param id The UID value read from the file.
     * @return true if the ID was successfully registered, false if it is out of range
     *         or a duplicate.
     */
    bool registerRestoredId(EntityID_t id);

    /**
     * @brief Sets the internal counter to max(all assigned IDs) + 1.
     *
     * Call after all restored IDs have been registered to guarantee that new IDs
     * are strictly greater than every existing one.
     */
    void synchronizeCounter();

    /**
     * @brief Clears all state and resets the counter to 1.
     *
     * Intended for use when the owning model is cleared before loading a new file.
     */
    void reset();

    /**
     * @brief Checks whether a given ID is currently tracked as assigned.
     * @param id The ID to query.
     * @return true if the ID has been generated or restored.
     */
    bool isAssigned(EntityID_t id) const;

    /**
     * @brief Returns the current value of the internal counter (next ID to be issued).
     * @return The counter value.
     */
    EntityID_t currentCounter() const;

private:
    EntityID_t mNextId = INVALID_MODEL_ID;
    EntityID_t mMaxRestoredId = INVALID_MODEL_ID;
    QSet<EntityID_t> mAssignedIds;
};

}  // namespace model

#endif  // ENTITYIDGENERATOR_HPP
