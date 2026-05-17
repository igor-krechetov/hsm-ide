#include <QtTest>

#include "view/theme/ThemeManager.hpp"
#include "view/widgets/HsmGraphicsView.hpp"

class HsmGraphicsViewTest : public QObject {
    Q_OBJECT

private slots:
    void snapToGrid();
};

void HsmGraphicsViewTest::snapToGrid() {
    /*

     A---|---W---|---B
     |       |       |
     |   -   6   -   |
     |     1 | 2     |
     Q---5---0---7---E
     |     4 | 3     |
     |   -   8   -   |
     |       |       |
     D---|---R---|---C
     0       10       20

      Grid size is 20x20, so points should snap to the nearest multiple of 20.

      0 -> A
      1 -> A
      2 -> B
      3 -> C
      4 -> D

      5 -> A
      6 -> A
      7 -> B
      8 -> D

      Q -> A
      W -> A
      E -> B
      R -> D

      A -> A
      B -> B
      C -> C
      D -> D
    */
    // Test snapping a point to the grid
    // create tests for all use-cases
    const int gridStep = ThemeManager::instance().theme().grid.minorLineStep;

    const QPointF oneUnit(1, 1);

    const QPointF pointA(0, 0);
    const QPointF pointB(gridStep, 0);
    const QPointF pointC(gridStep, gridStep);
    const QPointF pointD(0, gridStep);

    const QPointF point0(gridStep * 0.5, gridStep * 0.5);
    const QPointF point1 = point0 - oneUnit;
    const QPointF point2(point0.x() + 1, point0.y() - 1);
    const QPointF point3 = point0 + oneUnit;
    const QPointF point4(point0.x() - 1, point0.y() + 1);

    const QPointF point5(gridStep * 0.25, gridStep * 0.5);
    const QPointF point6(gridStep * 0.5, gridStep * 0.25);
    const QPointF point7(gridStep * 0.75, gridStep * 0.5);
    const QPointF point8(gridStep * 0.5, gridStep * 0.75);

    const QPointF pointQ(0, gridStep * 0.5);
    const QPointF pointW(gridStep * 0.5, 0);
    const QPointF pointE(gridStep, gridStep * 0.5);
    const QPointF pointR(gridStep * 0.5, gridStep);

    QCOMPARE(HsmGraphicsView::alignPointToGrid(point0), pointA);
    QCOMPARE(HsmGraphicsView::alignPointToGrid(point1), pointA);
    QCOMPARE(HsmGraphicsView::alignPointToGrid(point2), pointB);
    QCOMPARE(HsmGraphicsView::alignPointToGrid(point3), pointC);
    QCOMPARE(HsmGraphicsView::alignPointToGrid(point4), pointD);

    QCOMPARE(HsmGraphicsView::alignPointToGrid(point5), pointA);
    QCOMPARE(HsmGraphicsView::alignPointToGrid(point6), pointA);
    QCOMPARE(HsmGraphicsView::alignPointToGrid(point7), pointB);
    QCOMPARE(HsmGraphicsView::alignPointToGrid(point8), pointD);

    QCOMPARE(HsmGraphicsView::alignPointToGrid(pointQ), pointA);
    QCOMPARE(HsmGraphicsView::alignPointToGrid(pointW), pointA);
    QCOMPARE(HsmGraphicsView::alignPointToGrid(pointE), pointB);
    QCOMPARE(HsmGraphicsView::alignPointToGrid(pointR), pointD);

    QCOMPARE(HsmGraphicsView::alignPointToGrid(pointA), pointA);
    QCOMPARE(HsmGraphicsView::alignPointToGrid(pointB), pointB);
    QCOMPARE(HsmGraphicsView::alignPointToGrid(pointC), pointC);
    QCOMPARE(HsmGraphicsView::alignPointToGrid(pointD), pointD);
}

int runHsmGraphicsViewTest(int argc, char** argv) {
    HsmGraphicsViewTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "HsmGraphicsViewTest.moc"
