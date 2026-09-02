#include <QtTest>

#include "model/private/EntityIdGenerator.hpp"
#include "model/elements/EntryPoint.hpp"
#include "model/elements/ExitPoint.hpp"
#include "model/elements/HistoryState.hpp"
#include "model/elements/IncludeEntity.hpp"
#include "model/ModelElementsFactory.hpp"
#include "model/elements/ModelRootState.hpp"
#include "model/elements/RegularState.hpp"
#include "model/StateMachineModel.hpp"
#include "model/StateMachineSerializer.hpp"
#include "model/elements/Transition.hpp"

class StateMachineSerializerSerializationTest : public QObject {
    Q_OBJECT

private slots:
    void SerializeDeepHistory();
    void SerializeShallowHistory();
    void SerializeSubstatesHierarchy();
    void SerializeMultipleTransitionsForSingleState();
    void SerializeExternalAndInternalTransitions();
    void SerializeEntryPointTransitions();
    void SerializeExitPointAndTargetingTransition();
    void SerializeIncludeEntity();
    void SerializeWithoutScxmlWrapper();
    void RoundTripNestedStatePositionStability();
    void RoundTripMultiLevelNestingPositionStability();
    void RoundTripMultipleCyclesNoDrift();
    void RoundTripRootLevelStatePositionPreserved();
    void SerializationWritesSceneGeometryAttribute();
    void SerializeOnStateChangedActionAsInvokeAttribute();
};

/**
 * @brief Validate serialization of deep history with default transition.
 *
 * Use-case: Persist deep-history semantics to SCXML.
 */
void StateMachineSerializerSerializationTest::SerializeDeepHistory() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto s1 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                  .dynamicCast<model::RegularState>();
    s1->setName("S1");
    auto history = model::ModelElementsFactory::createUniqueState(model::StateType::HISTORY, model->idGenerator())
                       .dynamicCast<model::HistoryState>();
    history->setProperty(model::HistoryState::cKeyHistoryType, static_cast<int>(model::HistoryType::DEEP));

    root->addChildState(s1);
    s1->addChildState(history);
    auto tr = model::ModelElementsFactory::createTransitionWithId(history, s1, "resume", model->idGenerator().generateNextId());
    history->setDefaultTransition(tr);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model);

    QVERIFY(scxml.contains("<history id=\"H\" type=\"deep\"") || scxml.contains("type=\"deep\""));
    QVERIFY(scxml.contains("event=\"resume\""));
}

/**
 * @brief Validate serialization of shallow history.
 *
 * Use-case: Persist shallow history variant to SCXML.
 */
void StateMachineSerializerSerializationTest::SerializeShallowHistory() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto s1 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                  .dynamicCast<model::RegularState>();
    s1->setName("S1");
    auto history = model::ModelElementsFactory::createUniqueState(model::StateType::HISTORY, model->idGenerator())
                       .dynamicCast<model::HistoryState>();

    root->addChildState(s1);
    s1->addChildState(history);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model);

    QVERIFY(scxml.contains("type=\"shallow\""));
}

/**
 * @brief Validate serialization of nested substates.
 *
 * Use-case: Persist parent-child hierarchy for regular states.
 */
void StateMachineSerializerSerializationTest::SerializeSubstatesHierarchy() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto parent = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                      .dynamicCast<model::RegularState>();
    parent->setName("Parent");
    auto child = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                     .dynamicCast<model::RegularState>();
    child->setName("Child");

    root->addChildState(parent);
    parent->addChildState(child);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model);

    QVERIFY(scxml.contains("<state id=\"Parent\""));
    QVERIFY(scxml.contains("<state id=\"Child\""));
}

/**
 * @brief Validate serialization of multiple transitions on one state.
 *
 * Use-case: Persist multiple outgoing edges from a single source state.
 */
