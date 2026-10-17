#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPolygonF>
#include <QtTest>

#include "model/elements/RegularState.hpp"
#include "model/elements/Transition.hpp"
#include "view/elements/HsmStateElement.hpp"
#include "view/elements/HsmTransition.hpp"

using namespace view;

// ---------------------------------------------------------------------------
// Destination-endpoint rendering: the point where a transition enters its target
// element is the PERPENDICULAR FOOT dropped from the previous point (the last
// intermediate grip, or the transition start point when there are no intermediate
// grips) onto the nearest target edge - not the center of that edge. When no
// perpendicular is possible (the previous point projects outside the edge span),
// the endpoint falls back to the center of the nearest edge.
//
// The destination is observed as the last grip's position after recalculateLine
// (the same observation the preservation/property tests use). Expectations are
// derived from the ACTUAL mapped target rect, so the rectangular element's minimum
// size clamp does not affect the assertion.
// ---------------------------------------------------------------------------

static bool nearPoint(const QPointF& a, const QPointF& b, qreal eps = 0.6) {
    return qAbs(a.x() - b.x()) < eps && qAbs(a.y() - b.y()) < eps;
}

// Exposes the mapped target rect so the test computes expectations against the
// exact rect recalculateLine() uses (after any min-size clamp / init layout).
class ProbeTransition : public HsmTransition {
public:
    QRectF mappedRect(HsmElement* element) {
        return mapRectFromItem(element, element->elementRect());
    }
};

class HsmTransitionPerpendicularEndpointTest : public QObject {
    Q_OBJECT

private slots:
    void PerpendicularFootUsedWhenWithinEdgeSpan();
    void SourcePerpendicularFootFromFirstGrip();
    void FallsBackToEdgeCenterWhenNoPerpendicular();
    void StraightConnectionForVerticallyFacingOverlap();
    void DiagonalNoOverlapDoesNotUseStraightConnection();

private:
};

/**
 * @brief The destination endpoint is the perpendicular foot from the previous point.
 *
 * The target is placed well below the source and an interior grip is positioned
 * directly above the target, within the target's top-edge x-span but off its center.
 * The destination must land at the perpendicular foot (interior.x on the top edge),
 * NOT at the top-edge center.
 */
void HsmTransitionPerpendicularEndpointTest::PerpendicularFootUsedWhenWithinEdgeSpan() {
    QGraphicsScene scene;
    QGraphicsView view;
    view.setScene(&scene);

    auto fromModel = QSharedPointer<model::RegularState>::create("from");
    auto toModel = QSharedPointer<model::RegularState>::create("to");
    auto trModel = QSharedPointer<model::Transition>::create(fromModel, toModel, "e");

    // Source far above-left; target far below. Sizes >= 200x100 to avoid the min clamp.
    auto* fromE = new HsmStateElement(QSizeF(200.0, 100.0));
    fromE->init(fromModel);
    scene.addItem(fromE);
    fromE->setPos(0.0, 0.0);

    auto* toE = new HsmStateElement(QSizeF(300.0, 200.0));
    toE->init(toModel);
    scene.addItem(toE);
    toE->setPos(400.0, 800.0);

    // Target scene rect is pos + size; the transition item is at scene origin so item-local
    // coordinates match scene coordinates here. Interior grip sits directly above the target,
    // within its top-edge x-span but LEFT of center.
    const QRectF targetSceneRect(toE->pos(), toE->elementRect().size());
    const qreal interiorX = targetSceneRect.left() + targetSceneRect.width() * 0.25;
    const QPointF interior(interiorX, targetSceneRect.top() - 300.0);

    QPolygonF geometry;
    geometry.append(QPointF(0, 0));
    geometry.append(interior);
    geometry.append(QPointF(0, 0));
    trModel->setMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY, geometry);

    ProbeTransition* t = new ProbeTransition();
    t->init(trModel);
    t->connectElements(fromE, toE);
    t->recalculateLine();

    const QRectF rectTo = t->mappedRect(toE);

    // The resolved destination endpoint is the LAST point of the stored line path
    // (grips are not stored in path order, so read the path, not grips.last()).
    const QPolygonF path = trModel->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY).value<QPolygonF>();
    QVERIFY2(path.size() >= 2, qPrintable(QString("expected a resolved path with >= 2 points, got %1").arg(path.size())));

    const QPointF end = path.last();

    // Perpendicular foot onto the top edge at interiorX.
    const QPointF expected(interiorX, rectTo.top());
    QVERIFY2(nearPoint(end, expected),
             qPrintable(QString("expected perpendicular foot (%1,%2) on target top edge, got (%3,%4)")
                            .arg(expected.x())
                            .arg(expected.y())
                            .arg(end.x())
                            .arg(end.y())));
    // And it must NOT be the top-edge center.
    QVERIFY2(
        qAbs(end.x() - rectTo.center().x()) > 1.0,
        qPrintable(QString("destination x=%1 should NOT be the top-edge center x=%2").arg(end.x()).arg(rectTo.center().x())));
}

