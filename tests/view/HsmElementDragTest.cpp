#include <QGraphicsScene>
#include <QGraphicsView>
#include <QtTest>
#include <cmath>
#include <random>

#include "model/RegularState.hpp"
#include "view/elements/HsmStateElement.hpp"
#include "view/theme/ThemeManager.hpp"
#include "view/widgets/HsmGraphicsView.hpp"

#include "controllers/IProjectController.hpp"
#include "DragTestHelper.hpp"

using namespace view;

// ---------------------------------------------------------------------------
// HsmGraphicsView subclass that simulates Ctrl+R being pressed
// ---------------------------------------------------------------------------
class ReparentEnabledView : public HsmGraphicsView {
public:
    explicit ReparentEnabledView(QWidget* parent = nullptr)
        : HsmGraphicsView(parent) {}

    bool keyboardReparentModifierPressed() const override {
        return mReparentModifierPressed;
    }

    void setReparentModifierPressed(const bool pressed) {
        mReparentModifierPressed = pressed;
    }

private:
    bool mReparentModifierPressed = false;
};

// ---------------------------------------------------------------------------
// Stub IProjectController for tests that need dropElementEvent normalization
// ---------------------------------------------------------------------------
class StubProjectController : public IProjectController {
    Q_OBJECT
public:
    explicit StubProjectController(HsmGraphicsView* view = nullptr, QObject* parent = nullptr)
        : IProjectController(parent), mView(view) {}

    bool importModel(const QString&) override { return false; }
    bool exportModel() override { return false; }
    bool exportModel(const QString&) override { return false; }
    void updateModelPath(const QString&) override {}
    void handleViewDropEvent(const QString&, const QPointF&, const model::EntityID_t) override {}
    void handleViewMoveEvent(const model::EntityID_t entity, const model::EntityID_t parent) override {
        if (nullptr != mView) {
            mView->moveHsmElement(entity, parent);
        }
    }
    void handleDeleteElements(const QList<model::EntityID_t>&) override {}
    QString serializeElementsToScxml(const QList<model::EntityID_t>&) const override { return {}; }
    bool pasteScxmlElements(const QString&, const QList<model::EntityID_t>&, const QPointF&, const bool) override {
        return false;
    }
    void beginHistoryTransaction(const QString&) override {}
    void commitHistoryTransaction() override {}
    void cancelHistoryTransaction() override {}
    void markHistoryElement(const model::EntityID_t) override {}
    void unmarkHistoryElement(const model::EntityID_t) override {}
    bool undo() override { return false; }
    bool redo() override { return false; }
    bool canUndo() const override { return false; }
    bool canRedo() const override { return false; }

private:
    HsmGraphicsView* mView = nullptr;
};

// ---------------------------------------------------------------------------
// Fixture: builds a 3-level hierarchy for drag tests
// ---------------------------------------------------------------------------
struct DragTestFixture {
    QGraphicsScene scene;
    HsmGraphicsView view;
    QSharedPointer<StubProjectController> controller;

    view::HsmElement* grandparent = nullptr;
    view::HsmElement* parent = nullptr;
    view::HsmElement* child = nullptr;

    QSharedPointer<model::RegularState> grandparentModel;
    QSharedPointer<model::RegularState> parentModel;
    QSharedPointer<model::RegularState> childModel;

    DragTestFixture(const bool enableSnapToGrid = false) {
        view.setScene(&scene);
        view.setSnapToGridEnabled(enableSnapToGrid);

        controller = QSharedPointer<StubProjectController>::create();
        view.setProjectController(controller.toWeakRef());

        // Create model entities
        grandparentModel = QSharedPointer<model::RegularState>::create("grandparent");
        parentModel = QSharedPointer<model::RegularState>::create("parent");
        childModel = QSharedPointer<model::RegularState>::create("child");

        // Create view elements using the view's factory method
        grandparent = view.createHsmElement(grandparentModel, "state", QPointF(0, 0), QSizeF(400, 300), model::INVALID_MODEL_ID);
        parent = view.createHsmElement(parentModel, "state", QPointF(20, 60), QSizeF(260, 200), grandparentModel->id());
        child = view.createHsmElement(childModel, "state", QPointF(10, 20), QSizeF(200, 100), parentModel->id());
    }
};

// ---------------------------------------------------------------------------
// Test class
// ---------------------------------------------------------------------------
class HsmElementDragTest : public QObject {
    Q_OBJECT

private slots:
    void snapErrorBounded();
    void handleLayoutPreservesScenePos();
    void bodySectionTracksOuterRectLeft();
    void normalizeElementRectPreservesScenePos();
    void topLevelDragNoDrift();
    void twoLevelChildDragNoDrift();
    void twoLevelChildDragSnapToGridInternal();
    void twoLevelChildDragSnapToGridResizing();
    void threeLevelChildDragNoDrift();
    void threeLevelDragLeftThenUp();
    void reparentChildFromOneParentToAnother();
    void reparentChildToSiblingWithinSameParent();
    void dragZValueRaisedDuringDrag();
    void dragZValueRestoredAfterDrop();
    void preservationZValueAtRest();
    void preservationZValueClickWithoutDrag();
    void preservationZValueAfterDragCycle();
    void preservationZValueRandomInteractions();

private:
    void compareNormalized(const QString& elementName, const QRectF& oldRect, const QRectF& newRect);
};

// ---------------------------------------------------------------------------
// 2.1 — Snap error is bounded by gridStep/2
// Validates: Requirements 5.2 (Property 10)
// ---------------------------------------------------------------------------
void HsmElementDragTest::snapErrorBounded() {
    const int gridStep = ThemeManager::instance().theme().grid.minorLineStep;
    const qreal halfStep = gridStep / 2.0;

    // --- Part 1: 100 random points, verify error bounded by gridStep/2 ---
    std::mt19937 rng(42);  // fixed seed for reproducibility
    std::uniform_real_distribution<qreal> dist(-500.0, 500.0);

    for (int i = 0; i < 100; ++i) {
        const QPointF point(dist(rng), dist(rng));
        const QPointF snapped = HsmGraphicsView::alignPointToGrid(point);

        const qreal errorX = std::abs(snapped.x() - point.x());
        const qreal errorY = std::abs(snapped.y() - point.y());

        QVERIFY2(errorX <= halfStep + 1e-9,
                 qPrintable(QString("X snap error %1 exceeds gridStep/2 (%2) for point (%3, %4)")
                                .arg(errorX)
                                .arg(halfStep)
                                .arg(point.x())
                                .arg(point.y())));
        QVERIFY2(errorY <= halfStep + 1e-9,
                 qPrintable(QString("Y snap error %1 exceeds gridStep/2 (%2) for point (%3, %4)")
                                .arg(errorX)
                                .arg(halfStep)
                                .arg(point.x())
                                .arg(point.y())));
    }

    // --- Part 2: Points exactly on grid should not move ---
    for (int ix = -5; ix <= 5; ++ix) {
        for (int iy = -5; iy <= 5; ++iy) {
            const QPointF gridPoint(ix * gridStep, iy * gridStep);
            const QPointF snapped = HsmGraphicsView::alignPointToGrid(gridPoint);
            QCOMPARE(snapped, gridPoint);
        }
    }

    // --- Part 3: Points at gridStep/2 boundary — deterministic result ---
    for (int ix = -5; ix <= 5; ++ix) {
        for (int iy = -5; iy <= 5; ++iy) {
            const QPointF boundaryPoint(ix * gridStep + halfStep, iy * gridStep + halfStep);
            const QPointF snapped1 = HsmGraphicsView::alignPointToGrid(boundaryPoint);
            const QPointF snapped2 = HsmGraphicsView::alignPointToGrid(boundaryPoint);

            // Must be deterministic: same input gives same output
            QCOMPARE(snapped1, snapped2);

            // Result must still be on a grid point
            const qreal remX = std::fmod(std::abs(snapped1.x()), static_cast<qreal>(gridStep));
            const qreal remY = std::fmod(std::abs(snapped1.y()), static_cast<qreal>(gridStep));
            QVERIFY2(remX < 1e-9 || std::abs(remX - gridStep) < 1e-9,
                     qPrintable(QString("Boundary snap X not on grid: %1").arg(snapped1.x())));
            QVERIFY2(remY < 1e-9 || std::abs(remY - gridStep) < 1e-9,
                     qPrintable(QString("Boundary snap Y not on grid: %1").arg(snapped1.y())));
        }
    }
}

