#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPolygonF>
#include <QRandomGenerator>
#include <QtTest>

#include "model/elements/RegularState.hpp"
#include "model/elements/Transition.hpp"
#include "view/elements/HsmStateElement.hpp"
#include "view/elements/HsmTransition.hpp"
#include "view/elements/private/ElementGripItem.hpp"
#include "view/elements/private/styles/HsmRectangularElement.hpp"

using namespace view;

// ---------------------------------------------------------------------------
// Property 2 / Property 4 - Preservation of hint-less transition anchors and of
// the explicit-size constructor.
//
// These tests build transitions WITHOUT anchor-factor hints and assert the resolved endpoints
// equal the geometric helpers exactly (findStartingPointFromRect for the empty mLinePath case,
// findStartingPointFromPoint for the non-empty case), pinning the purely geometric routing.
// ---------------------------------------------------------------------------

// Test-only subclass that exposes the protected geometric helpers and the mapped
// endpoint rects, so the test can compare the actual recalculateLine() result
// against a reference computed with the SAME helpers on the SAME rects.
class ProbeTransition : public HsmTransition {
public:
    QPointF probeStartFromRect(const QRectF& rectFrom, const QRectF& rectTo) {
        return findStartingPointFromRect(rectFrom, rectTo);
    }

    QPointF probeStartFromPoint(const QRectF& rectFrom, const QPointF& pointTo) {
        return findStartingPointFromPoint(rectFrom, pointTo);
    }

    QPointF probeEndFromPoint(const QRectF& rectTo, const QPointF& prevPoint) {
        return findPerpendicularEdgePoint(rectTo, prevPoint);
    }

    bool probeStraight(const QRectF& rectFrom, const QRectF& rectTo, QPointF& outFrom, QPointF& outTo) {
        return findStraightPerpendicularConnection(rectFrom, rectTo, outFrom, outTo);
    }

    // Mapped source/target rect exactly as recalculateLine() computes it.
    QRectF mappedRect(HsmElement* element) {
        return mapRectFromItem(element, element->elementRect());
    }
};

class HsmTransitionAnchorPreservationTest : public QObject {
    Q_OBJECT

private slots:
    void PropertyHintlessEmptyPathMatchesRectHelper();
    void PropertyHintlessNonEmptyPathMatchesPointHelper();
    void ExplicitSizeConstructorIsNotSnapped();

private:
    // Collects the ElementGripItem children of a transition. The source grip is
    // the first element of mLinePath and the destination grip the last, so the
    // resolved endpoints are observable as grip positions after recalculateLine.
    static QList<ElementGripItem*> collectGrips(HsmTransition* transition);
};

QList<ElementGripItem*> HsmTransitionAnchorPreservationTest::collectGrips(HsmTransition* transition) {
    QList<ElementGripItem*> grips;

    for (auto* child : transition->childItems()) {
        auto* grip = dynamic_cast<ElementGripItem*>(child);

        if (nullptr != grip) {
            grips.append(grip);
        }
    }

    return grips;
}

/**
 * @brief Property 4 (Requirements 3.3, 3.4): empty-path hint-less endpoints.
 *
 * For many generated (rectFrom, rectTo) pairs - both overlapping and disjoint -
 * a transition that carries NO *TargetFactors and NO intermediate points must
 * resolve its start/end anchors through findStartingPointFromRect, exactly as
 * the unfixed code does today.
 *
 * The expected values are produced by the SAME helper the production code uses,
 * so this is a self-consistent baseline that MUST PASS on the unfixed code.
 *
 * Validates: Requirements 3.3, 3.4
 */
