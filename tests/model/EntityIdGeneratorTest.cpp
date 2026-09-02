#include <QtTest>

#include "model/private/EntityIdGenerator.hpp"

class EntityIdGeneratorTest : public QObject {
    Q_OBJECT

private slots:
    void MonotonicGenerationSequence();
    void RegisterRestoredIdValidValues();
    void RegisterRestoredIdInvalidValues();
    void RegisterRestoredIdDuplicateValues();
    void SynchronizeCounterSetsAboveMax();
    void ResetClearsAllState();
    void UidSpaceExhaustionReturnsInvalidId();
};

/**
 * @brief Verify generateNextId produces monotonically increasing UIDs starting from 1.
 *
 * Use-case: Each new entity receives a unique ID greater than the previous one.
 */
void EntityIdGeneratorTest::MonotonicGenerationSequence() {
    model::EntityIdGenerator gen;

    model::EntityID_t prev = gen.generateNextId();
    QCOMPARE(prev, static_cast<model::EntityID_t>(1));

    for (int i = 0; i < 10; ++i) {
        model::EntityID_t current = gen.generateNextId();
        QVERIFY(current > prev);
        prev = current;
    }
}

/**
 * @brief Verify registerRestoredId accepts valid IDs and tracks them.
 *
 * Use-case: Deserialization registers UIDs parsed from file.
 */
void EntityIdGeneratorTest::RegisterRestoredIdValidValues() {
    model::EntityIdGenerator gen;

    QVERIFY(gen.registerRestoredId(5));
    QVERIFY(gen.isAssigned(5));

    QVERIFY(gen.registerRestoredId(1));
    QVERIFY(gen.isAssigned(1));

    QVERIFY(gen.registerRestoredId(0xFFFFFFFE));
    QVERIFY(gen.isAssigned(0xFFFFFFFE));
}

/**
 * @brief Verify registerRestoredId rejects invalid IDs (0, INVALID_MODEL_ID).
 *
 * Use-case: Invalid UID values from corrupted files are rejected.
 */
void EntityIdGeneratorTest::RegisterRestoredIdInvalidValues() {
    model::EntityIdGenerator gen;

    QVERIFY(!gen.registerRestoredId(0));
    QVERIFY(!gen.isAssigned(0));

    QVERIFY(!gen.registerRestoredId(model::INVALID_MODEL_ID));
    QVERIFY(!gen.isAssigned(model::INVALID_MODEL_ID));
}

/**
 * @brief Verify registerRestoredId rejects duplicate IDs.
 *
 * Use-case: File with duplicate hsm:uid values — second occurrence is rejected.
 */
void EntityIdGeneratorTest::RegisterRestoredIdDuplicateValues() {
    model::EntityIdGenerator gen;

    QVERIFY(gen.registerRestoredId(42));
    QVERIFY(!gen.registerRestoredId(42));
}

/**
 * @brief Verify synchronizeCounter sets counter above max assigned ID.
 *
 * Use-case: After loading file UIDs, new entity creation avoids collisions.
 */
void EntityIdGeneratorTest::SynchronizeCounterSetsAboveMax() {
    model::EntityIdGenerator gen;

    gen.registerRestoredId(10);
    gen.registerRestoredId(50);
    gen.registerRestoredId(30);
    gen.synchronizeCounter();

    QCOMPARE(gen.currentCounter(), static_cast<model::EntityID_t>(51));

    model::EntityID_t next = gen.generateNextId();
    QCOMPARE(next, static_cast<model::EntityID_t>(51));
}

/**
 * @brief Verify reset clears all state and restores initial counter.
 *
 * Use-case: Model is cleared before loading a new file.
 */
void EntityIdGeneratorTest::ResetClearsAllState() {
    model::EntityIdGenerator gen;

    gen.generateNextId();
    gen.generateNextId();
    gen.registerRestoredId(100);

    gen.reset();

    QCOMPARE(gen.currentCounter(), static_cast<model::EntityID_t>(1));
    QVERIFY(!gen.isAssigned(1));
    QVERIFY(!gen.isAssigned(100));

    model::EntityID_t first = gen.generateNextId();
    QCOMPARE(first, static_cast<model::EntityID_t>(1));
}

/**
 * @brief Verify UID space exhaustion returns INVALID_MODEL_ID.
 *
 * Use-case: Counter at maximum valid value cannot produce more IDs.
 */
void EntityIdGeneratorTest::UidSpaceExhaustionReturnsInvalidId() {
    model::EntityIdGenerator gen;

    gen.registerRestoredId(0xFFFFFFFE);
    gen.synchronizeCounter();

    // synchronizeCounter sets mNextId to 0xFFFFFFFE (cannot go beyond max valid)
    // first call consumes the last valid value
    model::EntityID_t last = gen.generateNextId();
    QCOMPARE(last, static_cast<model::EntityID_t>(0xFFFFFFFE));

    // second call should fail — UID space exhausted
    model::EntityID_t result = gen.generateNextId();
    QCOMPARE(result, model::INVALID_MODEL_ID);
}

int runEntityIdGeneratorTest(int argc, char** argv) {
    EntityIdGeneratorTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "EntityIdGeneratorTest.moc"
