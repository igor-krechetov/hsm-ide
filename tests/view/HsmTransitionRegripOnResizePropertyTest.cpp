#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QList>
#include <QPointF>
#include <QPolygonF>
#include <QRandomGenerator>
#include <QSharedPointer>
#include <QSizeF>
#include <QtTest>
#include <algorithm>
#include <cmath>

#include "model/elements/RegularState.hpp"
#include "model/elements/Transition.hpp"
#include "view/elements/HsmStateElement.hpp"
#include "view/elements/HsmTransition.hpp"
#include "view/elements/private/ElementBoundaryGripItem.hpp"
#include "view/elements/private/ElementGripItem.hpp"
#include "view/elements/private/HsmElement.hpp"

using namespace view;

// ---------------------------------------------------------------------------
// Property 1: Bug Condition - Transition endpoints and grips stay stable under an
// origin-moving parent resize.
//
// BUGFIX bug-condition EXPLORATION test. It encodes the Expected Behavior (design
// Property 1): when a composite (parent) state is resized from a grip that moves
// its top-left origin (any grip except south-east) and the parent contains a
// transition drawn between two of its distinct child states, EVERY grip/anchor
// point of that transition - the source endpoint, the destination endpoint AND all
// intermediate grips - must keep its scene position relative to the connected child
// states, at every point during the resize, no matter how large newPositionDelta
// is (fast drag) or how many steps accumulate.
//
// ROOT CAUSE this test is designed to surface (design "Hypothesized Root Cause"):
// HsmResizableElement::onGripMoved() moves the parent origin, then compensates every
// direct child in a loop (child->setPos(child->pos() - newPositionDelta)). Each
// setPos synchronously fires that child's geometryChanged, which
// HsmTransition::connectElements() wired DIRECTLY to recalculateLine(). So
// recalculateLine() runs in the MIDDLE of the compensation loop and re-derives the
// transition's ENDPOINTS from mapRectFromItem(mFromElement/mToElement, ...) against
// a half-updated set of coordinate frames (parent moved, some children shifted, the
// transition item itself possibly not yet), writing a wrong GEOMETRY. The error
// scales with newPositionDelta, so a fast drag jumps visibly while a slow drag does
// not. The intermediate grips are NOT the problem - they are carried by the
// transition item's own setPos.
//
// Why the PREVIOUS version of this test missed the bug: it drove a single small clean
// onGripMoved() delta and inspected only the FINAL steady state, so it only saw the
// last recalculateLine() (which, once every frame has settled, happens to be correct)
// and never a large/accumulating delta. This rewrite instead:
//   - drives a LARGE newPositionDelta and a SEQUENCE of accumulating deltas (fast
//     multi-step drag), and
//   - records the transition's persisted GEOMETRY (mapped to scene) at EVERY
//     recalculateLine() emission during the resize - including the mid-loop ones -
//     via the transition model's modelDataChanged signal, so a transient mid-loop
//     endpoint jump is caught deterministically regardless of the sibling-iteration
//     order that decides which recompute happens to land last.
//
// On the UNFIXED code this is EXPECTED TO FAIL for every origin-moving grip: at least
// one mid-loop GEOMETRY snapshot has endpoints offset from the children by the
// half-updated newPositionDelta.
//
// The south-east control case (newPositionDelta == 0) does NOT move the origin, so the
// compensation loop performs no moves, emits no mid-loop geometryChanged, and the
// transition is unchanged: it must PASS even on the unfixed code.
//
// Validates: Requirements 1.1, 1.2, 1.3, 2.1, 2.2, 2.3
// ---------------------------------------------------------------------------

namespace {

constexpr qreal cEps = 1e-6;

bool pointsEqual(const QPointF& a, const QPointF& b) {
    return (std::abs(a.x() - b.x()) <= cEps) && (std::abs(a.y() - b.y()) <= cEps);
}

// A composite state that exposes the boundary grips so the test can drive a resize
// through onGripMoved() exactly the way an ElementBoundaryGripItem drag does.
class ProbeCompositeState : public HsmStateElement {
public:
    explicit ProbeCompositeState(const QSizeF& size)
        : HsmStateElement(size) {}

    ElementBoundaryGripItem* boundaryGrip(const GripDirection direction) {
        ElementBoundaryGripItem* result = nullptr;

        for (QGraphicsItem* child : childItems()) {
            auto* grip = dynamic_cast<ElementBoundaryGripItem*>(child);

            if ((nullptr != grip) && (grip->direction() == direction)) {
                result = grip;
                break;
            }
        }

        return result;
    }

    bool resizeFromGrip(const GripDirection direction, const QPointF& delta) {
        bool result = false;
        ElementBoundaryGripItem* grip = boundaryGrip(direction);

        if (nullptr != grip) {
            result = onGripMoved(grip, delta);
        }

        return result;
    }
};

// Scene points of the transition's CURRENTLY persisted line (the model GEOMETRY),
// in path order (source endpoint, intermediate grips, destination endpoint), mapped
// to the scene using the transition item's current transform. recalculateLine()
// writes the drawn poly-line to GEOMETRY, so sampling GEOMETRY whenever the model
// emits modelDataChanged captures exactly what the transition renders AND persists at
// that instant - including a half-updated mid-loop recompute.
QList<QPointF> lineScenePoints(HsmTransition* transition) {
    QList<QPointF> points;
    auto model = transition->modelElement<model::Transition>();

    if (model) {
        const QVariant geometryVar = model->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY);

        if (geometryVar.isValid()) {
            const QPolygonF geometry = geometryVar.value<QPolygonF>();

            for (const QPointF& localPoint : geometry) {
                points.append(transition->mapToScene(localPoint));
            }
        }
    }

    return points;
}

// Scene positions of every grip ITEM on the transition, sorted by (x,y) so the
// comparison against the stored GEOMETRY is order-independent.
QList<QPointF> gripScenePositionsSorted(HsmTransition* transition) {
    QList<QPointF> positions;

    for (QGraphicsItem* child : transition->childItems()) {
        auto* grip = dynamic_cast<ElementGripItem*>(child);

        if (nullptr != grip) {
            positions.append(grip->mapToScene(QPointF(0.0, 0.0)));
        }
    }

    std::sort(positions.begin(), positions.end(), [](const QPointF& a, const QPointF& b) {
        return (a.x() != b.x()) ? (a.x() < b.x()) : (a.y() < b.y());
    });

    return positions;
}

QList<QPointF> sortedCopy(QList<QPointF> points) {
    std::sort(points.begin(), points.end(), [](const QPointF& a, const QPointF& b) {
        return (a.x() != b.x()) ? (a.x() < b.x()) : (a.y() < b.y());
    });
    return points;
}

// A single sample of the transition line captured whenever the transition persists a
// new GEOMETRY, together with the source child's scene position at the same instant so
// the anchors can be compared RELATIVE to the child they connect (the children keep
// their scene positions across the resize, so a stable line keeps a constant offset).
struct LineSample {
    QList<QPointF> linePoints;
    QPointF sourceChildScene;
    QString label;  // "before", "mid-loop #N", or "after"
};

}  // namespace

class HsmTransitionRegripOnResizePropertyTest : public QObject {
    Q_OBJECT

private slots:
    void PropertyAnchorsStableUnderOriginMovingResize();
    void PropertyAnchorsStableUnderAccumulatingDrag();
    void SouthEastControlResizeLeavesTransitionUnchanged();

