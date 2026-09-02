#include <QtTest>

#include "model/private/EntityIdGenerator.hpp"
#include "model/elements/EntryPoint.hpp"
#include "model/elements/ExitPoint.hpp"
#include "model/elements/FinalState.hpp"
#include "model/elements/HistoryState.hpp"
#include "model/elements/IncludeEntity.hpp"
#include "model/elements/InitialState.hpp"
#include "model/ModelElementsFactory.hpp"
#include "model/elements/ModelRootState.hpp"
#include "model/elements/RegularState.hpp"
#include "model/StateMachineModel.hpp"
#include "model/StateMachineSerializer.hpp"
#include "model/elements/Transition.hpp"

class HsmUidSerializationTest : public QObject {
    Q_OBJECT

private slots:
    void HsmUidWrittenOnRegularState();
    void HsmUidWrittenOnTransition();
    void HsmUidWrittenOnEntryPoint();
    void HsmUidWrittenOnExitPoint();
    void HsmUidWrittenOnFinalState();
    void HsmUidWrittenOnHistoryState();
    void HsmUidWrittenOnInitialState();
    void HsmUidWrittenOnIncludeEntity();
    void HsmUidOmittedInQtCreatorFormat();
    void HsmUidWrittenAsDecimalInteger();
    void InvalidUidZeroParsedAsNew();
    void InvalidUidMaxParsedAsNew();
    void InvalidUidNonNumericParsedAsNew();
    void InvalidUidOverflowParsedAsNew();
    void DuplicateUidFirstPreservedSubsequentRegenerated();
    void DuplicateUidReplacementDoesNotCollideWithLaterUid();
    void RoundTripSerializeDeserializeUidsIdentical();
};

/**
 * @brief Verify hsm:uid attribute is written on a regular state in HSM format.
 *
 * Validates: Requirements 3.1, 3.2, 3.3
 */
void HsmUidSerializationTest::HsmUidWrittenOnRegularState() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto state = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                     .dynamicCast<model::RegularState>();
    state->setName("StateA");
    root->addChildState(state);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model, model::SerializationFormat::HSM);

    const QString expected = QString("hsm:uid=\"%1\"").arg(state->id());
    QVERIFY2(scxml.contains(expected), qPrintable(QString("Expected %1 in output").arg(expected)));
}

/**
 * @brief Verify hsm:uid attribute is written on a transition in HSM format.
 *
 * Validates: Requirements 3.1, 3.2, 3.3
 */
void HsmUidSerializationTest::HsmUidWrittenOnTransition() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto s1 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                  .dynamicCast<model::RegularState>();
    s1->setName("S1");
    auto s2 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                  .dynamicCast<model::RegularState>();
    s2->setName("S2");
    root->addChildState(s1);
    root->addChildState(s2);

    auto transition = model::ModelElementsFactory::createTransitionWithId(s1, s2, "go", model->idGenerator().generateNextId());
    s1->addTransition(transition);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model, model::SerializationFormat::HSM);

    const QString expected = QString("hsm:uid=\"%1\"").arg(transition->id());
    QVERIFY2(scxml.contains(expected), qPrintable(QString("Expected %1 in output").arg(expected)));
}

/**
 * @brief Verify hsm:uid attribute is written on an entry point in HSM format.
 *
 * Validates: Requirements 3.1, 3.2, 3.3
 */
void HsmUidSerializationTest::HsmUidWrittenOnEntryPoint() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto region = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                      .dynamicCast<model::RegularState>();
    region->setName("Region");
    auto target = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                      .dynamicCast<model::RegularState>();
    target->setName("Target");
    auto entry = model::ModelElementsFactory::createUniqueState(model::StateType::ENTRYPOINT, model->idGenerator())
                     .dynamicCast<model::EntryPoint>();

    root->addChildState(region);
    region->addChildState(target);
    region->addChildState(entry);
    auto entryTransition = model::ModelElementsFactory::createTransitionWithId(entry, target, "", model->idGenerator().generateNextId());
    entry->addTransition(entryTransition);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model, model::SerializationFormat::HSM);

    const QString expected = QString("hsm:uid=\"%1\"").arg(entry->id());
    QVERIFY2(scxml.contains(expected), qPrintable(QString("Expected %1 in output").arg(expected)));
}

/**
 * @brief Verify hsm:uid attribute is written on an exit point in HSM format.
 *
 * Validates: Requirements 3.1, 3.2, 3.3
 */
