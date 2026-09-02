#include <QDir>
#include <QPolygonF>
#include <QtTest>

#include "../TestPaths.hpp"
#include "model/elements/FinalState.hpp"
#include "model/elements/HistoryState.hpp"
#include "model/elements/InitialState.hpp"
#include "model/elements/ModelRootState.hpp"
#include "model/elements/RegularState.hpp"
#include "model/StateMachineModel.hpp"
#include "model/StateMachineSerializer.hpp"
#include "model/elements/Transition.hpp"

class FormatRoundTripIntegrationTest : public QObject {
    Q_OBJECT

private slots:
    // Task 8.1: Qt Creator backward compatibility
    void LoadQtCreatorFixtureHasValidIds();
    void LoadQtCreatorFixturePreservesGeometry();
    void LoadAllExistingFixturesWithoutError();

    // Task 8.3: Format round-trip integration tests
    void LoadQtCreatorFixtureSerializeAsHsm();
    void LoadHsmFixtureRoundTrip();
    void LoadPlainScxmlSerializeAsHsm();
    void LoadMixedUidsPreservesValidOnes();
    void LoadDuplicateUidsNoCrash();

private:
    void verifyAllEntitiesHaveValidIds(const QSharedPointer<model::StateMachineModel>& model);
    int countAllEntities(const QSharedPointer<model::StateMachineModel>& model);
};

void FormatRoundTripIntegrationTest::verifyAllEntitiesHaveValidIds(
    const QSharedPointer<model::StateMachineModel>& model) {
    QSet<model::EntityID_t> ids;
    bool hasError = false;
    QString errorMsg;

    model->root()->forEachChildElement(
        [&ids, &hasError, &errorMsg](QSharedPointer<model::StateMachineEntity> /*parent*/,
                                     QSharedPointer<model::StateMachineEntity> entity) -> bool {
            if (entity->id() == model::INVALID_MODEL_ID) {
                hasError = true;
                errorMsg = "Entity has INVALID_MODEL_ID";
                return false;
            }
            if (entity->id() < 1 || entity->id() > 0xFFFFFFFE) {
                hasError = true;
                errorMsg = QString("Entity ID %1 out of valid range").arg(entity->id());
                return false;
            }
            if (ids.contains(entity->id())) {
                hasError = true;
                errorMsg = QString("Duplicate entity ID: %1").arg(entity->id());
                return false;
            }
            ids.insert(entity->id());
            return true;
        },
        model::StateMachineEntity::DEPTH_INFINITE, false);

    QVERIFY2(!hasError, qPrintable(errorMsg));
}

int FormatRoundTripIntegrationTest::countAllEntities(const QSharedPointer<model::StateMachineModel>& model) {
    int count = 0;

    model->root()->forEachChildElement(
        [&count](QSharedPointer<model::StateMachineEntity> /*parent*/,
                 QSharedPointer<model::StateMachineEntity> /*entity*/) -> bool {
            ++count;
            return true;
        },
        model::StateMachineEntity::DEPTH_INFINITE, false);

    return count;
}

// ============================================================================
// Task 8.1: Qt Creator backward compatibility tests
// ============================================================================

/**
 * @brief Load a Qt Creator fixture file and verify all entities have valid, unique IDs.
 *
 * Validates: Requirements 6.1, 6.4, 6.6
 */
void FormatRoundTripIntegrationTest::LoadQtCreatorFixtureHasValidIds() {
    const QString scxml = test::loadScxmlFixture("qt_compatibility/qt_geometry_02.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);
    QVERIFY(model);

    verifyAllEntitiesHaveValidIds(model);
}

/**
 * @brief Load a Qt Creator fixture and verify geometry is correctly parsed.
 *
 * Validates: Requirements 6.2, 6.3, 6.6
 */
void FormatRoundTripIntegrationTest::LoadQtCreatorFixturePreservesGeometry() {
    const QString scxml = test::loadScxmlFixture("qt_compatibility/qt_geometry_02.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);
    QVERIFY(model);

    // Verify states have position data
    auto state1 = model->root()->findChildStateByName("state_1").dynamicCast<model::RegularState>();
    QVERIFY(state1);

    QVariant posX = state1->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X);
    QVariant posY = state1->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y);
    QVERIFY(posX.isValid());
    QVERIFY(posY.isValid());

    QVariant width = state1->getMetadata(model::StateMachineEntity::MetadataKey::WIDTH);
    QVariant height = state1->getMetadata(model::StateMachineEntity::MetadataKey::HEIGHT);
    QVERIFY(width.isValid());
    QVERIFY(height.isValid());
}

/**
 * @brief Load every .scxml file in the test data directory and verify deserialization succeeds.
 *
 * Validates: Requirements 6.6, 11.1
 */