void HsmTransitionAnchorPreservationTest::PropertyHintlessEmptyPathMatchesRectHelper() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 80; ++iteration) {
        QGraphicsScene scene;
        QGraphicsView view;
        view.setScene(&scene);

        const qreal fromW = 60.0 + rng.bounded(0, 200);
        const qreal fromH = 40.0 + rng.bounded(0, 160);
        const qreal toW = 60.0 + rng.bounded(0, 200);
        const qreal toH = 40.0 + rng.bounded(0, 160);

        // Even iterations overlap the source; odd iterations are disjoint in a
        // random one of the eight relative directions.
        const bool overlapping = (iteration % 2 == 0);
        QPointF toPos;

        if (overlapping) {
            toPos = QPointF(rng.bounded(0, static_cast<int>(fromW)), rng.bounded(0, static_cast<int>(fromH)));
        } else {
            const int dirX = rng.bounded(-1, 2);
            const int dirY = rng.bounded(-1, 2);
            const qreal gap = 300.0 + rng.bounded(0, 400);

            toPos = QPointF(dirX * gap, dirY * gap);

            if (dirX == 0 && dirY == 0) {
                toPos = QPointF(gap, 0.0);
            }
        }

        auto fromModel = QSharedPointer<model::RegularState>::create("from");
        auto toModel = QSharedPointer<model::RegularState>::create("to");
        auto transitionModel = QSharedPointer<model::Transition>::create(fromModel, toModel, "event");

        HsmStateElement* fromElement = new HsmStateElement(QSizeF(fromW, fromH));
        fromElement->init(fromModel);
        scene.addItem(fromElement);
        fromElement->setPos(0, 0);

        HsmStateElement* toElement = new HsmStateElement(QSizeF(toW, toH));
        toElement->init(toModel);
        scene.addItem(toElement);
        toElement->setPos(toPos);

        // Transition model carries NO GEOMETRY and NO *TargetFactors metadata, so
        // mLinePath stays empty and the geometric (hint-less) path is exercised.
        ProbeTransition* transition = new ProbeTransition();
        transition->init(transitionModel);
        transition->connectElements(fromElement, toElement);
        transition->recalculateLine();

        const QRectF rectFrom = transition->mappedRect(fromElement);
        const QRectF rectTo = transition->mappedRect(toElement);
        // With no interior points, a facing+overlapping pair is joined by a straight
        // perpendicular segment (centers of the overlapping edge spans); otherwise the
        // source uses findStartingPointFromRect and the destination the perpendicular foot.
        QPointF expectedFrom;
        QPointF expectedTo;
        if (false == transition->probeStraight(rectFrom, rectTo, expectedFrom, expectedTo)) {
            expectedFrom = transition->probeStartFromRect(rectFrom, rectTo);
            expectedTo = transition->probeEndFromPoint(rectTo, expectedFrom);
        }

        const QList<ElementGripItem*> grips = collectGrips(transition);
        QVERIFY2(grips.size() == 2,
                 qPrintable(QString("Iteration %1: expected exactly source and destination grips, got %2")
                                .arg(iteration)
                                .arg(grips.size())));

        const QPointF actualFrom = grips.first()->pos();
        const QPointF actualTo = grips.last()->pos();

        QVERIFY2(qFuzzyCompare(actualFrom.x(), expectedFrom.x()) && qFuzzyCompare(actualFrom.y(), expectedFrom.y()),
                 qPrintable(QString("Iteration %1 (%2): empty-path start anchor (%3,%4) != "
                                    "findStartingPointFromRect (%5,%6)")
                                .arg(iteration)
                                .arg(overlapping ? "overlapping" : "disjoint")
                                .arg(actualFrom.x())
                                .arg(actualFrom.y())
                                .arg(expectedFrom.x())
                                .arg(expectedFrom.y())));
        QVERIFY2(qFuzzyCompare(actualTo.x(), expectedTo.x()) && qFuzzyCompare(actualTo.y(), expectedTo.y()),
                 qPrintable(QString("Iteration %1 (%2): empty-path end anchor (%3,%4) != "
                                    "findPerpendicularEdgePoint (%5,%6)")
                                .arg(iteration)
                                .arg(overlapping ? "overlapping" : "disjoint")
                                .arg(actualTo.x())
                                .arg(actualTo.y())
                                .arg(expectedTo.x())
                                .arg(expectedTo.y())));
    }
}

/**
 * @brief Property 4 (Requirements 3.3, 3.4): non-empty-path hint-less endpoints.
 *
 * A transition that carries intermediate points (like an imported localGeometry)
 * but NO *TargetFactors must resolve its start/end anchors through
 * findStartingPointFromPoint using the first/last intermediate points, exactly
 * as the unfixed code does today, and must reproduce those intermediate points
 * unchanged.
 *
 * The interior points are supplied through the GEOMETRY metadata the way the
 * importer feeds localGeometry into connectElements. The expected endpoints are
 * produced by the SAME helper the production code uses, so this MUST PASS on the
 * unfixed code.
 *
 * Validates: Requirements 3.3, 3.4
 */
