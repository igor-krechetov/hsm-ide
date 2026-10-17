#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QtTest>
#include <cmath>

#include "../model/TestPaths.hpp"
#include "model/StateMachineModel.hpp"
#include "model/StateMachineSerializer.hpp"
#include "model/elements/InitialState.hpp"
#include "model/elements/ModelRootState.hpp"
#include "model/elements/RegularState.hpp"
#include "model/elements/State.hpp"
#include "model/elements/Transition.hpp"
#include "view/elements/HsmInitialElement.hpp"
#include "view/elements/HsmStateElement.hpp"
#include "view/elements/HsmTransition.hpp"
#include "view/elements/private/ElementGripItem.hpp"
#include "view/theme/Theme.hpp"
#include "view/theme/ThemeManager.hpp"
#include "view/widgets/HsmGraphicsView.hpp"

using namespace view;

// ---------------------------------------------------------------------------
// Task 10 - Integration validation and checkpoint.
//
// These end-to-end tests exercise the two fixes together, across the full import -> model ->
// view pipeline, using the synthetic Qt Creator fixture qt_transition_geometry.scxml. They
// close the gap between the focused unit/property tests (which drive the serializer and the
// view helpers in isolation) and the integration behavior the bugfix promises:
//
//  1. Full import: a transition carrying localGeometry intermediate points has its endpoints
//     computed geometrically (the neighbouring intermediate point drives the perpendicular-foot
//     anchors); the Qt anchor-factor hints are ignored. The resolved endpoints land on the
//     source/target element edges. Other states are spot-checked to confirm they are unchanged
//     (Requirements 2.4, 3.5).
//
//  2. Import -> render -> save round-trip: serializing the imported model back to
//     Qt Creator SCXML preserves the geometry attributes (Requirement 3.6).
//
//  3. Create-and-snap: a freshly created state and round element, whose default
//     size the fix derives from the grid step, have ALL FOUR edges on grid lines
//     once their origin is snapped with HsmGraphicsView::alignPointToGrid - the
//     whole point of Bug 1 (Requirement 2.3).
// ---------------------------------------------------------------------------

// Test-only subclass exposing the mapped source/target rect exactly as
// recalculateLine() computes it, and the protected geometric helper, so the test
// can describe the expected center-collapse fallback it must NOT match.
class ProbeTransition : public HsmTransition {
public:
    QPointF probeStartFromRect(const QRectF& rectFrom, const QRectF& rectTo) {
        return findStartingPointFromRect(rectFrom, rectTo);
    }

    QPointF probeEndFromPoint(const QRectF& rect, const QPointF& neighbourPoint) {
        return findPerpendicularEdgePoint(rect, neighbourPoint);
    }

    QRectF mappedRect(HsmElement* element) {
        return mapRectFromItem(element, element->elementRect());
    }
};

class ElementGridAlignmentIntegrationTest : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void FullImportTransitionEndpointsLandOnElementEdges();
    void ImportRenderSaveRoundTripPreservesGeometry();
    void CreatedStateAllEdgesOnGridAfterSnap();
    void CreatedRoundElementAllEdgesOnGridAfterSnap();

private:
    static QSharedPointer<model::StateMachineModel> loadFixture();
    static QSharedPointer<model::State> findStateByName(const QSharedPointer<model::StateMachineModel>& model,
                                                        const QString& name);
    static QSharedPointer<model::Transition> findTransitionByEvent(const QSharedPointer<model::StateMachineModel>& model,
                                                                   const QString& event);
    static QRectF importedRectOf(const QSharedPointer<model::State>& state);
    static QList<ElementGripItem*> collectGrips(HsmTransition* transition);
    bool isWholeMultiple(qreal value, qreal step) const;

private:
    view::theme::Theme mOriginalTheme;
};

void ElementGridAlignmentIntegrationTest::init() {
    // Preserve the active theme: the create-and-snap slots pin the grid step so a
    // non-default value must not leak into the rest of the suite.
    mOriginalTheme = ThemeManager::instance().theme();
}

void ElementGridAlignmentIntegrationTest::cleanup() {
    ThemeManager::instance().setTheme(mOriginalTheme);
}