/**
 * @brief When the previous point projects outside the chosen edge span, fall back to
 *        the center of that edge (legacy behavior).
 *
 * The interior grip is up-and-left of the target so the region analysis selects the
 * left edge, but the grip's y is above the target's y-span, so no perpendicular onto
 * the left edge is possible => the endpoint is the center of the left edge.
 */
void HsmTransitionPerpendicularEndpointTest::FallsBackToEdgeCenterWhenNoPerpendicular() {
    QGraphicsScene scene;
    QGraphicsView view;
    view.setScene(&scene);

    auto fromModel = QSharedPointer<model::RegularState>::create("from");
    auto toModel = QSharedPointer<model::RegularState>::create("to");
    auto trModel = QSharedPointer<model::Transition>::create(fromModel, toModel, "e");

    auto* fromE = new HsmStateElement(QSizeF(200.0, 100.0));
    fromE->init(fromModel);
    scene.addItem(fromE);
    fromE->setPos(0.0, 0.0);

    auto* toE = new HsmStateElement(QSizeF(300.0, 200.0));
    toE->init(toModel);
    scene.addItem(toE);
    toE->setPos(900.0, 900.0);

    const QRectF targetSceneRect(toE->pos(), toE->elementRect().size());
    // Interior up-and-left of the target: x left of the target, y above the target.
    // Region analysis => left edge; prevPoint.y is outside the left-edge y-span, so the
    // endpoint falls back to the center of the left edge.
    const QPointF interior(targetSceneRect.left() - 400.0, targetSceneRect.top() - 400.0);

    QPolygonF geometry;
    geometry.append(QPointF(0, 0));
    geometry.append(interior);
    geometry.append(QPointF(0, 0));
    trModel->setMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY, geometry);

    ProbeTransition* t = new ProbeTransition();
    t->init(trModel);
    t->connectElements(fromE, toE);
    t->recalculateLine();

    const QRectF rectTo = t->mappedRect(toE);

    // The resolved destination endpoint is the LAST point of the stored line path
    // (grips are not stored in path order, so read the path, not grips.last()).
    const QPolygonF path = trModel->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY).value<QPolygonF>();
    QVERIFY2(path.size() >= 2, qPrintable(QString("expected a resolved path with >= 2 points, got %1").arg(path.size())));

    const QPointF end = path.last();

    const QPointF expected(rectTo.left(), rectTo.center().y());
    QVERIFY2(nearPoint(end, expected),
             qPrintable(QString("expected fallback to left-edge center (%1,%2), got (%3,%4)")
                            .arg(expected.x())
                            .arg(expected.y())
                            .arg(end.x())
                            .arg(end.y())));
}

/**
 * @brief Issue 2: two elements facing each other with overlapping edges and NO intermediate
 *        grips are joined by a straight perpendicular segment anchored at the centers of the
 *        overlapping edge spans.
 *
 * The source sits directly above the target with overlapping x-spans. The connector must be a
 * straight vertical line: start = (overlapCenterX, source.bottom), end = (overlapCenterX,
 * target.top), with overlapCenterX the center of the shared horizontal range.
 */
