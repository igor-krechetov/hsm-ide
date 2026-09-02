#include <QtTest>

#include "model/elements/ModelRootState.hpp"
#include "model/elements/RegularState.hpp"
#include "model/StateMachineModel.hpp"
#include "model/StateMachineSerializer.hpp"
#include "model/elements/Transition.hpp"

class FormatDetectionTest : public QObject {
    Q_OBJECT

private slots:
    void DetectHsmFormat();
    void DetectQtCreatorFormat();
    void DetectPlainScxmlFormat();
    void HsmTakesPrecedenceWhenBothNamespacesPresent();
    void UnknownNamespacesIgnored();
};

/**
 * @brief Validate that HSM format is detected when xmlns:hsm is present.
 *
 * Use-case: SCXML file with xmlns:hsm="https://hsm-ide.dev/scxml" should be treated as HSM format,
 * and hsm:uid attributes should be preserved during deserialization.
 *
 * Validates: Requirements 11.1, 11.2
 */
void FormatDetectionTest::DetectHsmFormat() {
    const QString scxml = R"(<?xml version="1.0" encoding="UTF-8"?>
<scxml xmlns="http://www.w3.org/2005/07/scxml"
       xmlns:hsm="https://hsm-ide.dev/scxml"
       version="1.0" name="HsmTest">
    <state id="Idle" hsm:uid="42">
        <transition event="go" target="Running" hsm:uid="100"/>
    </state>
    <state id="Running" hsm:uid="7"/>
</scxml>)";

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    QVERIFY(model);

    // UIDs from hsm:uid attributes should be preserved
    auto idle = model->root()->findChildStateByName("Idle");
    auto running = model->root()->findChildStateByName("Running");

    QVERIFY(idle);
    QVERIFY(running);
    QCOMPARE(idle->id(), static_cast<model::EntityID_t>(42));
    QCOMPARE(running->id(), static_cast<model::EntityID_t>(7));

    // Find the transition and verify its UID
    QSharedPointer<model::Transition> transition;
    idle->forEachChildElement([&transition](QSharedPointer<model::StateMachineEntity> parent,
                                           QSharedPointer<model::StateMachineEntity> child) {
        Q_UNUSED(parent);
        bool keepWalking = true;

        if (child->type() == model::StateMachineEntity::Type::Transition) {
            transition = child.dynamicCast<model::Transition>();
            keepWalking = false;
        }

        return keepWalking;
    });

    QVERIFY(transition);
    QCOMPARE(transition->id(), static_cast<model::EntityID_t>(100));
}

/**
 * @brief Validate that Qt Creator format is detected when xmlns:qt is present without xmlns:hsm.
 *
 * Use-case: SCXML file with xmlns:qt but no xmlns:hsm should be treated as Qt Creator format,
 * and UIDs should be generated (not preserved from file).
 *
 * Validates: Requirements 11.1, 11.3
 */
void FormatDetectionTest::DetectQtCreatorFormat() {
    const QString scxml = R"(<?xml version="1.0" encoding="UTF-8"?>
<scxml xmlns="http://www.w3.org/2005/07/scxml"
       xmlns:qt="http://www.qt.io/2015/02/scxml-ext"
       version="1.0" name="QtTest">
    <state id="StateA">
        <qt:editorinfo scenegeometry="100;200;0;0;120;60"/>
    </state>
    <state id="StateB">
        <qt:editorinfo scenegeometry="300;200;0;0;120;60"/>
    </state>
</scxml>)";

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    QVERIFY(model);

    auto stateA = model->root()->findChildStateByName("StateA");
    auto stateB = model->root()->findChildStateByName("StateB");

    QVERIFY(stateA);
    QVERIFY(stateB);

    // UIDs should be generated (valid and unique, but NOT from file attributes)
    QVERIFY(stateA->id() != model::INVALID_MODEL_ID);
    QVERIFY(stateB->id() != model::INVALID_MODEL_ID);
    QVERIFY(stateA->id() != stateB->id());
}

/**
 * @brief Validate that Plain SCXML format is detected when no editor namespace is present.
 *
 * Use-case: SCXML file with neither xmlns:hsm nor xmlns:qt should be treated as Plain SCXML,
 * and UIDs should be generated with no geometry metadata.
 *
 * Validates: Requirements 11.1, 11.4
 */