void HsmUidSerializationTest::HsmUidWrittenOnExitPoint() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto parent = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                      .dynamicCast<model::RegularState>();
    parent->setName("Parent");
    auto xp = model::ModelElementsFactory::createUniqueState(model::StateType::EXITPOINT, model->idGenerator())
                  .dynamicCast<model::ExitPoint>();
    xp->setName("XP");
    xp->setEvent("leave");

    root->addChildState(parent);
    parent->addChildState(xp);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model, model::SerializationFormat::HSM);

    const QString expected = QString("hsm:uid=\"%1\"").arg(xp->id());
    QVERIFY2(scxml.contains(expected), qPrintable(QString("Expected %1 in output").arg(expected)));
}

/**
 * @brief Verify hsm:uid attribute is written on a final state in HSM format.
 *
 * Validates: Requirements 3.1, 3.2, 3.3
 */
void HsmUidSerializationTest::HsmUidWrittenOnFinalState() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto final = model::ModelElementsFactory::createUniqueState(model::StateType::FINAL, model->idGenerator())
                     .dynamicCast<model::FinalState>();
    final->setName("Done");

    root->addChildState(final);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model, model::SerializationFormat::HSM);

    const QString expected = QString("hsm:uid=\"%1\"").arg(final->id());
    QVERIFY2(scxml.contains(expected), qPrintable(QString("Expected %1 in output").arg(expected)));
}

/**
 * @brief Verify hsm:uid attribute is written on a history state in HSM format.
 *
 * Validates: Requirements 3.1, 3.2, 3.3
 */
void HsmUidSerializationTest::HsmUidWrittenOnHistoryState() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto parent = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                      .dynamicCast<model::RegularState>();
    parent->setName("Parent");
    auto history = model::ModelElementsFactory::createUniqueState(model::StateType::HISTORY, model->idGenerator())
                       .dynamicCast<model::HistoryState>();

    root->addChildState(parent);
    parent->addChildState(history);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model, model::SerializationFormat::HSM);

    const QString expected = QString("hsm:uid=\"%1\"").arg(history->id());
    QVERIFY2(scxml.contains(expected), qPrintable(QString("Expected %1 in output").arg(expected)));
}

/**
 * @brief Verify hsm:uid attribute is written on an initial state in HSM format.
 *
 * Validates: Requirements 3.1, 3.2, 3.3
 */
void HsmUidSerializationTest::HsmUidWrittenOnInitialState() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto parent = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                      .dynamicCast<model::RegularState>();
    parent->setName("Parent");
    auto initial = model::ModelElementsFactory::createUniqueState(model::StateType::INITIAL, model->idGenerator())
                       .dynamicCast<model::InitialState>();
    auto target = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                      .dynamicCast<model::RegularState>();
    target->setName("Target");

    root->addChildState(parent);
    parent->addChildState(target);
    parent->addChildState(initial);
    auto initTransition = model::ModelElementsFactory::createTransitionWithId(initial, target, "", model->idGenerator().generateNextId());
    initial->setTransition(initTransition);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model, model::SerializationFormat::HSM);

    const QString expected = QString("hsm:uid=\"%1\"").arg(initial->id());
    QVERIFY2(scxml.contains(expected), qPrintable(QString("Expected %1 in output").arg(expected)));
}

/**
 * @brief Verify hsm:uid attribute is written on an include entity in HSM format.
 *
 * Validates: Requirements 3.1, 3.2, 3.3
 */
void HsmUidSerializationTest::HsmUidWrittenOnIncludeEntity() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto include = model::ModelElementsFactory::createUniqueState(model::StateType::INCLUDE, model->idGenerator())
                       .dynamicCast<model::IncludeEntity>();
    include->setName("IncNode");
    include->setPath("sub.scxml");

    root->addChildState(include);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model, model::SerializationFormat::HSM);

    const QString expected = QString("hsm:uid=\"%1\"").arg(include->id());
    QVERIFY2(scxml.contains(expected), qPrintable(QString("Expected %1 in output").arg(expected)));
}

/**
 * @brief Verify hsm:uid attributes are omitted in Qt Creator format output.
 *
 * Validates: Requirements 3.1, 3.2, 3.3 (negative case)
 */