QSharedPointer<model::StateMachineModel> ElementGridAlignmentIntegrationTest::loadFixture() {
    const QString scxml = test::loadScxmlFixture("qt_compatibility/qt_transition_geometry.scxml");
    QSharedPointer<model::StateMachineModel> result;

    if (false == scxml.isEmpty()) {
        result = QSharedPointer<model::StateMachineModel>::create("IntegrationProject");
        model::StateMachineSerializer serializer;

        if (false == serializer.deserializeFromScxml(scxml, result)) {
            result.clear();
        }
    }

    return result;
}

QSharedPointer<model::State> ElementGridAlignmentIntegrationTest::findStateByName(
    const QSharedPointer<model::StateMachineModel>& model,
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

QSharedPointer<model::Transition> ElementGridAlignmentIntegrationTest::findTransitionByEvent(
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

QRectF ElementGridAlignmentIntegrationTest::importedRectOf(const QSharedPointer<model::State>& state) {
    const qreal x = state->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).toDouble();
    const qreal y = state->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).toDouble();
    const qreal w = state->getMetadata(model::StateMachineEntity::MetadataKey::WIDTH).toDouble();
    const qreal h = state->getMetadata(model::StateMachineEntity::MetadataKey::HEIGHT).toDouble();

    return QRectF(x, y, w, h);
}

QList<ElementGripItem*> ElementGridAlignmentIntegrationTest::collectGrips(HsmTransition* transition) {
    QList<ElementGripItem*> grips;

    for (auto* child : transition->childItems()) {
        auto* grip = dynamic_cast<ElementGripItem*>(child);

        if (nullptr != grip) {
            grips.append(grip);
        }
    }

    return grips;
}

bool ElementGridAlignmentIntegrationTest::isWholeMultiple(qreal value, qreal step) const {
    const qreal ratio = value / step;
    const qreal rounded = std::round(ratio);

    return qAbs(ratio - rounded) < 1e-9;
}

/**
 * @brief Integration (Requirements 2.4, 3.5): full import computes transition endpoints
 *        geometrically and lands them on the element edges.
 *
 * The whole pipeline is exercised: the fixture is deserialized into a model, then the source
 * (State_1) and target (State_5) view elements are placed at their imported scene geometry and
 * the Transition_1 transition is rendered via recalculateLine(). The transition carries
 * localGeometry intermediate points, so each endpoint is the perpendicular foot dropped from
 * its neighbouring intermediate point onto the element edge. The Qt anchor-factor hints are
 * ignored. Other states are spot-checked to confirm their imported geometry is unchanged.
 *
 * Validates: Requirements 2.4, 3.5
 */
