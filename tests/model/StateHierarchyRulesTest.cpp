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
    QVERIFY(model::StateHierarchyRules::canBeTopLevel(model::StateType::INITIAL));
    QVERIFY(model::StateHierarchyRules::canBeTopLevel(model::StateType::FINAL));
    QVERIFY(model::StateHierarchyRules::canBeTopLevel(model::StateType::REGULAR));
    QVERIFY(model::StateHierarchyRules::canBeTopLevel(model::StateType::INCLUDE));
    QVERIFY(model::StateHierarchyRules::canBeTopLevel(model::StateType::ENTRYPOINT) == false);
    QVERIFY(model::StateHierarchyRules::canBeTopLevel(model::StateType::EXITPOINT) == false);
    QVERIFY(model::StateHierarchyRules::canBeTopLevel(model::StateType::HISTORY) == false);
}

void StateHierarchyRulesTest::ParentChildRules() {
    QVERIFY(model::StateHierarchyRules::canStateBeChildOf(model::StateType::MODEL_ROOT, model::StateType::REGULAR));
    QVERIFY(model::StateHierarchyRules::canStateBeChildOf(model::StateType::MODEL_ROOT, model::StateType::HISTORY) == false);
    QVERIFY(model::StateHierarchyRules::canStateBeChildOf(model::StateType::REGULAR, model::StateType::HISTORY));
    QVERIFY(model::StateHierarchyRules::canStateBeChildOf(model::StateType::REGULAR, model::StateType::INITIAL));
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