    // --- Property 2: Preservation (non-bug inputs) - Requirements 3.1 - 3.7 ---
    void PreservationSouthEastResizeRendersChildToChild();       // Req 3.1
    void PreservationChildrenKeepScenePositionsInsideParent();   // Req 3.2
    void PreservationShrinkBelowChildrenIsRejected();            // Req 3.3
    void PreservationTopLevelAndBoundaryTransitionsUnchanged();  // Req 3.4
    void PreservationSelfTransitionRendersUnchanged();           // Req 3.5
    void PreservationNonResizeChildMoveAndGripEditsUnchanged();  // Req 3.6
    void PreservationChildToChildGripsRoundTripSaveReload();     // Req 3.7

private:
    struct Composite {
        QGraphicsScene* scene = nullptr;
        QGraphicsView* view = nullptr;
        ProbeCompositeState* parent = nullptr;
        HsmStateElement* sourceChild = nullptr;
        HsmStateElement* targetChild = nullptr;
        HsmTransition* transition = nullptr;
        // Keep model entities alive: the view elements hold only weak pointers.
        QSharedPointer<model::RegularState> parentModel;
        QSharedPointer<model::RegularState> sourceModel;
        QSharedPointer<model::RegularState> targetModel;
        QSharedPointer<model::Transition> transitionModel;
    };

    static Composite buildComposite(QGraphicsScene* scene,
                                    QGraphicsView* view,
                                    const QSizeF& parentSize,
                                    const QPointF& sourceChildPos,
                                    const QPointF& targetChildPos,
                                    const QList<QPointF>& intermediateGrips);

    static QPointF deltaForGrip(const GripDirection direction, const qreal grow);

    // Returns false and fills the diagnostic out-params when any line point of any
    // recorded sample moved (relative to the source child) away from its "before"
    // relative position. This is allAnchorsStableRelativeToChildren from the Bug
    // Condition / design Property 1, evaluated across EVERY emission (mid-loop
    // included) rather than only the final state.
    static bool allSamplesStableRelativeToChildren(const QList<LineSample>& samples,
                                                   QString& badLabel,
                                                   int& badIndex,
                                                   QPointF& expectedRel,
                                                   QPointF& actualRel);
};

HsmTransitionRegripOnResizePropertyTest::Composite HsmTransitionRegripOnResizePropertyTest::buildComposite(
    QGraphicsScene* scene,
    QGraphicsView* view,
    const QSizeF& parentSize,
    const QPointF& sourceChildPos,
    const QPointF& targetChildPos,
    const QList<QPointF>& intermediateGrips) {
    Q_UNUSED(view);
    Composite composite;
    composite.scene = scene;
    composite.view = view;

    auto parentModel = QSharedPointer<model::RegularState>::create("Active");
    auto sourceModel = QSharedPointer<model::RegularState>::create("Idle");
    auto targetModel = QSharedPointer<model::RegularState>::create("Heating");

    parentModel->addChildState(sourceModel);
    parentModel->addChildState(targetModel);

    auto transitionModel = QSharedPointer<model::Transition>::create(sourceModel, targetModel, "tick");
    parentModel->addTransition(transitionModel);

    composite.parentModel = parentModel;
    composite.sourceModel = sourceModel;
    composite.targetModel = targetModel;
    composite.transitionModel = transitionModel;

    // GEOMETRY: placeholder endpoints (overwritten by recalculateLine) + the
    // requested intermediate grips, in the transition's local frame (which, for a
    // child-to-child transition parented to the composite's body section, matches
    // the child-position frame the grips were authored in).
    QPolygonF geometry;
    geometry.append(QPointF(0.0, 0.0));
    for (const QPointF& grip : intermediateGrips) {
        geometry.append(grip);
    }
    geometry.append(QPointF(0.0, 0.0));
    transitionModel->setMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY, geometry);

    composite.parent = new ProbeCompositeState(parentSize);
    composite.parent->init(parentModel);
    scene->addItem(composite.parent);
    composite.parent->setPos(0.0, 0.0);

    composite.sourceChild = new HsmStateElement(QSizeF(80.0, 50.0));
    composite.sourceChild->init(sourceModel);
    composite.parent->addChildItem(composite.sourceChild);
    composite.sourceChild->setPos(sourceChildPos);

    composite.targetChild = new HsmStateElement(QSizeF(80.0, 50.0));
    composite.targetChild->init(targetModel);
    composite.parent->addChildItem(composite.targetChild);
    composite.targetChild->setPos(targetChildPos);

    composite.transition = new HsmTransition();
    composite.transition->init(transitionModel);
    composite.transition->connectElements(composite.sourceChild, composite.targetChild);
    composite.transition->recalculateLine();

    return composite;
}

QPointF HsmTransitionRegripOnResizePropertyTest::deltaForGrip(const GripDirection direction, const qreal grow) {
    QPointF result;

    switch (direction) {
        case GripDirection::West:
            result = QPointF(-grow, 0.0);
            break;
        case GripDirection::North:
            result = QPointF(0.0, -grow);
            break;
        case GripDirection::East:
            result = QPointF(grow, 0.0);
            break;
        case GripDirection::South:
            result = QPointF(0.0, grow);
            break;
        case GripDirection::NorthWest:
            result = QPointF(-grow, -grow);
            break;
        case GripDirection::NorthEast:
            result = QPointF(grow, -grow);
            break;
        case GripDirection::SouthWest:
            result = QPointF(-grow, grow);
            break;
        default:
            result = QPointF(0.0, 0.0);
            break;
    }

    return result;
}

bool HsmTransitionRegripOnResizePropertyTest::allSamplesStableRelativeToChildren(const QList<LineSample>& samples,
                                                                                 QString& badLabel,
                                                                                 int& badIndex,
                                                                                 QPointF& expectedRel,
                                                                                 QPointF& actualRel) {
    bool stable = true;
    badIndex = -1;

    if (samples.isEmpty() == false) {
        const LineSample& reference = samples.first();

        for (const LineSample& sample : samples) {
            if (sample.linePoints.size() != reference.linePoints.size()) {
                stable = false;
                badLabel = sample.label;
                badIndex = -2;  // point count changed
                break;
            }

            for (int i = 0; i < sample.linePoints.size(); ++i) {
                const QPointF refRel = reference.linePoints.at(i) - reference.sourceChildScene;
                const QPointF sampleRel = sample.linePoints.at(i) - sample.sourceChildScene;

                if (false == pointsEqual(refRel, sampleRel)) {
                    stable = false;
                    badLabel = sample.label;
                    badIndex = i;
                    expectedRel = refRel;
                    actualRel = sampleRel;
                    break;
                }
            }

            if (false == stable) {
                break;
            }
        }
    }

    return stable;
}

/**
 * @brief Property 1 - single large (fast-drag) origin-moving resize keeps every
 *        anchor/grip scene-stable relative to the children AT EVERY EMISSION.
 *
 * Validates: Requirements 1.1, 1.2, 1.3, 2.1, 2.2, 2.3
 */
