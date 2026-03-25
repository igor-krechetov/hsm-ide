#include <QtTest/QtTest>

#include "model/actions/ModelActionFactory.hpp"

using namespace model;

class ModelActionsTest : public QObject {
    Q_OBJECT

private slots:
    void CreateAndSerializeActions();
    void ParseActionStrings();
};

void ModelActionsTest::CreateAndSerializeActions() {
    auto startAction = createModelAction(ModelAction::TIMER_START);
    QVERIFY(startAction);
    QVERIFY(startAction->setProperty("timer_id", "T1"));
    QVERIFY(startAction->setProperty("interval", "1000"));
    QVERIFY(startAction->setProperty("singleshot", "true"));
    QCOMPARE(startAction->serialize(), QString("start_timer(T1, 1000, true)"));

    auto transitionAction = createModelAction(ModelAction::SEND_EVENT);
    QVERIFY(transitionAction);
    QVERIFY(transitionAction->setProperty("event_id", "E1"));
    QVERIFY(transitionAction->setProperty("arguments", "arg1"));
    QCOMPARE(transitionAction->serialize(), QString("transition(E1, arg1)"));
}

void ModelActionsTest::ParseActionStrings() {
    auto restartAction = createModelActionFromData("restart_timer(timer42)", ModelAction::CALLBACK);
    QVERIFY(restartAction);
    QCOMPARE(restartAction->type(), ModelAction::TIMER_RESTART);
    QCOMPARE(restartAction->getProperty("timer_id").toString(), QString("timer42"));

    auto callbackAction = createModelActionFromData("myCallback", ModelAction::CALLBACK);
    QVERIFY(callbackAction);
    QCOMPARE(callbackAction->type(), ModelAction::CALLBACK);
    QCOMPARE(callbackAction->getProperty("function").toString(), QString("myCallback"));
}

int runModelActionsTest(int argc, char** argv) {
    ModelActionsTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "ModelActionsTest.moc"
