#include <QtTest>

#include "model/ExitPoint.hpp"

class ExitPointTest : public QObject {
    Q_OBJECT

private slots:
    void EventAndCallbackProperties();
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

int runExitPointTest(int argc, char** argv) {
    ExitPointTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "ExitPointTest.moc"
