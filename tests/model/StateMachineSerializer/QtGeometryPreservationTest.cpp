#include <QPointF>
#include <QPolygonF>
#include <QtTest>

#include "../TestPaths.hpp"
#include "model/StateMachineModel.hpp"
#include "model/StateMachineSerializer.hpp"
#include "model/elements/ModelRootState.hpp"
#include "model/elements/RegularState.hpp"
#include "model/elements/State.hpp"
#include "model/elements/Transition.hpp"

// ---------------------------------------------------------------------------
// Property 2 / Property 4 - Preservation of imported geometry on the model side.
//
// These tests assert geometry preservation on the serializer, using the synthetic Qt Creator
// fixture qt_transition_geometry.scxml (anchor-factor hints in the fixture are ignored on
// import). They check that:
//   - intermediate localGeometry points are reproduced unchanged after import,
//   - every imported state keeps its authored position/size,
//   - a save round-trip preserves scenegeometry/geometry/localGeometry.
// ---------------------------------------------------------------------------

class QtGeometryPreservationTest : public QObject {
    Q_OBJECT

private slots:
    void IntermediateLocalGeometryPointsPreserved();
    void ImportedStatesKeepAuthoredGeometry();
    void SaveRoundTripPreservesGeometry();

private:
    static QSharedPointer<model::StateMachineModel> loadFixture(const QString& relativePath);
    static QSharedPointer<model::Transition> findTransitionByEvent(const QSharedPointer<model::StateMachineModel>& model,
                                                                   const QString& event);
    static QSharedPointer<model::State> findStateByName(const QSharedPointer<model::StateMachineModel>& model,
                                                        const QString& name);
};

QSharedPointer<model::StateMachineModel> QtGeometryPreservationTest::loadFixture(const QString& relativePath) {
    const QString scxml = test::loadScxmlFixture(relativePath);
    QSharedPointer<model::StateMachineModel> result;

    if (!scxml.isEmpty()) {
        result = QSharedPointer<model::StateMachineModel>::create("PreservationProject");
        model::StateMachineSerializer serializer;

        if (!serializer.deserializeFromScxml(scxml, result)) {
            result.clear();
        }
    }

    return result;
}

QSharedPointer<model::Transition> QtGeometryPreservationTest::findTransitionByEvent(
    const QSharedPointer<model::StateMachineModel>& model,
    const QString& event) {
    QSharedPointer<model::Transition> found;

    if (model && model->root()) {
        model->root()->forEachChildElement(
            [&found, &event](QSharedPointer<model::StateMachineEntity> /*parent*/,
                             QSharedPointer<model::StateMachineEntity> child) {
                bool continueTraversal = true;

                if (child->type() == model::StateMachineEntity::Type::Transition) {
                    auto transition = child.dynamicCast<model::Transition>();

                    if (transition && transition->event() == event) {
                        found = transition;
                        continueTraversal = false;
                    }
                }

                return continueTraversal;
            },
            model::StateMachineEntity::DEPTH_INFINITE,
            false);
    }

    return found;
}

QSharedPointer<model::State> QtGeometryPreservationTest::findStateByName(const QSharedPointer<model::StateMachineModel>& model,
                                                                         const QString& name) {
    QSharedPointer<model::State> found;

    if (model && model->root()) {
        model->root()->forEachChildElement(
            [&found, &name](QSharedPointer<model::StateMachineEntity> /*parent*/,
                            QSharedPointer<model::StateMachineEntity> child) {
                bool continueTraversal = true;

                if (child->type() == model::StateMachineEntity::Type::State) {
                    auto state = child.dynamicCast<model::State>();

                    if (state && state->name() == name) {
                        found = state;
                        continueTraversal = false;
                    }
                }

                return continueTraversal;
            },
            model::StateMachineEntity::DEPTH_INFINITE,
            false);
    }

    return found;
}

/**
 * @brief Property 4 (Requirement 3.3): intermediate localGeometry points preserved.
 *
 * A transition carrying localGeometry="150;-30;320;-30;320;80" from a source
 * state whose scene top-left is (0,0) with size 200x120 (so its QT_DELTA center
 * is (100,60)) must, after import, reproduce its intermediate points shifted by
 * the source delta exactly as the unfixed importer does today. The resolved
 * GEOMETRY polygon is [placeholderStart, interior..., placeholderEnd] with the
 * interior points offset by the source center.
 *
 * This encodes the observed baseline transformation and MUST PASS on the unfixed
 * code.
 *
 * Validates: Requirement 3.3
 */
