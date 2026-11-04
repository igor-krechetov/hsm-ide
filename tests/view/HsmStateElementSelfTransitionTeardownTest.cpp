#include <QtTest>

#include <QApplication>
#include <QGraphicsScene>
#include <QPointer>
#include <QSharedPointer>

#include "model/elements/RegularState.hpp"
#include "model/elements/Transition.hpp"
#include "view/elements/HsmStateElement.hpp"
#include "view/elements/HsmTransition.hpp"

/**
 * @brief Regression test for the crash when copy-pasting an element onto a document that
 *        was OPENED from a file (not a freshly created HSM).
 *
 * Reported symptom: hsm-ide crashes while rebuilding the view after a paste, but only on an
 * opened document. The distinguishing feature of the opened document (all_elements.scxml) is
 * a composite state that contains a SELF-TRANSITION. A freshly created HSM has none, which is
 * why the crash never reproduced there.
 *
 * Root cause: HsmStateElement::addChildItem(), on the self-transition branch, wires
 *   connect(child, &QObject::destroyed, this, [this, child]() { layoutSections(); });
 * QGraphicsObject tears its children down via the Qt parent-child mechanism. When the view is
 * cleared/rebuilt (clearAllHsmElements -> delete the state element), deleting the state
 * destroys its self-transition child, which emits QObject::destroyed, which re-enters
 * layoutSections() on the state that is ITSELF mid-destruction. By then its section items
 * (mBodySection / mSelfTransitionsSection) may already be freed, so layoutSections()
 * dereferences dangling pointers -> use-after-free -> SIGSEGV. The mInitialized guard does not
 * help because mInitialized is still true during destruction.
 *
 * Fix: an explicit ~HsmStateElement() sets mDestroying = true BEFORE the base destructor tears
 * the children down, and layoutSections() bails out immediately when mDestroying is true, so
 * the destroyed-signal re-entry never touches a half-freed object.
 *
 * This test drives the REAL production path: it builds a genuine self-transition via
 * HsmTransition::connectElements(state, state) (which routes through the self-transition branch
 * of HsmStateElement::addChildItem and installs the destroyed->layoutSections connection), then
 * deletes the state element and verifies teardown completes without crashing. On the unfixed
 * code this test crashes during the delete.
 */
class HsmStateElementSelfTransitionTeardownTest : public QObject {
    Q_OBJECT

private slots:
    void DeletingStateWithSelfTransitionDoesNotCrash();
};

void HsmStateElementSelfTransitionTeardownTest::DeletingStateWithSelfTransitionDoesNotCrash() {
    QGraphicsScene scene;

    // Model side: one state, plus two self-transitions (source == target) on it. Two are
    // used so the re-entrant layoutSections() (on the unfixed code) iterates a non-trivial
    // self-transitions section while its owning items are mid-destruction.
    auto stateModel = QSharedPointer<model::RegularState>::create("CompositeState");
    auto transitionModelA = QSharedPointer<model::Transition>::create(stateModel, stateModel, "SELF_EVENT_A");
    auto transitionModelB = QSharedPointer<model::Transition>::create(stateModel, stateModel, "SELF_EVENT_B");

    // View side: the state element, initialised so its section items exist.
    auto* stateElement = new view::HsmStateElement();
    stateElement->init(stateModel);
    scene.addItem(stateElement);

    // The self-transition elements. Routed through connectElements(state, state), which
    // exercises the exact self-transition branch of addChildItem that wires the
    // destroyed -> layoutSections() connection responsible for the crash.
    auto* transitionA = new view::HsmTransition();
    transitionA->init(transitionModelA);
    transitionA->connectElements(stateElement, stateElement);

    auto* transitionB = new view::HsmTransition();
    transitionB->init(transitionModelB);
    transitionB->connectElements(stateElement, stateElement);

    QVERIFY2(transitionA->isSelfTransition() && transitionB->isSelfTransition(),
             "precondition: connectElements(state, state) must yield self-transitions");

    // Track that the self-transitions are actually destroyed as a side effect of deleting the
    // parent state (the parent-child teardown that fires the destroyed signal), and that the
    // state itself is gone afterwards.
    QPointer<view::HsmTransition> transitionGuardA(transitionA);
    QPointer<view::HsmTransition> transitionGuardB(transitionB);
    QPointer<view::HsmStateElement> stateGuard(stateElement);

    // The operation that crashes on the unfixed code: deleting the state element destroys its
    // self-transition children, whose destroyed signal re-enters layoutSections() on the dying
    // state (with isInitialized() still true, so its old guard does not help) and dereferences
    // the section items that the same parent-child teardown is in the middle of freeing. With
    // the mDestroying guard that re-entry bails out and teardown completes cleanly.
    delete stateElement;

    QVERIFY2(stateGuard.isNull(), "the state element must be destroyed");
    QVERIFY2(transitionGuardA.isNull() && transitionGuardB.isNull(),
             "the self-transition children must be destroyed via the parent-child teardown");
}

int runHsmStateElementSelfTransitionTeardownTest(int argc, char** argv) {
    QApplication app(argc, argv);

    HsmStateElementSelfTransitionTeardownTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "HsmStateElementSelfTransitionTeardownTest.moc"