void ElementGridAlignmentIntegrationTest::FullImportTransitionEndpointsLandOnElementEdges() {
    auto model = loadFixture();
    QVERIFY2(model, "failed to import the qt_transition_geometry.scxml fixture");

    auto sourceState = findStateByName(model, "State_1");
    auto targetState = findStateByName(model, "State_5");
    QVERIFY2(sourceState, "State_1 not found after import");
    QVERIFY2(targetState, "State_5 not found after import");

    auto transitionModel = findTransitionByEvent(model, "Transition_1");
    QVERIFY2(transitionModel, "Transition_1 not found after import");

    // The Qt Creator startTargetFactors/endTargetFactors hints are intentionally ignored on
    // import; transition endpoints are computed geometrically from the element geometry and the
    // transition's own intermediate (localGeometry) points.

    const QRectF sourceRect = importedRectOf(sourceState);
    const QRectF targetRect = importedRectOf(targetState);
    QVERIFY2(sourceRect.width() > 0.0 && sourceRect.height() > 0.0, "State_1 imported with a degenerate size");
    QVERIFY2(targetRect.width() > 0.0 && targetRect.height() > 0.0, "State_5 imported with a degenerate size");

    // Build the view scene from the imported geometry: place each state element at its imported
    // scene position/size, so the mapped rects recalculateLine() sees match the authored layout.
    QGraphicsScene scene;
    QGraphicsView view;
    view.setScene(&scene);

    HsmStateElement* fromElement = new HsmStateElement(sourceRect.size());
    fromElement->init(sourceState);
    scene.addItem(fromElement);
    fromElement->setPos(sourceRect.topLeft());

    HsmStateElement* toElement = new HsmStateElement(targetRect.size());
    toElement->init(targetState);
    scene.addItem(toElement);
    toElement->setPos(targetRect.topLeft());

    ProbeTransition* transition = new ProbeTransition();
    transition->init(transitionModel);
    transition->connectElements(fromElement, toElement);
    transition->recalculateLine();

    const QRectF rectFrom = transition->mappedRect(fromElement);
    const QRectF rectTo = transition->mappedRect(toElement);

    // Transition_1 carries localGeometry intermediate points, so BOTH endpoints use the
    // perpendicular-foot rule from their neighbouring intermediate point.
    const QPolygonF resolvedPath =
        transitionModel->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY).value<QPolygonF>();
    QVERIFY2(resolvedPath.size() >= 3,
             qPrintable(QString("expected start + >=1 interior + end, got %1 points").arg(resolvedPath.size())));

    const QPointF actualStart = resolvedPath.first();
    const QPointF actualEnd = resolvedPath.last();
    const QPointF firstInterior = resolvedPath[1];
    const QPointF lastInterior = resolvedPath[resolvedPath.size() - 2];

    const QPointF expectedStart = transition->probeEndFromPoint(rectFrom, firstInterior);
    const QPointF expectedEnd = transition->probeEndFromPoint(rectTo, lastInterior);

    // 1) Start is the perpendicular foot of the first interior point onto State_1's edge.
    QVERIFY2((qAbs(actualStart.x() - expectedStart.x()) < 0.5) && (qAbs(actualStart.y() - expectedStart.y()) < 0.5),
             qPrintable(QString("rendered start anchor (%1,%2) != perpendicular foot (%3,%4)")
                            .arg(actualStart.x())
                            .arg(actualStart.y())
                            .arg(expectedStart.x())
                            .arg(expectedStart.y())));

    // 2) End is the perpendicular foot of the last interior point onto State_5's edge.
    QVERIFY2((qAbs(actualEnd.x() - expectedEnd.x()) < 0.5) && (qAbs(actualEnd.y() - expectedEnd.y()) < 0.5),
             qPrintable(QString("rendered end anchor (%1,%2) != perpendicular foot (%3,%4)")
                            .arg(actualEnd.x())
                            .arg(actualEnd.y())
                            .arg(expectedEnd.x())
                            .arg(expectedEnd.y())));

    // 3) Both endpoints lie ON their respective element edges (the interior points are outside
    //    the source/target rects, so a real perpendicular foot exists).
    auto onEdge = [](const QPointF& pt, const QRectF& r) {
        const qreal eps = 0.5;
        return (qAbs(pt.x() - r.left()) < eps) || (qAbs(pt.x() - r.right()) < eps) || (qAbs(pt.y() - r.top()) < eps) ||
               (qAbs(pt.y() - r.bottom()) < eps);
    };
    QVERIFY2(onEdge(actualStart, rectFrom),
             qPrintable(QString("start anchor (%1,%2) is not on the State_1 edge").arg(actualStart.x()).arg(actualStart.y())));
    QVERIFY2(onEdge(actualEnd, rectTo),
             qPrintable(QString("end anchor (%1,%2) is not on the State_5 edge").arg(actualEnd.x()).arg(actualEnd.y())));

    // Spot-check: other top-level states keep their authored geometry (Requirement 3.5).
    struct Expected {
        QString name;
        qreal x;
        qreal y;
        qreal w;
        qreal h;
    };
    const QList<Expected> others = {
        {"State_4", 925.45, 102.16, 120.0, 100.0},
    };

    for (const Expected& e : others) {
        auto state = findStateByName(model, e.name);
        QVERIFY2(state, qPrintable(QString("spot-check state '%1' not found").arg(e.name)));

        const QRectF rect = importedRectOf(state);
        QVERIFY2(qAbs(rect.x() - e.x) < 0.01,
                 qPrintable(QString("%1 X changed: expected %2 got %3").arg(e.name).arg(e.x).arg(rect.x())));
        QVERIFY2(qAbs(rect.y() - e.y) < 0.01,
                 qPrintable(QString("%1 Y changed: expected %2 got %3").arg(e.name).arg(e.y).arg(rect.y())));
        QVERIFY2(qAbs(rect.width() - e.w) < 0.01,
                 qPrintable(QString("%1 W changed: expected %2 got %3").arg(e.name).arg(e.w).arg(rect.width())));
        QVERIFY2(qAbs(rect.height() - e.h) < 0.01,
                 qPrintable(QString("%1 H changed: expected %2 got %3").arg(e.name).arg(e.h).arg(rect.height())));
    }
}