void QtGeometryPreservationTest::IntermediateLocalGeometryPointsPreserved() {
    auto model = loadFixture("qt_compatibility/qt_local_geometry.scxml");
    QVERIFY(model);

    auto transition = findTransitionByEvent(model, "GO");
    QVERIFY(transition);

    const QVariant geometryVar = transition->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY);
    QVERIFY2(geometryVar.isValid(), "Expected GEOMETRY metadata on the transition after import");

    const QPolygonF path = geometryVar.value<QPolygonF>();

    // placeholder start + 3 interior points + placeholder end
    QCOMPARE(path.size(), 5);

    // Source state_a: scene top-left (0,0), size 200x120 -> QT_DELTA center (100,60).
    const QPointF sourceDelta(100.0, 60.0);
    const QList<QPointF> rawInterior = {QPointF(150, -30), QPointF(320, -30), QPointF(320, 80)};

    for (int i = 0; i < rawInterior.size(); ++i) {
        const QPointF expected = rawInterior[i] + sourceDelta;
        const QPointF actual = path[i + 1];  // skip placeholder start

        QVERIFY2(qFuzzyCompare(actual.x(), expected.x()) && qFuzzyCompare(actual.y(), expected.y()),
                 qPrintable(QString("Interior point %1: expected (%2,%3) got (%4,%5)")
                                .arg(i)
                                .arg(expected.x())
                                .arg(expected.y())
                                .arg(actual.x())
                                .arg(actual.y())));
    }
}

/**
 * @brief Property 4 (Requirement 3.5): every imported state keeps its authored geometry.
 *
 * Importing qt_transition_geometry.scxml must place each state at its authored position and
 * size. For a top-level state (parent = root at scene origin) the model POSITION equals the
 * scenegeometry top-left (topX;topY) and the size equals (w;h). For a nested state the position
 * is relative to its parent, so only the size is asserted (read verbatim from scenegeometry) -
 * exactly what the importer computes.
 *
 * Validates: Requirement 3.5
 */
void QtGeometryPreservationTest::ImportedStatesKeepAuthoredGeometry() {
    auto model = loadFixture("qt_compatibility/qt_transition_geometry.scxml");
    QVERIFY(model);

    struct Expected {
        QString name;
        double x;
        double y;
        double w;
        double h;
    };

    // Top-level states (parent = root at scene origin). POSITION = scenegeometry (topX;topY),
    // size = (w;h), taken verbatim from the fixture.
    const QList<Expected> topLevel = {
        {"State_1", 635.43, 102.16, 120.0, 100.0},
        {"State_4", 925.45, 102.16, 120.0, 100.0},
        {"State_5", 164.67, 413.62, 488.90, 373.70},
    };

    for (const Expected& e : topLevel) {
        auto state = findStateByName(model, e.name);
        QVERIFY2(state, qPrintable(QString("State '%1' not found after import").arg(e.name)));

        const double x = state->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).toDouble();
        const double y = state->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).toDouble();
        const double w = state->getMetadata(model::StateMachineEntity::MetadataKey::WIDTH).toDouble();
        const double h = state->getMetadata(model::StateMachineEntity::MetadataKey::HEIGHT).toDouble();

        QVERIFY2(qAbs(x - e.x) < 0.01, qPrintable(QString("%1 X: expected %2 got %3").arg(e.name).arg(e.x).arg(x)));
        QVERIFY2(qAbs(y - e.y) < 0.01, qPrintable(QString("%1 Y: expected %2 got %3").arg(e.name).arg(e.y).arg(y)));
        QVERIFY2(qAbs(w - e.w) < 0.01, qPrintable(QString("%1 W: expected %2 got %3").arg(e.name).arg(e.w).arg(w)));
        QVERIFY2(qAbs(h - e.h) < 0.01, qPrintable(QString("%1 H: expected %2 got %3").arg(e.name).arg(e.h).arg(h)));
    }

    // Nested states (children of State_5): position is relative to the parent, so assert the
    // size, read verbatim from scenegeometry (w;h).
    const QList<Expected> nested = {
        {"State_2", 0.0, 0.0, 120.0, 100.0},
        {"State_3", 0.0, 0.0, 120.0, 100.0},
    };

    for (const Expected& e : nested) {
        auto state = findStateByName(model, e.name);
        QVERIFY2(state, qPrintable(QString("Nested state '%1' not found after import").arg(e.name)));

        const double w = state->getMetadata(model::StateMachineEntity::MetadataKey::WIDTH).toDouble();
        const double h = state->getMetadata(model::StateMachineEntity::MetadataKey::HEIGHT).toDouble();

        QVERIFY2(qAbs(w - e.w) < 0.01, qPrintable(QString("%1 W: expected %2 got %3").arg(e.name).arg(e.w).arg(w)));
        QVERIFY2(qAbs(h - e.h) < 0.01, qPrintable(QString("%1 H: expected %2 got %3").arg(e.name).arg(e.h).arg(h)));
    }
}