void StateMachineSerializerSerializationTest::SerializeMultipleTransitionsForSingleState() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto s1 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                  .dynamicCast<model::RegularState>();
    s1->setName("S1");
    auto s2 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                  .dynamicCast<model::RegularState>();
    s2->setName("S2");
    auto s3 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                  .dynamicCast<model::RegularState>();
    s3->setName("S3");

    root->addChildState(s1);
    root->addChildState(s2);
    root->addChildState(s3);
    s1->addTransition(model::ModelElementsFactory::createTransitionWithId(s1, s2, "e1", model->idGenerator().generateNextId()));
    s1->addTransition(model::ModelElementsFactory::createTransitionWithId(s1, s3, "e2", model->idGenerator().generateNextId()));

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model);

    QVERIFY(scxml.contains("event=\"e1\""));
    QVERIFY(scxml.contains("event=\"e2\""));
}

/**
 * @brief Validate serialization of external/internal transition types.
 *
 * Use-case: Persist explicit SCXML transition type attributes.
 */
void StateMachineSerializerSerializationTest::SerializeExternalAndInternalTransitions() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto s1 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                  .dynamicCast<model::RegularState>();
    s1->setName("S1");
    auto s2 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                  .dynamicCast<model::RegularState>();
    s2->setName("S2");
    auto s3 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                  .dynamicCast<model::RegularState>();
    s3->setName("S3");

    auto ext = model::ModelElementsFactory::createTransitionWithId(s1, s2, "ext", model->idGenerator().generateNextId());
    auto in = model::ModelElementsFactory::createTransitionWithId(s1, s3, "in", model->idGenerator().generateNextId());
    in->setTransitionType(model::TransitionType::INTERNAL);

    root->addChildState(s1);
    root->addChildState(s2);
    root->addChildState(s3);
    s1->addTransition(ext);
    s1->addTransition(in);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model);

    QVERIFY(scxml.contains("event=\"ext\" type=\"external\""));
    QVERIFY(scxml.contains("event=\"in\" type=\"internal\""));
}

/**
 * @brief Validate serialization of entry-point equivalent with multiple transitions.
 *
 * Use-case: Persist entrypoint as `<initial>` with multiple transitions.
 */
void StateMachineSerializerSerializationTest::SerializeEntryPointTransitions() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto region = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                      .dynamicCast<model::RegularState>();
    region->setName("Region");
    auto a = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                 .dynamicCast<model::RegularState>();
    a->setName("A");
    auto b = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                 .dynamicCast<model::RegularState>();
    b->setName("B");
    auto entry = model::ModelElementsFactory::createUniqueState(model::StateType::ENTRYPOINT, model->idGenerator())
                     .dynamicCast<model::EntryPoint>();

    entry->addTransition(model::ModelElementsFactory::createTransitionWithId(entry, a, "", model->idGenerator().generateNextId()));
    entry->addTransition(model::ModelElementsFactory::createTransitionWithId(entry, b, "", model->idGenerator().generateNextId()));

    root->addChildState(region);
    region->addChildState(a);
    region->addChildState(b);
    region->addChildState(entry);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model);

    QVERIFY(scxml.contains("<initial"));
    QVERIFY(scxml.contains("target=\"A\""));
    QVERIFY(scxml.contains("target=\"B\""));
    QVERIFY(scxml.contains("event=\"toA\"") == false);
    QVERIFY(scxml.contains("event=\"toB\"") == false);
}

/**
 * @brief Validate serialization of exit point and transition targeting it.
 *
 * Use-case: Persist nested exitpoint as `<final event=...>` and transition target id.
 */