void HsmTransitionAnchorPreservationTest::PropertyHintlessNonEmptyPathMatchesPointHelper() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 60; ++iteration) {
        QGraphicsScene scene;
        QGraphicsView view;
        view.setScene(&scene);

        const qreal fromW = 60.0 + rng.bounded(0, 200);
        const qreal fromH = 40.0 + rng.bounded(0, 160);
        const qreal toW = 60.0 + rng.bounded(0, 200);
        const qreal toH = 40.0 + rng.bounded(0, 160);

        const bool overlapping = (iteration % 2 == 0);
        QPointF toPos;

        if (overlapping) {
            toPos = QPointF(rng.bounded(0, static_cast<int>(fromW)), rng.bounded(0, static_cast<int>(fromH)));
        } else {
            const int dirX = rng.bounded(-1, 2);
            const int dirY = rng.bounded(-1, 2);
            const qreal gap = 300.0 + rng.bounded(0, 400);

            toPos = QPointF(dirX * gap, dirY * gap);

            if (dirX == 0 && dirY == 0) {
                toPos = QPointF(gap, 0.0);
            }
        }

        // One or two intermediate points placed somewhere between the states.
        const QPointF interior1(toPos.x() / 2.0 + rng.bounded(-40, 40), toPos.y() / 2.0 + rng.bounded(-40, 40));
        const QPointF interior2(toPos.x() / 2.0 + rng.bounded(-40, 40), toPos.y() / 2.0 + rng.bounded(-40, 40));
        const bool twoInterior = (iteration % 3 == 0);

        auto fromModel = QSharedPointer<model::RegularState>::create("from");
        auto toModel = QSharedPointer<model::RegularState>::create("to");
        auto transitionModel = QSharedPointer<model::Transition>::create(fromModel, toModel, "event");

        // Feed interior points as GEOMETRY metadata (first/last are placeholder
        // endpoints that connectElements/recalculateLine overwrite), mirroring the
        // way the importer stores localGeometry.
        QPolygonF geometry;
        geometry.append(QPointF(0, 0));  // placeholder start
        geometry.append(interior1);
        if (twoInterior) {
            geometry.append(interior2);
        }
        geometry.append(QPointF(0, 0));  // placeholder end
        transitionModel->setMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY, geometry);

        HsmStateElement* fromElement = new HsmStateElement(QSizeF(fromW, fromH));
        fromElement->init(fromModel);
        scene.addItem(fromElement);
        fromElement->setPos(0, 0);

        HsmStateElement* toElement = new HsmStateElement(QSizeF(toW, toH));
        toElement->init(toModel);
        scene.addItem(toElement);
        toElement->setPos(toPos);

        ProbeTransition* transition = new ProbeTransition();
        transition->init(transitionModel);
        transition->connectElements(fromElement, toElement);
        transition->recalculateLine();

        const QRectF rectFrom = transition->mappedRect(fromElement);
        const QRectF rectTo = transition->mappedRect(toElement);

        // recalculateLine resolves endpoints against the first/last interior points.
        const QPointF firstInterior = interior1;
        const QPointF lastInterior = twoInterior ? interior2 : interior1;
        // Both endpoints now use the perpendicular-foot rule from their neighbouring interior
        // point: the source from the first interior point, the destination from the last.
        const QPointF expectedFrom = transition->probeEndFromPoint(rectFrom, firstInterior);
        const QPointF expectedTo = transition->probeEndFromPoint(rectTo, lastInterior);

        // GEOMETRY metadata stored after recalculateLine must reproduce the
        // interior points unchanged (first/last are the resolved endpoints).
        const QVariant storedGeometry = transitionModel->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY);
        QVERIFY2(storedGeometry.isValid(), qPrintable(QString("Iteration %1: GEOMETRY metadata missing").arg(iteration)));

        const QPolygonF resolvedPath = storedGeometry.value<QPolygonF>();
        const int expectedPointCount = twoInterior ? 4 : 3;
        QVERIFY2(resolvedPath.size() == expectedPointCount,
                 qPrintable(QString("Iteration %1: resolved path has %2 points, expected %3")
                                .arg(iteration)
                                .arg(resolvedPath.size())
                                .arg(expectedPointCount)));

        const QPointF actualFrom = resolvedPath.first();
        const QPointF actualTo = resolvedPath.last();

        QVERIFY2(qFuzzyCompare(actualFrom.x(), expectedFrom.x()) && qFuzzyCompare(actualFrom.y(), expectedFrom.y()),
                 qPrintable(QString("Iteration %1 (%2): non-empty-path start anchor (%3,%4) != "
                                    "perpendicular foot findPerpendicularEdgePoint (%5,%6)")
                                .arg(iteration)
                                .arg(overlapping ? "overlapping" : "disjoint")
                                .arg(actualFrom.x())
                                .arg(actualFrom.y())
                                .arg(expectedFrom.x())
                                .arg(expectedFrom.y())));
        QVERIFY2(qFuzzyCompare(actualTo.x(), expectedTo.x()) && qFuzzyCompare(actualTo.y(), expectedTo.y()),
                 qPrintable(QString("Iteration %1 (%2): non-empty-path end anchor (%3,%4) != "
                                    "findPerpendicularEdgePoint (%5,%6)")
                                .arg(iteration)
                                .arg(overlapping ? "overlapping" : "disjoint")
                                .arg(actualTo.x())
                                .arg(actualTo.y())
                                .arg(expectedTo.x())
                                .arg(expectedTo.y())));

        // Intermediate points must be reproduced unchanged (Requirement 3.3).
        QVERIFY2(qFuzzyCompare(resolvedPath[1].x(), interior1.x()) && qFuzzyCompare(resolvedPath[1].y(), interior1.y()),
                 qPrintable(QString("Iteration %1: first intermediate point (%2,%3) altered to (%4,%5)")
                                .arg(iteration)
                                .arg(interior1.x())
                                .arg(interior1.y())
                                .arg(resolvedPath[1].x())
                                .arg(resolvedPath[1].y())));

        if (twoInterior) {
            QVERIFY2(qFuzzyCompare(resolvedPath[2].x(), interior2.x()) && qFuzzyCompare(resolvedPath[2].y(), interior2.y()),
                     qPrintable(QString("Iteration %1: second intermediate point (%2,%3) altered to (%4,%5)")
                                    .arg(iteration)
                                    .arg(interior2.x())
                                    .arg(interior2.y())
                                    .arg(resolvedPath[2].x())
                                    .arg(resolvedPath[2].y())));
        }
    }
}