void HsmTransitionRegripOnResizePropertyTest::PropertyAnchorsStableUnderOriginMovingResize() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    struct GripCase {
        GripDirection direction;
        const char* name;
    };
    const QList<GripCase> originMovingGrips = {
        {GripDirection::West, "West"},
        {GripDirection::North, "North"},
        {GripDirection::East, "East"},
        {GripDirection::South, "South"},
        {GripDirection::NorthWest, "NorthWest"},
        {GripDirection::NorthEast, "NorthEast"},
        {GripDirection::SouthWest, "SouthWest"},
    };

    for (const GripCase& gripCase : originMovingGrips) {
        const int shapeCount = 8;

        for (int shape = 0; shape < shapeCount; ++shape) {
            QGraphicsScene scene;
            QGraphicsView view;
            view.setScene(&scene);

            const QSizeF childSize(80.0, 50.0);

            // Scope the deterministic reported "thermostat" case (Active containing
            // Idle/Heating/Cooling, Idle -> Heating, west/north-west border drag) as an
            // explicit concrete case for reproducibility; randomize the rest.
            const bool reportedCase = (shape == 0) && ((gripCase.direction == GripDirection::West) ||
                                                       (gripCase.direction == GripDirection::NorthWest));

            QPointF sourceChildPos;
            QPointF targetChildPos;
            QList<QPointF> intermediateGrips;

            if (reportedCase) {
                sourceChildPos = QPointF(80.0, 100.0);   // Idle
                targetChildPos = QPointF(380.0, 300.0);  // Heating
                intermediateGrips = {QPointF(240.0, 220.0)};
            } else {
                sourceChildPos = QPointF(70.0 + rng.bounded(0, 40), 90.0 + rng.bounded(0, 40));
                targetChildPos = QPointF(360.0 + rng.bounded(0, 80), 280.0 + rng.bounded(0, 60));

                const int gripCount = rng.bounded(0, 3);  // 0..2 intermediate grips
                for (int g = 0; g < gripCount; ++g) {
                    intermediateGrips.append(QPointF(210.0 + rng.bounded(-20, 60), 180.0 + rng.bounded(-20, 60)));
                }
            }

            qreal maxX = qMax(sourceChildPos.x(), targetChildPos.x()) + childSize.width();
            qreal maxY = qMax(sourceChildPos.y(), targetChildPos.y()) + childSize.height();
            for (const QPointF& grip : intermediateGrips) {
                maxX = qMax(maxX, grip.x());
                maxY = qMax(maxY, grip.y());
            }
            constexpr qreal cSafetyMargin = 260.0;
            const QSizeF parentSize(maxX + cSafetyMargin, maxY + cSafetyMargin);

            Composite composite = buildComposite(&scene, &view, parentSize, sourceChildPos, targetChildPos, intermediateGrips);

            QVERIFY2(lineScenePoints(composite.transition).size() == (2 + intermediateGrips.size()),
                     qPrintable(QString("[%1 shape %2] rendered line has %3 points, expected %4")
                                    .arg(gripCase.name)
                                    .arg(shape)
                                    .arg(lineScenePoints(composite.transition).size())
                                    .arg(2 + intermediateGrips.size())));

            // Sample the transition line at EVERY GEOMETRY emission during the resize -
            // this is what catches a transient mid-loop endpoint jump that the final
            // steady state would hide. The model's modelDataChanged fires from
            // setMetadata(GEOMETRY, ...) inside recalculateLine() each time the stored
            // poly-line changes.
            QList<LineSample> samples;
            samples.append({lineScenePoints(composite.transition),
                            composite.sourceChild->mapToScene(QPointF(0.0, 0.0)),
                            QStringLiteral("before")});

            int midLoopCount = 0;
            HsmTransition* transition = composite.transition;
            HsmStateElement* sourceChild = composite.sourceChild;
            auto connection =
                QObject::connect(composite.transitionModel.data(),
                                 &model::StateMachineEntity::modelDataChanged,
                                 composite.transitionModel.data(),
                                 [&samples, &midLoopCount, transition, sourceChild](QWeakPointer<model::StateMachineEntity>) {
                                     samples.append({lineScenePoints(transition),
                                                     sourceChild->mapToScene(QPointF(0.0, 0.0)),
                                                     QStringLiteral("mid-loop #%1").arg(++midLoopCount)});
                                 });

            // Fast drag: one large origin-moving step.
            const qreal grow = reportedCase ? 90.0 : (70.0 + rng.bounded(0, 60));
            const QPointF delta = deltaForGrip(gripCase.direction, grow);
            const bool resized = composite.parent->resizeFromGrip(gripCase.direction, delta);

            QObject::disconnect(connection);

            QVERIFY2(resized,
                     qPrintable(QString("[%1 shape %2] onGripMoved refused to resize (delta %3,%4)")
                                    .arg(gripCase.name)
                                    .arg(shape)
                                    .arg(delta.x())
                                    .arg(delta.y())));

            samples.append({lineScenePoints(composite.transition),
                            composite.sourceChild->mapToScene(QPointF(0.0, 0.0)),
                            QStringLiteral("after")});

            // Assertion 1 (allAnchorsStableRelativeToChildren, every emission): the
            // whole rendered line - source endpoint, destination endpoint and all
            // intermediate grips - keeps its scene position relative to the connected
            // children through the entire resize, including every mid-loop recompute.
            QString badLabel;
            int badIndex = -1;
            QPointF expectedRel;
            QPointF actualRel;
            const bool stable = allSamplesStableRelativeToChildren(samples, badLabel, badIndex, expectedRel, actualRel);

            QVERIFY2(stable,
                     qPrintable(QString("[%1 shape %2] at '%3' line point %4 moved relative to children (delta %5,%6): "
                                        "expected rel (%7,%8), got rel (%9,%10) [%11 GEOMETRY emissions observed]")
                                    .arg(gripCase.name)
                                    .arg(shape)
                                    .arg(badLabel)
                                    .arg(badIndex)
                                    .arg(delta.x())
                                    .arg(delta.y())
                                    .arg(expectedRel.x())
                                    .arg(expectedRel.y())
                                    .arg(actualRel.x())
                                    .arg(actualRel.y())
                                    .arg(midLoopCount)));

            // Assertion 2: the FINAL stored GEOMETRY matches the grip items the user
            // grabs (stored geometry agrees with the rendered line after compensation).
            const QList<QPointF> geometrySorted = sortedCopy(lineScenePoints(composite.transition));
            const QList<QPointF> gripsSorted = gripScenePositionsSorted(composite.transition);

            QVERIFY2(geometrySorted.size() == gripsSorted.size(),
                     qPrintable(QString("[%1 shape %2] stored GEOMETRY has %3 points but there are %4 grip items")
                                    .arg(gripCase.name)
                                    .arg(shape)
                                    .arg(geometrySorted.size())
                                    .arg(gripsSorted.size())));

            for (int i = 0; i < geometrySorted.size(); ++i) {
                QVERIFY2(pointsEqual(geometrySorted.at(i), gripsSorted.at(i)),
                         qPrintable(QString("[%1 shape %2] stored GEOMETRY point (%3,%4) disagrees with rendered grip "
                                            "(%5,%6)")
                                        .arg(gripCase.name)
                                        .arg(shape)
                                        .arg(geometrySorted.at(i).x())
                                        .arg(geometrySorted.at(i).y())
                                        .arg(gripsSorted.at(i).x())
                                        .arg(gripsSorted.at(i).y())));
            }
        }
    }
}

/**
 * @brief Property 1 - a SEQUENCE of accumulating origin-moving steps (fast multi-step
 *        drag) keeps every anchor/grip scene-stable relative to the children at every
 *        emission across the whole sequence.
 *
 * Validates: Requirements 1.1, 1.2, 1.3, 2.1, 2.2, 2.3
 */