void StateMachineSerializerSerializationTest::SerializeExitPointAndTargetingTransition() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto parent = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                      .dynamicCast<model::RegularState>();
    parent->setName("Parent");
    auto worker = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                      .dynamicCast<model::RegularState>();
    worker->setName("Worker");
    auto xp = model::ModelElementsFactory::createUniqueState(model::StateType::EXITPOINT, model->idGenerator())
                  .dynamicCast<model::ExitPoint>();
    xp->setName("XP");
    xp->setEvent("leave");

    root->addChildState(parent);
    parent->addChildState(worker);
    parent->addChildState(xp);
    worker->addTransition(model::ModelElementsFactory::createTransitionWithId(worker, xp, "done", model->idGenerator().generateNextId()));

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model);

    QVERIFY(scxml.contains("<final"));
    QVERIFY(scxml.contains("id=\"XP\""));
    QVERIFY(scxml.contains("event=\"leave\""));
    QVERIFY(scxml.contains("event=\"done\""));
    QVERIFY(scxml.contains("target=\"XP\""));
}

/**
 * @brief Validate serialization of include entity to xi:include.
 *
 * Use-case: Persist include path with xi:include tag.
 */
void StateMachineSerializerSerializationTest::SerializeIncludeEntity() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto include = model::ModelElementsFactory::createUniqueState(model::StateType::INCLUDE, model->idGenerator())
                       .dynamicCast<model::IncludeEntity>();
    include->setName("IncludeNode");
    include->setPath("subchart.scxml");
    root->addChildState(include);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model);

    QVERIFY(scxml.contains("<xi:include href=\"subchart.scxml\" parse=\"xml\""));
}

/**
 * @brief Validate serialization without wrapping root <scxml> tag.
 */
void StateMachineSerializerSerializationTest::SerializeWithoutScxmlWrapper() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto state = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                     .dynamicCast<model::RegularState>();
    state->setName("StateA");

    root->addChildState(state);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model, model::SerializationFormat::HSM, false);

    QVERIFY(scxml.contains("<state id=\"StateA\""));
    QVERIFY(scxml.contains("<scxml") == false);
    QVERIFY(scxml.startsWith("<?xml") == false);
}

/**
 * @brief Validate round-trip serialization/deserialization preserves nested state positions.
 *
 * Validates: Requirements 1.1, 2.1
 */
void StateMachineSerializerSerializationTest::RoundTripNestedStatePositionStability() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto parent = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                      .dynamicCast<model::RegularState>();
    parent->setName("Parent");
    auto child = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                     .dynamicCast<model::RegularState>();
    child->setName("Child");

    parent->setPos(QPointF(100.0, 100.0));
    parent->setSize(QSizeF(200.0, 200.0));
    child->setPos(QPointF(20.0, 30.0));
    child->setSize(QSizeF(80.0, 60.0));

    root->addChildState(parent);
    parent->addChildState(child);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model, model::SerializationFormat::QtCreator);

    auto deserialized = serializer.deserializeFromScxml(scxml);
    QVERIFY(deserialized);

    auto deserializedChild = deserialized->root()->findChildStateByName("Child");
    QVERIFY(deserializedChild);

    const double childX = deserializedChild->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).toDouble();
    const double childY = deserializedChild->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).toDouble();
    const double childW = deserializedChild->getMetadata(model::StateMachineEntity::MetadataKey::WIDTH).toDouble();
    const double childH = deserializedChild->getMetadata(model::StateMachineEntity::MetadataKey::HEIGHT).toDouble();

    QVERIFY2(qAbs(childX - 20.0) < 0.01,
             qPrintable(QString("Child X: expected 20.0, got %1").arg(childX)));
    QVERIFY2(qAbs(childY - 30.0) < 0.01,
             qPrintable(QString("Child Y: expected 30.0, got %1").arg(childY)));
    QVERIFY2(qAbs(childW - 80.0) < 0.01,
             qPrintable(QString("Child W: expected 80.0, got %1").arg(childW)));
    QVERIFY2(qAbs(childH - 60.0) < 0.01,
             qPrintable(QString("Child H: expected 60.0, got %1").arg(childH)));
}

/**
 * @brief Validate round-trip for 3-level nesting: Root -> Parent -> Child.
 *
 * Validates: Requirements 1.1, 2.1
 */
