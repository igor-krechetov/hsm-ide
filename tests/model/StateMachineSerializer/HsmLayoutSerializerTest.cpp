#include <QPolygonF>
#include <QXmlStreamReader>
#include <QtTest>

#include "model/private/EntityIdGenerator.hpp"
#include "model/ModelElementsFactory.hpp"
#include "model/elements/ModelRootState.hpp"
#include "model/elements/RegularState.hpp"
#include "model/StateMachineModel.hpp"
#include "model/StateMachineSerializer.hpp"
#include "model/elements/Transition.hpp"

class HsmLayoutSerializerTest : public QObject {
    Q_OBJECT

private slots:
    void StateLayoutWriteReadRoundTrip();
    void TransitionLayoutWithIntermediatePoints();
    void TransitionLayoutWithLabelPosition();
    void TransitionLayoutWithoutPointsOrLabel();
    void UnmatchedUidInLayoutSkippedWithWarning();
    void MalformedGeometryValuesSkippedWithWarning();
    void HsmEditorIsLastChildOfScxml();
};

/**
 * @brief Test state layout write/read round-trip with 2 decimal precision.
 *
 * Validates: Requirements 12.1, 12.2, 13.1
 */
void HsmLayoutSerializerTest::StateLayoutWriteReadRoundTrip() {
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

    // Set position/size metadata on states
    s1->setMetadata(model::StateMachineEntity::MetadataKey::POSITION_X, 100.55);
    s1->setMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y, 200.33);
    s1->setMetadata(model::StateMachineEntity::MetadataKey::WIDTH, 120.00);
    s1->setMetadata(model::StateMachineEntity::MetadataKey::HEIGHT, 60.10);

    s2->setMetadata(model::StateMachineEntity::MetadataKey::POSITION_X, 400.99);
    s2->setMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y, 100.01);
    s2->setMetadata(model::StateMachineEntity::MetadataKey::WIDTH, 150.00);
    s2->setMetadata(model::StateMachineEntity::MetadataKey::HEIGHT, 80.00);

    // Serialize
    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model, model::SerializationFormat::HSM);

    // Verify hsm:editor section contains hsm:state elements
    QVERIFY(scxml.contains("hsm:editor"));
    QVERIFY(scxml.contains("hsm:layout"));
    QVERIFY(scxml.contains("hsm:state"));

    // Deserialize and verify geometry values are preserved within 2 decimal places
    auto loaded = serializer.deserializeFromScxml(scxml);
    QVERIFY(loaded);

    auto loadedS1 = loaded->root()->findChildStateByName("S1");
    auto loadedS2 = loaded->root()->findChildStateByName("S2");
    QVERIFY(loadedS1);
    QVERIFY(loadedS2);

    // Verify S1 geometry (2 decimal precision)
    QCOMPARE(loadedS1->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).toDouble(), 100.55);
    QCOMPARE(loadedS1->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).toDouble(), 200.33);
    QCOMPARE(loadedS1->getMetadata(model::StateMachineEntity::MetadataKey::WIDTH).toDouble(), 120.00);
    QCOMPARE(loadedS1->getMetadata(model::StateMachineEntity::MetadataKey::HEIGHT).toDouble(), 60.10);

    // Verify S2 geometry
    QCOMPARE(loadedS2->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).toDouble(), 400.99);
    QCOMPARE(loadedS2->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).toDouble(), 100.01);
    QCOMPARE(loadedS2->getMetadata(model::StateMachineEntity::MetadataKey::WIDTH).toDouble(), 150.00);
    QCOMPARE(loadedS2->getMetadata(model::StateMachineEntity::MetadataKey::HEIGHT).toDouble(), 80.00);
}

/**
 * @brief Test transition layout with intermediate points (semicolon-separated pairs).
 *
 * Validates: Requirements 12.4, 12.5, 12.9, 13.3
 */