// ---------------------------------------------------------------------------
// Test: mBodySection position tracks outerRect.left()
// ---------------------------------------------------------------------------
void HsmElementDragTest::bodySectionTracksOuterRectLeft() {
    QGraphicsScene scene;
    HsmGraphicsView view;
    view.setScene(&scene);

    // Create a state element with sufficient size (above min size 200x100)
    auto model = QSharedPointer<model::RegularState>::create("testState");
    auto childModel = QSharedPointer<model::RegularState>::create("child");

    view::HsmElement* element = view.createHsmElement(model, "state", QPointF(0, 0), QSizeF(200, 140), model::INVALID_MODEL_ID);
    view::HsmElement* child = view.createHsmElement(childModel, "state", QPointF(0, 0), QSizeF(100, 60), model->id());

    // child->parentItem() is mBodySection
    QGraphicsItem* bodySection = child->parentItem();
    QVERIFY(bodySection != nullptr);

    auto* resizable = dynamic_cast<view::HsmResizableElement*>(element);

    // Test 1: outerRect at (0, 0) — standard case
    resizable->resizeElement(QRectF(0, 0, 200, 140));
    QCOMPARE(bodySection->pos().x(), 0.0);

    // Test 2: outerRect at (-20, 0) — slightly negative left
    resizable->resizeElement(QRectF(-20, 0, 220, 140));
    QCOMPARE(bodySection->pos().x(), -20.0);

    // Test 3: outerRect at (-60, 0) — more negative left
    resizable->resizeElement(QRectF(-60, 0, 260, 140));
    QCOMPARE(bodySection->pos().x(), -60.0);
}

// ---------------------------------------------------------------------------
// 2.2 — handleLayoutBeforeCommit preserves scene position
// Verifies that when a child is moved (including to positions that force
// parent expansion), the child's scene position after the move matches the
// expected snapped target. This tests the core invariant that
// handleLayoutBeforeCommit is supposed to maintain: scene pos stability
// despite parent coordinate system shifts.
// ---------------------------------------------------------------------------
void HsmElementDragTest::handleLayoutPreservesScenePos() {
    const qreal gridStep = ThemeManager::instance().theme().grid.minorLineStep;

    // --- Set up a 2-level hierarchy: parent + child ---
    QGraphicsScene scene;
    HsmGraphicsView view;
    view.setScene(&scene);
    view.setSnapToGridEnabled(true);

    auto parentModel = QSharedPointer<model::RegularState>::create("parent");
    auto childModel = QSharedPointer<model::RegularState>::create("child");

    view::HsmElement* parent = view.createHsmElement(parentModel, "state", QPointF(0, 0), QSizeF(300, 200), model::INVALID_MODEL_ID);
    view::HsmElement* child = view.createHsmElement(childModel, "state", QPointF(40, 20), QSizeF(100, 60), parentModel->id());

    // --- Part 1: Move child to a position well inside parent boundary ---
    // Record initial scene position
    const QPointF initialScenePos = child->scenePos();

    // Begin drag on child — puts it in PREPARE state with mDragMode=SINGLE
    DragTestHelper::beginDrag(child);

    // Move one grid step to the right — stays well inside parent.
    // The first simulateDragMove triggers PREPARE→DRAGGING transition via dragElementBegin.
    DragTestHelper::simulateDragMove(child, QPointF(gridStep, 0));

    // Verify scene position is at the expected snapped location
    const QPointF expectedScenePos1 = HsmGraphicsView::alignPointToGrid(initialScenePos + QPointF(gridStep, 0));
    const QPointF actualScenePos1 = child->scenePos();

    QVERIFY2(qFuzzyCompare(actualScenePos1.x(), expectedScenePos1.x()) &&
                 qFuzzyCompare(actualScenePos1.y(), expectedScenePos1.y()),
             qPrintable(QString("Inside-parent move: expected scene pos (%1, %2), got (%3, %4)")
                            .arg(expectedScenePos1.x())
                            .arg(expectedScenePos1.y())
                            .arg(actualScenePos1.x())
                            .arg(actualScenePos1.y())));

    DragTestHelper::endDrag(child);

    // --- Part 2: Move child leftward past parent boundary ---
    // This should force handleLayoutBeforeCommit to call resizeToFitChildItem,
    // which expands the parent's outerRect leftward (goes negative).
    // The key test: child's scene position should still be at the expected snapped location.

    // Record child scene pos before the second drag
    const QPointF beforeSecondDragScenePos = child->scenePos();

    DragTestHelper::beginDrag(child);

    // Move far left to exceed parent's left edge — forces parent expansion
    DragTestHelper::simulateDragMove(child, QPointF(-200, 0));

    const QPointF expectedScenePos2 = HsmGraphicsView::alignPointToGrid(beforeSecondDragScenePos + QPointF(-200, 0));
    const QPointF actualScenePos2 = child->scenePos();

    QVERIFY2(qFuzzyCompare(actualScenePos2.x(), expectedScenePos2.x()) &&
                 qFuzzyCompare(actualScenePos2.y(), expectedScenePos2.y()),
             qPrintable(QString("Leftward-expand move: expected scene pos (%1, %2), got (%3, %4)")
                            .arg(expectedScenePos2.x())
                            .arg(expectedScenePos2.y())
                            .arg(actualScenePos2.x())
                            .arg(actualScenePos2.y())));

    // Verify parent actually expanded (outerRect left should be negative)
    QVERIFY2(parent->elementRect().left() < 0,
             qPrintable(QString("Parent should have expanded leftward, but outerRect.left() = %1")
                            .arg(parent->elementRect().left())));

    DragTestHelper::endDrag(child);
}

// ---------------------------------------------------------------------------
// 2.3 — normalizeElementRect preserves scene positions
// Verifies that normalizeElementRect moves the element's pos by the outerRect
// top-left offset while resetting outerRect to (0,0), preserving both the
// parent's visual scene position and child's scenePos().
// ---------------------------------------------------------------------------
void HsmElementDragTest::normalizeElementRectPreservesScenePos() {
    QGraphicsScene scene;
    HsmGraphicsView view;
    view.setScene(&scene);

    // Create parent state element
    auto parentModel = QSharedPointer<model::RegularState>::create("parent");
    auto childModel = QSharedPointer<model::RegularState>::create("child");

    view::HsmElement* parent = view.createHsmElement(parentModel, "state", QPointF(100, 50), QSizeF(280, 200), model::INVALID_MODEL_ID);
    view::HsmElement* child = view.createHsmElement(childModel, "state", QPointF(20, 20), QSizeF(100, 60), parentModel->id());

    auto* resizableParent = dynamic_cast<view::HsmResizableElement*>(parent);

    // Make outerRect have negative top-left by resizing
    resizableParent->resizeElement(QRectF(-40, 0, 280, 200));

    // Record scene positions before normalize
    // For the parent: the visual top-left (scenePos + outerRect.topLeft) is the meaningful position
    const QPointF parentVisualScenePosBefore = parent->scenePos() + parent->elementRect().topLeft();
    const QPointF childScenePosBefore = child->scenePos();

    // Call normalizeElementRect — should move pos by (-40, 0) and reset outerRect to (0,0)
    resizableParent->normalizeElementRect();

    // Verify: parent visual scene position unchanged
    // normalizeElementRect adjusts pos by outerRect.topLeft(), then sets outerRect to (0,0)
    // so the drawn area stays in the same place in the scene
    const QPointF parentVisualScenePosAfter = parent->scenePos() + parent->elementRect().topLeft();
    QVERIFY2(qFuzzyCompare(parentVisualScenePosAfter.x(), parentVisualScenePosBefore.x()) &&
                 qFuzzyCompare(parentVisualScenePosAfter.y(), parentVisualScenePosBefore.y()),
             qPrintable(QString("Parent visual scenePos changed: was (%1, %2), now (%3, %4)")
                            .arg(parentVisualScenePosBefore.x())
                            .arg(parentVisualScenePosBefore.y())
                            .arg(parentVisualScenePosAfter.x())
                            .arg(parentVisualScenePosAfter.y())));

    // Verify: child scenePos unchanged
    // The child is parented to mBodySection. When outerRect moves to (0,0), layoutSections
    // repositions mBodySection to compensate, so the child's scene position should be stable.
    QVERIFY2(qFuzzyCompare(child->scenePos().x(), childScenePosBefore.x()) &&
                 qFuzzyCompare(child->scenePos().y(), childScenePosBefore.y()),
             qPrintable(QString("Child scenePos changed: was (%1, %2), now (%3, %4)")
                            .arg(childScenePosBefore.x())
                            .arg(childScenePosBefore.y())
                            .arg(child->scenePos().x())
                            .arg(child->scenePos().y())));

    // Verify: parent outerRect starts at (0, 0)
    QCOMPARE(parent->elementRect().topLeft(), QPointF(0, 0));
}

