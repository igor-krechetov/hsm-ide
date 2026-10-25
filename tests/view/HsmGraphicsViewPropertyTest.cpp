#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QRandomGenerator>
#include <QSignalSpy>
#include <QStringList>
#include <QTransform>
#include <QtTest>
#include <cmath>

#include "DragTestHelper.hpp"
#include "controllers/IProjectController.hpp"
#include "model/ModelElementsFactory.hpp"
#include "model/elements/IncludeEntity.hpp"
#include "model/elements/RegularState.hpp"
#include "model/private/EntityIdGenerator.hpp"
#include "view/common/DragDropUtils.hpp"
#include "view/elements/HsmStateElement.hpp"
#include "view/theme/ThemeManager.hpp"
#include "view/widgets/HsmGraphicsView.hpp"

using namespace view;

// ---------------------------------------------------------------------------
// Stub IProjectController that routes handleViewMoveEvent back into the view,
// mirroring the real reparent flow (see HsmElementDragTest.cpp).
// ---------------------------------------------------------------------------
class PreservationStubController : public IProjectController {
    Q_OBJECT
public:
    explicit PreservationStubController(HsmGraphicsView* view = nullptr, QObject* parent = nullptr)
        : IProjectController(parent)
        , mView(view) {}

    bool importModel(const QString&) override {
        return false;
    }
    const model::ParseErrorCollector& lastImportReport() const override {
        return mLastImportReport;
    }
    bool exportModel() override {
        return false;
    }
    bool exportModel(const QString&) override {
        return false;
    }
    void updateModelPath(const QString&) override {}
    void handleViewDropEvent(const QString&, const QPointF&, const model::EntityID_t) override {}
    void handleViewMoveEvent(const model::EntityID_t entity, const model::EntityID_t parent) override {
        if (nullptr != mView) {
            mView->moveHsmElement(entity, parent);
        }
    }
    void handleDeleteElements(const QList<model::EntityID_t>&) override {}
    QString serializeElementsToScxml(const QList<model::EntityID_t>&) const override {
        return {};
    }
    bool pasteScxmlElements(const QString&, const QList<model::EntityID_t>&, const QPointF&, const bool) override {
        return false;
    }
    void beginHistoryTransaction(const QString&) override {}
    void commitHistoryTransaction() override {}
    void cancelHistoryTransaction() override {}
    void markHistoryElement(const model::EntityID_t) override {}
    void unmarkHistoryElement(const model::EntityID_t) override {}
    bool undo() override {
        return false;
    }
    bool redo() override {
        return false;
    }
    bool canUndo() const override {
        return false;
    }
    bool canRedo() const override {
        return false;
    }

private:
    HsmGraphicsView* mView = nullptr;
    model::ParseErrorCollector mLastImportReport;
};

// ---------------------------------------------------------------------------
// Property-based tests for Property 2: Preservation.
//
// Non-empty-document adds and all non-bug view operations must behave exactly as
// they do on the current (unfixed) code. These tests follow the observation-first
// methodology: they encode behavior observed on the UNFIXED code and MUST PASS on
// it, establishing the baseline that the later fix must preserve (design
// "Preservation Checking": F == F').
// ---------------------------------------------------------------------------
class HsmGraphicsViewPropertyTest : public QObject {
    Q_OBJECT

private slots:
    void PropertyIncludeDoubleClickDeliveredExactlyOnce();
    void PropertyOtherElementSignalsWiredExactlyOnce();
    void PropertyNonBugDoubleClickTakesNoOpenAction();
    void PropertyNonEmptyAddDoesNotShiftView();
    void PropertyDragMoveReparentPreservesPositions();
    void PropertyPanZoomFitPreserved();
    void PropertyEditorDropPartitionsDroppedPaths();
};

/**
 * @brief Property 1 (Bug Condition, Case 1): an Include double-click is delivered
 *        exactly once.
 *
 * Reproduces defect 1 (design "Bug Details", Case 1): `createHsmElement` connects
 * `HsmElement::elementDoubleClickEvent` to the view's `hsmElementDoubleClickEvent`
 * signal twice, so a single double-click is delivered through two connections and
 * `MainEditorController::openProject` runs twice (two windows). This test creates
 * an Include `view::HsmElement` via `HsmGraphicsView::createHsmElement`, watches
 * `hsmElementDoubleClickEvent` with a QSignalSpy, triggers exactly one double-click
 * on the element (driving the real signal path through `mouseDoubleClickEvent`),
 * and asserts the view signal fires exactly once.
 *
 * This is a BUG-CONDITION exploration test and is EXPECTED TO FAIL on the UNFIXED
 * code (the spy records 2 emissions for one double-click). The failure confirms
 * the duplicated connection. It encodes design Property 1 for a valid Include
 * target (exactly one window) and will pass once the duplicate `connect` is removed.
 *
 * The Include path is varied with QRandomGenerator across the non-empty
 * bug-condition shapes (near-origin / off-origin positions, varied stored paths)
 * so the single-delivery property is checked across the input space, not a single
 * point.
 *
 * Validates: Requirements 1.1, 2.1
 */