void HsmTransitionPerpendicularEndpointTest::StraightConnectionForVerticallyFacingOverlap() {
    QGraphicsScene scene;
    QGraphicsView view;
    view.setScene(&scene);

    auto fromModel = QSharedPointer<model::RegularState>::create("from");
    auto toModel = QSharedPointer<model::RegularState>::create("to");
    auto trModel = QSharedPointer<model::Transition>::create(fromModel, toModel, "e");

    // Source above, target below; x-spans overlap (source [0,300], target [100,400] => [100,300]).
    auto* fromE = new HsmStateElement(QSizeF(300.0, 120.0));
    fromE->init(fromModel);
    scene.addItem(fromE);
    fromE->setPos(0.0, 0.0);

    auto* toE = new HsmStateElement(QSizeF(300.0, 150.0));
    toE->init(toModel);
    scene.addItem(toE);
    toE->setPos(100.0, 600.0);

    ProbeTransition* t = new ProbeTransition();
    t->init(trModel);
    t->connectElements(fromE, toE);
    t->recalculateLine();

    const QRectF rectFrom = t->mappedRect(fromE);
    const QRectF rectTo = t->mappedRect(toE);

    const QPolygonF path = trModel->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY).value<QPolygonF>();
    QVERIFY2(path.size() == 2, qPrintable(QString("expected a straight 2-point path, got %1 points").arg(path.size())));

    const qreal overlapCenterX = (qMax(rectFrom.left(), rectTo.left()) + qMin(rectFrom.right(), rectTo.right())) / 2.0;
    const QPointF expectedStart(overlapCenterX, rectFrom.bottom());
    const QPointF expectedEnd(overlapCenterX, rectTo.top());

    QVERIFY2(nearPoint(path.first(), expectedStart),
             qPrintable(QString("start (%1,%2) != overlap-center on source bottom (%3,%4)")
                            .arg(path.first().x())
                            .arg(path.first().y())
                            .arg(expectedStart.x())
                            .arg(expectedStart.y())));
    QVERIFY2(nearPoint(path.last(), expectedEnd),
             qPrintable(QString("end (%1,%2) != overlap-center on target top (%3,%4)")
                            .arg(path.last().x())
                            .arg(path.last().y())
                            .arg(expectedEnd.x())
                            .arg(expectedEnd.y())));
    // The connector is a straight vertical line: both endpoints share x.
    QVERIFY2(qAbs(path.first().x() - path.last().x()) < 0.6, "straight connector endpoints must share x");
}

/**
 * @brief Issue 2 negative case: diagonally placed, non-overlapping elements must NOT use the
 *        straight connection - the endpoints follow the regular geometric / perpendicular rule.
 *
 * Source top-left, target far down-right with NO x- or y-span overlap, so a straight
 * perpendicular segment is impossible. The resolved endpoints must therefore differ from a
 * shared-coordinate straight line (they are the geometric edge anchors).
 */
void HsmTransitionPerpendicularEndpointTest::DiagonalNoOverlapDoesNotUseStraightConnection() {
    QGraphicsScene scene;
    QGraphicsView view;
    view.setScene(&scene);

    auto fromModel = QSharedPointer<model::RegularState>::create("from");
    auto toModel = QSharedPointer<model::RegularState>::create("to");
    auto trModel = QSharedPointer<model::Transition>::create(fromModel, toModel, "e");

    auto* fromE = new HsmStateElement(QSizeF(200.0, 100.0));
    fromE->init(fromModel);
    scene.addItem(fromE);
    fromE->setPos(0.0, 0.0);

    // Far down-right so x-spans [0,200] vs [700,900] and y-spans [0,100] vs [700,850] are disjoint.
    auto* toE = new HsmStateElement(QSizeF(200.0, 150.0));
    toE->init(toModel);
    scene.addItem(toE);
    toE->setPos(700.0, 700.0);

    ProbeTransition* t = new ProbeTransition();
    t->init(trModel);
    t->connectElements(fromE, toE);
    t->recalculateLine();

    const QPolygonF path = trModel->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY).value<QPolygonF>();
    QVERIFY2(path.size() == 2, qPrintable(QString("expected a 2-point path, got %1").arg(path.size())));

    // A straight connector would share an x or a y; the diagonal case must share neither.
    const bool sharesX = qAbs(path.first().x() - path.last().x()) < 0.6;
    const bool sharesY = qAbs(path.first().y() - path.last().y()) < 0.6;
    QVERIFY2(false == (sharesX || sharesY),
             qPrintable(QString("diagonal transition unexpectedly straight: start=(%1,%2) end=(%3,%4)")
                            .arg(path.first().x())
                            .arg(path.first().y())
                            .arg(path.last().x())
                            .arg(path.last().y())));
}