// ---------------------------------------------------------------------------
// 2.5 — Top-level element drag: no drift after N moves
// Verifies that dragging a top-level element with many small (1px) moves
// results in the correct final snapped position without intermediate jumps
// larger than gridStep.
// ---------------------------------------------------------------------------
void HsmElementDragTest::topLevelDragNoDrift() {
    const int gridStep = ThemeManager::instance().theme().grid.minorLineStep;

    QGraphicsScene scene;
    HsmGraphicsView view;
    view.setScene(&scene);
    view.setSnapToGridEnabled(true);

    // Create a single top-level element at pos (100, 100) — on grid
    auto model = QSharedPointer<model::RegularState>::create("topLevel");
    view::HsmElement* element = view.createHsmElement(model, "state", QPointF(100, 100), QSizeF(200, 100), model::INVALID_MODEL_ID);

    // Begin drag — puts element in PREPARE state with SINGLE mode
    DragTestHelper::beginDrag(element);

    QPointF prevScenePos = element->scenePos();

    // Simulate 50 sequential 1px moves to the left (total displacement = -50px in X)
    for (int i = 0; i < 50; ++i) {
        DragTestHelper::simulateDragMove(element, QPointF(-1, 0));

        const QPointF newScenePos = element->scenePos();

        // Verify: no jump larger than gridStep between consecutive committed positions
        const qreal jump = std::abs(newScenePos.x() - prevScenePos.x());
        QVERIFY2(jump <= gridStep,
                 qPrintable(QString("Jump %1 > gridStep %2 at step %3 (prev=%4, new=%5)")
                                .arg(jump)
                                .arg(gridStep)
                                .arg(i)
                                .arg(prevScenePos.x())
                                .arg(newScenePos.x())));

        prevScenePos = newScenePos;
    }

    // Expected final position: snap(100 - 50, 100) = snap(50, 100)
    const QPointF expectedFinal = HsmGraphicsView::alignPointToGrid(QPointF(50, 100));
    QCOMPARE(element->scenePos().x(), expectedFinal.x());
    QCOMPARE(element->scenePos().y(), expectedFinal.y());

    DragTestHelper::endDrag(element);
}

// ---------------------------------------------------------------------------
// 2.6 — 2-level child drag leftward past parent boundary: no acceleration
// Verifies that dragging a child element leftward in a 2-level hierarchy
// (parent → child) past the parent's left boundary causes the parent to expand
// while the child continues moving at cursor speed (no jumps > gridStep).
// The bug: once the parent boundary is reached and parent starts expanding,
// the child accelerates (jumps multiple gridSteps per cursor pixel) due to
// body section coordinate shifts not being properly accounted for.
// ---------------------------------------------------------------------------
void HsmElementDragTest::twoLevelChildDragNoDrift() {
    const int gridStep = ThemeManager::instance().theme().grid.minorLineStep;

    QGraphicsScene scene;
    HsmGraphicsView view;
    view.setScene(&scene);

    auto parentModel = QSharedPointer<model::RegularState>::create("parent");
    auto childModel = QSharedPointer<model::RegularState>::create("child");

    // Parent at grid-aligned position, child at left edge (X=0) of parent body.
    // With snap-to-grid enabled, moving the child leftward past the parent boundary
    // triggers parent resize + bodySection X-compensation.
    // Bug: the compensation shifts bodySection between mouse events, which corrupts
    // Qt's internal delta-based position tracking (event->pos() - event->lastPos()),
    // causing immediate re-snapping on the very next mouse event (acceleration bug).
    view::HsmStateElement* parent = (view::HsmStateElement*)view.createHsmElement(parentModel, "state", QPointF(0, 0), QSizeF(300, 200), model::INVALID_MODEL_ID);
    view::HsmStateElement* child = (view::HsmStateElement*)view.createHsmElement(childModel, "state", QPointF(0, 20), QSizeF(60, 40), parentModel->id());

    // Enable snap AFTER positioning to preserve setup
    view.setSnapToGridEnabled(true);

    // NOTE: This test uses actual QGraphicsSceneMouseEvents (press + move) sent through
    // the scene, which triggers Qt's real QGraphicsItem::mouseMoveEvent and its internal
    // delta-based position tracking. This correctly reproduces the acceleration bug that
    // DragTestHelper::simulateDragMove cannot expose (it uses an idealized scene-based
    // mapping that always recomputes through the current parent transform).

    // Send a mouse press to start the drag
    const QPointF pressScenePos = child->scenePos();
    {
        QGraphicsSceneMouseEvent pressEvent(QEvent::GraphicsSceneMousePress);
        pressEvent.setButton(Qt::LeftButton);
        pressEvent.setButtons(Qt::LeftButton);
        pressEvent.setScenePos(pressScenePos);
        pressEvent.setPos(child->mapFromScene(pressScenePos));
        scene.sendEvent(child, &pressEvent);
    }

    QPointF prevScenePos = child->scenePos();
    QPointF lastScenePos = pressScenePos;
    int lastSnapStep = -100;

    // Move left 1px at a time using mouse move events.
    // Qt's mouseMoveEvent uses event->pos() - event->lastPos() internally.
    for (int i = 0; i < (7 * gridStep); ++i) {
        QPointF moveScenePos = lastScenePos + QPointF(-1, 0);

        QGraphicsSceneMouseEvent moveEvent(QEvent::GraphicsSceneMouseMove);
        moveEvent.setButton(Qt::NoButton);
        moveEvent.setButtons(Qt::LeftButton);
        moveEvent.setScenePos(moveScenePos);
        moveEvent.setLastScenePos(lastScenePos);
        moveEvent.setPos(child->mapFromScene(moveScenePos));
        moveEvent.setLastPos(child->mapFromScene(lastScenePos));
        scene.sendEvent(child, &moveEvent);

        lastScenePos = moveScenePos;
        const QPointF newScenePos = child->scenePos();

        // Monotonicity: X should never increase (we're moving left)
        QVERIFY2(newScenePos.x() <= prevScenePos.x() + 1e-9,
                 qPrintable(QString("Non-monotonic at step %1: prev X=%2, new X=%3")
                                .arg(i)
                                .arg(prevScenePos.x())
                                .arg(newScenePos.x())));

        const QRectF newChildSceneRect = child->sceneBoundingRect();
        const QRectF newParentSceneRect = parent->sceneBoundingRect();
        const QRectF newParentBodySceneRect = parent->sceneBodyBoundingRect();

        qDebug() << "--- PARENT. rect=" << newParentSceneRect << ", body=" << newParentBodySceneRect;

         // parent and child should always be aligned (left edge)
        QVERIFY2(qFuzzyCompare(newChildSceneRect.left(), newParentSceneRect.left()),
                 qPrintable(QString("Parent and child were expected to have the same left edge at step %1: child.left=%2, parent.left=%3")
                                .arg(i)
                                .arg(newChildSceneRect.left())
                                .arg(newParentSceneRect.left())));

        // parent's rect and it's body section must be always aligned
        QVERIFY2(qFuzzyCompare(newParentBodySceneRect.left(), newParentSceneRect.left()) &&
                 qFuzzyCompare(newParentBodySceneRect.right(), newParentSceneRect.right()),
                 qPrintable(QString("Parent's body section (left/right) is not aligned at step %1: body=%2/%3, parent=%4/%5")
                                .arg(i)
                                .arg(newParentBodySceneRect.left())
                                .arg(newParentBodySceneRect.right())
                                .arg(newParentSceneRect.left())
                                .arg(newParentSceneRect.right())));

        // No large jumps: movement per step must not exceed gridStep.
        qreal jump = std::abs(newScenePos.x() - prevScenePos.x());
        QVERIFY2(jump <= gridStep,
                 qPrintable(QString("Jump %1 > gridStep %2 at step %3")
                                .arg(jump)
                                .arg(gridStep)
                                .arg(i)));

        // Double-snap detection: consecutive snaps must be spaced at least gridStep/2
        // cursor pixels apart.
        if (jump > 0) {
            const int stepsSinceLastSnap = i - lastSnapStep;
            QVERIFY2(stepsSinceLastSnap >= gridStep / 2,
                     qPrintable(QString("Double-snap detected at step %1: previous snap at step %2 "
                                        "(only %3 steps apart, expected >= %4)")
                                    .arg(i)
                                    .arg(lastSnapStep)
                                    .arg(stepsSinceLastSnap)
                                    .arg(gridStep / 2)));
            lastSnapStep = i;
        }

        prevScenePos = newScenePos;
    }

    // Verify parent actually expanded (outerRect left should be negative).
    QVERIFY2(parent->elementRect().left() < 0,
             qPrintable(QString("Parent should have expanded leftward, but outerRect.left() = %1. "
                                "The bodySection X-compensation is corrupting Qt's delta-based drag tracking.")
                            .arg(parent->elementRect().left())));

    DragTestHelper::endDrag(child);
}