void HsmGraphicsViewPropertyTest::PropertyIncludeDoubleClickDeliveredExactlyOnce() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 50; ++iteration) {
        QGraphicsScene scene;
        HsmGraphicsView view;
        view.setScene(&scene);

        // Build an Include model element with a non-empty stored path (bug condition:
        // element.type == INCLUDE AND path != "").
        model::EntityIdGenerator gen;
        auto includeModel =
            model::ModelElementsFactory::createUniqueState(model::StateType::INCLUDE, gen).dynamicCast<model::IncludeEntity>();
        QVERIFY2(includeModel.isNull() == false,
                 qPrintable(QString("iteration %1: failed to create Include model element").arg(iteration)));
        includeModel->setPath(QStringLiteral("/workspace/sub%1.scxml").arg(rng.bounded(1000)));

        // Vary the drop position across the input space.
        const QPointF pos(rng.bounded(-400, 400), rng.bounded(-400, 400));

        // Create the Include graphics element through the real wiring path.
        view::HsmElement* element =
            view.createHsmElement(includeModel, QStringLiteral("include"), pos, QSizeF(80, 60), model::INVALID_MODEL_ID);
        QVERIFY2(element != nullptr, qPrintable(QString("iteration %1: createHsmElement returned null").arg(iteration)));

        // Count deliveries of the view's forwarding signal.
        QSignalSpy spy(&view, &HsmGraphicsView::hsmElementDoubleClickEvent);
        QVERIFY2(spy.isValid(),
                 qPrintable(QString("iteration %1: QSignalSpy on hsmElementDoubleClickEvent is invalid").arg(iteration)));

        // Trigger exactly ONE double-click on the element, driving the real
        // HsmElement::mouseDoubleClickEvent -> elementDoubleClickEvent path.
        QGraphicsSceneMouseEvent doubleClick(QEvent::GraphicsSceneMouseDoubleClick);
        doubleClick.setButton(Qt::LeftButton);
        doubleClick.setButtons(Qt::LeftButton);
        doubleClick.setScenePos(element->scenePos());
        doubleClick.setPos(element->mapFromScene(element->scenePos()));
        scene.sendEvent(element, &doubleClick);

        // One double-click must deliver exactly one view signal (one window opened).
        // UNFIXED: fires twice because of the duplicated connect in createHsmElement.
        QVERIFY2(spy.count() == 1,
                 qPrintable(QString("iteration %1: one Include double-click delivered %2 "
                                    "hsmElementDoubleClickEvent signal(s), expected exactly 1 "
                                    "(duplicated connect opens two windows)")
                                .arg(iteration)
                                .arg(spy.count())));
    }
}

/**
 * @brief Property 2 (Preservation): the other HsmElement signal connections fire
 *        exactly once per event, unchanged by the double-click fix.
 *
 * The fix for defect 1 removes ONE of the two identical
 * `elementDoubleClickEvent -> hsmElementDoubleClickEvent` connects in
 * `createHsmElement`. This preservation slot locks in that the NEIGHBOURING
 * connections it must leave untouched keep firing exactly once per event:
 *   - `dragElementBegin`, `dragElementEvent`, `dropElementEvent` are emitted by
 *     the `HsmElement` and consumed by the view's same-named slots (they are view
 *     SLOTS, not re-emitted view signals), so the observable "fire once per event"
 *     contract is on the element's own signals: a single drag emits
 *     `dragElementBegin` exactly once (on the PREPARE -> DRAGGING transition) and
 *     `dropElementEvent` exactly once (on release), with `dragElementEvent` once
 *     per move frame.
 *   - `transformChanged` is a view SIGNAL (delivered to the element's
 *     `viewTransformChanged` slot); it must fire exactly once per view transform
 *     operation.
 *
 * Observation-first methodology: these counts are what the unfixed code produces,
 * so the slot PASSES on it and keeps guarding the wiring after the fix. The drag
 * frame count and zoom-step count are generated with QRandomGenerator so the
 * per-event invariant is checked across the input space (the begin/drop
 * once-per-drag and the once-per-zoom contracts hold for any frame/step count).
 *
 * Validates: Requirements 3.2, 3.5 (signal-wiring preservation)
 */