/**
 * @brief The SOURCE endpoint is the perpendicular foot from the first intermediate grip.
 *
 * Mirrors the destination rule for the start point: with an intermediate grip present, the
 * point where the line leaves the source element is the perpendicular foot dropped from that
 * first grip onto the nearest source edge, not the edge center.
 */
void HsmTransitionPerpendicularEndpointTest::SourcePerpendicularFootFromFirstGrip() {
    QGraphicsScene scene;
    QGraphicsView view;
    view.setScene(&scene);

    auto fromModel = QSharedPointer<model::RegularState>::create("from");
    auto toModel = QSharedPointer<model::RegularState>::create("to");
    auto trModel = QSharedPointer<model::Transition>::create(fromModel, toModel, "e");

    // Source at the top; target far below. The first interior grip is placed directly BELOW
    // the source, within its bottom-edge x-span but off center, so the perpendicular foot is
    // on the source bottom edge at the grip's x.
    auto* fromE = new HsmStateElement(QSizeF(300.0, 120.0));
    fromE->init(fromModel);
    scene.addItem(fromE);
    fromE->setPos(0.0, 0.0);

    auto* toE = new HsmStateElement(QSizeF(200.0, 150.0));
    toE->init(toModel);
    scene.addItem(toE);
    toE->setPos(100.0, 900.0);

    const QRectF sourceSceneRect(fromE->pos(), fromE->elementRect().size());
    const qreal interiorX = sourceSceneRect.left() + sourceSceneRect.width() * 0.25;  // off-center
    const QPointF interior(interiorX, sourceSceneRect.bottom() + 300.0);              // directly below the source

    QPolygonF geometry;
    geometry.append(QPointF(0, 0));
    geometry.append(interior);
    geometry.append(QPointF(0, 0));
    trModel->setMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY, geometry);

    ProbeTransition* t = new ProbeTransition();
    t->init(trModel);
    t->connectElements(fromE, toE);
    t->recalculateLine();

    const QRectF rectFrom = t->mappedRect(fromE);

    const QPolygonF path = trModel->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY).value<QPolygonF>();
    QVERIFY2(path.size() >= 2, qPrintable(QString("expected a resolved path, got %1 points").arg(path.size())));

    const QPointF start = path.first();

    // Perpendicular foot onto the source bottom edge at interiorX.
    const QPointF expected(interiorX, rectFrom.bottom());
    QVERIFY2(nearPoint(start, expected),
             qPrintable(QString("expected source perpendicular foot (%1,%2) on bottom edge, got (%3,%4)")
                            .arg(expected.x())
                            .arg(expected.y())
                            .arg(start.x())
                            .arg(start.y())));
    QVERIFY2(
        qAbs(start.x() - rectFrom.center().x()) > 1.0,
        qPrintable(QString("source x=%1 should NOT be the bottom-edge center x=%2").arg(start.x()).arg(rectFrom.center().x())));
}

int runHsmTransitionPerpendicularEndpointTest(int argc, char** argv) {
    QApplication app(argc, argv);

    HsmTransitionPerpendicularEndpointTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "HsmTransitionPerpendicularEndpointTest.moc"