void HsmTransitionRegripOnResizePropertyTest::PropertyAnchorsStableUnderAccumulatingDrag() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    struct GripCase {
        GripDirection direction;
        const char* name;
    };
    const QList<GripCase> originMovingGrips = {
        {GripDirection::West, "West"},
        {GripDirection::North, "North"},
        {GripDirection::NorthWest, "NorthWest"},
        {GripDirection::SouthWest, "SouthWest"},
        {GripDirection::NorthEast, "NorthEast"},
    };

    for (const GripCase& gripCase : originMovingGrips) {
        const int shapeCount = 4;

        for (int shape = 0; shape < shapeCount; ++shape) {
            QGraphicsScene scene;
            QGraphicsView view;
            view.setScene(&scene);

            const QPointF sourceChildPos(80.0 + rng.bounded(0, 30), 100.0 + rng.bounded(0, 30));
            const QPointF targetChildPos(380.0 + rng.bounded(0, 60), 300.0 + rng.bounded(0, 50));
            QList<QPointF> intermediateGrips = {QPointF(240.0, 220.0)};
            if (rng.bounded(0, 2) == 0) {
                intermediateGrips.append(QPointF(300.0, 260.0));
            }

            // Grow generously so a multi-step drag never shrinks below the children.
            const QSizeF parentSize(targetChildPos.x() + 400.0, targetChildPos.y() + 400.0);

            Composite composite = buildComposite(&scene, &view, parentSize, sourceChildPos, targetChildPos, intermediateGrips);

            QList<LineSample> samples;
            samples.append({lineScenePoints(composite.transition),
                            composite.sourceChild->mapToScene(QPointF(0.0, 0.0)),
                            QStringLiteral("before")});

            int emissionCount = 0;
            HsmTransition* transition = composite.transition;
            HsmStateElement* sourceChild = composite.sourceChild;
            auto connection =
                QObject::connect(composite.transitionModel.data(),
                                 &model::StateMachineEntity::modelDataChanged,
                                 composite.transitionModel.data(),
                                 [&samples, &emissionCount, transition, sourceChild](QWeakPointer<model::StateMachineEntity>) {
                                     samples.append({lineScenePoints(transition),
                                                     sourceChild->mapToScene(QPointF(0.0, 0.0)),
                                                     QStringLiteral("step-emission #%1").arg(++emissionCount)});
                                 });

            // Multi-step fast drag: several accumulating origin-moving steps.
            const int steps = 4;
            bool allResized = true;
            for (int step = 0; step < steps; ++step) {
                const qreal grow = 30.0 + rng.bounded(0, 30);
                const QPointF delta = deltaForGrip(gripCase.direction, grow);
                allResized = composite.parent->resizeFromGrip(gripCase.direction, delta) && allResized;
            }

            QObject::disconnect(connection);

            QVERIFY2(allResized,
                     qPrintable(
                         QString("[%1 shape %2] a step of the accumulating drag was rejected").arg(gripCase.name).arg(shape)));

            samples.append({lineScenePoints(composite.transition),
                            composite.sourceChild->mapToScene(QPointF(0.0, 0.0)),
                            QStringLiteral("after")});

            QString badLabel;
            int badIndex = -1;
            QPointF expectedRel;
            QPointF actualRel;
            const bool stable = allSamplesStableRelativeToChildren(samples, badLabel, badIndex, expectedRel, actualRel);

            QVERIFY2(stable,
                     qPrintable(QString("[%1 shape %2] at '%3' line point %4 moved relative to children during the "
                                        "accumulating drag: expected rel (%5,%6), got rel (%7,%8) [%9 emissions]")
                                    .arg(gripCase.name)
                                    .arg(shape)
                                    .arg(badLabel)
                                    .arg(badIndex)
                                    .arg(expectedRel.x())
                                    .arg(expectedRel.y())
                                    .arg(actualRel.x())
                                    .arg(actualRel.y())
                                    .arg(emissionCount)));
        }
    }
}

/**
 * @brief Control case (Requirement 3.1): south-east resize leaves the transition
 *        unchanged (newPositionDelta == 0). Must PASS even on unfixed code.
 *
 * Validates: Requirement 3.1
 */
void HsmTransitionRegripOnResizePropertyTest::SouthEastControlResizeLeavesTransitionUnchanged() {
    QGraphicsScene scene;
    QGraphicsView view;
    view.setScene(&scene);

    Composite composite = buildComposite(&scene,
                                         &view,
                                         QSizeF(760.0, 670.0),
                                         QPointF(80.0, 100.0),   // Idle
                                         QPointF(380.0, 300.0),  // Heating
                                         {QPointF(240.0, 220.0)});

    QList<LineSample> samples;
    samples.append({lineScenePoints(composite.transition),
                    composite.sourceChild->mapToScene(QPointF(0.0, 0.0)),
                    QStringLiteral("before")});

    QVERIFY2(samples.first().linePoints.size() == 3, "control case expects source, one intermediate and destination points");

    int emissionCount = 0;
    HsmTransition* transition = composite.transition;
    HsmStateElement* sourceChild = composite.sourceChild;
    auto connection =
        QObject::connect(composite.transitionModel.data(),
                         &model::StateMachineEntity::modelDataChanged,
                         composite.transitionModel.data(),
                         [&samples, &emissionCount, transition, sourceChild](QWeakPointer<model::StateMachineEntity>) {
                             samples.append({lineScenePoints(transition),
                                             sourceChild->mapToScene(QPointF(0.0, 0.0)),
                                             QStringLiteral("emission #%1").arg(++emissionCount)});
                         });

    const bool resized = composite.parent->resizeFromGrip(GripDirection::SouthEast, QPointF(60.0, 60.0));

    QObject::disconnect(connection);

    QVERIFY2(resized, "south-east resize was unexpectedly rejected");

    samples.append(
        {lineScenePoints(composite.transition), composite.sourceChild->mapToScene(QPointF(0.0, 0.0)), QStringLiteral("after")});

    QString badLabel;
    int badIndex = -1;
    QPointF expectedRel;
    QPointF actualRel;
    const bool stable = allSamplesStableRelativeToChildren(samples, badLabel, badIndex, expectedRel, actualRel);

    QVERIFY2(stable,
             qPrintable(QString("south-east control: at '%1' line point %2 moved relative to children: expected rel "
                                "(%3,%4), got rel (%5,%6)")
                            .arg(badLabel)
                            .arg(badIndex)
                            .arg(expectedRel.x())
                            .arg(expectedRel.y())
                            .arg(actualRel.x())
                            .arg(actualRel.y())));
}

// ===================================================================================================================
// Property 2: Preservation - Unchanged Behavior for Non-Bug Inputs (Requirements 3.1 - 3.7)
//
// Observation-first methodology: every slot below records behavior produced by the CURRENT
// (unfixed) code and asserts exactly that. They MUST PASS on the unfixed code - they lock in the
// baseline the later fix must not change (design "Preservation Checking": F == F'). The fix in
// task 3 only alters child-to-child transition endpoints during an ORIGIN-MOVING resize
// (isBugCondition == true); none of the inputs exercised here match that condition, so each of
// these slots must keep passing after the fix with no change.
// ===================================================================================================================

/**
 * @brief Property 2 (Requirement 3.1): a south-east resize renders child-to-child transitions
 *        correctly and the stored GEOMETRY equals its rendered grip positions.
 *
 * The south-east grip keeps the top-left origin fixed (newPositionDelta == 0), so the
 * compensation loop is a no-op and the transition is NOT in the bug condition. Across randomized
 * composites we observe that after the resize every stored GEOMETRY point coincides with the
 * transition's rendered grip items (source, intermediates, destination) - the behavior the unfixed
 * code already produces - and assert it stays that way.
 *
 * Validates: Requirement 3.1
 */
void HsmTransitionRegripOnResizePropertyTest::PreservationSouthEastResizeRendersChildToChild() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    const int shapeCount = 12;

    for (int shape = 0; shape < shapeCount; ++shape) {
        QGraphicsScene scene;
        QGraphicsView view;
        view.setScene(&scene);

        const QPointF sourceChildPos(70.0 + rng.bounded(0, 40), 90.0 + rng.bounded(0, 40));
        const QPointF targetChildPos(360.0 + rng.bounded(0, 80), 280.0 + rng.bounded(0, 60));

        QList<QPointF> intermediateGrips;
        const int gripCount = rng.bounded(0, 3);  // 0..2 intermediate grips
        for (int g = 0; g < gripCount; ++g) {
            intermediateGrips.append(QPointF(210.0 + rng.bounded(-20, 60), 180.0 + rng.bounded(-20, 60)));
        }

        const QSizeF parentSize(targetChildPos.x() + 260.0, targetChildPos.y() + 260.0);
        Composite composite = buildComposite(&scene, &view, parentSize, sourceChildPos, targetChildPos, intermediateGrips);

        // South-east grip: newPositionDelta == 0, not an origin move -> NOT isBugCondition.
        const qreal grow = 40.0 + rng.bounded(0, 80);
        const bool resized = composite.parent->resizeFromGrip(GripDirection::SouthEast, QPointF(grow, grow));

        QVERIFY2(resized, qPrintable(QString("[shape %1] south-east resize was unexpectedly rejected").arg(shape)));

        // The stored GEOMETRY must agree with the grip items the user grabs (rendered line).
        const QList<QPointF> geometrySorted = sortedCopy(lineScenePoints(composite.transition));
        const QList<QPointF> gripsSorted = gripScenePositionsSorted(composite.transition);

        QVERIFY2(geometrySorted.size() == gripsSorted.size(),
                 qPrintable(QString("[shape %1] stored GEOMETRY has %2 points but there are %3 grip items")
                                .arg(shape)
                                .arg(geometrySorted.size())
                                .arg(gripsSorted.size())));

        for (int i = 0; i < geometrySorted.size(); ++i) {
            QVERIFY2(pointsEqual(geometrySorted.at(i), gripsSorted.at(i)),
                     qPrintable(QString("[shape %1] after south-east resize GEOMETRY point (%2,%3) disagrees with "
                                        "rendered grip (%4,%5)")
                                    .arg(shape)
                                    .arg(geometrySorted.at(i).x())
                                    .arg(geometrySorted.at(i).y())
                                    .arg(gripsSorted.at(i).x())
                                    .arg(gripsSorted.at(i).y())));
        }
    }
}

