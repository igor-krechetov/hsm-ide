#include <QtTest>

#include "model/ExitPoint.hpp"

class ExitPointTest : public QObject {
    Q_OBJECT

private slots:
    void EventAndCallbackProperties();
};

/**
 * @brief Verify exit point event/callback properties.
 *
 * Use-case: Exit point emits auto event and lifecycle callbacks.
 */
void ExitPointTest::EventAndCallbackProperties() {
    auto exit = QSharedPointer<model::ExitPoint>::create("XP");

    QVERIFY(exit->setProperty("event", "done"));
    QVERIFY(exit->setProperty("onEnteringCallback", "inCb"));
    QVERIFY(exit->setProperty("onExitingCallback", "outCb"));

    QCOMPARE(QString("done"), exit->event());
    QCOMPARE(QString("inCb"), exit->onEnteringCallback());
    QCOMPARE(QString("outCb"), exit->onExitingCallback());
}

int runExitPointTest(int argc, char** argv) {
    ExitPointTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "ExitPointTest.moc"