void FormatDetectionTest::DetectPlainScxmlFormat() {
    const QString scxml = R"(<?xml version="1.0" encoding="UTF-8"?>
<scxml xmlns="http://www.w3.org/2005/07/scxml"
       version="1.0" name="PlainTest">
    <state id="Alpha">
        <transition event="next" target="Beta"/>
    </state>
    <state id="Beta"/>
</scxml>)";

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    QVERIFY(model);

    auto alpha = model->root()->findChildStateByName("Alpha");
    auto beta = model->root()->findChildStateByName("Beta");

    QVERIFY(alpha);
    QVERIFY(beta);

    // UIDs should be generated (valid and unique)
    QVERIFY(alpha->id() != model::INVALID_MODEL_ID);
    QVERIFY(beta->id() != model::INVALID_MODEL_ID);
    QVERIFY(alpha->id() != beta->id());

    // No geometry metadata should be present
    QVERIFY(!alpha->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).isValid());
    QVERIFY(!alpha->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).isValid());
}

/**
 * @brief Validate that HSM format takes precedence when both xmlns:hsm and xmlns:qt are present.
 *
 * Use-case: A file containing both namespace declarations should be treated as HSM format,
 * preserving hsm:uid attributes and ignoring qt:editorinfo elements.
 *
 * Validates: Requirements 11.1, 11.5
 */
void FormatDetectionTest::HsmTakesPrecedenceWhenBothNamespacesPresent() {
    const QString scxml = R"(<?xml version="1.0" encoding="UTF-8"?>
<scxml xmlns="http://www.w3.org/2005/07/scxml"
       xmlns:hsm="https://hsm-ide.dev/scxml"
       xmlns:qt="http://www.qt.io/2015/02/scxml-ext"
       version="1.0" name="BothTest">
    <state id="Main" hsm:uid="55">
        <qt:editorinfo scenegeometry="100;200;0;0;120;60"/>
        <transition event="done" target="End" hsm:uid="99"/>
    </state>
    <state id="End" hsm:uid="56"/>
</scxml>)";

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    QVERIFY(model);

    auto main = model->root()->findChildStateByName("Main");
    auto end = model->root()->findChildStateByName("End");

    QVERIFY(main);
    QVERIFY(end);

    // HSM UIDs should be preserved (HSM format detected)
    QCOMPARE(main->id(), static_cast<model::EntityID_t>(55));
    QCOMPARE(end->id(), static_cast<model::EntityID_t>(56));

    // Find the transition and verify its UID is preserved
    QSharedPointer<model::Transition> transition;
    main->forEachChildElement([&transition](QSharedPointer<model::StateMachineEntity> parent,
                                           QSharedPointer<model::StateMachineEntity> child) {
        Q_UNUSED(parent);
        bool keepWalking = true;

        if (child->type() == model::StateMachineEntity::Type::Transition) {
            transition = child.dynamicCast<model::Transition>();
            keepWalking = false;
        }

        return keepWalking;
    });

    QVERIFY(transition);
    QCOMPARE(transition->id(), static_cast<model::EntityID_t>(99));
}

/**
 * @brief Validate that unknown namespaces do not affect format detection.
 *
 * Use-case: SCXML file with unknown namespaces (e.g., xmlns:foo) but no xmlns:hsm or xmlns:qt
 * should be treated as Plain SCXML format. Elements/attributes in unknown namespaces are skipped.
 *
 * Validates: Requirements 11.4, 2.5
 */
void FormatDetectionTest::UnknownNamespacesIgnored() {
    const QString scxml = R"(<?xml version="1.0" encoding="UTF-8"?>
<scxml xmlns="http://www.w3.org/2005/07/scxml"
       xmlns:foo="http://example.com/foo"
       xmlns:bar="http://example.com/bar"
       version="1.0" name="UnknownNsTest">
    <state id="First" foo:custom="ignored">
        <foo:node value="abc"/>
        <transition event="advance" target="Second"/>
    </state>
    <state id="Second"/>
</scxml>)";

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    QVERIFY(model);

    auto first = model->root()->findChildStateByName("First");
    auto second = model->root()->findChildStateByName("Second");

    QVERIFY(first);
    QVERIFY(second);

    // UIDs should be generated (Plain SCXML format detected)
    QVERIFY(first->id() != model::INVALID_MODEL_ID);
    QVERIFY(second->id() != model::INVALID_MODEL_ID);
    QVERIFY(first->id() != second->id());

    // No geometry metadata should be present
    QVERIFY(!first->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).isValid());
    QVERIFY(!first->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).isValid());
}

int runFormatDetectionTest(int argc, char** argv) {
    FormatDetectionTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "FormatDetectionTest.moc"