/**
 * @brief Property 4 (Requirement 3.6): save after import preserves geometry.
 *
 * Serializing a model (imported from Qt Creator SCXML) back to Qt Creator SCXML
 * must preserve the previously supported geometry attributes without corruption:
 *   - every state's scenegeometry is written with its authored absolute scene
 *     position and size (and re-importing those states reproduces their geometry
 *     exactly, since state geometry is self-consistent across a round-trip);
 *   - a transition carrying localGeometry has its intermediate points written
 *     verbatim from the model (the serialization step itself does not corrupt
 *     them).
 *
 * Observation-first note: a *full* import -> serialize -> re-import of a
 * transition's localGeometry does NOT reproduce the interior points unchanged on
 * the unfixed code, because the importer re-applies the source-state delta on
 * every import (a pre-existing behavior independent of this bugfix). Requirement
 * 3.6 concerns the serialization step not corrupting previously supported
 * attributes, which this test asserts directly on both the state round-trip and
 * the written localGeometry values. The pre-existing re-import delta is left
 * untouched and intentionally not asserted here.
 *
 * This is the behavior the unfixed serializer already produces, so the test MUST
 * PASS on the baseline.
 *
 * Validates: Requirement 3.6
 */
void QtGeometryPreservationTest::SaveRoundTripPreservesGeometry() {
    model::StateMachineSerializer serializer;

    // --- Part 1: state geometry survives a full round-trip on qt_transition_geometry ---
    auto original = loadFixture("qt_compatibility/qt_transition_geometry.scxml");
    QVERIFY(original);

    const QString roundTripped = serializer.serializeToScxml(original, model::SerializationFormat::QtCreator);
    QVERIFY2(roundTripped.isEmpty() == false, "Serialization produced empty output");
    QVERIFY2(roundTripped.contains("scenegeometry="), "Serialized output dropped the scenegeometry attribute");

    auto reloaded = serializer.deserializeFromScxml(roundTripped);
    QVERIFY2(reloaded != nullptr, "Round-tripped SCXML failed to deserialize");

    const QStringList sampleStates = {"State_1", "State_4", "State_5", "State_2", "State_3"};

    for (const QString& name : sampleStates) {
        auto before = findStateByName(original, name);
        auto after = findStateByName(reloaded, name);

        QVERIFY2(before, qPrintable(QString("State '%1' missing in original import").arg(name)));
        QVERIFY2(after, qPrintable(QString("State '%1' missing after round-trip").arg(name)));

        const double bx = before->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).toDouble();
        const double by = before->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).toDouble();
        const double bw = before->getMetadata(model::StateMachineEntity::MetadataKey::WIDTH).toDouble();
        const double bh = before->getMetadata(model::StateMachineEntity::MetadataKey::HEIGHT).toDouble();

        const double ax = after->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).toDouble();
        const double ay = after->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).toDouble();
        const double aw = after->getMetadata(model::StateMachineEntity::MetadataKey::WIDTH).toDouble();
        const double ah = after->getMetadata(model::StateMachineEntity::MetadataKey::HEIGHT).toDouble();

        QVERIFY2(qAbs(bx - ax) < 0.01, qPrintable(QString("%1 X drift: %2 -> %3").arg(name).arg(bx).arg(ax)));
        QVERIFY2(qAbs(by - ay) < 0.01, qPrintable(QString("%1 Y drift: %2 -> %3").arg(name).arg(by).arg(ay)));
        QVERIFY2(qAbs(bw - aw) < 0.01, qPrintable(QString("%1 W drift: %2 -> %3").arg(name).arg(bw).arg(aw)));
        QVERIFY2(qAbs(bh - ah) < 0.01, qPrintable(QString("%1 H drift: %2 -> %3").arg(name).arg(bh).arg(ah)));
    }

    // --- Part 2: localGeometry is written verbatim from the model's interior points ---
    // Use the small deterministic fixture so the written values are unambiguous.
    auto localGeometryModel = loadFixture("qt_compatibility/qt_local_geometry.scxml");
    QVERIFY(localGeometryModel);

    auto transition = findTransitionByEvent(localGeometryModel, "GO");
    QVERIFY2(transition, "GO transition missing after import");

    const QVariant geometryVar = transition->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY);
    QVERIFY2(geometryVar.isValid(), "Transition missing GEOMETRY metadata");

    const QPolygonF modelPath = geometryVar.value<QPolygonF>();
    QVERIFY2(modelPath.size() >= 3, "Expected at least one interior point in the model path");

    const QString serialized = serializer.serializeToScxml(localGeometryModel, model::SerializationFormat::QtCreator);
    QVERIFY2(serialized.contains("localGeometry="),
             qPrintable(QString("Serialized output dropped the localGeometry attribute:\n%1").arg(serialized)));

    // The serializer writes the interior points (model path minus the first/last
    // placeholder endpoints). Confirm each interior point's exact text appears in
    // the written localGeometry, proving the values are not corrupted on write.
    for (int i = 1; i < modelPath.size() - 1; ++i) {
        const QString xText = QString::number(modelPath[i].x(), 'f', 2);
        const QString yText = QString::number(modelPath[i].y(), 'f', 2);

        QVERIFY2(serialized.contains(xText) && serialized.contains(yText),
                 qPrintable(QString("Interior point %1 (%2;%3) not found verbatim in serialized localGeometry:\n%4")
                                .arg(i)
                                .arg(xText)
                                .arg(yText)
                                .arg(serialized)));
    }
}

int runQtGeometryPreservationTest(int argc, char** argv) {
    QtGeometryPreservationTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "QtGeometryPreservationTest.moc"
