#include <QApplication>
#include <QRectF>
#include <QtTest>

#include "view/widgets/HsmGraphicsView.hpp"

/**
 * @brief Unit tests for HsmGraphicsView::isOverlapViolation -- the pure predicate behind
 *        the non-blocking "element overlaps a container it is not a child of" layout warning.
 *
 * The warning is intentionally display-only: it never changes placement, it only flags an
 * element whose body visually intrudes into a container it is not nested in (e.g. after a
 * multi-element drag leaves a non-accepted element sitting on top of another state, or a
 * single element is silently placed at top level while visually inside another box).
 *
 * The predicate factors the two independent conditions that drive the flag:
 *   - relationship: a legitimate parent/child/containment relationship suppresses the warning;
 *   - geometry: otherwise an intersection with the other container's (valid) body triggers it.
 */
class HsmGraphicsViewOverlapWarningTest : public QObject {
    Q_OBJECT

private slots:
    void RelatedElementsNeverWarnEvenWhenOverlapping();
    void UnrelatedOverlappingBodyWarns();
    void UnrelatedNonOverlappingDoesNotWarn();
    void InvalidOtherBodyDoesNotWarn();
    void TouchingEdgesOnlyDoesNotWarn();
};

/**
 * A legitimate parent/child/containment relationship means overlap is expected (a parent
 * always overlaps its children), so the predicate must never flag it regardless of geometry.
 */
void HsmGraphicsViewOverlapWarningTest::RelatedElementsNeverWarnEvenWhenOverlapping() {
    const QRectF elementRect(0, 0, 100, 100);
    const QRectF fullyOverlappingBody(0, 0, 100, 100);

    QVERIFY2(HsmGraphicsView::isOverlapViolation(elementRect, fullyOverlappingBody, /*related=*/true) == false,
             "a parent/child/contained relationship must suppress the overlap warning even on full overlap");
}

/**
 * The reported bug: an unrelated element whose rect intersects another container's droppable
 * body is misplaced and must be flagged.
 */
void HsmGraphicsViewOverlapWarningTest::UnrelatedOverlappingBodyWarns() {
    const QRectF elementRect(0, 0, 100, 100);
    const QRectF otherBody(50, 50, 100, 100);  // overlaps the lower-right quadrant

    QVERIFY2(HsmGraphicsView::isOverlapViolation(elementRect, otherBody, /*related=*/false) == true,
             "an unrelated element intersecting another container's body must be flagged");
}

/**
 * Two unrelated elements that do not intersect are a valid layout and must not warn.
 */
void HsmGraphicsViewOverlapWarningTest::UnrelatedNonOverlappingDoesNotWarn() {
    const QRectF elementRect(0, 0, 100, 100);
    const QRectF otherBody(200, 200, 100, 100);  // clearly separate

    QVERIFY2(HsmGraphicsView::isOverlapViolation(elementRect, otherBody, /*related=*/false) == false,
             "non-overlapping unrelated elements must not be flagged");
}

/**
 * A container with no body section yields an invalid rect; it has no droppable interior to
 * intrude into, so it can never be an overlap target.
 */
void HsmGraphicsViewOverlapWarningTest::InvalidOtherBodyDoesNotWarn() {
    const QRectF elementRect(0, 0, 100, 100);
    const QRectF invalidBody;  // default-constructed QRectF is invalid

    QVERIFY2(HsmGraphicsView::isOverlapViolation(elementRect, invalidBody, /*related=*/false) == false,
             "an invalid (empty) other-body rect must never trigger the warning");
}

/**
 * QRectF::intersects is strict: rectangles that only share an edge do not intersect. An element
 * placed flush against (but not inside) another container is a valid adjacent layout, not an
 * overlap.
 */
void HsmGraphicsViewOverlapWarningTest::TouchingEdgesOnlyDoesNotWarn() {
    const QRectF elementRect(0, 0, 100, 100);
    const QRectF otherBody(100, 0, 100, 100);  // shares the right edge only

    QVERIFY2(HsmGraphicsView::isOverlapViolation(elementRect, otherBody, /*related=*/false) == false,
             "elements that only touch edges must not be flagged as overlapping");
}

int runHsmGraphicsViewOverlapWarningTest(int argc, char** argv) {
    QApplication app(argc, argv);
    HsmGraphicsViewOverlapWarningTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "HsmGraphicsViewOverlapWarningTest.moc"