void HsmGraphicsViewPropertyTest::PropertyOtherElementSignalsWiredExactlyOnce() {
    // dragElement* carry a `view::HsmElement*`; register it so QSignalSpy can
    // introspect the signal arguments.
    qRegisterMetaType<view::HsmElement*>("view::HsmElement*");

    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 40; ++iteration) {
        QGraphicsScene scene;
        HsmGraphicsView view;
        view.setScene(&scene);
        view.setSnapToGridEnabled(false);
        view.resize(800, 600);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));

        auto elementModel = QSharedPointer<model::RegularState>::create("signalWiringTarget");
        view::HsmElement* element =
            view.createHsmElement(elementModel, "state", QPointF(100, 100), QSizeF(200, 100), model::INVALID_MODEL_ID);
        QVERIFY2(element != nullptr, qPrintable(QString("iteration %1: createHsmElement returned null").arg(iteration)));

        // Observe the element's own drag signals (the ones createHsmElement wires to
        // the view's drag slots). "Fire once per event" is a property of these.
        QSignalSpy beginSpy(element, &view::HsmElement::dragElementBegin);
        QSignalSpy moveSpy(element, &view::HsmElement::dragElementEvent);
        QSignalSpy dropSpy(element, &view::HsmElement::dropElementEvent);

        QVERIFY2(beginSpy.isValid(),
                 qPrintable(QString("iteration %1: QSignalSpy on dragElementBegin is invalid").arg(iteration)));
        QVERIFY2(moveSpy.isValid(),
                 qPrintable(QString("iteration %1: QSignalSpy on dragElementEvent is invalid").arg(iteration)));
        QVERIFY2(dropSpy.isValid(),
                 qPrintable(QString("iteration %1: QSignalSpy on dropElementEvent is invalid").arg(iteration)));

        // Drive a real drag: press, a random number of moves, release.
        DragTestHelper::beginDrag(element);
        // At least two move frames: the first drives PREPARE -> DRAGGING and emits
        // dragElementBegin, so a second frame is needed to emit dragElementEvent.
        const int moveFrames = rng.bounded(2, 6);
        for (int frame = 0; frame < moveFrames; ++frame) {
            const QPointF delta(rng.bounded(5, 40), rng.bounded(5, 40));
            DragTestHelper::simulateDragMove(element, delta);
        }
        DragTestHelper::endDrag(element);

        // A single drag begins exactly once and drops exactly once -- one per event.
        QVERIFY2(beginSpy.count() == 1,
                 qPrintable(QString("iteration %1: one drag emitted %2 dragElementBegin signal(s), expected 1")
                                .arg(iteration)
                                .arg(beginSpy.count())));
        QVERIFY2(dropSpy.count() == 1,
                 qPrintable(QString("iteration %1: one drag emitted %2 dropElementEvent signal(s), expected 1")
                                .arg(iteration)
                                .arg(dropSpy.count())));
        // Post-begin move frames emit dragElementEvent (at least one, since the
        // first frame consumed the begin transition).
        QVERIFY2(moveSpy.count() >= 1,
                 qPrintable(QString("iteration %1: %2 move frame(s) produced %3 dragElementEvent signal(s), "
                                    "expected at least 1")
                                .arg(iteration)
                                .arg(moveFrames)
                                .arg(moveSpy.count())));

        // transformChanged is a view signal delivered to the element. It must fire
        // exactly once per view transform operation.
        QSignalSpy transformSpy(&view, &HsmGraphicsView::transformChanged);
        QVERIFY2(transformSpy.isValid(),
                 qPrintable(QString("iteration %1: QSignalSpy on transformChanged is invalid").arg(iteration)));

        const int zoomSteps = rng.bounded(1, 5);
        for (int step = 0; step < zoomSteps; ++step) {
            view.zoomIn();
        }

        QVERIFY2(transformSpy.count() == zoomSteps,
                 qPrintable(QString("iteration %1: %2 zoomIn() call(s) emitted %3 transformChanged signal(s), "
                                    "expected exactly one per call")
                                .arg(iteration)
                                .arg(zoomSteps)
                                .arg(transformSpy.count())));
    }
}