// ---------------------------------------------------------------------------
// 2.8 — 2-level child drag: no double-snap when moving with snap to grid
// Reproduces: when a child element is dragged inside its parent with snap-to-grid
// enabled, consecutive 1px cursor moves must not cause more than one grid snap
// each. When dragging a child internally (not reparenting), the view's
// dragElementBegin slot calls setDragMode(false), which puts the element in
// DragState::DRAGGING but DragMode::NONE. The itemChange code must still use
// the drift-free path in this state.
// ---------------------------------------------------------------------------
void HsmElementDragTest::twoLevelChildDragSnapToGridInternal() {
    const int gridStep = ThemeManager::instance().theme().grid.minorLineStep;

    QGraphicsScene scene;
    HsmGraphicsView view;
    view.setScene(&scene);

    auto parentModel = QSharedPointer<model::RegularState>::create("parent");
    auto childModel = QSharedPointer<model::RegularState>::create("child");

    // Place parent at a non-grid-aligned position. This makes the body section
    // (and thus the child) have a non-grid-aligned scene position, which is the
    // condition that triggers the double-snap bug.
    view::HsmElement* parent = view.createHsmElement(parentModel, "state", QPointF(3, 5), QSizeF(400, 200), model::INVALID_MODEL_ID);
    view::HsmElement* child = view.createHsmElement(childModel, "state", QPointF(2 * gridStep, gridStep), QSizeF(60, 40), parentModel->id());

    // Enable snap AFTER positioning — simulates the real app scenario where
    // elements may have been placed without grid alignment and the user later
    // drags with snap enabled.
    view.setSnapToGridEnabled(true);

    const QPointF startScenePos = child->scenePos();
    QPointF prevScenePos = startScenePos;

    DragTestHelper::beginDrag(child);

    // The first simulateDragMove transitions from PREPARE to DRAGGING and triggers
    // dragElementBegin signal. Since child has a parent and reparent modifier is not
    // pressed, the view automatically calls setDragMode(false) putting element in
    // DragState::DRAGGING but DragMode::NONE.
    DragTestHelper::simulateDragMove(child, QPointF(1, 0));
    prevScenePos = child->scenePos();

    int snapCount = 0;
    int lastSnapStep = -100;

    // Move rightward 1px at a time. Even with DragMode::NONE, the drift-free path
    // must be active (since DragState::DRAGGING) to prevent double-snaps.
    for (int i = 0; i < 4 * gridStep; ++i) {
        QPointF newScenePos = DragTestHelper::simulateDragMove(child, QPointF(1, 0));

        qreal jump = newScenePos.x() - prevScenePos.x();

        // The element must never jump more than one gridStep at a time
        QVERIFY2(jump <= gridStep,
                 qPrintable(QString("Jump %1 > gridStep %2 at step %3: prev=%4, new=%5")
                                .arg(jump)
                                .arg(gridStep)
                                .arg(i)
                                .arg(prevScenePos.x())
                                .arg(newScenePos.x())));

        if (jump > 0) {
            // When a snap occurs, it must be exactly one gridStep
            QVERIFY2(qFuzzyCompare(jump, static_cast<qreal>(gridStep)),
                     qPrintable(QString("Snap jump %1 != gridStep %2 at step %3")
                                    .arg(jump)
                                    .arg(gridStep)
                                    .arg(i)));

            // Consecutive snaps must be separated by at least gridStep/2 steps of
            // cursor movement. A double-snap (two snaps within a few pixels of
            // cursor movement) indicates the position tracking bug.
            const int stepsSinceLastSnap = i - lastSnapStep;
            QVERIFY2(stepsSinceLastSnap >= gridStep / 2,
                     qPrintable(QString("Double-snap detected: snap at step %1, previous snap at step %2 "
                                        "(only %3 steps apart, expected >= %4)")
                                    .arg(i)
                                    .arg(lastSnapStep)
                                    .arg(stepsSinceLastSnap)
                                    .arg(gridStep / 2)));

            lastSnapStep = i;
            snapCount++;
        }

        prevScenePos = newScenePos;
    }

    // After moving 4*gridStep pixels, we expect approximately 4 snaps.
    // Allow ±1 tolerance for boundary effects from initial non-grid position.
    QVERIFY2(std::abs(snapCount - 4) <= 1,
             qPrintable(QString("Expected ~4 snaps after %1px of cursor movement, got %2")
                            .arg(4 * gridStep)
                            .arg(snapCount)));

    DragTestHelper::endDrag(child);
}

// ---------------------------------------------------------------------------
// 2.9 — 2-level child drag outside parent's boundary shoudl result in parent's resizing
// and monotonic movement of a child element
// ---------------------------------------------------------------------------
void HsmElementDragTest::twoLevelChildDragSnapToGridResizing() {
    const int gridStep = ThemeManager::instance().theme().grid.minorLineStep;

    QGraphicsScene scene;
    HsmGraphicsView view;
    view.setScene(&scene);

    auto parentModel = QSharedPointer<model::RegularState>::create("parent");
    auto childModel = QSharedPointer<model::RegularState>::create("child");

    // Place parent at a non-grid-aligned position. This makes the body section
    // (and thus the child) have a non-grid-aligned scene position, which is the
    // condition that triggers the double-snap bug.
    view::HsmElement* parent = view.createHsmElement(parentModel, "state", QPointF(0, 0), QSizeF(400, 200), model::INVALID_MODEL_ID);
    // set element on the left edge of the parent
    view::HsmElement* child = view.createHsmElement(childModel, "state", QPointF(0, 50), QSizeF(60, 40), parentModel->id());

    // Enable snap AFTER positioning — simulates the real app scenario where
    // elements may have been placed without grid alignment and the user later
    // drags with snap enabled.
    view.setSnapToGridEnabled(true);

    const QPointF startScenePos = child->scenePos();
    QPointF prevScenePos = startScenePos;

    DragTestHelper::beginDrag(child);

    // The first simulateDragMove transitions from PREPARE to DRAGGING and triggers
    // dragElementBegin signal. Since child has a parent and reparent modifier is not
    // pressed, the view automatically calls setDragMode(false).
    // Move enough to trigger grid snapping
    QPointF newScenePos = DragTestHelper::simulateDragMove(child, QPointF(-gridStep/2, 0));
    qreal jump = prevScenePos.x() - newScenePos.x();
    QVERIFY2(qFuzzyCompare(jump, static_cast<qreal>(gridStep)),
            qPrintable(QString("Snap jump %1 != gridStep %2 at step 1")
                        .arg(jump)
                        .arg(gridStep)));

    // make one more move to the left
    prevScenePos = newScenePos;
    newScenePos = DragTestHelper::simulateDragMove(child, QPointF(-2, 0));
    jump = newScenePos.x() - prevScenePos.x();

    QVERIFY2(qFuzzyCompare(jump, 0.0),
            qPrintable(QString("Snap jump %1 != 0.0 at step 2")
                        .arg(jump)));

    DragTestHelper::endDrag(child);
}

// ---------------------------------------------------------------------------
// 2.7 — 3-level child drag leftward: no drift, parent stays in grandparent
// Verifies that dragging a child in a 3-level hierarchy (grandparent → parent → child)
// produces monotonically decreasing X positions and maintains containment at each level.
// This test is EXPECTED TO FAIL on the original buggy code due to drift.
// ---------------------------------------------------------------------------
void HsmElementDragTest::threeLevelChildDragNoDrift() {
    DragTestFixture fixture;

    const QPointF startScenePos = fixture.child->scenePos();

    DragTestHelper::beginDrag(fixture.child);

    QRectF childSceneRect;
    QRectF parentSceneRect;
    QRectF grandparentSceneRect;
    QPointF prevScenePos = startScenePos;

    // Simulate 120 sequential 1px moves to the left.
    for (int i = 0; i < 120; ++i) {
        DragTestHelper::simulateDragMove(fixture.child, QPointF(-1, 0));
        const QPointF newScenePos = fixture.child->scenePos();

        // Monotonicity: X should never increase
        QVERIFY2(newScenePos.x() <= prevScenePos.x(),
                 qPrintable(QString("Non-monotonic at step %1: prev X=%2, new X=%3")
                                .arg(i)
                                .arg(prevScenePos.x())
                                .arg(newScenePos.x())));

        // Containment: child within parent
        childSceneRect = fixture.child->mapRectToScene(fixture.child->elementRect());
        parentSceneRect = fixture.parent->mapRectToScene(fixture.parent->elementRect());
        grandparentSceneRect = fixture.grandparent->mapRectToScene(fixture.grandparent->elementRect());

        qDebug() << "====== childSceneRect=" << childSceneRect << " (" << fixture.child->elementRect() << ")";
        qDebug() << "====== parentSceneRect=" << parentSceneRect << " (" << fixture.parent->elementRect() << ")";
        qDebug() << "====== grandparentSceneRect=" << grandparentSceneRect;

        QVERIFY2(parentSceneRect.contains(childSceneRect),
                 qPrintable(QString("Child not in parent at step %1: child=(%2,%3,%4,%5) parent=(%6,%7,%8,%9)")
                                .arg(i)
                                .arg(childSceneRect.x())
                                .arg(childSceneRect.y())
                                .arg(childSceneRect.width())
                                .arg(childSceneRect.height())
                                .arg(parentSceneRect.x())
                                .arg(parentSceneRect.y())
                                .arg(parentSceneRect.width())
                                .arg(parentSceneRect.height())));

        QVERIFY2(grandparentSceneRect.contains(parentSceneRect),
                 qPrintable(QString("Parent not in grandparent at step %1: parent=(%2,%3,%4,%5) grandparent=(%6,%7,%8,%9)")
                                .arg(i)
                                .arg(parentSceneRect.x())
                                .arg(parentSceneRect.y())
                                .arg(parentSceneRect.width())
                                .arg(parentSceneRect.height())
                                .arg(grandparentSceneRect.x())
                                .arg(grandparentSceneRect.y())
                                .arg(grandparentSceneRect.width())
                                .arg(grandparentSceneRect.height())));

        prevScenePos = newScenePos;
    }

    QRectF oldChildLocalRect = fixture.child->elementRect();
    QRectF oldParentLocalRect = fixture.parent->elementRect();
    QRectF oldGrandparentLocalRect = fixture.grandparent->elementRect();

    DragTestHelper::endDrag(fixture.child);

    // Validate how rects were normalized after finishing dragging. Expected:
    // - boundaries respected
    // - X,Y normalized to start from 0
    // - width/height didn't change
    QRectF newChildSceneRect = fixture.child->mapRectToScene(fixture.child->elementRect());
    QRectF newParentSceneRect = fixture.parent->mapRectToScene(fixture.parent->elementRect());
    QRectF newGrandparentSceneRect = fixture.grandparent->mapRectToScene(fixture.grandparent->elementRect());

    compareNormalized("child", oldChildLocalRect, fixture.child->elementRect());
    compareNormalized("parent", oldParentLocalRect, fixture.parent->elementRect());
    compareNormalized("grandparent", oldGrandparentLocalRect, fixture.grandparent->elementRect());

    QVERIFY2(newParentSceneRect.contains(newChildSceneRect), "Child is outside parent's boundary");
    QVERIFY2(newGrandparentSceneRect.contains(newParentSceneRect), "Parent is outside grandparent's boundary");
}

