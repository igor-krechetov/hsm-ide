#include <QtTest>

#include <QRandomGenerator>

#include "model/private/EntityIdGenerator.hpp"

class EntityIdGeneratorPropertyTest : public QObject {
    Q_OBJECT

private slots:
    void PropertyUidValidityAndUniqueness();
    void PropertyMonotonicallyIncreasingGeneration();
    void PropertyMonotonicallyIncreasingAfterSynchronize();
    void PropertyNoUidReuseAfterDeletion();
};

/**
 * @brief Property 1: UID Validity and Uniqueness.
 *
 * For any sequence of entity creation operations, every assigned UID SHALL be
 * in the range [1, 0xFFFFFFFE], never equal to INVALID_MODEL_ID, and no two
 * entities share the same UID value. Also tests with interleaved
 * registerRestoredId() calls — uniqueness still holds.
 *
 * Validates: Requirements 1.1, 1.2, 9.1, 10.1, 10.3
 */
void EntityIdGeneratorPropertyTest::PropertyUidValidityAndUniqueness() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 100; ++iteration) {
        model::EntityIdGenerator generator;
        const int count = static_cast<int>(rng.bounded(1, 1001));
        QSet<model::EntityID_t> seen;

        for (int i = 0; i < count; ++i) {
            const model::EntityID_t id = generator.generateNextId();

            QVERIFY2(id >= 1, qPrintable(QString("ID %1 below valid range").arg(id)));
            QVERIFY2(id <= 0xFFFFFFFE, qPrintable(QString("ID %1 above valid range").arg(id)));
            QVERIFY2(id != model::INVALID_MODEL_ID,
                     qPrintable(QString("ID equals INVALID_MODEL_ID")));
            QVERIFY2(!seen.contains(id),
                     qPrintable(QString("Duplicate ID %1 at index %2").arg(id).arg(i)));

            seen.insert(id);
        }
    }

    // Extended: interleaved registerRestoredId — uniqueness still holds
    for (int iteration = 0; iteration < 100; ++iteration) {
        model::EntityIdGenerator generator;
        const int count = static_cast<int>(rng.bounded(1, 1001));
        QSet<model::EntityID_t> seen;

        for (int i = 0; i < count; ++i) {
            if (rng.bounded(4) == 0) {
                // Occasionally register a restored ID in a high range
                const model::EntityID_t restoredId =
                    static_cast<model::EntityID_t>(rng.bounded(5000u, 50000u));

                if (!seen.contains(restoredId)) {
                    if (generator.registerRestoredId(restoredId)) {
                        QVERIFY2(restoredId >= 1,
                                 qPrintable(QString("Restored ID %1 below valid range").arg(restoredId)));
                        QVERIFY2(restoredId <= 0xFFFFFFFE,
                                 qPrintable(QString("Restored ID %1 above valid range").arg(restoredId)));
                        seen.insert(restoredId);
                    }
                }
            } else {
                const model::EntityID_t id = generator.generateNextId();

                QVERIFY2(id >= 1, qPrintable(QString("ID %1 below valid range").arg(id)));
                QVERIFY2(id <= 0xFFFFFFFE, qPrintable(QString("ID %1 above valid range").arg(id)));
                QVERIFY2(id != model::INVALID_MODEL_ID,
                         qPrintable(QString("ID equals INVALID_MODEL_ID")));
                QVERIFY2(!seen.contains(id),
                         qPrintable(QString("Duplicate ID %1 at index %2").arg(id).arg(i)));

                seen.insert(id);
            }
        }
    }
}

/**
 * @brief Property 4: Monotonically Increasing Generation.
 *
 * For any sequence of generated (not restored) UIDs within a model instance,
 * each generated UID SHALL be strictly greater than the previously generated UID.
 *
 * Validates: Requirements 10.1
 */
void EntityIdGeneratorPropertyTest::PropertyMonotonicallyIncreasingGeneration() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 100; ++iteration) {
        model::EntityIdGenerator generator;
        const int count = static_cast<int>(rng.bounded(2, 501));

        model::EntityID_t previousId = generator.generateNextId();
        QVERIFY(previousId != model::INVALID_MODEL_ID);

        for (int i = 1; i < count; ++i) {
            const model::EntityID_t currentId = generator.generateNextId();

            QVERIFY2(currentId != model::INVALID_MODEL_ID,
                     qPrintable(QString("Generation failed at index %1").arg(i)));
            QVERIFY2(currentId > previousId,
                     qPrintable(QString("ID %1 not greater than previous %2 at index %3")
                                    .arg(currentId)
                                    .arg(previousId)
                                    .arg(i)));

            previousId = currentId;
        }
    }
}

/**
 * @brief Property 4 (extended): Monotonically Increasing After Synchronize.
 *
 * After registering restored IDs and calling synchronizeCounter(), subsequently
 * generated IDs SHALL be monotonically increasing among themselves.
 *
 * Validates: Requirements 10.1
 */