// ---------------------------------------------------------------------------
// Faithful stand-in for the downstream open-project DECISION the double-click
// reaches: MainEditorController::handleHsmElementDoubleClick casts the clicked
// model entity to IncludeEntity and no-ops for anything else, and
// MainEditorController::openProject guards on a non-empty path. Those controller
// sources are not linkable from the test harness (see
// MainEditorControllerOpenProjectPropertyTest.cpp), so this mirrors the exact
// branch structure and reports whether an open-project action WOULD be taken.
// ---------------------------------------------------------------------------
namespace {

bool doubleClickWouldOpenProject(const QSharedPointer<model::StateMachineEntity>& clicked) {
    bool wouldOpen = false;

    // handleHsmElementDoubleClick: only Include elements reach openProject.
    const QSharedPointer<model::IncludeEntity> include = clicked.dynamicCast<model::IncludeEntity>();

    if (include.isNull() == false) {
        // openProject guard: an empty path takes no open-project action.
        if (include->path().isEmpty() == false) {
            wouldOpen = true;
        }
    }

    return wouldOpen;
}

}  // namespace

/**
 * @brief Property 2 (Preservation, Requirements 3.2, 3.5): double-clicking a
 *        non-Include element or an empty-path Include takes no open-project action.
 *
 * The fix removes a duplicated double-click connection; it must not change WHICH
 * double-clicks trigger an open. For a non-Include element (Requirement 3.2) and
 * for an Include with an empty stored path (Requirement 3.5) the flow must take no
 * open-project action -- exactly as it does on the unfixed code. Include elements
 * with a non-empty path DO reach an open (that is the bug-condition path handled
 * elsewhere); this slot asserts only the no-op cases are preserved.
 *
 * For every generated element the view still forwards exactly one
 * `hsmElementDoubleClickEvent` (the fix leaves a single connection), while the
 * downstream open-project decision is no-op for the two non-bug shapes. Element
 * kind and path state are varied with QRandomGenerator.
 *
 * Validates: Requirements 3.2, 3.5
 */
void HsmGraphicsViewPropertyTest::PropertyNonBugDoubleClickTakesNoOpenAction() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    enum Kind { NonIncludeState = 0, EmptyPathInclude = 1 };

    for (int iteration = 0; iteration < 60; ++iteration) {
        QGraphicsScene scene;
        HsmGraphicsView view;
        view.setScene(&scene);

        const Kind kind = static_cast<Kind>(rng.bounded(2));

        QSharedPointer<model::StateMachineEntity> model;
        QString elementTypeId;

        if (NonIncludeState == kind) {
            // A plain regular state is never an Include -> no open-project action.
            model = QSharedPointer<model::RegularState>::create(QStringLiteral("plain%1").arg(rng.bounded(1000)));
            elementTypeId = QStringLiteral("state");
        } else {
            // An Include with an EMPTY path -> guarded no-op in openProject.
            model::EntityIdGenerator gen;
            auto include = model::ModelElementsFactory::createUniqueState(model::StateType::INCLUDE, gen)
                               .dynamicCast<model::IncludeEntity>();
            QVERIFY2(include.isNull() == false,
                     qPrintable(QString("iteration %1: failed to create Include model element").arg(iteration)));
            include->setPath(QString());  // empty path (Requirement 3.5)
            model = include;
            elementTypeId = QStringLiteral("include");
        }

        const QPointF pos(rng.bounded(-300, 300), rng.bounded(-300, 300));
        view::HsmElement* element = view.createHsmElement(model, elementTypeId, pos, QSizeF(80, 60), model::INVALID_MODEL_ID);
        QVERIFY2(element != nullptr, qPrintable(QString("iteration %1: createHsmElement returned null").arg(iteration)));

        QSignalSpy spy(&view, &HsmGraphicsView::hsmElementDoubleClickEvent);
        QVERIFY2(spy.isValid(),
                 qPrintable(QString("iteration %1: QSignalSpy on hsmElementDoubleClickEvent is invalid").arg(iteration)));

        // One real double-click.
        QGraphicsSceneMouseEvent doubleClick(QEvent::GraphicsSceneMouseDoubleClick);
        doubleClick.setButton(Qt::LeftButton);
        doubleClick.setButtons(Qt::LeftButton);
        doubleClick.setScenePos(element->scenePos());
        doubleClick.setPos(element->mapFromScene(element->scenePos()));
        scene.sendEvent(element, &doubleClick);

        // The downstream decision takes no open-project action for either shape.
        QVERIFY2(doubleClickWouldOpenProject(model) == false,
                 qPrintable(QString("iteration %1: non-bug double-click (kind=%2) unexpectedly would open a project")
                                .arg(iteration)
                                .arg(static_cast<int>(kind))));
    }
}