void StateMachineSerializerSerializationTest::RoundTripMultiLevelNestingPositionStability() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto stateA = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                      .dynamicCast<model::RegularState>();
    stateA->setName("StateA");
    auto stateB = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                      .dynamicCast<model::RegularState>();
    stateB->setName("StateB");
    auto stateC = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                      .dynamicCast<model::RegularState>();
    stateC->setName("StateC");

    stateA->setPos(QPointF(50.0, 60.0));
    stateA->setSize(QSizeF(400.0, 400.0));
    stateB->setPos(QPointF(30.0, 40.0));
    stateB->setSize(QSizeF(300.0, 300.0));
    stateC->setPos(QPointF(10.0, 15.0));
    stateC->setSize(QSizeF(100.0, 80.0));

    root->addChildState(stateA);
    stateA->addChildState(stateB);
    stateB->addChildState(stateC);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model, model::SerializationFormat::QtCreator);

    auto deserialized = serializer.deserializeFromScxml(scxml);
    QVERIFY(deserialized);

    auto dStateA = deserialized->root()->findChildStateByName("StateA");
    auto dStateB = deserialized->root()->findChildStateByName("StateB");
    auto dStateC = deserialized->root()->findChildStateByName("StateC");

    QVERIFY(dStateA);
    QVERIFY(dStateB);
    QVERIFY(dStateC);

    const double aX = dStateA->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).toDouble();
    const double aY = dStateA->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).toDouble();
    QVERIFY2(qAbs(aX - 50.0) < 0.01,
             qPrintable(QString("StateA X: expected 50.0, got %1").arg(aX)));
    QVERIFY2(qAbs(aY - 60.0) < 0.01,
             qPrintable(QString("StateA Y: expected 60.0, got %1").arg(aY)));

    const double bX = dStateB->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).toDouble();
    const double bY = dStateB->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).toDouble();
    QVERIFY2(qAbs(bX - 30.0) < 0.01,
             qPrintable(QString("StateB X: expected 30.0, got %1").arg(bX)));
    QVERIFY2(qAbs(bY - 40.0) < 0.01,
             qPrintable(QString("StateB Y: expected 40.0, got %1").arg(bY)));

    const double cX = dStateC->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).toDouble();
    const double cY = dStateC->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).toDouble();
    QVERIFY2(qAbs(cX - 10.0) < 0.01,
             qPrintable(QString("StateC X: expected 10.0, got %1").arg(cX)));
    QVERIFY2(qAbs(cY - 15.0) < 0.01,
             qPrintable(QString("StateC Y: expected 15.0, got %1").arg(cY)));
}

/**
 * @brief Validate that multiple serialize/deserialize cycles do not accumulate position drift.
 *
 * Validates: Requirements 1.2, 2.2
 */
void StateMachineSerializerSerializationTest::RoundTripMultipleCyclesNoDrift() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto parent = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                      .dynamicCast<model::RegularState>();
    parent->setName("Parent");
    auto child = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                     .dynamicCast<model::RegularState>();
    child->setName("Child");

    parent->setPos(QPointF(150.0, 200.0));
    parent->setSize(QSizeF(300.0, 300.0));
    child->setPos(QPointF(25.0, 35.0));
    child->setSize(QSizeF(100.0, 80.0));

    root->addChildState(parent);
    parent->addChildState(child);

    model::StateMachineSerializer serializer;
    QSharedPointer<model::StateMachineModel> currentModel = model;

    // Perform 3 serialize/deserialize cycles
    for (int cycle = 0; cycle < 3; ++cycle) {
        const QString scxml = serializer.serializeToScxml(currentModel, model::SerializationFormat::QtCreator);
        currentModel = serializer.deserializeFromScxml(scxml);
        QVERIFY2(currentModel != nullptr,
                 qPrintable(QString("Deserialization failed on cycle %1").arg(cycle + 1)));
    }

    auto dChild = currentModel->root()->findChildStateByName("Child");
    QVERIFY(dChild);

    const double childX = dChild->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).toDouble();
    const double childY = dChild->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).toDouble();
    const double childW = dChild->getMetadata(model::StateMachineEntity::MetadataKey::WIDTH).toDouble();
    const double childH = dChild->getMetadata(model::StateMachineEntity::MetadataKey::HEIGHT).toDouble();

    QVERIFY2(qAbs(childX - 25.0) < 0.01,
             qPrintable(QString("After 3 cycles, Child X: expected 25.0, got %1").arg(childX)));
    QVERIFY2(qAbs(childY - 35.0) < 0.01,
             qPrintable(QString("After 3 cycles, Child Y: expected 35.0, got %1").arg(childY)));
    QVERIFY2(qAbs(childW - 100.0) < 0.01,
             qPrintable(QString("After 3 cycles, Child W: expected 100.0, got %1").arg(childW)));
    QVERIFY2(qAbs(childH - 80.0) < 0.01,
             qPrintable(QString("After 3 cycles, Child H: expected 80.0, got %1").arg(childH)));
}