/**
 * @brief Property 2 (Requirement 3.2): a resize repositions child states so they keep their
 *        scene positions and stay inside the resized parent.
 *
 * For each randomized composite and each origin-moving grip we record both children's scene
 * positions before the resize, perform the resize, and assert each child kept its scene position
 * (the per-child compensation the fix must preserve) and still lies within the parent's element
 * rect in scene coordinates. This is pure child-repositioning behavior, independent of the
 * transition endpoint defect, and must be identical before and after the fix.
 *
 * Validates: Requirement 3.2
 */
void HsmTransitionRegripOnResizePropertyTest::PreservationChildrenKeepScenePositionsInsideParent() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    struct GripCase {
        GripDirection direction;
        const char* name;
    };
    const QList<GripCase> grips = {
        {GripDirection::West, "West"},
        {GripDirection::North, "North"},
        {GripDirection::East, "East"},
        {GripDirection::South, "South"},
        {GripDirection::NorthWest, "NorthWest"},
        {GripDirection::NorthEast, "NorthEast"},
        {GripDirection::SouthWest, "SouthWest"},
        {GripDirection::SouthEast, "SouthEast"},
    };

    for (const GripCase& gripCase : grips) {
        const int shapeCount = 4;

        for (int shape = 0; shape < shapeCount; ++shape) {
            QGraphicsScene scene;
            QGraphicsView view;
            view.setScene(&scene);

            const QPointF sourceChildPos(80.0 + rng.bounded(0, 40), 100.0 + rng.bounded(0, 40));
            const QPointF targetChildPos(360.0 + rng.bounded(0, 80), 300.0 + rng.bounded(0, 60));
            QList<QPointF> intermediateGrips = {QPointF(240.0, 220.0)};

            const QSizeF parentSize(targetChildPos.x() + 300.0, targetChildPos.y() + 300.0);
            Composite composite = buildComposite(&scene, &view, parentSize, sourceChildPos, targetChildPos, intermediateGrips);

            const QPointF sourceSceneBefore = composite.sourceChild->mapToScene(QPointF(0.0, 0.0));
            const QPointF targetSceneBefore = composite.targetChild->mapToScene(QPointF(0.0, 0.0));

            // Grow only (positive) so the resize is never rejected.
            const qreal grow = 50.0 + rng.bounded(0, 60);
            const QPointF delta = deltaForGrip(gripCase.direction, grow);
            const bool resized = (gripCase.direction == GripDirection::SouthEast)
                                     ? composite.parent->resizeFromGrip(GripDirection::SouthEast, QPointF(grow, grow))
                                     : composite.parent->resizeFromGrip(gripCase.direction, delta);

            QVERIFY2(resized, qPrintable(QString("[%1 shape %2] resize unexpectedly rejected").arg(gripCase.name).arg(shape)));

            const QPointF sourceSceneAfter = composite.sourceChild->mapToScene(QPointF(0.0, 0.0));
            const QPointF targetSceneAfter = composite.targetChild->mapToScene(QPointF(0.0, 0.0));

            QVERIFY2(pointsEqual(sourceSceneBefore, sourceSceneAfter) && pointsEqual(targetSceneBefore, targetSceneAfter),
                     qPrintable(QString("[%1 shape %2] a child moved in scene across the resize: source (%3,%4)->(%5,%6), "
                                        "target (%7,%8)->(%9,%10)")
                                    .arg(gripCase.name)
                                    .arg(shape)
                                    .arg(sourceSceneBefore.x())
                                    .arg(sourceSceneBefore.y())
                                    .arg(sourceSceneAfter.x())
                                    .arg(sourceSceneAfter.y())
                                    .arg(targetSceneBefore.x())
                                    .arg(targetSceneBefore.y())
                                    .arg(targetSceneAfter.x())
                                    .arg(targetSceneAfter.y())));

            // Each child must stay within the resized parent (parent rect mapped to scene).
            const QRectF parentSceneRect = composite.parent->mapRectToScene(composite.parent->elementRect());
            const QRectF sourceSceneRect = composite.sourceChild->mapRectToScene(composite.sourceChild->elementRect());
            const QRectF targetSceneRect = composite.targetChild->mapRectToScene(composite.targetChild->elementRect());

            QVERIFY2(parentSceneRect.contains(sourceSceneRect) && parentSceneRect.contains(targetSceneRect),
                     qPrintable(QString("[%1 shape %2] a child left the resized parent bounds").arg(gripCase.name).arg(shape)));
        }
    }
}

/**
 * @brief Property 2 (Requirement 3.3): a shrink below the size needed to contain the children is
 *        rejected.
 *
 * Starting from a composite whose children fill most of the body, we attempt to shrink it from an
 * origin-moving grip by an amount large enough that the children would no longer fit. The unfixed
 * code's childrenRect() fit/reject guard refuses such a resize (returns false) and leaves the
 * transition's stored GEOMETRY untouched; we assert both. The fix does not touch the fit/reject
 * logic, so this must stay rejected.
 *
 * Validates: Requirement 3.3
 */
void HsmTransitionRegripOnResizePropertyTest::PreservationShrinkBelowChildrenIsRejected() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    struct GripCase {
        GripDirection direction;
        const char* name;
    };
    const QList<GripCase> grips = {
        {GripDirection::West, "West"},
        {GripDirection::North, "North"},
        {GripDirection::NorthWest, "NorthWest"},
        {GripDirection::East, "East"},
        {GripDirection::South, "South"},
    };

    for (const GripCase& gripCase : grips) {
        const int shapeCount = 4;

        for (int shape = 0; shape < shapeCount; ++shape) {
            QGraphicsScene scene;
            QGraphicsView view;
            view.setScene(&scene);

            // Children placed so they nearly fill the parent body: little room to shrink.
            const QPointF sourceChildPos(40.0 + rng.bounded(0, 20), 40.0 + rng.bounded(0, 20));
            const QPointF targetChildPos(260.0 + rng.bounded(0, 40), 220.0 + rng.bounded(0, 40));
            QList<QPointF> intermediateGrips = {QPointF(180.0, 150.0)};

            // Tight parent: children occupy most of it (child size is 80x50 in buildComposite).
            const QSizeF parentSize(targetChildPos.x() + 80.0 + 20.0, targetChildPos.y() + 50.0 + 20.0);
            Composite composite = buildComposite(&scene, &view, parentSize, sourceChildPos, targetChildPos, intermediateGrips);

            const QList<QPointF> geometryBefore = lineScenePoints(composite.transition);

            // A large shrink toward the children: the fit/reject guard must refuse it.
            qreal shrink = 0.0;
            QPointF delta;
            switch (gripCase.direction) {
                case GripDirection::West:
                    shrink = parentSize.width();  // push the left edge far past the children
                    delta = QPointF(shrink, 0.0);
                    break;
                case GripDirection::North:
                    shrink = parentSize.height();
                    delta = QPointF(0.0, shrink);
                    break;
                case GripDirection::NorthWest:
                    delta = QPointF(parentSize.width(), parentSize.height());
                    break;
                case GripDirection::East:
                    delta = QPointF(-parentSize.width(), 0.0);  // pull the right edge inward
                    break;
                case GripDirection::South:
                    delta = QPointF(0.0, -parentSize.height());
                    break;
                default:
                    delta = QPointF(0.0, 0.0);
                    break;
            }

            const bool resized = composite.parent->resizeFromGrip(gripCase.direction, delta);

            QVERIFY2(false == resized,
                     qPrintable(
                         QString("[%1 shape %2] shrink below children bounds was NOT rejected").arg(gripCase.name).arg(shape)));

            // A rejected resize must leave the transition geometry untouched.
            const QList<QPointF> geometryAfter = lineScenePoints(composite.transition);
            QVERIFY2(
                geometryBefore.size() == geometryAfter.size(),
                qPrintable(
                    QString("[%1 shape %2] rejected resize changed the transition point count").arg(gripCase.name).arg(shape)));
            for (int i = 0; i < geometryBefore.size(); ++i) {
                QVERIFY2(pointsEqual(geometryBefore.at(i), geometryAfter.at(i)),
                         qPrintable(QString("[%1 shape %2] rejected resize moved transition point %3")
                                        .arg(gripCase.name)
                                        .arg(shape)
                                        .arg(i)));
            }
        }
    }
}

