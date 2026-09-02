#include <QtTest>

#include "../TestPaths.hpp"
#include "model/elements/EntryPoint.hpp"
#include "model/elements/InitialState.hpp"
#include "model/elements/ModelRootState.hpp"
#include "model/elements/RegularState.hpp"
#include "model/elements/Transition.hpp"
#include "model/StateMachineModel.hpp"
#include "model/StateMachineSerializer.hpp"

class QtGeometryDeserializationTest : public QObject {
    Q_OBJECT

private slots:
    void TopLevelStateGeometry();
    void NestedStateLocalPosition();
    void InitialStateGeometryInsideParent();
    void AllStatesHaveWidthAndHeight();
    void TransitionExistsBetweenStates();
    void RootIdNotReusedByChildren();
    void TopLevelInitialStateFromAttribute();

private:
    QSharedPointer<model::StateMachineModel> loadFixture();
};

QSharedPointer<model::StateMachineModel> QtGeometryDeserializationTest::loadFixture() {
    const QString scxml = test::loadScxmlFixture("qt_compatibility/qt_geometry_02.scxml");
    QSharedPointer<model::StateMachineModel> result;

    if (!scxml.isEmpty()) {
        // Simulate the app flow: create model first (as ProjectController does),
        // then deserialize into it (which calls clearModel + re-parse).
        result = QSharedPointer<model::StateMachineModel>::create("ExistingProject");
        model::StateMachineSerializer serializer;
        if (!serializer.deserializeFromScxml(scxml, result)) {
            result.clear();
        }
    }

    return result;
}

/**
 * @brief Verify top-level states get correct position and size.
 *
 * state_1: sceneTopLeft=(0,0), size=(400,300)
 *   Expected: posX=0, posY=0, w=400, h=300
 *
 * state_2: sceneTopLeft=(500,0), size=(300,200)
 *   Expected: posX=500, posY=0, w=300, h=200
 */
void QtGeometryDeserializationTest::TopLevelStateGeometry() {
    auto model = loadFixture();
    QVERIFY(model);

    auto state1 = model->root()->findChildStateByName("state_1").dynamicCast<model::RegularState>();
    QVERIFY(state1);
    QCOMPARE(state1->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).toDouble(), 0.0);
    QCOMPARE(state1->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).toDouble(), 0.0);
    QCOMPARE(state1->getMetadata(model::StateMachineEntity::MetadataKey::WIDTH).toDouble(), 400.0);
    QCOMPARE(state1->getMetadata(model::StateMachineEntity::MetadataKey::HEIGHT).toDouble(), 300.0);

    auto state2 = model->root()->findChildStateByName("state_2").dynamicCast<model::RegularState>();
    QVERIFY(state2);
    QCOMPARE(state2->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).toDouble(), 500.0);
    QCOMPARE(state2->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).toDouble(), 0.0);
    QCOMPARE(state2->getMetadata(model::StateMachineEntity::MetadataKey::WIDTH).toDouble(), 300.0);
    QCOMPARE(state2->getMetadata(model::StateMachineEntity::MetadataKey::HEIGHT).toDouble(), 200.0);
}

/**
 * @brief Verify nested states have local position = sceneTopLeft - parent sceneTopLeft.
 *
 * state_1_1 (child of state_1):
 *   sceneTopLeft=(220,180), parent sceneTopLeft=(0,0)
 *   Expected local: posX=220, posY=180, w=160, h=80
 *
 * state_2_1 (child of state_2):
 *   sceneTopLeft=(640,100), parent sceneTopLeft=(500,0)
 *   Expected local: posX=140, posY=100, w=180, h=100
 */