void FormatRoundTripIntegrationTest::LoadAllExistingFixturesWithoutError() {
    const QString root = test::scxmlDataRoot();
    QDir dir(root);
    QVERIFY(dir.exists());

    QStringList filters;
    filters << "*.scxml";
    QStringList fixtures = dir.entryList(filters, QDir::Files);

    // Also check subdirectories
    const QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto& subdir : subdirs) {
        QDir sub(root + "/" + subdir);
        const QStringList subFiles = sub.entryList(filters, QDir::Files);
        for (const auto& file : subFiles) {
            fixtures.append(subdir + "/" + file);
        }
    }

    QVERIFY2(!fixtures.isEmpty(), "No .scxml fixture files found");

    // Skip known malformed files
    const QStringList malformedFiles = {"malformed_not_xml.scxml", "malformed_missing_state_id.scxml"};

    model::StateMachineSerializer serializer;

    for (const auto& fixture : fixtures) {
        if (malformedFiles.contains(fixture)) {
            continue;
        }

        const QString scxml = test::loadScxmlFixture(fixture);
        QVERIFY2(!scxml.isEmpty(), qPrintable(QString("Failed to load fixture: %1").arg(fixture)));

        auto model = serializer.deserializeFromScxml(scxml);
        QVERIFY2(model != nullptr, qPrintable(QString("Deserialization returned null for: %1").arg(fixture)));

        verifyAllEntitiesHaveValidIds(model);
    }
}

// ============================================================================
// Task 8.3: Format round-trip integration tests
// ============================================================================

/**
 * @brief Load a Qt Creator fixture, serialize as HSM format, deserialize, verify model equivalence.
 *
 * Validates: Requirements 5.5, 6.6, 11.1
 */
void FormatRoundTripIntegrationTest::LoadQtCreatorFixtureSerializeAsHsm() {
    const QString scxml = test::loadScxmlFixture("qt_compatibility/qt_geometry_02.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto originalModel = serializer.deserializeFromScxml(scxml);
    QVERIFY(originalModel);

    // Count entities in original model
    const int originalCount = countAllEntities(originalModel);
    QVERIFY(originalCount > 0);

    // Collect original state names
    QStringList originalStateNames;
    originalModel->root()->forEachChildElement(
        [&originalStateNames](QSharedPointer<model::StateMachineEntity> /*parent*/,
                              QSharedPointer<model::StateMachineEntity> entity) -> bool {
            if (entity->type() == model::StateMachineEntity::Type::State) {
                auto state = entity.dynamicCast<model::State>();
                if (state && !state->name().isEmpty()) {
                    originalStateNames.append(state->name());
                }
            }
            return true;
        },
        model::StateMachineEntity::DEPTH_INFINITE, false);

    // Serialize as HSM format
    const QString hsmScxml = serializer.serializeToScxml(originalModel, model::SerializationFormat::HSM);
    QVERIFY(!hsmScxml.isEmpty());
    QVERIFY(hsmScxml.contains("xmlns:hsm"));
    QVERIFY(hsmScxml.contains("hsm:uid"));

    // Deserialize the HSM output
    auto reloadedModel = serializer.deserializeFromScxml(hsmScxml);
    QVERIFY(reloadedModel);

    // Verify same states exist
    for (const auto& name : originalStateNames) {
        auto found = reloadedModel->root()->findChildStateByName(name);
        QVERIFY2(found, qPrintable(QString("State '%1' not found after round-trip").arg(name)));
    }

    verifyAllEntitiesHaveValidIds(reloadedModel);
}

/**
 * @brief Load HSM format fixture, serialize, deserialize, verify UIDs and geometry preserved.
 *
 * Validates: Requirements 5.1, 5.5, 5.6, 13.10
 */
void FormatRoundTripIntegrationTest::LoadHsmFixtureRoundTrip() {
    const QString scxml = test::loadScxmlFixture("hsm_format_basic.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);
    QVERIFY(model);

    // Verify UIDs were restored from file
    auto idle = model->root()->findChildStateByName("Idle");
    auto running = model->root()->findChildStateByName("Running");
    QVERIFY(idle);
    QVERIFY(running);
    QCOMPARE(idle->id(), static_cast<model::EntityID_t>(10));
    QCOMPARE(running->id(), static_cast<model::EntityID_t>(20));

    // Find transition and verify its UID
    auto idleRegular = idle.dynamicCast<model::RegularState>();
    QVERIFY(idleRegular);
    auto transition = idleRegular->findTransition(30);
    QVERIFY(transition);

    // Verify geometry was loaded
    QVariant posX = idle->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X);
    QVERIFY(posX.isValid());
    QVERIFY(qAbs(posX.toDouble() - 100.0) < 0.01);

    QVariant posY = idle->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y);
    QVERIFY(posY.isValid());
    QVERIFY(qAbs(posY.toDouble() - 200.0) < 0.01);

    // Serialize and reload (round-trip)
    const QString reserializedScxml = serializer.serializeToScxml(model, model::SerializationFormat::HSM);
    QVERIFY(!reserializedScxml.isEmpty());

    auto reloaded = serializer.deserializeFromScxml(reserializedScxml);
    QVERIFY(reloaded);

    // Verify UIDs preserved
    auto reloadedIdle = reloaded->root()->findChildStateByName("Idle");
    auto reloadedRunning = reloaded->root()->findChildStateByName("Running");
    QVERIFY(reloadedIdle);
    QVERIFY(reloadedRunning);
    QCOMPARE(reloadedIdle->id(), static_cast<model::EntityID_t>(10));
    QCOMPARE(reloadedRunning->id(), static_cast<model::EntityID_t>(20));

    // Verify geometry preserved
    QVariant reloadedPosX = reloadedIdle->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X);
    QVERIFY(reloadedPosX.isValid());
    QVERIFY(qAbs(reloadedPosX.toDouble() - 100.0) < 0.01);
}

