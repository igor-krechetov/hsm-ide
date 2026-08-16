#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsView>
#include <QtTest>

#include "view/elements/private/AutoGroupItem.hpp"

using namespace view;

// ---------------------------------------------------------------------------
// Test class
// ---------------------------------------------------------------------------
class AutoGroupItemCursorTest : public QObject {
    Q_OBJECT

private slots:
    void hoverMovableLabelSetsSizeAllCursor();
    void leaveMovableLabelRestoresDefaultCursor();
    void hoverNonMovableLabelDoesNotChangeCursor();
    void makeMovableDisabledUnsetsCursor();
    void cursorRoundTripProperty();
    void nonMovableNeverSetsCursorProperty();
};

// ---------------------------------------------------------------------------
// Helper: sends a hover-enter event to an item via its scene
// ---------------------------------------------------------------------------
static void sendHoverEnter(QGraphicsScene& scene, QGraphicsItem* item) {
    QGraphicsSceneHoverEvent hoverEvent(QEvent::GraphicsSceneHoverEnter);
    hoverEvent.setPos(QPointF(0, 0));
    hoverEvent.setScenePos(item->mapToScene(QPointF(0, 0)));
    scene.sendEvent(item, &hoverEvent);
}

// ---------------------------------------------------------------------------
// Helper: sends a hover-leave event to an item via its scene
// ---------------------------------------------------------------------------
static void sendHoverLeave(QGraphicsScene& scene, QGraphicsItem* item) {
    QGraphicsSceneHoverEvent hoverEvent(QEvent::GraphicsSceneHoverLeave);
    hoverEvent.setPos(QPointF(0, 0));
    hoverEvent.setScenePos(item->mapToScene(QPointF(0, 0)));
    scene.sendEvent(item, &hoverEvent);
}

// ---------------------------------------------------------------------------
// Test: hovering a movable label sets Qt::SizeAllCursor
// Validates: Requirement 1.1
// ---------------------------------------------------------------------------
void AutoGroupItemCursorTest::hoverMovableLabelSetsSizeAllCursor() {
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    AutoGroupItem* label = new AutoGroupItem();
    scene.addItem(label);
    label->makeMovable(true);

    sendHoverEnter(scene, label);

    QVERIFY2(label->cursor().shape() == Qt::SizeAllCursor,
             qPrintable(QString("Expected SizeAllCursor, got cursor shape %1").arg(label->cursor().shape())));
}

// ---------------------------------------------------------------------------
// Test: leaving a movable label restores the default cursor
// Validates: Requirement 1.2
// ---------------------------------------------------------------------------
void AutoGroupItemCursorTest::leaveMovableLabelRestoresDefaultCursor() {
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    AutoGroupItem* label = new AutoGroupItem();
    scene.addItem(label);
    label->makeMovable(true);

    sendHoverEnter(scene, label);
    QVERIFY(label->cursor().shape() == Qt::SizeAllCursor);

    sendHoverLeave(scene, label);

    QVERIFY2(false == label->hasCursor(),
             "Expected cursor to be unset after hover leave, but item still has a custom cursor");
}

// ---------------------------------------------------------------------------
// Test: hovering a non-movable label (self-transition) does not change cursor
// Validates: Requirement 1.3
// ---------------------------------------------------------------------------
void AutoGroupItemCursorTest::hoverNonMovableLabelDoesNotChangeCursor() {
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    AutoGroupItem* label = new AutoGroupItem();
    scene.addItem(label);
    label->makeMovable(false);

    sendHoverEnter(scene, label);

    QVERIFY2(false == label->hasCursor(),
             "Expected no cursor change for non-movable label, but item has a custom cursor");
}

// ---------------------------------------------------------------------------
// Test: calling makeMovable(false) while hovered unsets the cursor
// Validates: Requirement 1.3 (transitional behavior)
// ---------------------------------------------------------------------------
void AutoGroupItemCursorTest::makeMovableDisabledUnsetsCursor() {
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    AutoGroupItem* label = new AutoGroupItem();
    scene.addItem(label);
    label->makeMovable(true);

    sendHoverEnter(scene, label);
    QVERIFY(label->cursor().shape() == Qt::SizeAllCursor);

    // Disable movability while hovered — cursor should be unset
    label->makeMovable(false);

    QVERIFY2(false == label->hasCursor(),
             "Expected cursor to be unset after makeMovable(false), but item still has a custom cursor");
}

// ---------------------------------------------------------------------------
// Property 1: Cursor round-trip for movable labels
// For any AutoGroupItem where makeMovable(true) has been called, triggering
// hover-enter followed by hover-leave shall result in the cursor being set to
// Qt::SizeAllCursor during hover and restored after leaving.
// Validates: Requirements 1.1, 1.2
// ---------------------------------------------------------------------------
void AutoGroupItemCursorTest::cursorRoundTripProperty() {
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    AutoGroupItem* label = new AutoGroupItem();
    scene.addItem(label);
    label->makeMovable(true);

    // Multiple hover-enter/leave cycles should always produce the same result
    for (int i = 0; i < 10; ++i) {
        sendHoverEnter(scene, label);
        QVERIFY2(label->cursor().shape() == Qt::SizeAllCursor,
                 qPrintable(QString("Cycle %1: expected SizeAllCursor during hover").arg(i)));

        sendHoverLeave(scene, label);
        QVERIFY2(false == label->hasCursor(),
                 qPrintable(QString("Cycle %1: expected no cursor after leave").arg(i)));
    }
}

// ---------------------------------------------------------------------------
// Property 2: Non-movable labels do not change cursor
// For any AutoGroupItem where makeMovable(false) has been called, triggering
// hover-enter shall NOT change the cursor to Qt::SizeAllCursor.
// Validates: Requirements 1.3
// ---------------------------------------------------------------------------
void AutoGroupItemCursorTest::nonMovableNeverSetsCursorProperty() {
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    AutoGroupItem* label = new AutoGroupItem();
    scene.addItem(label);

    // Test various sequences of makeMovable(false) and hover events
    label->makeMovable(false);

    for (int i = 0; i < 10; ++i) {
        sendHoverEnter(scene, label);
        QVERIFY2(false == label->hasCursor(),
                 qPrintable(QString("Cycle %1: non-movable label should not set cursor on hover").arg(i)));

        sendHoverLeave(scene, label);
        QVERIFY2(false == label->hasCursor(),
                 qPrintable(QString("Cycle %1: non-movable label should not have cursor after leave").arg(i)));
    }

    // Also test: transition from movable to non-movable then hover
    label->makeMovable(true);
    label->makeMovable(false);

    sendHoverEnter(scene, label);
    QVERIFY2(false == label->hasCursor(),
             "After makeMovable(true) then makeMovable(false), hover should not set cursor");
}

int runAutoGroupItemCursorTest(int argc, char** argv) {
    QApplication app(argc, argv);
    AutoGroupItemCursorTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "AutoGroupItemCursorTest.moc"