void HsmUidSerializationTest::HsmUidOmittedInQtCreatorFormat() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto s1 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                  .dynamicCast<model::RegularState>();
    s1->setName("S1");
    auto s2 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                  .dynamicCast<model::RegularState>();
    s2->setName("S2");
    root->addChildState(s1);
    root->addChildState(s2);
    auto tr = model::ModelElementsFactory::createTransitionWithId(s1, s2, "go", model->idGenerator().generateNextId());
    s1->addTransition(tr);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model, model::SerializationFormat::QtCreator);

    QVERIFY2(!scxml.contains("hsm:uid"), "hsm:uid should NOT appear in QtCreator format");
    QVERIFY2(!scxml.contains("xmlns:hsm"), "xmlns:hsm should NOT appear in QtCreator format");
}

/**
 * @brief Verify UID values are written as plain decimal integers without leading zeros or whitespace.
 *
 * Validates: Requirements 3.2
 */
void HsmUidSerializationTest::HsmUidWrittenAsDecimalInteger() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto s1 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                  .dynamicCast<model::RegularState>();
    s1->setName("S1");
    auto s2 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                  .dynamicCast<model::RegularState>();
    s2->setName("S2");
    root->addChildState(s1);
    root->addChildState(s2);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model, model::SerializationFormat::HSM);

    // Find all hsm:uid="..." values using regex
    QRegularExpression re("hsm:uid=\"([^\"]*)\"");
    QRegularExpressionMatchIterator iter = re.globalMatch(scxml);

    int count = 0;
    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        const QString value = match.captured(1);
        count++;

        // Must be non-empty
        QVERIFY2(!value.isEmpty(), "UID value should not be empty");
        // Must not have leading zeros (except "0" itself, which is invalid anyway)
        QVERIFY2(!value.startsWith("0"), qPrintable(QString("UID has leading zero: %1").arg(value)));
        // Must not have whitespace
        QVERIFY2(!value.contains(QRegularExpression("\\s")),
                 qPrintable(QString("UID contains whitespace: '%1'").arg(value)));
        // Must be a valid positive integer
        bool ok = false;
        quint64 numericValue = value.toULongLong(&ok);
        QVERIFY2(ok, qPrintable(QString("UID is not a valid number: %1").arg(value)));
        QVERIFY2(numericValue >= 1 && numericValue <= 0xFFFFFFFE,
                 qPrintable(QString("UID out of valid range: %1").arg(value)));
    }

    QVERIFY2(count >= 2, "Expected at least 2 hsm:uid attributes in output");
}

/**
 * @brief Verify that hsm:uid="0" is treated as invalid and entity gets a new valid ID.
 *
 * Validates: Requirements 5.3
 */
void HsmUidSerializationTest::InvalidUidZeroParsedAsNew() {
    const QString scxml = R"(<?xml version="1.0" encoding="UTF-8"?>
<scxml xmlns="http://www.w3.org/2005/07/scxml"
       xmlns:hsm="https://hsm-ide.dev/scxml"
       version="1.0" name="Test">
    <state id="S1" hsm:uid="0"/>
</scxml>)";

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);
    QVERIFY(model);

    auto state = model->root()->findChildStateByName("S1");
    QVERIFY(state);
    QVERIFY2(state->id() != 0, "UID 0 should be regenerated to a valid value");
    QVERIFY2(state->id() != model::INVALID_MODEL_ID, "UID should not be INVALID_MODEL_ID");
}

/**
 * @brief Verify that hsm:uid="4294967295" (0xFFFFFFFF) is treated as invalid.
 *
 * Validates: Requirements 5.3
 */
void HsmUidSerializationTest::InvalidUidMaxParsedAsNew() {
    const QString scxml = R"(<?xml version="1.0" encoding="UTF-8"?>
<scxml xmlns="http://www.w3.org/2005/07/scxml"
       xmlns:hsm="https://hsm-ide.dev/scxml"
       version="1.0" name="Test">
    <state id="S1" hsm:uid="4294967295"/>
</scxml>)";

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);
    QVERIFY(model);

    auto state = model->root()->findChildStateByName("S1");
    QVERIFY(state);
    QVERIFY2(state->id() != model::INVALID_MODEL_ID, "UID 0xFFFFFFFF should be regenerated");
    QVERIFY2(state->id() >= 1 && state->id() <= 0xFFFFFFFE, "UID should be in valid range");
}

/**
 * @brief Verify that non-numeric hsm:uid values are treated as invalid.
 *
 * Validates: Requirements 5.3
 */