void HsmLayoutSerializerTest::TransitionLayoutWithIntermediatePoints() {
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

    // Create a polygon with start, 2 intermediate points, and end (4 points total)
    QPolygonF linePath;
    linePath << QPointF(50.00, 100.00);   // start (endpoint - not stored)
    linePath << QPointF(150.25, 100.50);  // intermediate 1
    linePath << QPointF(250.75, 200.33);  // intermediate 2
    linePath << QPointF(350.00, 200.00);  // end (endpoint - not stored)

    transition->setMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY, QVariant::fromValue(linePath));

    // Serialize
    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model, model::SerializationFormat::HSM);

    // Verify hsm:points element is present with intermediate points only
    QVERIFY(scxml.contains("hsm:points"));
    QVERIFY(scxml.contains("150.25,100.50;250.75,200.33"));

    // Verify start and end endpoint coordinates are NOT in the geometry
    QVERIFY(!scxml.contains("50.00,100.00"));
    QVERIFY(!scxml.contains("350.00,200.00"));

    // Deserialize and verify intermediate points restored with placeholder endpoints
    auto loaded = serializer.deserializeFromScxml(scxml);
    QVERIFY(loaded);

    auto loadedS1 = loaded->root()->findChildStateByName("S1");
    QVERIFY(loadedS1);

    QSharedPointer<model::Transition> loadedTransition;
    loadedS1->forEachChildElement(
        [&loadedTransition](QSharedPointer<model::StateMachineEntity> /*parent*/,
                            QSharedPointer<model::StateMachineEntity> child) {
            if (child->type() == model::StateMachineEntity::Type::Transition) {
                loadedTransition = child.dynamicCast<model::Transition>();
                return false;
            }
            return true;
        },
        model::StateMachineEntity::DEPTH_INFINITE,
        false);

    QVERIFY(loadedTransition);
    QVariant geomData = loadedTransition->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY);
    QVERIFY(geomData.isValid());

    QPolygonF loadedPath = geomData.value<QPolygonF>();
    // Should have: placeholder start + 2 intermediate + placeholder end = 4 points
    QCOMPARE(loadedPath.size(), 4);

    // Intermediate points (indices 1 and 2) should match original values
    QCOMPARE(loadedPath.at(1).x(), 150.25);
    QCOMPARE(loadedPath.at(1).y(), 100.50);
    QCOMPARE(loadedPath.at(2).x(), 250.75);
    QCOMPARE(loadedPath.at(2).y(), 200.33);
}

/**
 * @brief Test transition layout with label position.
 *
 * Validates: Requirements 12.5
 */
void HsmLayoutSerializerTest::TransitionLayoutWithLabelPosition() {
    const QString scxml = R"(<?xml version="1.0" encoding="UTF-8"?>
<scxml xmlns="http://www.w3.org/2005/07/scxml"
       xmlns:xi="http://www.w3.org/2001/XInclude"
       xmlns:hsm="https://hsm-ide.dev/scxml"
       version="1.0" name="LabelTest">
    <state id="S1" hsm:uid="10">
        <transition event="go" target="S2" hsm:uid="30"/>
    </state>
    <state id="S2" hsm:uid="20"/>
    <hsm:editor>
        <hsm:layout>
            <hsm:transition uid="30">
                <hsm:points geometry="200.00,150.00;300.00,150.00"/>
                <hsm:label x="250.00" y="130.00"/>
            </hsm:transition>
        </hsm:layout>
    </hsm:editor>
</scxml>)";

    model::StateMachineSerializer serializer;
    auto loaded = serializer.deserializeFromScxml(scxml);

    // Should not crash and model should be valid
    QVERIFY(loaded);

    auto s1 = loaded->root()->findChildStateByName("S1");
    auto s2 = loaded->root()->findChildStateByName("S2");
    QVERIFY(s1);
    QVERIFY(s2);

    // Verify the transition has routing geometry from <hsm:points>
    QSharedPointer<model::Transition> transition;
    s1->forEachChildElement(
        [&transition](QSharedPointer<model::StateMachineEntity> /*parent*/,
                      QSharedPointer<model::StateMachineEntity> child) {
            if (child->type() == model::StateMachineEntity::Type::Transition) {
                transition = child.dynamicCast<model::Transition>();
                return false;
            }
            return true;
        },
        model::StateMachineEntity::DEPTH_INFINITE,
        false);

    QVERIFY(transition);
    QCOMPARE(transition->id(), static_cast<model::EntityID_t>(30));

    QVariant geomData = transition->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY);
    QVERIFY(geomData.isValid());
}