/**
 * @brief Load plain SCXML, serialize as HSM format, verify UIDs generated and no geometry in layout.
 *
 * Validates: Requirements 11.1, 6.6, 13.10
 */
void FormatRoundTripIntegrationTest::LoadPlainScxmlSerializeAsHsm() {
    const QString scxml = test::loadScxmlFixture("plain_scxml_no_editor.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);
    QVERIFY(model);

    // Verify entities exist with valid IDs
    auto alpha = model->root()->findChildStateByName("Alpha");
    auto beta = model->root()->findChildStateByName("Beta");
    QVERIFY(alpha);
    QVERIFY(beta);
    verifyAllEntitiesHaveValidIds(model);

    // Verify no geometry metadata (plain SCXML has no editor info)
    QVariant alphaX = alpha->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X);
    QVERIFY2(!alphaX.isValid(), "Plain SCXML should have no geometry metadata");

    // Serialize as HSM format
    const QString hsmOutput = serializer.serializeToScxml(model, model::SerializationFormat::HSM);
    QVERIFY(!hsmOutput.isEmpty());
    QVERIFY(hsmOutput.contains("xmlns:hsm"));
    QVERIFY(hsmOutput.contains("hsm:uid"));

    // Verify no layout section has state entries (since no geometry set)
    QVERIFY(!hsmOutput.contains("<hsm:state "));
}

/**
 * @brief Load mixed UID file, verify elements with valid UIDs have those preserved.
 *
 * Validates: Requirements 5.2, 7.1
 */
void FormatRoundTripIntegrationTest::LoadMixedUidsPreservesValidOnes() {
    const QString scxml = test::loadScxmlFixture("hsm_format_mixed_uids.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);
    QVERIFY(model);

    // Elements with explicit UIDs should preserve them
    auto withUid = model->root()->findChildStateByName("WithUid");
    auto alsoWithUid = model->root()->findChildStateByName("AlsoWithUid");
    auto noUid = model->root()->findChildStateByName("NoUid");
    QVERIFY(withUid);
    QVERIFY(alsoWithUid);
    QVERIFY(noUid);

    QCOMPARE(withUid->id(), static_cast<model::EntityID_t>(10));
    QCOMPARE(alsoWithUid->id(), static_cast<model::EntityID_t>(50));

    // NoUid should have a generated valid unique ID (not necessarily > max file UID during parse)
    QVERIFY2(noUid->id() >= 1 && noUid->id() <= 0xFFFFFFFE,
             qPrintable(QString("Generated UID %1 out of valid range").arg(noUid->id())));
    QVERIFY2(noUid->id() != 10, "Generated UID should not collide with existing UID 10");
    QVERIFY2(noUid->id() != 50, "Generated UID should not collide with existing UID 50");
    QVERIFY2(noUid->id() != 51, "Generated UID should not collide with existing UID 51");

    // All IDs must be unique
    QSet<model::EntityID_t> ids;
    ids.insert(withUid->id());
    ids.insert(alsoWithUid->id());
    ids.insert(noUid->id());
    QCOMPARE(ids.size(), 3);

    verifyAllEntitiesHaveValidIds(model);
}

/**
 * @brief Load file with duplicate UIDs, verify all entities present with unique IDs.
 *
 * Validates: Requirements 5.4, 8.1, 8.3, 8.5
 */
void FormatRoundTripIntegrationTest::LoadDuplicateUidsNoCrash() {
    const QString scxml = test::loadScxmlFixture("hsm_format_duplicate_uids.scxml");
    QVERIFY(!scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);
    QVERIFY(model);

    // All three states must be present
    auto first = model->root()->findChildStateByName("First");
    auto second = model->root()->findChildStateByName("Second");
    auto third = model->root()->findChildStateByName("Third");
    QVERIFY(first);
    QVERIFY(second);
    QVERIFY(third);

    // First occurrence keeps UID 7
    QCOMPARE(first->id(), static_cast<model::EntityID_t>(7));
    // Third has its own unique UID 20
    QCOMPARE(third->id(), static_cast<model::EntityID_t>(20));
    // Second (duplicate) should get a different ID
    QVERIFY2(second->id() != 7, "Duplicate UID should be regenerated");

    // All UIDs must be unique
    QSet<model::EntityID_t> ids;
    ids.insert(first->id());
    ids.insert(second->id());
    ids.insert(third->id());
    QCOMPARE(ids.size(), 3);

    verifyAllEntitiesHaveValidIds(model);
}

int runFormatRoundTripIntegrationTest(int argc, char** argv) {
    FormatRoundTripIntegrationTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "FormatRoundTripIntegrationTest.moc"