/**
 * @brief Property 2 (Requirement 3.4): top-level transitions and boundary-crossing transitions
 *        (one endpoint outside the composite) render correctly through a resize.
 *
 * Such transitions are NOT direct children of the resized composite, so the compensation loop
 * never drives their recalculateLine(); they are outside the bug condition. We build a composite
 * with one inner child and one SIBLING state outside it, connect an inner->outer transition
 * (boundary-crossing), resize the composite from an origin-moving grip, and assert the transition's
 * endpoints stay scene-stable relative to the two states it connects (both of which keep their
 * scene positions). This holds on the unfixed code and must continue to hold.
 *
 * Validates: Requirement 3.4
 */
void HsmTransitionRegripOnResizePropertyTest::PreservationTopLevelAndBoundaryTransitionsUnchanged() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    struct GripCase {
        GripDirection direction;
        const char* name;
    };
    const QList<GripCase> grips = {
        {GripDirection::West, "West"},
        {GripDirection::North, "North"},
        {GripDirection::NorthWest, "NorthWest"},
        {GripDirection::SouthEast, "SouthEast"},
    };

    for (const GripCase& gripCase : grips) {
        const int shapeCount = 4;

        for (int shape = 0; shape < shapeCount; ++shape) {
            QGraphicsScene scene;
            QGraphicsView view;
            view.setScene(&scene);

            // Model: a composite "Active" with ONE inner child "Idle"; a sibling top-level
            // state "Outside". A boundary-crossing transition connects Idle -> Outside.
            auto parentModel = QSharedPointer<model::RegularState>::create("Active");
            auto innerModel = QSharedPointer<model::RegularState>::create("Idle");
            auto outsideModel = QSharedPointer<model::RegularState>::create("Outside");

            parentModel->addChildState(innerModel);
            auto crossingModel = QSharedPointer<model::Transition>::create(innerModel, outsideModel, "leave");
            parentModel->addTransition(crossingModel);

            const QSizeF parentSize(380.0 + rng.bounded(0, 80), 320.0 + rng.bounded(0, 60));

            auto* parent = new ProbeCompositeState(parentSize);
            parent->init(parentModel);
            scene.addItem(parent);
            parent->setPos(0.0, 0.0);

            auto* innerChild = new HsmStateElement(QSizeF(80.0, 50.0));
            innerChild->init(innerModel);
            parent->addChildItem(innerChild);
            innerChild->setPos(QPointF(80.0 + rng.bounded(0, 30), 100.0 + rng.bounded(0, 30)));

            // Sibling state OUTSIDE the composite, added to the scene directly.
            auto* outsideElement = new HsmStateElement(QSizeF(80.0, 50.0));
            outsideElement->init(outsideModel);
            scene.addItem(outsideElement);
            outsideElement->setPos(QPointF(parentSize.width() + 120.0, 200.0 + rng.bounded(0, 40)));

            auto* crossing = new HsmTransition();
            crossing->init(crossingModel);
            crossing->connectElements(innerChild, outsideElement);
            crossing->recalculateLine();

            // Keep model entities alive beyond this scope end? The scene owns the items; the
            // models are held by the shared pointers until the loop iteration ends, which is
            // after all assertions, so they stay valid for the whole observation.

            const QPointF innerSceneBefore = innerChild->mapToScene(QPointF(0.0, 0.0));
            const QPointF outsideSceneBefore = outsideElement->mapToScene(QPointF(0.0, 0.0));
            const QList<QPointF> lineBefore = lineScenePoints(crossing);

            const qreal grow = 50.0 + rng.bounded(0, 60);
            const QPointF delta = deltaForGrip(gripCase.direction, grow);
            const bool resized = (gripCase.direction == GripDirection::SouthEast)
                                     ? parent->resizeFromGrip(GripDirection::SouthEast, QPointF(grow, grow))
                                     : parent->resizeFromGrip(gripCase.direction, delta);

            QVERIFY2(resized, qPrintable(QString("[%1 shape %2] resize unexpectedly rejected").arg(gripCase.name).arg(shape)));

            const QPointF innerSceneAfter = innerChild->mapToScene(QPointF(0.0, 0.0));
            const QPointF outsideSceneAfter = outsideElement->mapToScene(QPointF(0.0, 0.0));
            const QList<QPointF> lineAfter = lineScenePoints(crossing);

            // Both endpoints keep their scene positions, so the crossing transition line must keep
            // its scene position relative to the inner child (its source).
            QVERIFY2(
                pointsEqual(outsideSceneBefore, outsideSceneAfter),
                qPrintable(
                    QString("[%1 shape %2] the outside state moved in scene during the resize").arg(gripCase.name).arg(shape)));

            QVERIFY2(
                lineBefore.size() == lineAfter.size(),
                qPrintable(QString("[%1 shape %2] boundary transition point count changed").arg(gripCase.name).arg(shape)));

            for (int i = 0; i < lineBefore.size(); ++i) {
                const QPointF relBefore = lineBefore.at(i) - innerSceneBefore;
                const QPointF relAfter = lineAfter.at(i) - innerSceneAfter;
                QVERIFY2(pointsEqual(relBefore, relAfter),
                         qPrintable(QString("[%1 shape %2] boundary transition point %3 moved relative to the inner child: "
                                            "rel (%4,%5) -> (%6,%7)")
                                        .arg(gripCase.name)
                                        .arg(shape)
                                        .arg(i)
                                        .arg(relBefore.x())
                                        .arg(relBefore.y())
                                        .arg(relAfter.x())
                                        .arg(relAfter.y())));
            }
        }
    }
}

/**
 * @brief Property 2 (Requirement 3.5): a self-transition (source == target) on a child of the
 *        resized composite renders correctly through the resize.
 *
 * A self-transition is NOT wired to a from/to pair of distinct children (connectElements skips the
 * geometryChanged->recalculateLine connection for self-transitions and parents it to the child
 * itself), so it is outside the bug condition. Its geometry is derived from its parent child's
 * rect with fixed offsets. We resize the composite from an origin-moving grip and assert the
 * self-transition line keeps its scene position relative to the child it loops on (which keeps its
 * scene position). This holds on the unfixed code and must continue to hold.
 *
 * Validates: Requirement 3.5
 */