/**
 * @brief Test transition layout without points or label (omitted elements).
 *
 * Validates: Requirements 13.4
 */
void HsmLayoutSerializerTest::TransitionLayoutWithoutPointsOrLabel() {
    const QString scxml = R"(<?xml version="1.0" encoding="UTF-8"?>
<scxml xmlns="http://www.w3.org/2005/07/scxml"
       xmlns:xi="http://www.w3.org/2001/XInclude"
       xmlns:hsm="https://hsm-ide.dev/scxml"
       version="1.0" name="NoPointsTest">
    <state id="A" hsm:uid="10">
        <transition event="x" target="B" hsm:uid="30"/>
    </state>
    <state id="B" hsm:uid="20"/>
    <hsm:editor>
        <hsm:layout>
            <hsm:transition uid="30"/>
        </hsm:layout>
    </hsm:editor>
</scxml>)";

    model::StateMachineSerializer serializer;
    auto loaded = serializer.deserializeFromScxml(scxml);

    QVERIFY(loaded);

    auto a = loaded->root()->findChildStateByName("A");
    QVERIFY(a);

    QSharedPointer<model::Transition> transition;
    a->forEachChildElement(
        [&transition](QSharedPointer<model::StateMachineEntity> /*parent*/,
                      QSharedPointer<model::StateMachineEntity> child) {
            if (child->type() == model::StateMachineEntity::Type::Transition) {
                transition = child.dynamicCast<model::Transition>();
                return false;
            }
            return true;
        },
        model::StateMachineEntity::DEPTH_INFINITE,
        false);

    QVERIFY(transition);
    QCOMPARE(transition->id(), static_cast<model::EntityID_t>(30));

    // No GEOMETRY metadata should be set (no points element)
    QVariant geomData = transition->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY);
    QVERIFY(!geomData.isValid());
}

/**
 * @brief Test that unmatched UID in layout is skipped with warning.
 *
 * Validates: Requirements 13.7
 */
void HsmLayoutSerializerTest::UnmatchedUidInLayoutSkippedWithWarning() {
    const QString scxml = R"(<?xml version="1.0" encoding="UTF-8"?>
<scxml xmlns="http://www.w3.org/2005/07/scxml"
       xmlns:xi="http://www.w3.org/2001/XInclude"
       xmlns:hsm="https://hsm-ide.dev/scxml"
       version="1.0" name="UnmatchedTest">
    <state id="Existing" hsm:uid="10"/>
    <hsm:editor>
        <hsm:layout>
            <hsm:state uid="999" x="100.00" y="200.00" width="120.00" height="60.00"/>
            <hsm:state uid="10" x="50.00" y="75.00" width="80.00" height="40.00"/>
        </hsm:layout>
    </hsm:editor>
</scxml>)";

    model::StateMachineSerializer serializer;
    auto loaded = serializer.deserializeFromScxml(scxml);

    // Should not crash, model should be valid
    QVERIFY(loaded);

    auto existing = loaded->root()->findChildStateByName("Existing");
    QVERIFY(existing);
    // UID 1 is reserved by the model root, so fixtures use 10 for the child.
    QCOMPARE(existing->id(), static_cast<model::EntityID_t>(10));

    // The valid entry (uid=1) should still have geometry applied
    QCOMPARE(existing->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).toDouble(), 50.00);
    QCOMPARE(existing->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).toDouble(), 75.00);
    QCOMPARE(existing->getMetadata(model::StateMachineEntity::MetadataKey::WIDTH).toDouble(), 80.00);
    QCOMPARE(existing->getMetadata(model::StateMachineEntity::MetadataKey::HEIGHT).toDouble(), 40.00);
}

/**
 * @brief Test that malformed geometry values are skipped with warning.
 *
 * Validates: Requirements 13.8, 13.9
 */
