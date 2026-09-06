#include <QtTest>

#include "model/StateHierarchyRules.hpp"
#include "model/elements/InitialState.hpp"
#include "model/elements/ModelRootState.hpp"
#include "model/elements/RegularState.hpp"

class StateHierarchyRulesTest : public QObject {
    Q_OBJECT

private slots:
    void TopLevelRules();
    void ParentChildRules();
    void TransitionParentRules();
    void SingleInitialStateRule();
};

void StateHierarchyRulesTest::TopLevelRules() {
    QCOMPARE(model::StateHierarchyRules::canBeTopLevel(model::StateType::INITIAL), true);
    QCOMPARE(model::StateHierarchyRules::canBeTopLevel(model::StateType::FINAL), true);
    QCOMPARE(model::StateHierarchyRules::canBeTopLevel(model::StateType::REGULAR), true);
    QCOMPARE(model::StateHierarchyRules::canBeTopLevel(model::StateType::INCLUDE), true);
    QCOMPARE(model::StateHierarchyRules::canBeTopLevel(model::StateType::ENTRYPOINT), false);
    QCOMPARE(model::StateHierarchyRules::canBeTopLevel(model::StateType::EXITPOINT), false);
    QCOMPARE(model::StateHierarchyRules::canBeTopLevel(model::StateType::HISTORY), false);
}

void StateHierarchyRulesTest::ParentChildRules() {
    QCOMPARE(model::StateHierarchyRules::canStateBeChildOf(model::StateType::MODEL_ROOT, model::StateType::HISTORY), false);
    QCOMPARE(model::StateHierarchyRules::canStateBeChildOf(model::StateType::MODEL_ROOT, model::StateType::INITIAL), true);
    QCOMPARE(model::StateHierarchyRules::canStateBeChildOf(model::StateType::MODEL_ROOT, model::StateType::REGULAR), true);
    QCOMPARE(model::StateHierarchyRules::canStateBeChildOf(model::StateType::MODEL_ROOT, model::StateType::ENTRYPOINT), false);
    QCOMPARE(model::StateHierarchyRules::canStateBeChildOf(model::StateType::MODEL_ROOT, model::StateType::EXITPOINT), false);
    QCOMPARE(model::StateHierarchyRules::canStateBeChildOf(model::StateType::MODEL_ROOT, model::StateType::FINAL), true);
    QCOMPARE(model::StateHierarchyRules::canStateBeChildOf(model::StateType::MODEL_ROOT, model::StateType::INCLUDE), true);

    QCOMPARE(model::StateHierarchyRules::canStateBeChildOf(model::StateType::REGULAR, model::StateType::HISTORY), true);
    QCOMPARE(model::StateHierarchyRules::canStateBeChildOf(model::StateType::REGULAR, model::StateType::INITIAL), false);
    QCOMPARE(model::StateHierarchyRules::canStateBeChildOf(model::StateType::REGULAR, model::StateType::REGULAR), true);
    QCOMPARE(model::StateHierarchyRules::canStateBeChildOf(model::StateType::REGULAR, model::StateType::ENTRYPOINT), true);
    QCOMPARE(model::StateHierarchyRules::canStateBeChildOf(model::StateType::REGULAR, model::StateType::EXITPOINT), true);
    QCOMPARE(model::StateHierarchyRules::canStateBeChildOf(model::StateType::REGULAR, model::StateType::FINAL), false);
    QCOMPARE(model::StateHierarchyRules::canStateBeChildOf(model::StateType::REGULAR, model::StateType::INCLUDE), true);
}

void StateHierarchyRulesTest::TransitionParentRules() {
    QVERIFY(model::StateHierarchyRules::canTransitionBeChildOf(model::StateType::REGULAR));
    QVERIFY(model::StateHierarchyRules::canTransitionBeChildOf(model::StateType::INCLUDE));
    QVERIFY(model::StateHierarchyRules::canTransitionBeChildOf(model::StateType::INITIAL));
    QVERIFY(model::StateHierarchyRules::canTransitionBeChildOf(model::StateType::HISTORY));
    QVERIFY(model::StateHierarchyRules::canTransitionBeChildOf(model::StateType::FINAL) == false);
    QVERIFY(model::StateHierarchyRules::canTransitionBeChildOf(model::StateType::MODEL_ROOT) == false);
}

/**
 * @brief REQ-103f6: only one initial state is allowed at the model root.
 *
 * The type-only rule allows an initial state at the root, but canAddEntityToParent
 * must additionally reject a second one once the root already contains one.
 */
void StateHierarchyRulesTest::SingleInitialStateRule() {
    auto root = QSharedPointer<model::ModelRootState>::create("Root");
    auto firstInitial = QSharedPointer<model::InitialState>::create();
    auto secondInitial = QSharedPointer<model::InitialState>::create();
    auto regular = QSharedPointer<model::RegularState>::create("S1");

    // Empty root: the first initial state is allowed.
    QCOMPARE(root->hasInitialState(), false);
    QCOMPARE(model::StateHierarchyRules::canAddEntityToParent(root, firstInitial), true);

    // A regular state is always allowed and must not affect the initial-state check.
    QCOMPARE(model::StateHierarchyRules::canAddEntityToParent(root, regular), true);

    // After the root actually contains an initial state, a second one is rejected.
    root->addChildState(firstInitial);
    QCOMPARE(root->hasInitialState(), true);
    QCOMPARE(model::StateHierarchyRules::canAddEntityToParent(root, secondInitial), false);

    // A regular state is still allowed alongside the existing initial state.
    QCOMPARE(model::StateHierarchyRules::canAddEntityToParent(root, regular), true);
}

int runStateHierarchyRulesTest(int argc, char** argv) {
    StateHierarchyRulesTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "StateHierarchyRulesTest.moc"
