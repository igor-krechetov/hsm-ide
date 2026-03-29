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

void RegularStateTest::CallbackPropertiesRoundTrip() {
    auto state = QSharedPointer<model::RegularState>::create("S");

    QVERIFY(state->setProperty("onEnteringAction", "enterCb"));
    QVERIFY(state->setProperty("onExitingAction", "exitCb"));
    QVERIFY(state->setProperty("onStateChangedAction", "stateCb"));

    QCOMPARE(QString("enterCb"), state->onEnteringAction()->serialize());
    QCOMPARE(QString("exitCb"), state->onExitingAction()->serialize());
    QCOMPARE(QString("stateCb"), state->onStateChangedAction()->serialize());
}

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