void HsmUidSerializationTest::InvalidUidNonNumericParsedAsNew() {
    const QString scxml = R"(<?xml version="1.0" encoding="UTF-8"?>
<scxml xmlns="http://www.w3.org/2005/07/scxml"
       xmlns:hsm="https://hsm-ide.dev/scxml"
       version="1.0" name="Test">
    <state id="S1" hsm:uid="abc"/>
</scxml>)";

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);
    QVERIFY(model);

    auto state = model->root()->findChildStateByName("S1");
    QVERIFY(state);
    QVERIFY2(state->id() != model::INVALID_MODEL_ID, "Non-numeric UID should be regenerated");
    QVERIFY2(state->id() >= 1 && state->id() <= 0xFFFFFFFE, "UID should be in valid range");
}

/**
 * @brief Verify that overflow hsm:uid values (>0xFFFFFFFF) are treated as invalid.
 *
 * Validates: Requirements 5.3
 */
void HsmUidSerializationTest::InvalidUidOverflowParsedAsNew() {
    const QString scxml = R"(<?xml version="1.0" encoding="UTF-8"?>
<scxml xmlns="http://www.w3.org/2005/07/scxml"
       xmlns:hsm="https://hsm-ide.dev/scxml"
       version="1.0" name="Test">
    <state id="S1" hsm:uid="99999999999"/>
</scxml>)";

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);
    QVERIFY(model);

    auto state = model->root()->findChildStateByName("S1");
    QVERIFY(state);
    QVERIFY2(state->id() != model::INVALID_MODEL_ID, "Overflow UID should be regenerated");
    QVERIFY2(state->id() >= 1 && state->id() <= 0xFFFFFFFE, "UID should be in valid range");
}

/**
 * @brief Verify duplicate UIDs: first occurrence preserved, subsequent get new unique IDs.
 *
 * Validates: Requirements 5.4
 */
void HsmUidSerializationTest::DuplicateUidFirstPreservedSubsequentRegenerated() {
    const QString scxml = R"(<?xml version="1.0" encoding="UTF-8"?>
<scxml xmlns="http://www.w3.org/2005/07/scxml"
       xmlns:hsm="https://hsm-ide.dev/scxml"
       version="1.0" name="Test">
    <state id="S1" hsm:uid="42"/>
    <state id="S2" hsm:uid="42"/>
    <state id="S3" hsm:uid="100"/>
</scxml>)";

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);
    QVERIFY(model);

    auto s1 = model->root()->findChildStateByName("S1");
    auto s2 = model->root()->findChildStateByName("S2");
    auto s3 = model->root()->findChildStateByName("S3");
    QVERIFY(s1);
    QVERIFY(s2);
    QVERIFY(s3);

    // First occurrence preserves UID
    QCOMPARE(s1->id(), static_cast<model::EntityID_t>(42));
    // Third state preserves its unique UID
    QCOMPARE(s3->id(), static_cast<model::EntityID_t>(100));
    // Second (duplicate) gets a new unique UID
    QVERIFY2(s2->id() != 42, "Duplicate UID should be regenerated");
    QVERIFY2(s2->id() != 100, "Regenerated UID should not collide with other UIDs");
    QVERIFY2(s2->id() >= 1 && s2->id() <= 0xFFFFFFFE, "Regenerated UID should be in valid range");

    // All UIDs must be unique
    QVERIFY2(s1->id() != s2->id(), "S1 and S2 should have different UIDs");
    QVERIFY2(s1->id() != s3->id(), "S1 and S3 should have different UIDs");
    QVERIFY2(s2->id() != s3->id(), "S2 and S3 should have different UIDs");
}

/**
 * @brief Regression test for duplicate-UID replacement colliding with a later explicit UID.
 *
 * When an early element has a duplicate hsm:uid, the deserializer generates a replacement
 * from the id counter. Because the counter is only fully synchronized at end-of-parse, a
 * replacement issued mid-parse can equal an explicit hsm:uid that appears later in document
 * order. That later element must still receive a unique id rather than silently reusing the
 * value already handed out to the replacement.
 *
 * Scenario (the root reserves uid 1, so we start above it):
 *   S1 uid=10 -> preserved as 10 (advances the id counter to 11)
 *   S2 uid=10 -> duplicate, regenerated; counter yields 11
 *   S3 uid=11 -> equals the replacement given to S2; must itself be regenerated
 *
 * Under the bug S2 and S3 both end up with id 11.
 *
 * Validates: Requirements 5.4, 7.1, 8.3
 */
