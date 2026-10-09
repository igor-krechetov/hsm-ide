#include <QtTest>

#include <QApplication>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QRandomGenerator>

#include "view/theme/ThemeManager.hpp"
#include "view/widgets/HsmGraphicsView.hpp"

class HsmGraphicsViewTest : public QObject {
    Q_OBJECT

private slots:
    void snapToGrid();
    void PropertyFirstElementOnEmptyDocumentStaysPut();
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

/**
 * @brief Property 1: Bug Condition - First element on an empty document stays put.
 *
 * Encodes the Bug Condition from the design: isBugCondition(input) is
 * input.documentIsEmptyBeforeAdd == true (the very first item added to an empty
 * scene). The trigger is deterministic, so the property is scoped to that concrete
 * empty-to-first-item transition while the drop position is varied with
 * QRandomGenerator across near-origin, off-origin and near-diagram-bound positions.
 *
 * For each drop position a fresh HsmGraphicsView is built with a plain scene created
 * exactly as MainWindow::projectOpened does (new QGraphicsScene(), no explicit scene
 * rect, ScrollBarAlwaysOn on both axes). While the scene is empty we record the view's
 * sceneRect() and the view-coordinate mapping of a fixed scene point, add the first
 * item at the drop position, then record the same two values again.
 *
 * Expected Behavior (design Property 1): the sceneRect() is unchanged across the first
 * add, mapFromScene(fixedScenePoint) is unchanged, and the added item's scenePos()
 * equals the requested drop position (no viewport jump).
 *
 * This test MUST FAIL on the unfixed code: with no explicit scene rect Qt derives the
 * scene rect from itemsBoundingRect(), which is degenerate on an empty scene, so adding
 * the first item recomputes the scene rect and shifts the scene-to-view mapping.
 *
 * Validates: Requirements 1.1, 1.2
 */
void HsmGraphicsViewTest::PropertyFirstElementOnEmptyDocumentStaysPut() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    // A fixed scene point whose view-coordinate mapping must stay stable across the add.
    const QPointF fixedScenePoint(0.0, 0.0);
    // Size of the first dropped item (independent of the drop position).
    const QSizeF itemSize(60.0, 40.0);

    for (int iteration = 0; iteration < 50; ++iteration) {
        // Vary the drop position across the input space: near-origin, off-origin and
        // near a realistic diagram bound.
        QPointF dropPosition;

        switch (iteration % 3) {
            case 0:
                // Near-origin
                dropPosition = QPointF(rng.bounded(-20, 20), rng.bounded(-20, 20));
                break;
            case 1:
                // Off-origin
                dropPosition = QPointF(rng.bounded(80, 400), rng.bounded(60, 300));
                break;
            default:
                // Near a realistic diagram bound
                dropPosition = QPointF(rng.bounded(-900, 900), rng.bounded(-900, 900));
                break;
        }

        // Build a fresh view + scene exactly as MainWindow::projectOpened does.
        HsmGraphicsView view;
        QGraphicsScene scene;
        view.setScene(&scene);
        view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        view.resize(800, 600);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));

        // Bug Condition: the document is empty before the add.
        QVERIFY2(scene.items().isEmpty(),
                 qPrintable(QString("Iteration %1: scene not empty before first add").arg(iteration)));

        // Record state while the scene is empty.
        const QRectF sceneRectBefore = view.sceneRect();
        const QPoint mappingBefore = view.mapFromScene(fixedScenePoint);

        // Add the first item at the generated drop position.
        QGraphicsRectItem* firstItem = new QGraphicsRectItem(QRectF(QPointF(0.0, 0.0), itemSize));
        firstItem->setPos(dropPosition);
        scene.addItem(firstItem);

        // Record the same two values after the first add.
        const QRectF sceneRectAfter = view.sceneRect();
        const QPoint mappingAfter = view.mapFromScene(fixedScenePoint);

        // Assertion: the scene rect must be unchanged across the first add.
        QVERIFY2(sceneRectBefore == sceneRectAfter,
                 qPrintable(QString("Iteration %1 (drop %2,%3): sceneRect changed across first add: "
                                    "before [%4,%5 %6x%7] -> after [%8,%9 %10x%11]")
                                .arg(iteration)
                                .arg(dropPosition.x())
                                .arg(dropPosition.y())
                                .arg(sceneRectBefore.x())
                                .arg(sceneRectBefore.y())
                                .arg(sceneRectBefore.width())
                                .arg(sceneRectBefore.height())
                                .arg(sceneRectAfter.x())
                                .arg(sceneRectAfter.y())
                                .arg(sceneRectAfter.width())
                                .arg(sceneRectAfter.height())));

        // Assertion: the scene-to-view mapping of a fixed scene point must be unchanged.
        QVERIFY2(mappingBefore == mappingAfter,
                 qPrintable(QString("Iteration %1 (drop %2,%3): mapFromScene((0,0)) shifted across first add: "
                                    "before (%4,%5) -> after (%6,%7)")
                                .arg(iteration)
                                .arg(dropPosition.x())
                                .arg(dropPosition.y())
                                .arg(mappingBefore.x())
                                .arg(mappingBefore.y())
                                .arg(mappingAfter.x())
                                .arg(mappingAfter.y())));

        // Assertion: the added item must stay at the requested drop position.
        QVERIFY2(firstItem->scenePos() == dropPosition,
                 qPrintable(QString("Iteration %1: item scenePos (%2,%3) != drop position (%4,%5)")
                                .arg(iteration)
                                .arg(firstItem->scenePos().x())
                                .arg(firstItem->scenePos().y())
                                .arg(dropPosition.x())
                                .arg(dropPosition.y())));
    }
}

int runHsmGraphicsViewTest(int argc, char** argv) {
    QApplication app(argc, argv);

    HsmGraphicsViewTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "HsmGraphicsViewTest.moc"
