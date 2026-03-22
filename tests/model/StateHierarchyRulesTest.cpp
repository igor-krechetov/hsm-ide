#include <QtTest>

#include "model/StateHierarchyRules.hpp"

class StateHierarchyRulesTest : public QObject {
    Q_OBJECT

private slots:
    void TopLevelRules();
    void ParentChildRules();
    void TransitionParentRules();
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

int runStateHierarchyRulesTest(int argc, char** argv) {
    StateHierarchyRulesTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "StateHierarchyRulesTest.moc"
