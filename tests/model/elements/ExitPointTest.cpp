#include <QtTest>

#include "model/actions/ModelActionFactory.hpp"
#include "model/elements/ExitPoint.hpp"

class ExitPointTest : public QObject {
    Q_OBJECT

private slots:
    void EventAndCallbackProperties();
    void MultipleEntryExitActions();
};

void ExitPointTest::EventAndCallbackProperties() {
    auto exit = QSharedPointer<model::ExitPoint>::create("XP");

    QVERIFY(exit->setProperty("event", "done"));
    QVERIFY(exit->setProperty("onEnteringAction", "inCb"));
    QVERIFY(exit->setProperty("onExitingAction", "outCb"));

    QCOMPARE(QString("done"), exit->event());
    QCOMPARE(QString("inCb"), exit->onEnteringAction()->serialize());
    QCOMPARE(QString("outCb"), exit->onExitingAction()->serialize());
}

void ExitPointTest::MultipleEntryExitActions() {
    auto exit = QSharedPointer<model::ExitPoint>::create("XP");

    auto a = model::ModelActionFactory::createModelAction(model::ModelAction::CALLBACK);
    a->setProperty("function", "a");
    auto b = model::ModelActionFactory::createModelAction(model::ModelAction::CALLBACK);
    b->setProperty("function", "b");

    exit->addOnEnteringAction(a);
    exit->addOnEnteringAction(b);

    QCOMPARE(exit->onEnteringActions().size(), 2);
    QCOMPARE(exit->onEnteringActions().at(0)->serialize(), QString("a"));
    QCOMPARE(exit->onEnteringActions().at(1)->serialize(), QString("b"));

    // onStateChanged remains single-action
    QVERIFY(exit->setProperty("onStateChangedAction", "stateCb"));
    QCOMPARE(QString("stateCb"), exit->onStateChangedAction()->serialize());
}

int runExitPointTest(int argc, char** argv) {
    ExitPointTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "ExitPointTest.moc"
