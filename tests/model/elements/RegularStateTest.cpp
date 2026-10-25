#include <QtTest>

#include "model/private/EntityIdGenerator.hpp"
#include "model/ModelElementsFactory.hpp"
#include "model/elements/RegularState.hpp"
#include "model/elements/Transition.hpp"

class RegularStateTest : public QObject {
    Q_OBJECT

private slots:
    void CallbackPropertiesRoundTrip();
    void ChildAndTransitionSearch();
    void DeleteStateRemovesIncomingAndOutgoingTransitions();
    void CopyEntityDataPreservesCallback();
    void AssignmentOperatorPreservesCallback();
};

void RegularStateTest::CallbackPropertiesRoundTrip() {
    model::EntityIdGenerator gen;
    auto state = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                     .dynamicCast<model::RegularState>();
    state->setName("S");

    QVERIFY(state->setProperty("onEnteringAction", "enterCb"));
    QVERIFY(state->setProperty("onExitingAction", "exitCb"));
    QVERIFY(state->setProperty("onStateChangedAction", "stateCb"));

    QCOMPARE(QString("enterCb"), state->onEnteringAction()->serialize());
    QCOMPARE(QString("exitCb"), state->onExitingAction()->serialize());
    QCOMPARE(QString("stateCb"), state->onStateChangedAction()->serialize());
}

void RegularStateTest::ChildAndTransitionSearch() {
    model::EntityIdGenerator gen;
    auto parent = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                      .dynamicCast<model::RegularState>();
    parent->setName("Parent");
    auto child = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                     .dynamicCast<model::RegularState>();
    child->setName("Child");
    parent->addChildState(child);

    auto tr = model::ModelElementsFactory::createTransitionWithId(parent, child, "ev", gen.generateNextId());
    parent->addTransition(tr);

    QCOMPARE(child, parent->findChildStateByName("Child"));
    QCOMPARE(child, parent->findState(child->id()));
    QCOMPARE(tr, parent->findTransition(tr->id()));
    QCOMPARE(parent, parent->findParentState(child->id()));

    // excludeId: excluding the sole match by name returns null.
    QCOMPARE(QSharedPointer<model::State>(), parent->findChildStateByName("Child", child->id()));

    // excludeId: with two states sharing a name, excluding one returns the other.
    auto dupA = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                    .dynamicCast<model::RegularState>();
    dupA->setName("Dup");
    auto dupB = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                    .dynamicCast<model::RegularState>();
    dupB->setName("Dup");
    parent->addChildState(dupA);
    parent->addChildState(dupB);

    QCOMPARE(dupB, parent->findChildStateByName("Dup", dupA->id()));

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
    model::EntityIdGenerator gen;
    auto root = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                    .dynamicCast<model::RegularState>();
    root->setName("Root");
    auto stateA = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                      .dynamicCast<model::RegularState>();
    stateA->setName("A");
    auto stateB = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                      .dynamicCast<model::RegularState>();
    stateB->setName("B");
    auto stateC = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                      .dynamicCast<model::RegularState>();
    stateC->setName("C");
    auto stateH = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                      .dynamicCast<model::RegularState>();
    stateH->setName("H");

    root->addChildState(stateA);
    root->addChildState(stateB);
    root->addChildState(stateC);
    stateC->addChildState(stateH);

    auto transitionTop = model::ModelElementsFactory::createTransitionWithId(stateA, stateC, "A_to_C", gen.generateNextId());
    auto transitionSubstate = model::ModelElementsFactory::createTransitionWithId(stateB, stateH, "B_to_H", gen.generateNextId());
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

void RegularStateTest::CopyEntityDataPreservesCallback() {
    model::EntityIdGenerator gen;
    auto source = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                      .dynamicCast<model::RegularState>();
    source->setName("S");
    // A plain callback name matches no action prefix, so copy must fall back to CALLBACK (not NONE)
    // to preserve the name.
    source->setOnStateChangedAction(QString("notifyCb"));
    source->setOnEnteringAction(QString("enterCb"));
    source->setOnExitingAction(QString("exitCb"));

    auto copy = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                    .dynamicCast<model::RegularState>();
    copy->copyEntityData(*source);

    QVERIFY(copy->hasOnStateChangedAction());
    QCOMPARE(QString("notifyCb"), copy->onStateChangedAction()->serialize());
    QCOMPARE(QString("enterCb"), copy->onEnteringAction()->serialize());
    QCOMPARE(QString("exitCb"), copy->onExitingAction()->serialize());
}

void RegularStateTest::AssignmentOperatorPreservesCallback() {
    auto source = QSharedPointer<model::RegularState>::create("Src");
    source->setOnStateChangedAction(QString("notifyCb"));
    source->setOnEnteringAction(QString("enterCb"));
    source->setOnExitingAction(QString("exitCb"));

    auto dst = QSharedPointer<model::RegularState>::create("Dst");
    *dst = *source;

    QVERIFY(dst->hasOnStateChangedAction());
    QCOMPARE(QString("notifyCb"), dst->onStateChangedAction()->serialize());
    QCOMPARE(QString("enterCb"), dst->onEnteringAction()->serialize());
    QCOMPARE(QString("exitCb"), dst->onExitingAction()->serialize());
}

int runRegularStateTest(int argc, char** argv) {
    RegularStateTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "RegularStateTest.moc"