/**
 * @brief Property 3 preservation (design Preservation Test Case 4): the
 *        explicit-size constructor must NOT snap the caller-supplied size.
 *
 * Only the default/creation-time size is grid-derived by the fix; a size passed
 * explicitly to HsmRectangularElement(type, QSizeF(w,h)) must be stored verbatim.
 * The element rect is read immediately after construction (before init()), which
 * is exactly the constructor path the fix touches - later init()/layout content
 * sizing is a separate, pre-existing behavior independent of grid snapping and so
 * is intentionally not asserted here.
 *
 * Observation-first note: on the unfixed code the explicit-size constructor
 * stores the size as given (clamped only by the pre-existing 200x100 minimum,
 * which the sizes below stay above), so this MUST PASS on the baseline and keeps
 * guarding against the fix accidentally snapping programmatic/import sizes.
 *
 * Validates: Requirements 3.1, 3.2
 */
void HsmTransitionAnchorPreservationTest::ExplicitSizeConstructorIsNotSnapped() {
    // Deliberately off-grid dimensions that are NOT multiples of the grid step,
    // so any accidental snapping would be observable. All sizes are above the
    // rectangular element minimum (200x100) so the pre-existing min-size clamp -
    // which is unrelated to grid snapping - does not come into play.
    const QList<QSizeF> explicitSizes = {
        QSizeF(237.0, 143.0),
        QSizeF(333.0, 217.0),
        QSizeF(451.0, 109.0),
        QSizeF(289.0, 163.0),
    };

    for (const QSizeF& size : explicitSizes) {
        // Read the rect right after construction, before init()/layout runs, so
        // the assertion isolates the constructor's handling of the explicit size.
        HsmStateElement* element = new HsmStateElement(size);

        const QRectF rect = element->elementRect();

        QVERIFY2(qFuzzyCompare(rect.width(), size.width()),
                 qPrintable(QString("Explicit width %1 was altered to %2").arg(size.width()).arg(rect.width())));
        QVERIFY2(qFuzzyCompare(rect.height(), size.height()),
                 qPrintable(QString("Explicit height %1 was altered to %2").arg(size.height()).arg(rect.height())));

        delete element;
    }
}

int runHsmTransitionAnchorPreservationTest(int argc, char** argv) {
    QApplication app(argc, argv);

    HsmTransitionAnchorPreservationTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "HsmTransitionAnchorPreservationTest.moc"