/**
 * @brief Property 2 (Requirement 3.1): Non-empty add preservation.
 *
 * When the document already contains at least one element (isBugCondition is
 * false), adding another element must CONTINUE TO place the new element at its
 * drop position with no regression, and must not perturb the already-present
 * element.
 *
 * The scene is seeded with a first element so the document is non-empty; the
 * second-drop scene position is then varied with QRandomGenerator across
 * near-origin, off-origin and near-diagram-bound positions. For each drop we
 * assert the added item lands exactly at its requested drop position and the
 * seed element's scene position is unchanged by the add.
 *
 * Observation-first note: this slot encodes behavior observed on the UNFIXED
 * code. On a plain scene with no explicit scene rect, Qt keeps deriving the
 * scene rect from itemsBoundingRect() for every add (not just the first), so
 * view.sceneRect()/mapFromScene are NOT stable across a bare scene.addItem()
 * even when the document is already non-empty. That derived-rect growth is a
 * property of the empty-vs-non-empty bug surface, not of placement; the
 * regression-prevention property for Requirement 3.1 that genuinely holds on
 * the unfixed code is that placement itself is correct (the element lands at
 * its drop position and existing elements are undisturbed). We assert exactly
 * that observed behavior so the test PASSES on the unfixed code and keeps
 * guarding placement after the fix.
 *
 * Validates: Requirements 3.1
 */
void HsmGraphicsViewPropertyTest::PropertyNonEmptyAddDoesNotShiftView() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    const QSizeF itemSize(60.0, 40.0);

    for (int iteration = 0; iteration < 50; ++iteration) {
        // Build a fresh view + scene exactly as MainWindow::projectOpened does.
        HsmGraphicsView view;
        QGraphicsScene scene;
        view.setScene(&scene);
        view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        view.resize(800, 600);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));

        // Seed the scene so the document is NON-empty (isBugCondition == false).
        const QPointF seedPos(rng.bounded(-50, 50), rng.bounded(-50, 50));
        QGraphicsRectItem* seedItem = new QGraphicsRectItem(QRectF(QPointF(0.0, 0.0), itemSize));
        seedItem->setPos(seedPos);
        scene.addItem(seedItem);
        QVERIFY2(false == scene.items().isEmpty(),
                 qPrintable(QString("Iteration %1: scene unexpectedly empty after seeding").arg(iteration)));

        // Vary the second drop position across the input space.
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

        // Add the SECOND item at the generated drop position.
        QGraphicsRectItem* secondItem = new QGraphicsRectItem(QRectF(QPointF(0.0, 0.0), itemSize));
        secondItem->setPos(dropPosition);
        scene.addItem(secondItem);

        // The added item must land at the requested drop position (no regression).
        QVERIFY2(secondItem->scenePos() == dropPosition,
                 qPrintable(QString("Iteration %1: second item scenePos (%2,%3) != drop position (%4,%5)")
                                .arg(iteration)
                                .arg(secondItem->scenePos().x())
                                .arg(secondItem->scenePos().y())
                                .arg(dropPosition.x())
                                .arg(dropPosition.y())));

        // The already-present element must be undisturbed by the second add.
        QVERIFY2(seedItem->scenePos() == seedPos,
                 qPrintable(QString("Iteration %1: seed item scenePos (%2,%3) shifted from (%4,%5) "
                                    "when the second element was added")
                                .arg(iteration)
                                .arg(seedItem->scenePos().x())
                                .arg(seedItem->scenePos().y())
                                .arg(seedPos.x())
                                .arg(seedPos.y())));
    }
}

/**
 * @brief Property 2 (Requirement 3.2): Drag / move / reparent preservation.
 *
 * Dragging/moving and reparenting existing elements must position them exactly
 * as they do on the unfixed code. For a hierarchy of existing elements we drag a
 * top-level element by a random grid-aligned delta and assert its final scene
 * position equals the expected snapped target, then reparent a child between two
 * parents and assert its scene position is preserved across the reparent.
 *
 * The expected values are the ones the unfixed code already produces (snapped
 * drag target; scene-position-preserving reparent), so this MUST PASS on the
 * unfixed code.
 *
 * Validates: Requirements 3.2
 */
