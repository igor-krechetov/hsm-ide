#include <QtTest>

#include "model/actions/ModelActionFactory.hpp"
#include "model/elements/FinalState.hpp"

class FinalStateTest : public QObject {
    Q_OBJECT

private slots:
    void CallbackPropertyRoundTrip();
    void EnteringActionsSetGetPreservesOrder();
    void HasOnEnteringActionReflectsList();
    void StateChangedActionRoundTripUnchanged();
    void EnteringActionPropertyRoundTrip();
    void CopyEntityDataClonesActions();
    void NoExitingActionApi();
};

void FinalStateTest::CallbackPropertyRoundTrip() {
    auto state = QSharedPointer<model::FinalState>::create("Done");

    QVERIFY(state->setProperty("onStateChangedAction", "completeCb"));
    QVERIFY(state->hasOnStateChangedAction());
    QCOMPARE(QString("completeCb"), state->onStateChangedAction()->serialize());
    QCOMPARE(QString("completeCb"),
             state->getProperty("onStateChangedAction").value<QSharedPointer<model::IModelAction>>()->serialize());
}

void FinalStateTest::EnteringActionsSetGetPreservesOrder() {
    auto state = QSharedPointer<model::FinalState>::create("Done");

    model::ModelActionList actions;
    actions.append(model::ModelActionFactory::createModelActionFromData("a", model::ModelAction::CALLBACK));
    actions.append(model::ModelActionFactory::createModelActionFromData("b", model::ModelAction::CALLBACK));
    state->setOnEnteringActions(actions);

    QCOMPARE(2, state->onEnteringActions().size());
    QCOMPARE(QString("a"), state->onEnteringActions().at(0)->serialize());
    QCOMPARE(QString("b"), state->onEnteringActions().at(1)->serialize());
}

void FinalStateTest::HasOnEnteringActionReflectsList() {
    auto state = QSharedPointer<model::FinalState>::create("Done");

    QVERIFY(!state->hasOnEnteringAction());

    state->addOnEnteringAction(model::ModelActionFactory::createModelActionFromData("enter", model::ModelAction::CALLBACK));
    QVERIFY(state->hasOnEnteringAction());
    QCOMPARE(QString("enter"), state->onEnteringAction()->serialize());
}

void FinalStateTest::StateChangedActionRoundTripUnchanged() {
    auto state = QSharedPointer<model::FinalState>::create("Done");

    QVERIFY(!state->hasOnStateChangedAction());
    state->setOnStateChangedAction(QString("notifyCb"));
    QVERIFY(state->hasOnStateChangedAction());
    QCOMPARE(QString("notifyCb"), state->onStateChangedAction()->serialize());
}

void FinalStateTest::EnteringActionPropertyRoundTrip() {
    auto state = QSharedPointer<model::FinalState>::create("Done");

    QVERIFY(state->properties().contains("onEnteringAction"));
    QVERIFY(state->setProperty("onEnteringAction", "enterCb"));

    const model::ModelActionList stored = state->getProperty("onEnteringAction").value<model::ModelActionList>();
    QCOMPARE(1, stored.size());
    QCOMPARE(QString("enterCb"), stored.at(0)->serialize());
}

void FinalStateTest::CopyEntityDataClonesActions() {
    auto source = QSharedPointer<model::FinalState>::create("Done");
    source->setEvent("finished");
    source->setOnEnteringAction(QString("enterCb"));
    source->setOnStateChangedAction(QString("notifyCb"));

    auto copy = QSharedPointer<model::FinalState>::create("Copy");
    copy->copyEntityData(*source);

    QCOMPARE(QString("finished"), copy->event());
    QCOMPARE(1, copy->onEnteringActions().size());
    QCOMPARE(QString("enterCb"), copy->onEnteringActions().at(0)->serialize());
    QCOMPARE(QString("notifyCb"), copy->onStateChangedAction()->serialize());

    // Deep copy: mutating the source must not affect the copy.
    source->setOnEnteringAction(QString("changed"));
    QCOMPARE(QString("enterCb"), copy->onEnteringActions().at(0)->serialize());
}

void FinalStateTest::NoExitingActionApi() {
    auto state = QSharedPointer<model::FinalState>::create("Done");

    // A top-level final has no exit action; the property must not be exposed and must not round-trip.
    QVERIFY(!state->properties().contains("onExitingAction"));

    const QVariant before = state->getProperty("onExitingAction");
    QVERIFY(!state->setProperty("onExitingAction", "cleanupCb"));
    const QVariant after = state->getProperty("onExitingAction");
    QCOMPARE(before, after);
}

int runFinalStateTest(int argc, char** argv) {
    FinalStateTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "FinalStateTest.moc"