// ---------------------------------------------------------------------------
// 2.10 — 3-level child drag leftward then upward: no drift
// Verifies that a continuous drag gesture (no mouse release between phases)
// first moving left for 100 steps then upward for 100 steps produces monotonic
// movement in the expected axis, no jumps exceeding gridStep, and maintains
// containment throughout both phases.
// ---------------------------------------------------------------------------
void HsmElementDragTest::threeLevelDragLeftThenUp() {
    const int gridStep = ThemeManager::instance().theme().grid.minorLineStep;
    DragTestFixture fixture;

    const QPointF startScenePos = fixture.child->scenePos();

    DragTestHelper::beginDrag(fixture.child);

    QPointF prevScenePos = startScenePos;

    // Phase 1: 100 moves to the left
    for (int i = 0; i < 100; ++i) {
        DragTestHelper::simulateDragMove(fixture.child, QPointF(-1, 0));
        const QPointF newScenePos = fixture.child->scenePos();

        // X monotonicity during leftward phase
        QVERIFY2(newScenePos.x() <= prevScenePos.x(),
                 qPrintable(QString("Phase 1 non-monotonic X at step %1: prev=%2, new=%3")
                                .arg(i)
                                .arg(prevScenePos.x())
                                .arg(newScenePos.x())));

        // No large jumps in either axis
        QVERIFY2(std::abs(newScenePos.x() - prevScenePos.x()) <= gridStep,
                 qPrintable(QString("Phase 1 X jump > gridStep at step %1: prev=%2, new=%3")
                                .arg(i)
                                .arg(prevScenePos.x())
                                .arg(newScenePos.x())));
        QVERIFY2(std::abs(newScenePos.y() - prevScenePos.y()) <= gridStep,
                 qPrintable(QString("Phase 1 Y jump > gridStep at step %1: prev=%2, new=%3")
                                .arg(i)
                                .arg(prevScenePos.y())
                                .arg(newScenePos.y())));

        // Containment: child within parent
        QRectF childSceneRect = fixture.child->mapRectToScene(fixture.child->elementRect());
        QRectF parentSceneRect = fixture.parent->mapRectToScene(fixture.parent->elementRect());
        QVERIFY2(parentSceneRect.contains(childSceneRect),
                 qPrintable(QString("Phase 1: child not in parent at step %1").arg(i)));

        prevScenePos = newScenePos;
    }

    // Phase 2: 100 moves upward (no mouse release between phases)
    for (int i = 0; i < 100; ++i) {
        DragTestHelper::simulateDragMove(fixture.child, QPointF(0, -1));
        const QPointF newScenePos = fixture.child->scenePos();

        // Y monotonicity during upward phase (Y decreases)
        QVERIFY2(newScenePos.y() <= prevScenePos.y(),
                 qPrintable(QString("Phase 2 non-monotonic Y at step %1: prev=%2, new=%3")
                                .arg(i)
                                .arg(prevScenePos.y())
                                .arg(newScenePos.y())));

        // No large jumps in either axis
        QVERIFY2(std::abs(newScenePos.x() - prevScenePos.x()) <= gridStep,
                 qPrintable(QString("Phase 2 X jump > gridStep at step %1: prev=%2, new=%3")
                                .arg(i)
                                .arg(prevScenePos.x())
                                .arg(newScenePos.x())));
        QVERIFY2(std::abs(newScenePos.y() - prevScenePos.y()) <= gridStep,
                 qPrintable(QString("Phase 2 Y jump > gridStep at step %1: prev=%2, new=%3")
                                .arg(i)
                                .arg(prevScenePos.y())
                                .arg(newScenePos.y())));

        // Containment: child within parent
        QRectF childSceneRect = fixture.child->mapRectToScene(fixture.child->elementRect());
        QRectF parentSceneRect = fixture.parent->mapRectToScene(fixture.parent->elementRect());
        QVERIFY2(parentSceneRect.contains(childSceneRect),
                 qPrintable(QString("Phase 2: child not in parent at step %1").arg(i)));

        prevScenePos = newScenePos;
    }

    QRectF oldChildLocalRect = fixture.child->elementRect();
    QRectF oldParentLocalRect = fixture.parent->elementRect();
    QRectF oldGrandparentLocalRect = fixture.grandparent->elementRect();

    DragTestHelper::endDrag(fixture.child);

    // Validate how rects were normalized after finishing dragging. Expected:
    // - boundaries respected
    // - X,Y normalized to start from 0
    // - width/height didn't change
    QRectF newChildSceneRect = fixture.child->mapRectToScene(fixture.child->elementRect());
    QRectF newParentSceneRect = fixture.parent->mapRectToScene(fixture.parent->elementRect());
    QRectF newGrandparentSceneRect = fixture.grandparent->mapRectToScene(fixture.grandparent->elementRect());

    compareNormalized("child", oldChildLocalRect, fixture.child->elementRect());
    compareNormalized("parent", oldParentLocalRect, fixture.parent->elementRect());
    compareNormalized("grandparent", oldGrandparentLocalRect, fixture.grandparent->elementRect());

    QVERIFY2(newParentSceneRect.contains(newChildSceneRect), "Child is outside parent's boundary");
    QVERIFY2(newGrandparentSceneRect.contains(newParentSceneRect), "Parent is outside grandparent's boundary");
}