void HsmLayoutSerializerTest::MalformedGeometryValuesSkippedWithWarning() {
    const QString scxml = R"(<?xml version="1.0" encoding="UTF-8"?>
<scxml xmlns="http://www.w3.org/2005/07/scxml"
       xmlns:xi="http://www.w3.org/2001/XInclude"
       xmlns:hsm="https://hsm-ide.dev/scxml"
       version="1.0" name="MalformedTest">
    <state id="Good" hsm:uid="10"/>
    <state id="Bad" hsm:uid="11"/>
    <state id="BadTrans" hsm:uid="12">
        <transition event="go" target="Good" hsm:uid="13"/>
    </state>
    <hsm:editor>
        <hsm:layout>
            <hsm:state uid="10" x="100.00" y="200.00" width="120.00" height="60.00"/>
            <hsm:state uid="11" x="abc" y="200.00" width="120.00" height="60.00"/>
            <hsm:transition uid="13">
                <hsm:points geometry="not;valid;data"/>
            </hsm:transition>
        </hsm:layout>
    </hsm:editor>
</scxml>)";

    model::StateMachineSerializer serializer;
    auto loaded = serializer.deserializeFromScxml(scxml);

    // Should not crash
    QVERIFY(loaded);

    auto good = loaded->root()->findChildStateByName("Good");
    auto bad = loaded->root()->findChildStateByName("Bad");
    auto badTrans = loaded->root()->findChildStateByName("BadTrans");
    QVERIFY(good);
    QVERIFY(bad);
    QVERIFY(badTrans);

    // Good state should have geometry applied
    QCOMPARE(good->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).toDouble(), 100.00);
    QCOMPARE(good->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).toDouble(), 200.00);

    // Bad state should NOT have geometry applied (malformed x value)
    QVERIFY(!bad->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).isValid());
    QVERIFY(!bad->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).isValid());

    // Transition with malformed points should NOT have GEOMETRY metadata
    QSharedPointer<model::Transition> transition;
    badTrans->forEachChildElement(
        [&transition](QSharedPointer<model::StateMachineEntity> /*parent*/,
                      QSharedPointer<model::StateMachineEntity> child) {
            if (child->type() == model::StateMachineEntity::Type::Transition) {
                transition = child.dynamicCast<model::Transition>();
                return false;
            }
            return true;
        },
        model::StateMachineEntity::DEPTH_INFINITE,
        false);

    QVERIFY(transition);
    QVariant geomData = transition->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY);
    QVERIFY(!geomData.isValid());
}

/**
 * @brief Test that hsm:editor is the last child element of scxml.
 *
 * Validates: Requirements 3.6, 12.1
 */
void HsmLayoutSerializerTest::HsmEditorIsLastChildOfScxml() {
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

    s1->setMetadata(model::StateMachineEntity::MetadataKey::POSITION_X, 100.0);
    s1->setMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y, 200.0);
    s1->setMetadata(model::StateMachineEntity::MetadataKey::WIDTH, 120.0);
    s1->setMetadata(model::StateMachineEntity::MetadataKey::HEIGHT, 60.0);

    // Serialize
    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model, model::SerializationFormat::HSM);

    // Parse the XML to find the last child element of <scxml>
    QXmlStreamReader reader(scxml);
    QString lastChildElementName;

    // Navigate to the root <scxml> element
    bool inScxml = false;
    int depth = 0;

    while (!reader.atEnd()) {
        reader.readNext();

        if (reader.isStartElement()) {
            if (!inScxml && reader.name() == QStringView(u"scxml")) {
                inScxml = true;
                depth = 1;
            } else if (inScxml) {
                depth++;
                if (depth == 2) {
                    // Direct child of <scxml>
                    lastChildElementName = reader.qualifiedName().toString();
                }
            }
        } else if (reader.isEndElement() && inScxml) {
            depth--;
            if (depth == 0) {
                break;  // End of <scxml>
            }
        }
    }

    QVERIFY2(!reader.hasError(), qPrintable(reader.errorString()));
    QCOMPARE(lastChildElementName, QString("hsm:editor"));
}

int runHsmLayoutSerializerTest(int argc, char** argv) {
    HsmLayoutSerializerTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "HsmLayoutSerializerTest.moc"
