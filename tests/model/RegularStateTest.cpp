#include <QtTest>

#include "model/ModelElementsFactory.hpp"
#include "model/RegularState.hpp"
#include "model/Transition.hpp"

class RegularStateTest : public QObject {
    Q_OBJECT

private slots:
    void CallbackPropertiesRoundTrip();
    void ChildAndTransitionSearch();
    void DeleteStateRemovesIncomingAndOutgoingTransitions();
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

/*
@startuml
state A
state B
state C {
    state H
}

A -> C
B -> H
@enduml
*/
void RegularStateTest::DeleteStateRemovesIncomingAndOutgoingTransitions() {
    auto root = QSharedPointer<model::RegularState>::create("Root");
    auto stateA = QSharedPointer<model::RegularState>::create("A");
    auto stateB = QSharedPointer<model::RegularState>::create("B");
    auto stateC = QSharedPointer<model::RegularState>::create("C");
    auto stateH = QSharedPointer<model::RegularState>::create("H");

    root->addChildState(stateA);
    root->addChildState(stateB);
    root->addChildState(stateC);
    stateC->addChildState(stateH);

    auto transitionTop = QSharedPointer<model::Transition>::create(stateA, stateC, "A_to_C");
    auto transitionSubstate = QSharedPointer<model::Transition>::create(stateB, stateH, "B_to_H");
    stateA->addTransition(transitionTop);
    stateB->addTransition(transitionSubstate);

    QVERIFY(root->findTransition(transitionTop->id()));
    QVERIFY(root->findTransition(transitionSubstate->id()));

    root->deleteChild(stateC->id());

    QCOMPARE(QSharedPointer<model::State>(), root->findState(stateC->id()));
    QCOMPARE(QSharedPointer<model::State>(), root->findState(stateH->id()));
    QCOMPARE(QSharedPointer<model::Transition>(), root->findTransition(transitionTop->id()));
    QCOMPARE(QSharedPointer<model::Transition>(), root->findTransition(transitionSubstate->id()));
}

int runRegularStateTest(int argc, char** argv) {
    RegularStateTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "RegularStateTest.moc"