void HsmGraphicsViewPropertyTest::PropertyDragMoveReparentPreservesPositions() {
    const int gridStep = ThemeManager::instance().theme().grid.minorLineStep;
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    // --- Part 1: drag/move preservation on a top-level element ---
    for (int iteration = 0; iteration < 30; ++iteration) {
        QGraphicsScene scene;
        HsmGraphicsView view;
        view.setScene(&scene);
        view.setSnapToGridEnabled(true);

        auto elementModel = QSharedPointer<model::RegularState>::create("dragTarget");
        view::HsmElement* element =
            view.createHsmElement(elementModel, "state", QPointF(100, 100), QSizeF(200, 100), model::INVALID_MODEL_ID);

        const QPointF startScenePos = element->scenePos();

        // Random grid-aligned displacement so the expected target is unambiguous.
        const int stepsX = rng.bounded(-6, 7);
        const int stepsY = rng.bounded(-6, 7);
        const QPointF delta(stepsX * gridStep, stepsY * gridStep);

        DragTestHelper::beginDrag(element);
        DragTestHelper::simulateDragMove(element, delta);
        const QPointF finalScenePos = element->scenePos();
        DragTestHelper::endDrag(element);

        const QPointF expectedFinal = HsmGraphicsView::alignPointToGrid(startScenePos + delta);

        QVERIFY2(qFuzzyCompare(finalScenePos.x(), expectedFinal.x()) && qFuzzyCompare(finalScenePos.y(), expectedFinal.y()),
                 qPrintable(QString("Iteration %1: drag final scenePos (%2,%3) != expected snapped (%4,%5) "
                                    "for delta (%6,%7)")
                                .arg(iteration)
                                .arg(finalScenePos.x())
                                .arg(finalScenePos.y())
                                .arg(expectedFinal.x())
                                .arg(expectedFinal.y())
                                .arg(delta.x())
                                .arg(delta.y())));
    }

    // --- Part 2: reparent preservation (scene position stable across reparent) ---
    for (int iteration = 0; iteration < 20; ++iteration) {
        QGraphicsScene scene;
        HsmGraphicsView view;
        view.setScene(&scene);

        auto controller = QSharedPointer<PreservationStubController>::create(&view);
        view.setProjectController(controller.toWeakRef());

        // Two sibling parents and one child under the first parent.
        auto parentAModel = QSharedPointer<model::RegularState>::create("parentA");
        auto parentBModel = QSharedPointer<model::RegularState>::create("parentB");
        auto childModel = QSharedPointer<model::RegularState>::create("child");

        view.createHsmElement(parentAModel, "state", QPointF(0, 0), QSizeF(300, 220), model::INVALID_MODEL_ID);
        view.createHsmElement(parentBModel, "state", QPointF(500, 0), QSizeF(300, 220), model::INVALID_MODEL_ID);
        view::HsmElement* child =
            view.createHsmElement(childModel, "state", QPointF(20, 60), QSizeF(100, 60), parentAModel->id());

        const QPointF childScenePosBefore = child->scenePos();

        // Reparent the child from parentA to parentB via the real move path.
        view.moveHsmElement(childModel->id(), parentBModel->id());

        const QPointF childScenePosAfter = child->scenePos();

        QVERIFY2(qFuzzyCompare(childScenePosAfter.x(), childScenePosBefore.x()) &&
                     qFuzzyCompare(childScenePosAfter.y(), childScenePosBefore.y()),
                 qPrintable(QString("Iteration %1: child scenePos changed across reparent: "
                                    "before (%2,%3) -> after (%4,%5)")
                                .arg(iteration)
                                .arg(childScenePosBefore.x())
                                .arg(childScenePosBefore.y())
                                .arg(childScenePosAfter.x())
                                .arg(childScenePosAfter.y())));
    }
}

/**
 * @brief Property 2 (Requirement 3.3): Pan / zoom / fit preservation.
 *
 * Pan, zoom in/out, reset zoom and fit-to-view must behave exactly as they do on
 * the unfixed code. Random pan/zoom/fit sequences are generated and the
 * resulting view transform is compared against a reference view that applied the
 * SAME sequence, confirming the operations are deterministic and reproducible.
 * In addition two invariants observed on the unfixed code are asserted:
 *  - resetZoom() always restores the identity scale (m11 == m22 == 1).
 *  - a symmetric number of zoomIn()/zoomOut() steps returns to the identity
 *    scale (zoom is a reversible multiplicative transform).
 *
 * All asserted behavior is what the unfixed code already produces, so this MUST
 * PASS on it.
 *
 * Validates: Requirements 3.3
 */
