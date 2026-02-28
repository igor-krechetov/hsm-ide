#include <QtTest>

#include "model/ModelElementsFactory.hpp"
#include "model/RegularState.hpp"
#include "model/Transition.hpp"

class RegularStateTest : public QObject {
    Q_OBJECT

private slots:
    void CallbackPropertiesRoundTrip();
    void ChildAndTransitionSearch();
};

/**
 * @brief Verify regular state stores callbacks and supports property API.
 *
 * Use-case: User edits state callbacks in property panel.
 */
void RegularStateTest::CallbackPropertiesRoundTrip() {
    auto state = QSharedPointer<model::RegularState>::create("S");

    QVERIFY(state->setProperty("onEnteringCallback", "enterCb"));
    QVERIFY(state->setProperty("onExitingCallback", "exitCb"));
    QVERIFY(state->setProperty("onStateChangedCallback", "stateCb"));

    QCOMPARE(QString("enterCb"), state->onEnteringCallback());
    QCOMPARE(QString("exitCb"), state->onExitingCallback());
    QCOMPARE(QString("stateCb"), state->onStateChangedCallback());
}

/**
 * @brief Verify regular state child management and recursive search.
 *
 * Use-case: Hierarchical model resolves nested states and transitions by id and name.
 */
void RegularStateTest::ChildAndTransitionSearch() {
    auto parent = QSharedPointer<model::RegularState>::create("Parent");
    auto child = QSharedPointer<model::RegularState>::create("Child");
    parent->addChildState(child);

    auto tr = QSharedPointer<model::Transition>::create(parent, child, "ev");
    parent->addTransition(tr);

    QCOMPARE(child, parent->findChildStateByName("Child"));
    QCOMPARE(child, parent->findState(child->id()));
    QCOMPARE(tr, parent->findTransition(tr->id()));
    QCOMPARE(parent, parent->findParentState(child->id()));

    parent->deleteChild(child->id());
    QCOMPARE(QSharedPointer<model::State>(), parent->findState(child->id()));
}

int runRegularStateTest(int argc, char** argv) {
    RegularStateTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "RegularStateTest.moc"
