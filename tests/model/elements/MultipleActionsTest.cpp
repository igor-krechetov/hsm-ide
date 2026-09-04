#include <QtTest>

#include "model/ModelElementsFactory.hpp"
#include "model/StateMachineModel.hpp"
#include "model/StateMachineSerializer.hpp"
#include "model/actions/ModelActionFactory.hpp"
#include "model/elements/ModelRootState.hpp"
#include "model/elements/RegularState.hpp"
#include "model/elements/Transition.hpp"
#include "model/private/EntityIdGenerator.hpp"

class MultipleActionsTest : public QObject {
    Q_OBJECT

private slots:
    void EmptySlotReportsNoAction();
    void AddAndOrderIsPreserved();
    void InsertRemoveMove();
    void SingleActionCompatSetters();
    void DeepCopyIsIndependent();
    void SerializeMultipleStateActions();
    void DeserializeMultipleStateActions();
    void SerializeMultipleTransitionActions();

private:
    QSharedPointer<model::RegularState> makeState(model::EntityIdGenerator& gen, const QString& name);
    QSharedPointer<model::IModelAction> callback(const QString& fn);
};

QSharedPointer<model::RegularState> MultipleActionsTest::makeState(model::EntityIdGenerator& gen, const QString& name) {
    auto state =
        model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen).dynamicCast<model::RegularState>();
    state->setName(name);

    return state;
}

QSharedPointer<model::IModelAction> MultipleActionsTest::callback(const QString& fn) {
    auto action = model::ModelActionFactory::createModelAction(model::ModelAction::CALLBACK);
    action->setProperty("function", fn);

    return action;
}

void MultipleActionsTest::EmptySlotReportsNoAction() {
    model::EntityIdGenerator gen;
    auto state = makeState(gen, "S");

    QVERIFY(state->onEnteringActions().isEmpty());
    QVERIFY(state->onExitingActions().isEmpty());
    QVERIFY(state->hasOnEnteringAction() == false);
    QVERIFY(state->hasOnExitingAction() == false);
}

void MultipleActionsTest::AddAndOrderIsPreserved() {
    model::EntityIdGenerator gen;
    auto state = makeState(gen, "S");

    state->addOnEnteringAction(callback("a"));
    state->addOnEnteringAction(callback("b"));
    state->addOnEnteringAction(callback("c"));

    QCOMPARE(state->onEnteringActions().size(), 3);
    QCOMPARE(state->onEnteringActions().at(0)->serialize(), QString("a"));
    QCOMPARE(state->onEnteringActions().at(1)->serialize(), QString("b"));
    QCOMPARE(state->onEnteringActions().at(2)->serialize(), QString("c"));
    QVERIFY(state->hasOnEnteringAction());

    // NONE actions are not stored
    state->addOnEnteringAction(model::ModelActionFactory::createModelAction(model::ModelAction::NONE));
    QCOMPARE(state->onEnteringActions().size(), 3);
}

void MultipleActionsTest::InsertRemoveMove() {
    model::EntityIdGenerator gen;
    auto state = makeState(gen, "S");

    state->addOnExitingAction(callback("a"));
    state->addOnExitingAction(callback("c"));
    state->insertOnExitingAction(1, callback("b"));

    QCOMPARE(state->onExitingActions().size(), 3);
    QCOMPARE(state->onExitingActions().at(1)->serialize(), QString("b"));

    state->moveOnExitingAction(2, 0);
    QCOMPARE(state->onExitingActions().at(0)->serialize(), QString("c"));

    state->removeOnExitingAction(0);
    QCOMPARE(state->onExitingActions().size(), 2);
    QCOMPARE(state->onExitingActions().at(0)->serialize(), QString("a"));
}