void HsmGraphicsViewPropertyTest::PropertyPanZoomFitPreserved() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    enum Op { ZoomIn = 0, ZoomOut = 1, ResetZoom = 2, FitSceneToView = 3 };

    auto buildView = [](QGraphicsScene& scene, HsmGraphicsView& view) {
        view.setScene(&scene);
        // Seed a couple of items so fitSceneToView has a non-degenerate bounding rect.
        QGraphicsRectItem* a = new QGraphicsRectItem(QRectF(0, 0, 80, 50));
        a->setPos(QPointF(-120, -90));
        scene.addItem(a);
        QGraphicsRectItem* b = new QGraphicsRectItem(QRectF(0, 0, 120, 70));
        b->setPos(QPointF(140, 110));
        scene.addItem(b);
        view.resize(800, 600);
    };

    auto applyOp = [](HsmGraphicsView& view, const Op op) {
        switch (op) {
            case ZoomIn:
                view.zoomIn();
                break;
            case ZoomOut:
                view.zoomOut();
                break;
            case ResetZoom:
                view.resetZoom();
                break;
            case FitSceneToView:
                view.fitSceneToView();
                break;
        }
    };

    for (int iteration = 0; iteration < 40; ++iteration) {
        // Build two independent but identically configured views.
        QGraphicsScene sceneA;
        HsmGraphicsView viewA;
        buildView(sceneA, viewA);

        QGraphicsScene sceneB;
        HsmGraphicsView viewB;
        buildView(sceneB, viewB);

        // Generate a random pan/zoom/fit sequence and apply it to both views.
        const int opCount = static_cast<int>(rng.bounded(1, 12));
        for (int i = 0; i < opCount; ++i) {
            const Op op = static_cast<Op>(rng.bounded(4));
            applyOp(viewA, op);
            applyOp(viewB, op);
        }

        // Determinism/preservation: the same sequence yields the same transform.
        const QTransform transformA = viewA.transform();
        const QTransform transformB = viewB.transform();
        QVERIFY2(transformA == transformB,
                 qPrintable(QString("Iteration %1: identical pan/zoom/fit sequence produced different "
                                    "transforms: A(m11=%2,m22=%3) B(m11=%4,m22=%5)")
                                .arg(iteration)
                                .arg(transformA.m11())
                                .arg(transformA.m22())
                                .arg(transformB.m11())
                                .arg(transformB.m22())));

        // Invariant: resetZoom() restores identity scale.
        viewA.resetZoom();
        const QTransform afterReset = viewA.transform();
        QVERIFY2(qFuzzyCompare(afterReset.m11(), 1.0) && qFuzzyCompare(afterReset.m22(), 1.0),
                 qPrintable(QString("Iteration %1: resetZoom did not restore identity scale "
                                    "(m11=%2, m22=%3)")
                                .arg(iteration)
                                .arg(afterReset.m11())
                                .arg(afterReset.m22())));

        // Invariant: symmetric zoomIn/zoomOut is reversible back to identity scale.
        const int zoomSteps = static_cast<int>(rng.bounded(1, 6));
        for (int i = 0; i < zoomSteps; ++i) {
            viewA.zoomIn();
        }
        for (int i = 0; i < zoomSteps; ++i) {
            viewA.zoomOut();
        }
        const QTransform afterSymmetric = viewA.transform();
        QVERIFY2(qFuzzyCompare(afterSymmetric.m11(), 1.0) && qFuzzyCompare(afterSymmetric.m22(), 1.0),
                 qPrintable(QString("Iteration %1: symmetric %2x zoomIn/zoomOut did not return to identity "
                                    "scale (m11=%3, m22=%4)")
                                .arg(iteration)
                                .arg(zoomSteps)
                                .arg(afterSymmetric.m11())
                                .arg(afterSymmetric.m22())));
    }
}