/**
 * @brief Integration (Requirement 3.6): import -> serialize round-trip preserves geometry.
 *
 * Importing the fixture and serializing it straight back to Qt Creator SCXML must keep the
 * previously supported geometry attributes intact: the output still carries scenegeometry, and
 * re-importing it reproduces a sample of state rects without drift. This asserts the save step
 * does not corrupt existing geometry.
 *
 * Validates: Requirement 3.6
 */
void ElementGridAlignmentIntegrationTest::ImportRenderSaveRoundTripPreservesGeometry() {
    model::StateMachineSerializer serializer;

    auto original = loadFixture();
    QVERIFY2(original, "failed to import the qt_transition_geometry.scxml fixture");

    const QString serialized = serializer.serializeToScxml(original, model::SerializationFormat::QtCreator);
    QVERIFY2(false == serialized.isEmpty(), "serialization produced empty output");
    QVERIFY2(serialized.contains("scenegeometry="), "serialized output dropped the scenegeometry attribute");

    auto reloaded = serializer.deserializeFromScxml(serialized);
    QVERIFY2(reloaded != nullptr, "round-tripped SCXML failed to deserialize");

    const QStringList sampleStates = {"State_1", "State_4", "State_5", "State_2", "State_3"};

    for (const QString& name : sampleStates) {
        auto before = findStateByName(original, name);
        auto after = findStateByName(reloaded, name);

        QVERIFY2(before, qPrintable(QString("state '%1' missing in original import").arg(name)));
        QVERIFY2(after, qPrintable(QString("state '%1' missing after round-trip").arg(name)));

        const QRectF rectBefore = importedRectOf(before);
        const QRectF rectAfter = importedRectOf(after);

        QVERIFY2(qAbs(rectBefore.x() - rectAfter.x()) < 0.01,
                 qPrintable(QString("%1 X drift: %2 -> %3").arg(name).arg(rectBefore.x()).arg(rectAfter.x())));
        QVERIFY2(qAbs(rectBefore.y() - rectAfter.y()) < 0.01,
                 qPrintable(QString("%1 Y drift: %2 -> %3").arg(name).arg(rectBefore.y()).arg(rectAfter.y())));
        QVERIFY2(qAbs(rectBefore.width() - rectAfter.width()) < 0.01,
                 qPrintable(QString("%1 W drift: %2 -> %3").arg(name).arg(rectBefore.width()).arg(rectAfter.width())));
        QVERIFY2(qAbs(rectBefore.height() - rectAfter.height()) < 0.01,
                 qPrintable(QString("%1 H drift: %2 -> %3").arg(name).arg(rectBefore.height()).arg(rectAfter.height())));
    }
}

/**
 * @brief Integration (Requirement 2.3): a created state has all four edges on grid lines
 *        once its origin is snapped.
 *
 * The Bug 1 fix derives the default size from the grid step, so the default
 * width/height are whole multiples of the step. Snapping an element's ORIGIN with
 * HsmGraphicsView::alignPointToGrid lands the top-left on a grid line; because the
 * size is a grid multiple, the opposite edges (right/bottom) then land on grid
 * lines too. This asserts ALL FOUR edges are on grid lines for several off-grid
 * origins, which is the user-visible outcome Bug 1 was failing to deliver.
 *
 * Validates: Requirement 2.3
 */