void QtGeometryDeserializationTest::NestedStateLocalPosition() {
    auto model = loadFixture();
    QVERIFY(model);

    auto state1 = model->root()->findChildStateByName("state_1").dynamicCast<model::RegularState>();
    QVERIFY(state1);
    auto state1_1 = state1->findChildStateByName("state_1_1").dynamicCast<model::RegularState>();
    QVERIFY(state1_1);

    QCOMPARE(state1_1->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).toDouble(), 220.0);
    QCOMPARE(state1_1->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).toDouble(), 180.0);
    QCOMPARE(state1_1->getMetadata(model::StateMachineEntity::MetadataKey::WIDTH).toDouble(), 160.0);
    QCOMPARE(state1_1->getMetadata(model::StateMachineEntity::MetadataKey::HEIGHT).toDouble(), 80.0);

    auto state2 = model->root()->findChildStateByName("state_2").dynamicCast<model::RegularState>();
    QVERIFY(state2);
    auto state2_1 = state2->findChildStateByName("state_2_1").dynamicCast<model::RegularState>();
    QVERIFY(state2_1);

    QCOMPARE(state2_1->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).toDouble(), 140.0);
    QCOMPARE(state2_1->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).toDouble(), 100.0);
    QCOMPARE(state2_1->getMetadata(model::StateMachineEntity::MetadataKey::WIDTH).toDouble(), 180.0);
    QCOMPARE(state2_1->getMetadata(model::StateMachineEntity::MetadataKey::HEIGHT).toDouble(), 100.0);
}

/**
 * @brief Verify the initial pseudo-state inside state_1 has correct local geometry.
 *
 * initial: sceneTopLeft=(180,130), parent sceneTopLeft=(0,0)
 *   Expected local: posX=180, posY=130, w=40, h=40
 */
void QtGeometryDeserializationTest::InitialStateGeometryInsideParent() {
    auto model = loadFixture();
    QVERIFY(model);

    auto state1 = model->root()->findChildStateByName("state_1").dynamicCast<model::RegularState>();
    QVERIFY(state1);

    // The <initial> element inside a non-root state is parsed as an EntryPoint
    QSharedPointer<model::State> entryPoint;

    state1->forEachChildElement(
        [&entryPoint](QSharedPointer<model::StateMachineEntity> /*parent*/,
                      QSharedPointer<model::StateMachineEntity> child) {
            bool continueTraversal = true;

            if (auto state = child.dynamicCast<model::State>()) {
                if (state->stateType() == model::StateType::ENTRYPOINT) {
                    entryPoint = state;
                    continueTraversal = false;
                }
            }

            return continueTraversal;
        },
        1,
        false);

    QVERIFY(entryPoint);
    QCOMPARE(entryPoint->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).toDouble(), 180.0);
    QCOMPARE(entryPoint->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).toDouble(), 130.0);
    QCOMPARE(entryPoint->getMetadata(model::StateMachineEntity::MetadataKey::WIDTH).toDouble(), 40.0);
    QCOMPARE(entryPoint->getMetadata(model::StateMachineEntity::MetadataKey::HEIGHT).toDouble(), 40.0);
}

/**
 * @brief Verify all states have valid positive width and height.
 */
void QtGeometryDeserializationTest::AllStatesHaveWidthAndHeight() {
    auto model = loadFixture();
    QVERIFY(model);

    bool allHaveSize = true;
    QString failMsg;

    model->root()->forEachChildElement(
        [&allHaveSize, &failMsg](QSharedPointer<model::StateMachineEntity> /*parent*/,
                                 QSharedPointer<model::StateMachineEntity> child) {
            bool continueTraversal = true;

            if (child->type() == model::StateMachineEntity::Type::State) {
                QVariant w = child->getMetadata(model::StateMachineEntity::MetadataKey::WIDTH);
                QVariant h = child->getMetadata(model::StateMachineEntity::MetadataKey::HEIGHT);

                if (!w.isValid() || !h.isValid() || w.toDouble() <= 0 || h.toDouble() <= 0) {
                    auto state = child.dynamicCast<model::State>();

                    if (state) {
                        allHaveSize = false;
                        failMsg = QString("State '%1' (id=%2) missing valid size: w=%3, h=%4")
                                      .arg(state->name())
                                      .arg(state->id())
                                      .arg(w.toString(), h.toString());
                        continueTraversal = false;
                    }
                }
            }

            return continueTraversal;
        },
        model::StateMachineEntity::DEPTH_INFINITE,
        false);

    QVERIFY2(allHaveSize, qPrintable(failMsg));
}