/**
 * @brief Validate round-trip serialization/deserialization preserves root-level state positions.
 *
 * Validates: Requirements 3.3
 */
void StateMachineSerializerSerializationTest::RoundTripRootLevelStatePositionPreserved() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto stateA = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                      .dynamicCast<model::RegularState>();
    stateA->setName("StateA");
    auto stateB = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                      .dynamicCast<model::RegularState>();
    stateB->setName("StateB");
    auto stateC = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                      .dynamicCast<model::RegularState>();
    stateC->setName("StateC");

    stateA->setPos(QPointF(200.0, 300.0));
    stateA->setSize(QSizeF(120.0, 80.0));
    stateB->setPos(QPointF(-50.0, 100.0));
    stateB->setSize(QSizeF(150.0, 90.0));
    stateC->setPos(QPointF(0.0, 0.0));
    stateC->setSize(QSizeF(60.0, 40.0));

    root->addChildState(stateA);
    root->addChildState(stateB);
    root->addChildState(stateC);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model, model::SerializationFormat::QtCreator);

    auto deserialized = serializer.deserializeFromScxml(scxml);
    QVERIFY(deserialized);

    auto dStateA = deserialized->root()->findChildStateByName("StateA");
    auto dStateB = deserialized->root()->findChildStateByName("StateB");
    auto dStateC = deserialized->root()->findChildStateByName("StateC");

    QVERIFY(dStateA);
    QVERIFY(dStateB);
    QVERIFY(dStateC);

    const double aX = dStateA->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).toDouble();
    const double aY = dStateA->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).toDouble();
    const double aW = dStateA->getMetadata(model::StateMachineEntity::MetadataKey::WIDTH).toDouble();
    const double aH = dStateA->getMetadata(model::StateMachineEntity::MetadataKey::HEIGHT).toDouble();
    QVERIFY2(qAbs(aX - 200.0) < 0.01,
             qPrintable(QString("StateA X: expected 200.0, got %1").arg(aX)));
    QVERIFY2(qAbs(aY - 300.0) < 0.01,
             qPrintable(QString("StateA Y: expected 300.0, got %1").arg(aY)));
    QVERIFY2(qAbs(aW - 120.0) < 0.01,
             qPrintable(QString("StateA W: expected 120.0, got %1").arg(aW)));
    QVERIFY2(qAbs(aH - 80.0) < 0.01,
             qPrintable(QString("StateA H: expected 80.0, got %1").arg(aH)));

    const double bX = dStateB->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).toDouble();
    const double bY = dStateB->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).toDouble();
    const double bW = dStateB->getMetadata(model::StateMachineEntity::MetadataKey::WIDTH).toDouble();
    const double bH = dStateB->getMetadata(model::StateMachineEntity::MetadataKey::HEIGHT).toDouble();
    QVERIFY2(qAbs(bX - (-50.0)) < 0.01,
             qPrintable(QString("StateB X: expected -50.0, got %1").arg(bX)));
    QVERIFY2(qAbs(bY - 100.0) < 0.01,
             qPrintable(QString("StateB Y: expected 100.0, got %1").arg(bY)));
    QVERIFY2(qAbs(bW - 150.0) < 0.01,
             qPrintable(QString("StateB W: expected 150.0, got %1").arg(bW)));
    QVERIFY2(qAbs(bH - 90.0) < 0.01,
             qPrintable(QString("StateB H: expected 90.0, got %1").arg(bH)));

    const double cX = dStateC->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).toDouble();
    const double cY = dStateC->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).toDouble();
    const double cW = dStateC->getMetadata(model::StateMachineEntity::MetadataKey::WIDTH).toDouble();
    const double cH = dStateC->getMetadata(model::StateMachineEntity::MetadataKey::HEIGHT).toDouble();
    QVERIFY2(qAbs(cX - 0.0) < 0.01,
             qPrintable(QString("StateC X: expected 0.0, got %1").arg(cX)));
    QVERIFY2(qAbs(cY - 0.0) < 0.01,
             qPrintable(QString("StateC Y: expected 0.0, got %1").arg(cY)));
    QVERIFY2(qAbs(cW - 60.0) < 0.01,
             qPrintable(QString("StateC W: expected 60.0, got %1").arg(cW)));
    QVERIFY2(qAbs(cH - 40.0) < 0.01,
             qPrintable(QString("StateC H: expected 40.0, got %1").arg(cH)));
}