void MultipleActionsTest::SingleActionCompatSetters() {
    model::EntityIdGenerator gen;
    auto state = makeState(gen, "S");

    state->setOnEnteringAction(QString("enterCb"));
    QCOMPARE(state->onEnteringActions().size(), 1);
    QCOMPARE(state->onEnteringAction()->serialize(), QString("enterCb"));

    // setting again replaces the whole list with a single action
    state->setOnEnteringAction(QString("otherCb"));
    QCOMPARE(state->onEnteringActions().size(), 1);
    QCOMPARE(state->onEnteringAction()->serialize(), QString("otherCb"));

    // property-bag path accepts a full list
    model::ModelActionList list = {callback("x"), callback("y")};
    QVERIFY(state->setProperty("onEnteringAction", QVariant::fromValue(list)));
    QCOMPARE(state->onEnteringActions().size(), 2);
}

void MultipleActionsTest::DeepCopyIsIndependent() {
    model::EntityIdGenerator gen;
    auto original = makeState(gen, "S");
    original->addOnEnteringAction(callback("a"));
    original->addOnEnteringAction(callback("b"));

    auto copy = makeState(gen, "Copy");
    copy->copyEntityData(*original);

    QCOMPARE(copy->onEnteringActions().size(), 2);
    QCOMPARE(copy->onEnteringActions().at(0)->serialize(), QString("a"));

    // mutating the original must not affect the copy
    original->addOnEnteringAction(callback("c"));
    QCOMPARE(copy->onEnteringActions().size(), 2);

    // mutating an element on the copy must not affect the original
    copy->onEnteringActions().at(0)->setProperty("function", "changed");
    QCOMPARE(original->onEnteringActions().at(0)->serialize(), QString("a"));
}

void MultipleActionsTest::SerializeMultipleStateActions() {
    model::EntityIdGenerator gen;
    auto model = QSharedPointer<model::StateMachineModel>::create("M");
    auto state = makeState(gen, "S");
    state->addOnEnteringAction(callback("enterA"));
    state->addOnEnteringAction(callback("enterB"));
    state->addOnExitingAction(callback("exitA"));
    model->root()->addChildState(state);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model);

    // one <onentry> wrapper containing both scripts, in order
    QVERIFY(scxml.contains("<onentry>"));
    const int enterAPos = scxml.indexOf("enterA");
    const int enterBPos = scxml.indexOf("enterB");
    QVERIFY(enterAPos >= 0);
    QVERIFY(enterBPos > enterAPos);
    QVERIFY(scxml.contains("<onexit>"));
    QVERIFY(scxml.contains("exitA"));
}

void MultipleActionsTest::DeserializeMultipleStateActions() {
    const QString scxml = QStringLiteral(
        "<scxml version=\"1.0\" xmlns=\"http://www.w3.org/2005/07/scxml\" name=\"M\">"
        "<state id=\"S\">"
        "<onentry><script>a</script><script>b</script></onentry>"
        "<onexit><script>c</script></onexit>"
        "</state>"
        "</scxml>");

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);
    QVERIFY(model);

    auto state = model->root()->findChildStateByName("S").dynamicCast<model::RegularState>();
    QVERIFY(state);
    QCOMPARE(state->onEnteringActions().size(), 2);
    QCOMPARE(state->onEnteringActions().at(0)->serialize(), QString("a"));
    QCOMPARE(state->onEnteringActions().at(1)->serialize(), QString("b"));
    QCOMPARE(state->onExitingActions().size(), 1);
    QCOMPARE(state->onExitingActions().at(0)->serialize(), QString("c"));
}

void MultipleActionsTest::SerializeMultipleTransitionActions() {
    model::EntityIdGenerator gen;
    auto model = QSharedPointer<model::StateMachineModel>::create("M");
    auto s1 = makeState(gen, "S1");
    auto s2 = makeState(gen, "S2");
    model->root()->addChildState(s1);
    model->root()->addChildState(s2);

    auto tr = model::ModelElementsFactory::createTransitionWithId(s1, s2, "E", gen.generateNextId());
    tr->addTransitionAction(callback("act1"));
    tr->addTransitionAction(callback("act2"));
    s1->addTransition(tr);

    QCOMPARE(tr->transitionActions().size(), 2);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model);

    const int a1 = scxml.indexOf("act1");
    const int a2 = scxml.indexOf("act2");
    QVERIFY(a1 >= 0);
    QVERIFY(a2 > a1);
}

int runMultipleActionsTest(int argc, char** argv) {
    MultipleActionsTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "MultipleActionsTest.moc"