void HsmTransitionRegripOnResizePropertyTest::PreservationSelfTransitionRendersUnchanged() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    struct GripCase {
        GripDirection direction;
        const char* name;
    };
    const QList<GripCase> grips = {
        {GripDirection::West, "West"},
        {GripDirection::North, "North"},
        {GripDirection::NorthWest, "NorthWest"},
        {GripDirection::SouthWest, "SouthWest"},
    };

    for (const GripCase& gripCase : grips) {
        const int shapeCount = 4;

        for (int shape = 0; shape < shapeCount; ++shape) {
            QGraphicsScene scene;
            QGraphicsView view;
            view.setScene(&scene);

            auto parentModel = QSharedPointer<model::RegularState>::create("Active");
            auto childModel = QSharedPointer<model::RegularState>::create("Idle");

            parentModel->addChildState(childModel);
            auto selfModel = QSharedPointer<model::Transition>::create(childModel, childModel, "loop");
            parentModel->addTransition(selfModel);

            const QSizeF parentSize(360.0 + rng.bounded(0, 80), 320.0 + rng.bounded(0, 60));

            auto* parent = new ProbeCompositeState(parentSize);
            parent->init(parentModel);
            scene.addItem(parent);
            parent->setPos(0.0, 0.0);

            auto* child = new HsmStateElement(QSizeF(80.0, 50.0));
            child->init(childModel);
            parent->addChildItem(child);
            child->setPos(QPointF(90.0 + rng.bounded(0, 40), 110.0 + rng.bounded(0, 40)));

            auto* self = new HsmTransition();
            self->init(selfModel);
            self->connectElements(child, child);
            self->recalculateLine();

            QVERIFY2(self->isSelfTransition(), "constructed transition is not recognised as a self-transition");

            const QPointF childSceneBefore = child->mapToScene(QPointF(0.0, 0.0));
            const QList<QPointF> lineBefore = lineScenePoints(self);

            const qreal grow = 50.0 + rng.bounded(0, 60);
            const QPointF delta = deltaForGrip(gripCase.direction, grow);
            const bool resized = parent->resizeFromGrip(gripCase.direction, delta);

            QVERIFY2(resized, qPrintable(QString("[%1 shape %2] resize unexpectedly rejected").arg(gripCase.name).arg(shape)));

            const QPointF childSceneAfter = child->mapToScene(QPointF(0.0, 0.0));
            const QList<QPointF> lineAfter = lineScenePoints(self);

            QVERIFY2(lineBefore.size() == lineAfter.size(),
                     qPrintable(QString("[%1 shape %2] self-transition point count changed").arg(gripCase.name).arg(shape)));

            for (int i = 0; i < lineBefore.size(); ++i) {
                const QPointF relBefore = lineBefore.at(i) - childSceneBefore;
                const QPointF relAfter = lineAfter.at(i) - childSceneAfter;
                QVERIFY2(pointsEqual(relBefore, relAfter),
                         qPrintable(QString("[%1 shape %2] self-transition point %3 moved relative to its child: "
                                            "rel (%4,%5) -> (%6,%7)")
                                        .arg(gripCase.name)
                                        .arg(shape)
                                        .arg(i)
                                        .arg(relBefore.x())
                                        .arg(relBefore.y())
                                        .arg(relAfter.x())
                                        .arg(relAfter.y())));
            }
        }
    }
}

/**
 * @brief Property 2 (Requirement 3.6): with NO resize in progress, moving a child or
 *        adding/dragging an intermediate transition grip recomputes and renders the transition
 *        correctly.
 *
 * These are the ordinary interactive edits the direct geometryChanged->recalculateLine connection
 * is meant to serve. Because no origin-moving resize loop is running, they are outside the bug
 * condition. We (a) move a connected child and assert the transition still agrees with its grip
 * items, and (b) drag an intermediate grip and assert the new grip position is reflected in the
 * stored GEOMETRY. These behaviors hold on the unfixed code and the fix (which only changes the
 * resize loop) must not alter them.
 *
 * Validates: Requirement 3.6
 */
void HsmTransitionRegripOnResizePropertyTest::PreservationNonResizeChildMoveAndGripEditsUnchanged() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    const int shapeCount = 10;

    for (int shape = 0; shape < shapeCount; ++shape) {
        QGraphicsScene scene;
        QGraphicsView view;
        view.setScene(&scene);

        const QSizeF childSize(80.0, 50.0);
        const QPointF sourceChildPos(80.0 + rng.bounded(0, 30), 100.0 + rng.bounded(0, 30));
        const QPointF targetChildPos(360.0 + rng.bounded(0, 60), 300.0 + rng.bounded(0, 50));

        // Author the single interior grip at the midpoint of the two children's centres. That point
        // lies strictly inside the open gap between the two child rectangles, so it can never
        // coincide with a re-derived endpoint (endpoints are anchored ON a child rect edge by
        // findPerpendicularEdgePoint). This keeps the interior grip provably distinct from BOTH
        // endpoint grips regardless of the randomised child placement, which is what makes the
        // grip selection below deterministic (see the root-cause note on grip selection).
        const QPointF sourceCentre = sourceChildPos + QPointF(childSize.width() / 2.0, childSize.height() / 2.0);
        const QPointF targetCentre = targetChildPos + QPointF(childSize.width() / 2.0, childSize.height() / 2.0);
        const QPointF interiorGripLocal = (sourceCentre + targetCentre) / 2.0;
        QList<QPointF> intermediateGrips = {interiorGripLocal};

        const QSizeF parentSize(targetChildPos.x() + 300.0, targetChildPos.y() + 300.0);
        Composite composite = buildComposite(&scene, &view, parentSize, sourceChildPos, targetChildPos, intermediateGrips);

        // --- (a) Move a connected child WITHOUT any resize. recalculateLine fires via the direct
        //         geometryChanged connection; afterward GEOMETRY must agree with the grip items. ---
        const QPointF moveDelta(20.0 + rng.bounded(0, 40), -15.0 + rng.bounded(0, 30));
        composite.targetChild->setPos(composite.targetChild->pos() + moveDelta);

        {
            const QList<QPointF> geometrySorted = sortedCopy(lineScenePoints(composite.transition));
            const QList<QPointF> gripsSorted = gripScenePositionsSorted(composite.transition);

            QVERIFY2(geometrySorted.size() == gripsSorted.size(),
                     qPrintable(QString("[shape %1] after child move GEOMETRY/grip count mismatch (%2 vs %3)")
                                    .arg(shape)
                                    .arg(geometrySorted.size())
                                    .arg(gripsSorted.size())));
            for (int i = 0; i < geometrySorted.size(); ++i) {
                QVERIFY2(pointsEqual(geometrySorted.at(i), gripsSorted.at(i)),
                         qPrintable(QString("[shape %1] after child move GEOMETRY point (%2,%3) disagrees with grip (%4,%5)")
                                        .arg(shape)
                                        .arg(geometrySorted.at(i).x())
                                        .arg(geometrySorted.at(i).y())
                                        .arg(gripsSorted.at(i).x())
                                        .arg(gripsSorted.at(i).y())));
            }
        }

        // --- (b) Drag the intermediate grip WITHOUT any resize. The dragged grip's new local
        //         position must be reflected in the stored GEOMETRY (an interior point). ---
        // Select the interior grip by its INDEX in the stored line path, NOT by matching a fixed
        // coordinate. The stored GEOMETRY is [source endpoint, interior grip, destination endpoint],
        // so the only interior point is at index 1 while index 0 and the last index are the
        // re-derived endpoints. Matching by coordinate value is nondeterministic: recalculateLine()
        // can re-derive an endpoint that coincidentally lands on the authored interior coordinate,
        // in which case we grab an ENDPOINT grip instead. Dragging an endpoint grip is ignored by
        // HsmTransition::onGripMoved (only grips with 0 < index < last are interior-draggable), so
        // the drag never reaches GEOMETRY and the assertion fails. We instead read the interior
        // point straight from GEOMETRY index 1 and pick the grip sitting on it, having guaranteed by
        // construction above that this point is distinct from both endpoints.
        const QVariant lineGeometryVar =
            composite.transitionModel->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY);
        QVERIFY2(lineGeometryVar.isValid(), qPrintable(QString("[shape %1] GEOMETRY missing before grip drag").arg(shape)));
        const QPolygonF lineGeometry = lineGeometryVar.value<QPolygonF>();

        QVERIFY2(lineGeometry.size() == 3,
                 qPrintable(QString("[shape %1] expected [source, interior, destination] line, got %2 points")
                                .arg(shape)
                                .arg(lineGeometry.size())));

        const QPointF interiorPoint = lineGeometry.at(1);
        const QPointF sourceEndpoint = lineGeometry.first();
        const QPointF destEndpoint = lineGeometry.last();

        // The construction above must hold: the interior point is distinct from both endpoints, so a
        // position match below is unambiguous.
        QVERIFY2((false == pointsEqual(interiorPoint, sourceEndpoint)) && (false == pointsEqual(interiorPoint, destEndpoint)),
                 qPrintable(QString("[shape %1] interior point (%2,%3) coincides with an endpoint; selection would be "
                                    "ambiguous")
                                .arg(shape)
                                .arg(interiorPoint.x())
                                .arg(interiorPoint.y())));

        ElementGripItem* intermediate = nullptr;
        for (QGraphicsItem* item : composite.transition->childItems()) {
            auto* grip = dynamic_cast<ElementGripItem*>(item);

            if ((nullptr != grip) && pointsEqual(grip->pos(), interiorPoint)) {
                intermediate = grip;
                break;
            }
        }

        QVERIFY2(nullptr != intermediate, qPrintable(QString("[shape %1] could not locate the interior grip").arg(shape)));

        const QPointF gripDragDelta(15.0 + rng.bounded(0, 30), 10.0 + rng.bounded(0, 30));
        const QPointF newLocal = intermediate->pos() + gripDragDelta;
        intermediate->setPos(newLocal);
        // Mimic an interactive grip drag through the real signal path: the grip emits gripMoved,
        // which HsmTransition wires to its onGripMoved slot (a protected slot, invoked here via the
        // meta-object system exactly as the queued/direct signal delivery would).
        const bool gripHandled = QMetaObject::invokeMethod(composite.transition,
                                                           "onGripMoved",
                                                           Qt::DirectConnection,
                                                           Q_ARG(view::ElementGripItem*, intermediate),
                                                           Q_ARG(QPointF, gripDragDelta));
        QVERIFY2(gripHandled, qPrintable(QString("[shape %1] onGripMoved slot invocation failed").arg(shape)));

        {
            const QVariant geometryVar =
                composite.transitionModel->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY);
            QVERIFY2(geometryVar.isValid(), qPrintable(QString("[shape %1] GEOMETRY missing after grip drag").arg(shape)));
            const QPolygonF geometry = geometryVar.value<QPolygonF>();

            bool found = false;
            for (const QPointF& p : geometry) {
                if (pointsEqual(p, newLocal)) {
                    found = true;
                    break;
                }
            }

            QVERIFY2(found,
                     qPrintable(QString("[shape %1] dragged grip local position (%2,%3) not reflected in stored GEOMETRY")
                                    .arg(shape)
                                    .arg(newLocal.x())
                                    .arg(newLocal.y())));
        }
    }
}