/**
 * @brief Baseline observation: verify what geometry attribute format the serializer currently writes.
 *
 * Validates: Requirements 2.3
 */
void StateMachineSerializerSerializationTest::SerializationWritesSceneGeometryAttribute() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto state = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                     .dynamicCast<model::RegularState>();
    state->setName("TestState");

    state->setPos(QPointF(100.0, 200.0));
    state->setSize(QSizeF(150.0, 100.0));

    root->addChildState(state);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model, model::SerializationFormat::QtCreator);

    // After fix: `scenegeometry` attribute is used with absolute coordinates
    QVERIFY2(scxml.contains("scenegeometry="), "Expected serialized output to contain scenegeometry attribute");
    // Ensure no standalone `geometry` attribute (only `scenegeometry` should be present)
    const int sceneGeoCount = scxml.count("scenegeometry=");
    const int geoCount = scxml.count("geometry=");
    QCOMPARE(sceneGeoCount, geoCount);  // all occurrences of "geometry=" should be within "scenegeometry="
    QVERIFY2(scxml.contains("qt:editorinfo"), "Expected serialized output to contain qt:editorinfo element");
}

/**
 * @brief Verify onStateChanged action serializes as <invoke srcexpr="..."/> attribute form,
 *        not as a nested <srcexpr> child element.
 */
void StateMachineSerializerSerializationTest::SerializeOnStateChangedActionAsInvokeAttribute() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto state = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                     .dynamicCast<model::RegularState>();
    state->setName("S1");
    state->setProperty("onStateChangedAction", "handleLcmInit");
    root->addChildState(state);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model);

    // Attribute form expected
    QVERIFY2(scxml.contains("<invoke srcexpr=\"handleLcmInit\"/>"),
             qPrintable(QString("Expected attribute-form invoke element, got:\n%1").arg(scxml)));
    // Must NOT use nested child element form
    QVERIFY2(!scxml.contains("<srcexpr>"),
             "invoke must not serialize srcexpr as a child element");

    // Round-trip: deserialize and confirm the action is preserved
    auto loaded = serializer.deserializeFromScxml(scxml);
    QVERIFY(loaded);
    auto loadedState = loaded->root()->findChildStateByName("S1").dynamicCast<model::RegularState>();
    QVERIFY(loadedState);
    QVERIFY(loadedState->hasOnStateChangedAction());
    QCOMPARE(loadedState->onStateChangedAction()->serialize(), QString("handleLcmInit"));
}

int runStateMachineSerializerSerializationTest(int argc, char** argv) {
    StateMachineSerializerSerializationTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "StateMachineSerializerSerializationTest.moc"