// ---------------------------------------------------------------------------
// 2.10 — Reparent child from one parent to another via Ctrl+R
// Scenario:
//   - Snap to grid disabled
//   - Two top-level states A and B
//   - One child state C inside A
//   - Part 1: drag C leftward WITHOUT reparent modifier — A should resize
//   - Part 2: drag C from A toward B WITH reparent modifier (Ctrl+R) pressed
//     — C becomes a child of B, A size is unchanged
// Verifies: without reparent modifier dragging expands parent; with reparent
//           modifier the element is reparented, parent A preserves its size,
//           and new parent B resizes to fit the child.
// ---------------------------------------------------------------------------
void HsmElementDragTest::reparentChildFromOneParentToAnother() {
    QGraphicsScene scene;
    ReparentEnabledView view;

    view.setScene(&scene);
    view.setSnapToGridEnabled(false);

    // Create model entities
    auto modelA = QSharedPointer<model::RegularState>::create("stateA");
    auto modelB = QSharedPointer<model::RegularState>::create("stateB");
    auto modelC = QSharedPointer<model::RegularState>::create("stateC");

    // Create two top-level states (A on the left, B on the right) and child C inside A
    view::HsmElement* stateA = view.createHsmElement(modelA, "state", QPointF(0, 0), QSizeF(300, 200), model::INVALID_MODEL_ID);
    view::HsmElement* stateB = view.createHsmElement(modelB, "state", QPointF(400, 0), QSizeF(300, 200), model::INVALID_MODEL_ID);
    view::HsmElement* stateC = view.createHsmElement(modelC, "state", QPointF(20, 20), QSizeF(100, 60), modelA->id());

    // Verify initial state: C is a child of A
    QVERIFY(stateC->hsmParentItem() == stateA);
    QVERIFY(stateA->containsChild(stateC));
    QVERIFY(!stateB->containsChild(stateC));

    // =====================================================================
    // Part 1: Drag C leftward WITHOUT reparent modifier — A should resize
    // =====================================================================
    const QSizeF stateASizeBeforeInternalDrag = stateA->elementRect().size();

    DragTestHelper::beginDrag(stateC);
    // No reparent modifier — dragElementBegin will call setDragMode(false) for internal moves
    view.setReparentModifierPressed(false);

    // Move C far to the left to force parent A to expand
    DragTestHelper::simulateDragMove(stateC, QPointF(-150, 0));
    DragTestHelper::endDrag(stateC);

    // Verify A expanded (its width should have increased)
    const QSizeF stateASizeAfterInternalDrag = stateA->elementRect().size();
    QVERIFY2(stateASizeAfterInternalDrag.width() > stateASizeBeforeInternalDrag.width(),
             qPrintable(QString("stateA should have expanded: width before=%1, after=%2")
                            .arg(stateASizeBeforeInternalDrag.width())
                            .arg(stateASizeAfterInternalDrag.width())));

    // Normalize A after internal drag (as the app does on drop)
    dynamic_cast<view::HsmResizableElement*>(stateA)->normalizeElementRect();

    // =====================================================================
    // Part 2: Drag C from A to B WITH reparent modifier (Ctrl+R)
    // =====================================================================
    const QSizeF stateASizeBeforeReparent = stateA->elementRect().size();
    const QPointF childScenePosBeforeDrag = stateC->scenePos();

    DragTestHelper::beginDrag(stateC);

    // Simulate pressing Ctrl+R (reparent modifier) before the first move,
    // so dragElementBegin will call setDragMode(true) for reparenting
    view.setReparentModifierPressed(true);

    // Move C toward state B (move rightward)
    const QPointF targetScenePos = stateB->scenePos() + QPointF(50, 50);
    DragTestHelper::simulateDragMove(stateC, targetScenePos - childScenePosBeforeDrag);

    const QPointF childScenePosAfterDrag = stateC->scenePos();

    // End the drag
    DragTestHelper::endDrag(stateC);

    // --- Reparent C from A to B (what moveHsmElement does on drop in ProjectController) ---
    // TODO: can use stub ProjectController
    view.moveHsmElement(modelC->id(), modelB->id());

    // --- Verify reparenting results ---
    // C is now a child of B
    QVERIFY2(stateC->hsmParentItem() == stateB,
             "stateC should be reparented to stateB");
    QVERIFY2(stateB->containsChild(stateC),
             "stateB should contain stateC as a child");
    QVERIFY2(!stateA->containsChild(stateC),
             "stateA should no longer contain stateC");

    // A should NOT have changed size during reparent drag
    const QSizeF stateASizeAfterReparent = stateA->elementRect().size();
    QVERIFY2(stateASizeAfterReparent == stateASizeBeforeReparent,
             qPrintable(QString("stateA size should be unchanged after reparent: "
                                "before=(%1,%2) after=(%3,%4)")
                            .arg(stateASizeBeforeReparent.width())
                            .arg(stateASizeBeforeReparent.height())
                            .arg(stateASizeAfterReparent.width())
                            .arg(stateASizeAfterReparent.height())));

    // C's scene position should be close to where it was dropped
    const QPointF childScenePosAfterReparent = stateC->scenePos();
    const qreal positionTolerance = 1.0;
    QVERIFY2(std::abs(childScenePosAfterReparent.x() - childScenePosAfterDrag.x()) <= positionTolerance &&
                 std::abs(childScenePosAfterReparent.y() - childScenePosAfterDrag.y()) <= positionTolerance,
             qPrintable(QString("Child scene pos after reparent (%1, %2) differs from drop pos (%3, %4)")
                            .arg(childScenePosAfterReparent.x())
                            .arg(childScenePosAfterReparent.y())
                            .arg(childScenePosAfterDrag.x())
                            .arg(childScenePosAfterDrag.y())));

    // B should have resized to contain C
    QRectF childSceneRect = stateC->mapRectToScene(stateC->elementRect());
    QRectF parentBSceneRect = stateB->mapRectToScene(stateB->elementRect());
    QVERIFY2(parentBSceneRect.contains(childSceneRect),
             qPrintable(QString("stateB should contain stateC after reparenting: "
                                "child=(%1,%2,%3,%4) parentB=(%5,%6,%7,%8)")
                            .arg(childSceneRect.x())
                            .arg(childSceneRect.y())
                            .arg(childSceneRect.width())
                            .arg(childSceneRect.height())
                            .arg(parentBSceneRect.x())
                            .arg(parentBSceneRect.y())
                            .arg(parentBSceneRect.width())
                            .arg(parentBSceneRect.height())));
}

// ---------------------------------------------------------------------------
// 2.11 — Move child onto sibling within the same parent (no reparent modifier)
// Scenario:
//   - Snap to grid disabled
//   - One parent state P with two children: C1 and C2 (siblings)
//   - Drag C1 onto C2 WITHOUT reparent modifier (Ctrl+R)
//     — Expected: C1 becomes a child of C2 (reparented to sibling)
//     — Actual (bug): C1 stays as a child of P (no reparent happens)
// Verifies: dragging a child element onto a sibling of the same parent
//           (without special modifier) triggers reparenting to that sibling.
// ---------------------------------------------------------------------------
void HsmElementDragTest::reparentChildToSiblingWithinSameParent() {
    QGraphicsScene scene;
    ReparentEnabledView view;
    view.setScene(&scene);
    view.setSnapToGridEnabled(false);

    auto controller = QSharedPointer<StubProjectController>::create(&view);
    view.setProjectController(controller.toWeakRef());

    // Create model entities
    auto modelP = QSharedPointer<model::RegularState>::create("parent");
    auto modelC1 = QSharedPointer<model::RegularState>::create("childOne");
    auto modelC2 = QSharedPointer<model::RegularState>::create("childTwo");

    // Create parent P and two children: C1 (left side) and C2 (right side, larger)
    // C2 is placed far enough right that the grab offset (50px) will cause
    // the element position to NOT land on C2 even when cursor IS on C2.
    view::HsmElement* stateP = view.createHsmElement(modelP, "state", QPointF(0, 0), QSizeF(600, 300), model::INVALID_MODEL_ID);
    view::HsmElement* stateC1 = view.createHsmElement(modelC1, "state", QPointF(10, 20), QSizeF(100, 60), modelP->id());
    view::HsmElement* stateC2 = view.createHsmElement(modelC2, "state", QPointF(300, 20), QSizeF(200, 150), modelP->id());

    // Verify initial state: C1 and C2 are both direct children of P
    QVERIFY(stateC1->hsmParentItem() == stateP);
    QVERIFY(stateC2->hsmParentItem() == stateP);
    QVERIFY(stateP->containsChild(stateC1));
    QVERIFY(stateP->containsChild(stateC2));
    QVERIFY(!stateC2->containsChild(stateC1));

    // =====================================================================
    // Step 1: Move C1 leftward past P's boundary to trigger parent
    // expansion with body section suppression, then move onto C2.
    // This reproduces the real UI scenario where body section coordinate
    // shifts corrupt the cursor-to-scene mapping.
    // =====================================================================
    const QPointF c1ScenePosBeforeDrag = stateC1->scenePos();

    // Select C1 (simulates real UI where clicking to drag also selects)
    stateC1->setSelected(true);

    // Use real QGraphicsSceneMouseEvents to reproduce the actual UI behavior
    // Grab C1 near its center — this creates a significant offset between cursor
    // and element position, which is what causes P1 to highlight instead of C2.
    const QPointF pressScenePos = stateC1->scenePos() + QPointF(50, 30);
    {
        QGraphicsSceneMouseEvent pressEvent(QEvent::GraphicsSceneMousePress);
        pressEvent.setButton(Qt::LeftButton);
        pressEvent.setButtons(Qt::LeftButton);
        pressEvent.setScenePos(pressScenePos);
        pressEvent.setPos(stateC1->mapFromScene(pressScenePos));
        scene.sendEvent(stateC1, &pressEvent);
    }

    // No reparent modifier pressed — regular drag within parent
    view.setReparentModifierPressed(false);

    // Phase 1: Move C1 leftward to trigger parent expansion (body section suppression)
    QPointF lastScenePos = pressScenePos;
    const int leftSteps = 30;
    // The buttonDownPos must be set on move events for Qt's mouseMoveEvent to correctly
    // compute the grab offset (this is what makes element position != cursor position)
    const QPointF buttonDownLocalPos = stateC1->mapFromScene(pressScenePos);

    for (int i = 0; i < leftSteps; ++i) {
        QPointF moveScenePos = lastScenePos + QPointF(-3, 0);

        QGraphicsSceneMouseEvent moveEvent(QEvent::GraphicsSceneMouseMove);
        moveEvent.setButton(Qt::NoButton);
        moveEvent.setButtons(Qt::LeftButton);
        moveEvent.setScenePos(moveScenePos);
        moveEvent.setLastScenePos(lastScenePos);
        moveEvent.setPos(stateC1->mapFromScene(moveScenePos));
        moveEvent.setLastPos(stateC1->mapFromScene(lastScenePos));
        moveEvent.setButtonDownPos(Qt::LeftButton, buttonDownLocalPos);
        moveEvent.setButtonDownScenePos(Qt::LeftButton, pressScenePos);
        scene.sendEvent(stateC1, &moveEvent);

        lastScenePos = moveScenePos;
    }

    // Phase 2: Move cursor so it's over C2's area, but the element's
    // top-left (mDragCursorScenePos) lands OUTSIDE C2 due to grab offset.
    // This reproduces the real UI bug: highlight uses element position, not cursor.
    //
    // Grab offset is (50, 30). C2's left edge is at ~300 in scene.
    // If cursor is at C2.left + 30 = 330 (inside C2),
    // the element's top-left will be at 330 - 50 = 280 (outside C2).
    // So handleElementDragEvent receives position 280 which is on P1, not C2.
    const QRectF c2SceneRect = stateC2->mapRectToScene(stateC2->elementRect());
    // Target cursor at X = C2 left edge + 30 (inside C2 but grab offset puts element outside)
    const QPointF insideC2Target = QPointF(c2SceneRect.left() + 30, c2SceneRect.center().y());
    const QPointF rightDelta = insideC2Target - lastScenePos;
    const int rightSteps = 10;
    const QPointF rightStepDelta = rightDelta / rightSteps;

    for (int i = 0; i < rightSteps; ++i) {
        QPointF moveScenePos = lastScenePos + rightStepDelta;

        QGraphicsSceneMouseEvent moveEvent(QEvent::GraphicsSceneMouseMove);
        moveEvent.setButton(Qt::NoButton);
        moveEvent.setButtons(Qt::LeftButton);
        moveEvent.setScenePos(moveScenePos);
        moveEvent.setLastScenePos(lastScenePos);
        moveEvent.setPos(stateC1->mapFromScene(moveScenePos));
        moveEvent.setLastPos(stateC1->mapFromScene(lastScenePos));
        moveEvent.setButtonDownPos(Qt::LeftButton, buttonDownLocalPos);
        moveEvent.setButtonDownScenePos(Qt::LeftButton, pressScenePos);
        scene.sendEvent(stateC1, &moveEvent);

        lastScenePos = moveScenePos;
    }

    // Verify: cursor is now over C2 (the scene rect we computed above)
    // C2 should be highlighted, NOT P1
    QVERIFY2(stateC2->isHighligted(),
             qPrintable(QString("stateC2 should be highlighted when cursor is over it. "
                                "C2 scene rect=(%1,%2,%3,%4), cursor at (%5,%6), P1 highlighted=%7")
                            .arg(c2SceneRect.x())
                            .arg(c2SceneRect.y())
                            .arg(c2SceneRect.width())
                            .arg(c2SceneRect.height())
                            .arg(lastScenePos.x())
                            .arg(lastScenePos.y())
                            .arg(stateP->isHighligted())));
    QVERIFY2(!stateP->isHighligted(),
             "stateP should NOT be highlighted when cursor is over C2 (a child of P)");

    // End drag with release event
    {
        QGraphicsSceneMouseEvent releaseEvent(QEvent::GraphicsSceneMouseRelease);
        releaseEvent.setButton(Qt::LeftButton);
        releaseEvent.setButtons(Qt::NoButton);
        releaseEvent.setScenePos(lastScenePos);
        releaseEvent.setPos(stateC1->mapFromScene(lastScenePos));
        scene.sendEvent(stateC1, &releaseEvent);
    }
    DragTestHelper::dragCursorPositions().remove(stateC1);

    // --- Verify: C1 should now be reparented to C2 ---
    // The drop landed on C2, so the view should have triggered
    // handleViewMoveEvent(C1.id, C2.id) to reparent C1 under C2.
    QVERIFY2(stateC1->hsmParentItem() == stateC2,
             qPrintable(QString("stateC1 should be reparented to stateC2 (sibling). "
                                "Actual parent: %1 (expected C2=%2, P=%3)")
                            .arg(stateC1->hsmParentItem() == stateP ? "P (unchanged - BUG)" : "other")
                            .arg(reinterpret_cast<quintptr>(stateC2), 0, 16)
                            .arg(reinterpret_cast<quintptr>(stateP), 0, 16)));
    QVERIFY2(stateC2->containsChild(stateC1),
             "stateC2 should contain stateC1 as a child after drop");
    QVERIFY2(!stateP->isDirectChild(stateC1),
             "stateP should no longer have stateC1 as a direct child");
}