/**
 * @brief Property 2 (Requirement 3.7): child-to-child transition grips round-trip through the
 *        localGeometry (GEOMETRY metadata) save/reload path.
 *
 * recalculateLine() persists the resolved poly-line to MetadataKey::GEOMETRY (the localGeometry the
 * importer/exporter serializes). We capture that stored geometry, build a FRESH transition from the
 * SAME model (as a reload would), and assert connectElements reconstructs intermediate grips at
 * exactly the stored local positions and reproduces the same rendered scene line. This exercises the
 * persistence path the fix leaves untouched.
 *
 * Validates: Requirement 3.7
 */
void HsmTransitionRegripOnResizePropertyTest::PreservationChildToChildGripsRoundTripSaveReload() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    const int shapeCount = 10;

    for (int shape = 0; shape < shapeCount; ++shape) {
        QGraphicsScene scene;
        QGraphicsView view;
        view.setScene(&scene);

        const QPointF sourceChildPos(80.0 + rng.bounded(0, 40), 100.0 + rng.bounded(0, 40));
        const QPointF targetChildPos(360.0 + rng.bounded(0, 80), 300.0 + rng.bounded(0, 60));

        QList<QPointF> intermediateGrips;
        const int gripCount = 1 + rng.bounded(0, 2);  // 1..2 intermediate grips
        for (int g = 0; g < gripCount; ++g) {
            intermediateGrips.append(QPointF(210.0 + rng.bounded(-20, 60), 180.0 + rng.bounded(-20, 60)));
        }

        const QSizeF parentSize(targetChildPos.x() + 260.0, targetChildPos.y() + 260.0);
        Composite composite = buildComposite(&scene, &view, parentSize, sourceChildPos, targetChildPos, intermediateGrips);

        // "Save": capture the persisted localGeometry and the rendered scene line.
        const QVariant savedGeometryVar =
            composite.transitionModel->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY);
        QVERIFY2(savedGeometryVar.isValid(), qPrintable(QString("[shape %1] no GEOMETRY to save").arg(shape)));
        const QPolygonF savedGeometry = savedGeometryVar.value<QPolygonF>();
        const QList<QPointF> savedSceneLine = lineScenePoints(composite.transition);

        // "Reload": a fresh transition item built from the SAME model entity reconstructs grips from
        // the stored localGeometry (connectElements reads GEOMETRY to recreate intermediate grips).
        auto* reloaded = new HsmTransition();
        reloaded->init(composite.transitionModel);
        reloaded->connectElements(composite.sourceChild, composite.targetChild);
        reloaded->recalculateLine();

        // Intermediate grip local positions must be restored exactly (endpoints are re-derived).
        const QVariant reloadedGeometryVar =
            composite.transitionModel->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY);
        QVERIFY2(reloadedGeometryVar.isValid(), qPrintable(QString("[shape %1] no GEOMETRY after reload").arg(shape)));
        const QPolygonF reloadedGeometry = reloadedGeometryVar.value<QPolygonF>();

        QVERIFY2(reloadedGeometry.size() == savedGeometry.size(),
                 qPrintable(QString("[shape %1] reloaded geometry point count %2 != saved %3")
                                .arg(shape)
                                .arg(reloadedGeometry.size())
                                .arg(savedGeometry.size())));

        // Interior points (index 1 .. size-2) are the stored localGeometry grips; they must be
        // restored unchanged.
        for (int i = 1; i < savedGeometry.size() - 1; ++i) {
            QVERIFY2(pointsEqual(savedGeometry.at(i), reloadedGeometry.at(i)),
                     qPrintable(QString("[shape %1] interior grip %2 not restored: saved (%3,%4) -> reloaded (%5,%6)")
                                    .arg(shape)
                                    .arg(i)
                                    .arg(savedGeometry.at(i).x())
                                    .arg(savedGeometry.at(i).y())
                                    .arg(reloadedGeometry.at(i).x())
                                    .arg(reloadedGeometry.at(i).y())));
        }

        // The rendered scene line must be reproduced identically after reload.
        const QList<QPointF> reloadedSceneLine = lineScenePoints(reloaded);
        QVERIFY2(reloadedSceneLine.size() == savedSceneLine.size(),
                 qPrintable(QString("[shape %1] reloaded scene line point count changed").arg(shape)));
        for (int i = 0; i < savedSceneLine.size(); ++i) {
            QVERIFY2(pointsEqual(savedSceneLine.at(i), reloadedSceneLine.at(i)),
                     qPrintable(QString("[shape %1] reloaded scene line point %2 differs: (%3,%4) -> (%5,%6)")
                                    .arg(shape)
                                    .arg(i)
                                    .arg(savedSceneLine.at(i).x())
                                    .arg(savedSceneLine.at(i).y())
                                    .arg(reloadedSceneLine.at(i).x())
                                    .arg(reloadedSceneLine.at(i).y())));
        }
    }
}

int runHsmTransitionRegripOnResizePropertyTest(int argc, char** argv) {
    QApplication app(argc, argv);

    HsmTransitionRegripOnResizePropertyTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "HsmTransitionRegripOnResizePropertyTest.moc"