void HsmUidSerializationTest::DuplicateUidReplacementDoesNotCollideWithLaterUid() {
    const QString scxml = R"(<?xml version="1.0" encoding="UTF-8"?>
<scxml xmlns="http://www.w3.org/2005/07/scxml"
       xmlns:hsm="https://hsm-ide.dev/scxml"
       version="1.0" name="Test">
    <state id="S1" hsm:uid="10"/>
    <state id="S2" hsm:uid="10"/>
    <state id="S3" hsm:uid="11"/>
</scxml>)";

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);
    QVERIFY(model);

    auto s1 = model->root()->findChildStateByName("S1");
    auto s2 = model->root()->findChildStateByName("S2");
    auto s3 = model->root()->findChildStateByName("S3");
    QVERIFY(s1);
    QVERIFY(s2);
    QVERIFY(s3);

    // First occurrence of the duplicate preserves its UID.
    QCOMPARE(s1->id(), static_cast<model::EntityID_t>(10));

    // All three states must have mutually unique IDs; under the bug S2 and S3 collide on 11.
    QVERIFY2(s1->id() != s2->id(),
             qPrintable(QString("S1 (%1) and S2 (%2) must differ").arg(s1->id()).arg(s2->id())));
    QVERIFY2(s1->id() != s3->id(),
             qPrintable(QString("S1 (%1) and S3 (%2) must differ").arg(s1->id()).arg(s3->id())));
    QVERIFY2(s2->id() != s3->id(),
             qPrintable(QString("S2 (%1) and S3 (%2) must differ").arg(s2->id()).arg(s3->id())));

    // Every id is valid.
    for (const auto& s : {s1, s2, s3}) {
        QVERIFY2(s->id() >= 1 && s->id() <= 0xFFFFFFFE, "id must be in valid range");
    }
}

/**
 * @brief Verify round-trip: serialize in HSM format, deserialize, UIDs are identical.
 *
 * Validates: Requirements 5.1, 5.5
 */
void HsmUidSerializationTest::RoundTripSerializeDeserializeUidsIdentical() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto s1 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                  .dynamicCast<model::RegularState>();
    s1->setName("S1");
    auto s2 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                  .dynamicCast<model::RegularState>();
    s2->setName("S2");
    auto history = model::ModelElementsFactory::createUniqueState(model::StateType::HISTORY, model->idGenerator())
                       .dynamicCast<model::HistoryState>();
    history->setName("H");

    root->addChildState(s1);
    root->addChildState(s2);
    s1->addChildState(history);

    auto transition = model::ModelElementsFactory::createTransitionWithId(s1, s2, "go", model->idGenerator().generateNextId());
    s1->addTransition(transition);

    // Record original UIDs
    const model::EntityID_t s1Uid = s1->id();
    const model::EntityID_t s2Uid = s2->id();
    const model::EntityID_t historyUid = history->id();
    const model::EntityID_t transitionUid = transition->id();

    // Serialize
    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model, model::SerializationFormat::HSM);

    // Deserialize
    auto loaded = serializer.deserializeFromScxml(scxml);
    QVERIFY(loaded);

    // Find entities and verify UIDs match
    auto loadedS1 = loaded->root()->findChildStateByName("S1");
    auto loadedS2 = loaded->root()->findChildStateByName("S2");
    QVERIFY(loadedS1);
    QVERIFY(loadedS2);

    QCOMPARE(loadedS1->id(), s1Uid);
    QCOMPARE(loadedS2->id(), s2Uid);

    // Find history state inside S1
    auto loadedHistory = loadedS1->findChildStateByName("H");
    QVERIFY(loadedHistory);
    QCOMPARE(loadedHistory->id(), historyUid);

    // Find transition from S1
    auto loadedS1Regular = loadedS1.dynamicCast<model::RegularState>();
    QVERIFY(loadedS1Regular);
    auto loadedTransition = loadedS1Regular->findTransition(transitionUid);
    QVERIFY(loadedTransition);
    QCOMPARE(loadedTransition->id(), transitionUid);
}

int runHsmUidSerializationTest(int argc, char** argv) {
    HsmUidSerializationTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "HsmUidSerializationTest.moc"