// ---------------------------------------------------------------------------
// Bug Condition Exploration: Z-value raised to 100 during active drag
// Validates: Requirements 1.1, 1.2, 2.1, 2.2
// Property 1: Bug Condition - Dragged Element Z-Value Not Raised
// EXPECTED TO FAIL on unfixed code (confirms bug exists)
// ---------------------------------------------------------------------------
void HsmElementDragTest::dragZValueRaisedDuringDrag() {
    QGraphicsScene scene;
    HsmGraphicsView view;
    view.setScene(&scene);

    auto model = QSharedPointer<model::RegularState>::create("testState");
    view::HsmElement* element =
        view.createHsmElement(model, "state", QPointF(100, 100), QSizeF(200, 100), model::INVALID_MODEL_ID);

    // Verify initial z-value is 3
    QCOMPARE(element->zValue(), 3.0);

    // Begin drag — puts element in PREPARE state with SINGLE mode
    DragTestHelper::beginDrag(element);

    // First simulateDragMove triggers PREPARE->DRAGGING transition
    DragTestHelper::simulateDragMove(element, QPointF(1, 0));

    // Bug condition: z-value should be raised to 100 during active drag
    // On unfixed code this will be 3 (BUG) — test expected to FAIL
    QVERIFY2(element->zValue() == 100.0,
             qPrintable(QString("Bug condition confirmed: z-value during drag is %1 (expected 100). "
                                "Element renders behind transitions (z=5) and grip items (z=11).")
                            .arg(element->zValue())));

    DragTestHelper::endDrag(element);
}

// ---------------------------------------------------------------------------
// Bug Condition Exploration: Z-value restored to original after drop
// Validates: Requirements 2.1, 2.2
// Property 1: Bug Condition - Z-value restoration after drag ends
// EXPECTED TO FAIL on unfixed code (since z-value is never raised, restoration is N/A,
// but we test the full expected behavior cycle)
// ---------------------------------------------------------------------------
void HsmElementDragTest::dragZValueRestoredAfterDrop() {
    QGraphicsScene scene;
    HsmGraphicsView view;
    view.setScene(&scene);

    auto model = QSharedPointer<model::RegularState>::create("testState");
    view::HsmElement* element =
        view.createHsmElement(model, "state", QPointF(100, 100), QSizeF(200, 100), model::INVALID_MODEL_ID);

    const qreal originalZValue = element->zValue();
    QCOMPARE(originalZValue, 3.0);

    // Perform a complete drag cycle: begin, move, end
    DragTestHelper::beginDrag(element);
    DragTestHelper::simulateDragMove(element, QPointF(10, 0));

    // During drag, z-value should be 100 (bug condition — will fail on unfixed code)
    QVERIFY2(element->zValue() == 100.0,
             qPrintable(QString("Bug condition confirmed: z-value during drag is %1 (expected 100).")
                            .arg(element->zValue())));

    DragTestHelper::endDrag(element);

    // After drop, z-value should be restored to original
    QVERIFY2(element->zValue() == originalZValue,
             qPrintable(QString("Z-value after drop is %1 (expected %2).")
                            .arg(element->zValue())
                            .arg(originalZValue)));
}

// ---------------------------------------------------------------------------
// Preservation Property: Z-value at rest is always 3
// Validates: Requirements 3.1
// Property 2: Preservation - Non-Drag Z-Value Unchanged
// Elements that are not being dragged should always have z-value == 3
// ---------------------------------------------------------------------------
void HsmElementDragTest::preservationZValueAtRest() {
    QGraphicsScene scene;
    HsmGraphicsView view;
    view.setScene(&scene);

    // Create multiple elements at various positions
    auto model1 = QSharedPointer<model::RegularState>::create("state1");
    auto model2 = QSharedPointer<model::RegularState>::create("state2");
    auto model3 = QSharedPointer<model::RegularState>::create("state3");

    view::HsmElement* element1 =
        view.createHsmElement(model1, "state", QPointF(0, 0), QSizeF(200, 100), model::INVALID_MODEL_ID);
    view::HsmElement* element2 =
        view.createHsmElement(model2, "state", QPointF(250, 0), QSizeF(200, 100), model::INVALID_MODEL_ID);
    view::HsmElement* element3 =
        view.createHsmElement(model3, "state", QPointF(500, 0), QSizeF(200, 100), model::INVALID_MODEL_ID);

    // All elements at rest should have z-value == 3
    QCOMPARE(element1->zValue(), 3.0);
    QCOMPARE(element2->zValue(), 3.0);
    QCOMPARE(element3->zValue(), 3.0);

    // Select elements — selection should not affect z-value
    element1->setSelected(true);
    QCOMPARE(element1->zValue(), 3.0);

    element2->setSelected(true);
    QCOMPARE(element2->zValue(), 3.0);

    // Deselect — z-value still unchanged
    element1->setSelected(false);
    QCOMPARE(element1->zValue(), 3.0);
}