void EntityIdGeneratorPropertyTest::PropertyMonotonicallyIncreasingAfterSynchronize() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 100; ++iteration) {
        model::EntityIdGenerator generator;

        // Register a random set of restored IDs
        const int restoredCount = static_cast<int>(rng.bounded(1, 50));
        QSet<model::EntityID_t> restoredIds;

        while (restoredIds.size() < restoredCount) {
            const model::EntityID_t restoredId =
                static_cast<model::EntityID_t>(rng.bounded(1u, 10000u));
            if (!restoredIds.contains(restoredId)) {
                restoredIds.insert(restoredId);
                generator.registerRestoredId(restoredId);
            }
        }

        generator.synchronizeCounter();

        // Generate new IDs and verify monotonic ordering
        const int newCount = static_cast<int>(rng.bounded(2, 100));
        model::EntityID_t previousId = generator.generateNextId();
        QVERIFY(previousId != model::INVALID_MODEL_ID);

        for (int i = 1; i < newCount; ++i) {
            const model::EntityID_t currentId = generator.generateNextId();

            QVERIFY2(currentId != model::INVALID_MODEL_ID,
                     qPrintable(QString("Generation failed at index %1").arg(i)));
            QVERIFY2(currentId > previousId,
                     qPrintable(QString("ID %1 not greater than previous %2 at index %3")
                                    .arg(currentId)
                                    .arg(previousId)
                                    .arg(i)));

            previousId = currentId;
        }
    }
}

/**
 * @brief Property 3: No UID Reuse After Deletion.
 *
 * For any sequence of entity creation and deletion operations within a single
 * model instance, a UID assigned to a deleted entity SHALL never be assigned
 * to any subsequently created entity. Since EntityIdGenerator uses a
 * monotonically increasing counter, deleted IDs can never reappear — this test
 * verifies the property holds even with interleaved registerRestoredId and
 * synchronizeCounter calls.
 *
 * Validates: Requirements 1.6, 9.5
 */
void EntityIdGeneratorPropertyTest::PropertyNoUidReuseAfterDeletion() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 100; ++iteration) {
        model::EntityIdGenerator generator;
        QSet<model::EntityID_t> deletedIds;
        QSet<model::EntityID_t> allGeneratedIds;

        // Phase 1: Generate some IDs, then mark a subset as "deleted"
        const int initialCount = static_cast<int>(rng.bounded(5, 51));

        for (int i = 0; i < initialCount; ++i) {
            const model::EntityID_t id = generator.generateNextId();
            QVERIFY2(id != model::INVALID_MODEL_ID,
                     qPrintable(QString("Iteration %1: generation failed at initial step %2")
                                    .arg(iteration)
                                    .arg(i)));
            allGeneratedIds.insert(id);
        }

        // Simulate deletion: pick random subset of generated IDs as "deleted"
        const int deleteCount = static_cast<int>(rng.bounded(1, initialCount));
        QList<model::EntityID_t> idList(allGeneratedIds.begin(), allGeneratedIds.end());

        for (int i = 0; i < deleteCount; ++i) {
            const int idx = static_cast<int>(rng.bounded(static_cast<quint32>(idList.size())));
            deletedIds.insert(idList[idx]);
            idList.removeAt(idx);
        }

        // Phase 2: Optionally register some restored IDs and synchronize
        if (rng.bounded(2) == 1) {
            const int restoredCount = static_cast<int>(rng.bounded(1, 20));

            for (int i = 0; i < restoredCount; ++i) {
                const model::EntityID_t restoredId =
                    static_cast<model::EntityID_t>(rng.bounded(1u, 50000u));

                if (!generator.isAssigned(restoredId)) {
                    generator.registerRestoredId(restoredId);
                }
            }

            generator.synchronizeCounter();
        }

        // Phase 3: Generate more IDs and verify none match previously used IDs
        const int postDeleteCount = static_cast<int>(rng.bounded(5, 101));

        for (int i = 0; i < postDeleteCount; ++i) {
            const model::EntityID_t newId = generator.generateNextId();

            QVERIFY2(newId != model::INVALID_MODEL_ID,
                     qPrintable(QString("Iteration %1: generation failed at post-delete step %2")
                                    .arg(iteration)
                                    .arg(i)));
            QVERIFY2(!deletedIds.contains(newId),
                     qPrintable(QString("Iteration %1: reused deleted UID %2 at post-delete step %3")
                                    .arg(iteration)
                                    .arg(newId)
                                    .arg(i)));
            QVERIFY2(!allGeneratedIds.contains(newId),
                     qPrintable(QString("Iteration %1: collided with prior UID %2 at post-delete step %3")
                                    .arg(iteration)
                                    .arg(newId)
                                    .arg(i)));

            allGeneratedIds.insert(newId);
        }
    }
}

int runEntityIdGeneratorPropertyTest(int argc, char** argv) {
    EntityIdGeneratorPropertyTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "EntityIdGeneratorPropertyTest.moc"