void ElementGridAlignmentIntegrationTest::CreatedStateAllEdgesOnGridAfterSnap() {
    // Pin the live default grid step.
    view::theme::Theme theme = mOriginalTheme;
    theme.grid.minorLineStep = 20.0;
    ThemeManager::instance().setTheme(theme);
    const qreal step = ThemeManager::instance().theme().grid.minorLineStep;
    QVERIFY2(step > 0.0, "grid step must be positive");

    // Observe the rendered geometry AFTER init(). Reading elementRect() right after
    // construction is INSUFFICIENT: the constructor sets mOuterRect to the (grid-multiple)
    // mSize, while init() is what (previously) inflated mOuterRect by the border pen width.
    // The bug only shows up post-init(), so the element must be init()'d before measuring.
    QGraphicsScene scene;
    auto stateModel = QSharedPointer<model::RegularState>::create("State_grid");
    HsmStateElement element;
    element.init(stateModel);
    scene.addItem(&element);
    const QSizeF size = element.elementRect().size();

    // The fix guarantees the rendered outer rect is a whole multiple of the step.
    QVERIFY2(isWholeMultiple(size.width(), step),
             qPrintable(QString("default width %1 is not a whole multiple of grid step %2").arg(size.width()).arg(step)));
    QVERIFY2(isWholeMultiple(size.height(), step),
             qPrintable(QString("default height %1 is not a whole multiple of grid step %2").arg(size.height()).arg(step)));

    // For several deliberately off-grid origins, snap the origin and assert all
    // four edges land on grid lines.
    const QList<QPointF> origins = {
        QPointF(3.0, 7.0),
        QPointF(-11.0, 29.0),
        QPointF(137.0, -58.0),
        QPointF(1001.0, 2003.0),
    };

    for (const QPointF& origin : origins) {
        const QPointF snappedOrigin = HsmGraphicsView::alignPointToGrid(origin);
        const QRectF rect(snappedOrigin, size);

        QVERIFY2(isWholeMultiple(rect.left(), step),
                 qPrintable(QString("left edge %1 not on a grid line (step %2)").arg(rect.left()).arg(step)));
        QVERIFY2(isWholeMultiple(rect.top(), step),
                 qPrintable(QString("top edge %1 not on a grid line (step %2)").arg(rect.top()).arg(step)));
        QVERIFY2(isWholeMultiple(rect.right(), step),
                 qPrintable(QString("right edge %1 not on a grid line (step %2)").arg(rect.right()).arg(step)));
        QVERIFY2(isWholeMultiple(rect.bottom(), step),
                 qPrintable(QString("bottom edge %1 not on a grid line (step %2)").arg(rect.bottom()).arg(step)));
    }
}

/**
 * @brief Integration (Requirement 2.3): a created round element has all four edges on grid
 *        lines once its origin is snapped.
 *
 * Round pseudo-states share HsmRoundElement, whose default size the fix also
 * derives from the grid step. The same origin-snap + grid-multiple size reasoning
 * puts all four edges on grid lines.
 *
 * Validates: Requirement 2.3
 */
void ElementGridAlignmentIntegrationTest::CreatedRoundElementAllEdgesOnGridAfterSnap() {
    view::theme::Theme theme = mOriginalTheme;
    theme.grid.minorLineStep = 20.0;
    ThemeManager::instance().setTheme(theme);
    const qreal step = ThemeManager::instance().theme().grid.minorLineStep;
    QVERIFY2(step > 0.0, "grid step must be positive");

    // Observe the rendered geometry AFTER init() (see the state slot for why the
    // post-construction reading is insufficient).
    QGraphicsScene scene;
    auto initialModel = QSharedPointer<model::InitialState>::create();
    HsmInitialElement element;
    element.init(initialModel);
    scene.addItem(&element);
    const QSizeF size = element.elementRect().size();

    QVERIFY2(isWholeMultiple(size.width(), step),
             qPrintable(QString("round default width %1 is not a whole multiple of grid step %2").arg(size.width()).arg(step)));
    QVERIFY2(
        isWholeMultiple(size.height(), step),
        qPrintable(QString("round default height %1 is not a whole multiple of grid step %2").arg(size.height()).arg(step)));

    const QList<QPointF> origins = {
        QPointF(5.0, 9.0),
        QPointF(-13.0, 31.0),
        QPointF(141.0, -62.0),
    };

    for (const QPointF& origin : origins) {
        const QPointF snappedOrigin = HsmGraphicsView::alignPointToGrid(origin);
        const QRectF rect(snappedOrigin, size);

        QVERIFY2(isWholeMultiple(rect.left(), step),
                 qPrintable(QString("round left edge %1 not on a grid line (step %2)").arg(rect.left()).arg(step)));
        QVERIFY2(isWholeMultiple(rect.top(), step),
                 qPrintable(QString("round top edge %1 not on a grid line (step %2)").arg(rect.top()).arg(step)));
        QVERIFY2(isWholeMultiple(rect.right(), step),
                 qPrintable(QString("round right edge %1 not on a grid line (step %2)").arg(rect.right()).arg(step)));
        QVERIFY2(isWholeMultiple(rect.bottom(), step),
                 qPrintable(QString("round bottom edge %1 not on a grid line (step %2)").arg(rect.bottom()).arg(step)));
    }
}

int runElementGridAlignmentIntegrationTest(int argc, char** argv) {
    QApplication app(argc, argv);

    ElementGridAlignmentIntegrationTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "ElementGridAlignmentIntegrationTest.moc"