// ---------------------------------------------------------------------------
// Preservation Property: Click without drag leaves z-value unchanged
// Validates: Requirements 3.2
// Property 2: Preservation - Non-Drag Z-Value Unchanged
// Press + release without movement should never modify z-value
// ---------------------------------------------------------------------------
void HsmElementDragTest::preservationZValueClickWithoutDrag() {
    QGraphicsScene scene;
    HsmGraphicsView view;
    view.setScene(&scene);

    auto model = QSharedPointer<model::RegularState>::create("testState");
    view::HsmElement* element =
        view.createHsmElement(model, "state", QPointF(100, 100), QSizeF(200, 100), model::INVALID_MODEL_ID);

    const qreal originalZValue = element->zValue();
    QCOMPARE(originalZValue, 3.0);

    // Simulate multiple click-without-drag cycles using random positions on the element
    std::mt19937 rng(123);
    std::uniform_real_distribution<qreal> distX(0.0, 200.0);
    std::uniform_real_distribution<qreal> distY(0.0, 100.0);

    for (int i = 0; i < 20; ++i) {
        const QPointF clickLocalPos(distX(rng), distY(rng));
        const QPointF clickScenePos = element->mapToScene(clickLocalPos);

        // Press event
        QGraphicsSceneMouseEvent pressEvent(QEvent::GraphicsSceneMousePress);
        pressEvent.setButton(Qt::LeftButton);
        pressEvent.setButtons(Qt::LeftButton);
        pressEvent.setScenePos(clickScenePos);
        pressEvent.setPos(clickLocalPos);
        scene.sendEvent(element, &pressEvent);

        // Immediately release without any movement
        QGraphicsSceneMouseEvent releaseEvent(QEvent::GraphicsSceneMouseRelease);
        releaseEvent.setButton(Qt::LeftButton);
        releaseEvent.setButtons(Qt::NoButton);
        releaseEvent.setScenePos(clickScenePos);
        releaseEvent.setPos(clickLocalPos);
        scene.sendEvent(element, &releaseEvent);

        // Z-value must remain unchanged after click-without-drag
        QVERIFY2(element->zValue() == originalZValue,
                 qPrintable(QString("Click %1: z-value changed from %2 to %3 after click-without-drag at (%4, %5)")
                                .arg(i)
                                .arg(originalZValue)
                                .arg(element->zValue())
                                .arg(clickScenePos.x())
                                .arg(clickScenePos.y())));
    }
}

// ---------------------------------------------------------------------------
// Preservation Property: Z-value restored after full drag+release cycle
// Validates: Requirements 3.3
// Property 2: Preservation - Non-Drag Z-Value Unchanged
// After a complete drag operation (begin, move, end), z-value returns to original
// On unfixed code: z-value is never raised so it trivially stays at 3 throughout
// ---------------------------------------------------------------------------
void HsmElementDragTest::preservationZValueAfterDragCycle() {
    QGraphicsScene scene;
    HsmGraphicsView view;
    view.setScene(&scene);

    auto model = QSharedPointer<model::RegularState>::create("testState");
    view::HsmElement* element =
        view.createHsmElement(model, "state", QPointF(100, 100), QSizeF(200, 100), model::INVALID_MODEL_ID);

    const qreal originalZValue = element->zValue();
    QCOMPARE(originalZValue, 3.0);

    // Perform multiple drag cycles with varying displacements
    std::mt19937 rng(456);
    std::uniform_real_distribution<qreal> distDelta(-50.0, 50.0);

    for (int i = 0; i < 10; ++i) {
        const QPointF delta(distDelta(rng), distDelta(rng));

        DragTestHelper::beginDrag(element);
        DragTestHelper::simulateDragMove(element, delta);
        DragTestHelper::endDrag(element);

        // After each complete drag cycle, z-value must be back to original
        QVERIFY2(element->zValue() == originalZValue,
                 qPrintable(QString("Drag cycle %1: z-value after drop is %2 (expected %3). "
                                    "Delta was (%4, %5)")
                                .arg(i)
                                .arg(element->zValue())
                                .arg(originalZValue)
                                .arg(delta.x())
                                .arg(delta.y())));
    }
}

// ---------------------------------------------------------------------------
// Preservation Property: Random interaction sequences preserve z-value
// Validates: Requirements 3.1, 3.2, 3.3, 3.4
// Property 2: Preservation - Non-Drag Z-Value Unchanged
// For random sequences of clicks, selections, and complete drag cycles,
// the element's z-value is always == originalZValue when not actively dragged
// ---------------------------------------------------------------------------
void HsmElementDragTest::preservationZValueRandomInteractions() {
    QGraphicsScene scene;
    HsmGraphicsView view;
    view.setScene(&scene);

    auto model = QSharedPointer<model::RegularState>::create("testState");
    view::HsmElement* element =
        view.createHsmElement(model, "state", QPointF(100, 100), QSizeF(200, 100), model::INVALID_MODEL_ID);

    const qreal originalZValue = element->zValue();
    QCOMPARE(originalZValue, 3.0);

    // Random interaction types:
    // 0 = click-without-drag (press + release, no movement)
    // 1 = select/deselect toggle
    // 2 = full drag cycle (begin, move, end)
    // 3 = double-click simulation (two presses and releases)
    std::mt19937 rng(789);
    std::uniform_int_distribution<int> actionDist(0, 3);
    std::uniform_real_distribution<qreal> posDist(-50.0, 50.0);

    for (int i = 0; i < 50; ++i) {
        const int action = actionDist(rng);

        switch (action) {
            case 0: {
                // Click-without-drag
                const QPointF clickScenePos = element->scenePos() + QPointF(50, 30);

                QGraphicsSceneMouseEvent pressEvent(QEvent::GraphicsSceneMousePress);
                pressEvent.setButton(Qt::LeftButton);
                pressEvent.setButtons(Qt::LeftButton);
                pressEvent.setScenePos(clickScenePos);
                pressEvent.setPos(element->mapFromScene(clickScenePos));
                scene.sendEvent(element, &pressEvent);

                QGraphicsSceneMouseEvent releaseEvent(QEvent::GraphicsSceneMouseRelease);
                releaseEvent.setButton(Qt::LeftButton);
                releaseEvent.setButtons(Qt::NoButton);
                releaseEvent.setScenePos(clickScenePos);
                releaseEvent.setPos(element->mapFromScene(clickScenePos));
                scene.sendEvent(element, &releaseEvent);
                break;
            }
            case 1: {
                // Toggle selection
                element->setSelected(!element->isSelected());
                break;
            }
            case 2: {
                // Full drag cycle
                const QPointF delta(posDist(rng), posDist(rng));
                DragTestHelper::beginDrag(element);
                DragTestHelper::simulateDragMove(element, delta);
                DragTestHelper::endDrag(element);
                break;
            }
            case 3: {
                // Double-click simulation (two quick press+release)
                const QPointF clickScenePos = element->scenePos() + QPointF(50, 30);

                for (int click = 0; click < 2; ++click) {
                    QGraphicsSceneMouseEvent pressEvent(QEvent::GraphicsSceneMousePress);
                    pressEvent.setButton(Qt::LeftButton);
                    pressEvent.setButtons(Qt::LeftButton);
                    pressEvent.setScenePos(clickScenePos);
                    pressEvent.setPos(element->mapFromScene(clickScenePos));
                    scene.sendEvent(element, &pressEvent);

                    QGraphicsSceneMouseEvent releaseEvent(QEvent::GraphicsSceneMouseRelease);
                    releaseEvent.setButton(Qt::LeftButton);
                    releaseEvent.setButtons(Qt::NoButton);
                    releaseEvent.setScenePos(clickScenePos);
                    releaseEvent.setPos(element->mapFromScene(clickScenePos));
                    scene.sendEvent(element, &releaseEvent);
                }
                break;
            }
        }

        // After every interaction, z-value must be at original when not actively in drag
        QVERIFY2(element->zValue() == originalZValue,
                 qPrintable(QString("Interaction %1 (action=%2): z-value is %3 (expected %4)")
                                .arg(i)
                                .arg(action)
                                .arg(element->zValue())
                                .arg(originalZValue)));
    }
}

void HsmElementDragTest::compareNormalized(const QString& elementName, const QRectF& oldElementRect, const QRectF& newElementRect) {
    QVERIFY2(oldElementRect.size() == newElementRect.size(), qPrintable(QString("Element <%1> changed size").arg(elementName)));
    QVERIFY2(newElementRect.topLeft() == QPointF(0, 0), qPrintable(QString("Element <%1> X,Y not normalized to 0,0 position").arg(elementName)));
}

int runHsmElementDragTest(int argc, char** argv) {
    QApplication app(argc, argv);
    HsmElementDragTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "HsmElementDragTest.moc"