/**
 * @brief Verify the transition from state_1 to state_2 with event EVENT_1 exists.
 */
void QtGeometryDeserializationTest::TransitionExistsBetweenStates() {
    auto model = loadFixture();
    QVERIFY(model);

    auto state1 = model->root()->findChildStateByName("state_1").dynamicCast<model::RegularState>();
    QVERIFY(state1);
    auto state2 = model->root()->findChildStateByName("state_2").dynamicCast<model::RegularState>();
    QVERIFY(state2);

    QSharedPointer<model::Transition> foundTransition;

    state1->forEachChildElement(
        [&foundTransition](QSharedPointer<model::StateMachineEntity> /*parent*/,
                           QSharedPointer<model::StateMachineEntity> child) {
            bool continueTraversal = true;

            if (child->type() == model::StateMachineEntity::Type::Transition) {
                auto transition = child.dynamicCast<model::Transition>();

                if (transition && transition->event() == "EVENT_1") {
                    foundTransition = transition;
                    continueTraversal = false;
                }
            }

            return continueTraversal;
        },
        1,
        false);

    QVERIFY(foundTransition);
    QVERIFY(foundTransition->target());
    QCOMPARE(foundTransition->target()->name(), QString("state_2"));
}

/**
 * @brief Verify that no child entity shares the same ID as the model root.
 *
 * This catches a bug where clearModel() resets the ID generator to 1 but the
 * root retains id=1, causing the first deserialized entity to get id=1 as well.
 * The view then confuses root and child when resolving parent elements.
 */
void QtGeometryDeserializationTest::RootIdNotReusedByChildren() {
    auto model = loadFixture();
    QVERIFY(model);

    const model::EntityID_t rootId = model->root()->id();

    // After deserialization the root must have a unique id
    // that no child entity shares.
    // Bug: clearModel() resets the ID generator to 1 but the root retains id=1,
    // so the first deserialized entity also gets id=1. The view then confuses
    // root and the first child when resolving parent elements (findHsmElement).
    auto state1 = model->root()->findChildStateByName("state_1");
    QVERIFY(state1);
    QVERIFY2(state1->id() != rootId,
             qPrintable(QString("state_1 id=%1 collides with root id=%2. "
                                "View will place sibling states as children of state_1.")
                            .arg(state1->id())
                            .arg(rootId)));
}

/**
 * @brief Verify the top-level InitialState is created from scxml initial="" attribute
 *        with geometry from initialGeometry="-200;160;0;0;40;40".
 *
 * Expected: posX=-200, posY=160, w=40, h=40, transition targets state_1
 */
void QtGeometryDeserializationTest::TopLevelInitialStateFromAttribute() {
    auto model = loadFixture();
    QVERIFY(model);

    // Find InitialState at root level
    QSharedPointer<model::State> initialState;

    model->root()->forEachChildElement(
        [&initialState](QSharedPointer<model::StateMachineEntity> /*parent*/,
                        QSharedPointer<model::StateMachineEntity> child) {
            bool continueTraversal = true;

            if (auto state = child.dynamicCast<model::State>()) {
                if (state->stateType() == model::StateType::INITIAL) {
                    initialState = state;
                    continueTraversal = false;
                }
            }

            return continueTraversal;
        },
        1,
        false);

    QVERIFY(initialState);
    QCOMPARE(initialState->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).toDouble(), -200.0);
    QCOMPARE(initialState->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).toDouble(), 160.0);
    QCOMPARE(initialState->getMetadata(model::StateMachineEntity::MetadataKey::WIDTH).toDouble(), 40.0);
    QCOMPARE(initialState->getMetadata(model::StateMachineEntity::MetadataKey::HEIGHT).toDouble(), 40.0);
}

int runQtGeometryDeserializationTest(int argc, char** argv) {
    QtGeometryDeserializationTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "QtGeometryDeserializationTest.moc"