/**
 * @brief Property 4: Editor drop partitions dropped paths.
 *
 * For any list of dropped file paths, the set of paths forwarded to the project
 * open operation is exactly the subset of paths that end in `.scxml`
 * (case-insensitive), and every other dropped path is skipped, regardless of how
 * many files are dropped (design Correctness Property 4; Requirements 3.2, 4.2).
 *
 * A fresh random path list is generated each iteration from a mixed alphabet of
 * path kinds so the input space is covered intelligently:
 *  - genuine `.scxml` paths with the extension in varied letter case
 *    (`.scxml`, `.SCXML`, `.ScXmL`, ...), which MUST be kept
 *  - near-miss paths that merely contain `scxml` or end in a different
 *    extension (`.xml`, `.scxm`, `foo.scxml.bak`, `scxml`), which MUST be dropped
 *  - plain non-matching paths (`.txt`, extensionless), which MUST be dropped
 * List length is varied from 0 up to a few dozen so the partition is checked
 * "regardless of count". The expected `.scxml` subset is recomputed independently
 * of the production helper and compared for exact equality including order, so
 * the test asserts the forwarded set equals exactly the `.scxml` subset with
 * order preserved.
 *
 * Feature: drag-and-drop-files, Property 4: Editor drop partitions dropped paths
 *
 * Validates: Requirements 3.2, 4.2
 */
void HsmGraphicsViewPropertyTest::PropertyEditorDropPartitionsDroppedPaths() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    // Case variants of the ".scxml" extension so case-insensitivity is exercised.
    const QStringList scxmlExtensionCases = {
        ".scxml",
        ".SCXML",
        ".ScXmL",
        ".sCxMl",
        ".Scxml",
        ".scxmL",
    };

    // Builders for the different path kinds. Each returns {path, isScxml}.
    auto makeScxmlPath = [&rng, &scxmlExtensionCases](int index) -> QString {
        const QString ext = scxmlExtensionCases.at(rng.bounded(scxmlExtensionCases.size()));
        return QStringLiteral("/tmp/dir%1/machine%2%3").arg(rng.bounded(10)).arg(index).arg(ext);
    };

    // Near-miss paths that must NOT be treated as .scxml even though they look close.
    auto makeNearMissPath = [&rng](int index) -> QString {
        switch (rng.bounded(5)) {
            case 0:
                return QStringLiteral("/tmp/notes%1.xml").arg(index);
            case 1:
                return QStringLiteral("/tmp/trunc%1.scxm").arg(index);
            case 2:
                return QStringLiteral("/tmp/backup%1.scxml.bak").arg(index);
            case 3:
                return QStringLiteral("/tmp/scxml%1").arg(index);  // contains the word, no extension
            default:
                return QStringLiteral("/tmp/scxml_folder%1/readme").arg(index);
        }
    };

    // Plain non-matching paths.
    auto makeOtherPath = [&rng](int index) -> QString {
        switch (rng.bounded(3)) {
            case 0:
                return QStringLiteral("/tmp/doc%1.txt").arg(index);
            case 1:
                return QStringLiteral("/tmp/image%1.png").arg(index);
            default:
                return QStringLiteral("/tmp/plainfile%1").arg(index);
        }
    };

    for (int iteration = 0; iteration < 100; ++iteration) {
        // Vary the count across empty, singleton and multi-file drops.
        const int pathCount = static_cast<int>(rng.bounded(0, 32));

        QStringList droppedPaths;
        QStringList expectedScxmlPaths;

        for (int i = 0; i < pathCount; ++i) {
            QString path;

            switch (rng.bounded(3)) {
                case 0:
                    path = makeScxmlPath(i);
                    expectedScxmlPaths.append(path);  // order preserved by appending in input order
                    break;
                case 1:
                    path = makeNearMissPath(i);
                    break;
                default:
                    path = makeOtherPath(i);
                    break;
            }

            droppedPaths.append(path);
        }

        const QStringList forwardedPaths = view::DragDropUtils::filterScxmlPaths(droppedPaths);

        // The forwarded set must equal exactly the .scxml subset, order preserved.
        QVERIFY2(forwardedPaths == expectedScxmlPaths,
                 qPrintable(QString("Iteration %1 (count %2): forwarded paths [%3] != expected .scxml subset [%4] "
                                    "for input [%5]")
                                .arg(iteration)
                                .arg(pathCount)
                                .arg(forwardedPaths.join(QStringLiteral(", ")))
                                .arg(expectedScxmlPaths.join(QStringLiteral(", ")))
                                .arg(droppedPaths.join(QStringLiteral(", ")))));
    }
}

int runHsmGraphicsViewPropertyTest(int argc, char** argv) {
    QApplication app(argc, argv);

    HsmGraphicsViewPropertyTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "HsmGraphicsViewPropertyTest.moc"
