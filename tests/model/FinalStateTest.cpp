#include <QtTest>

#include "model/FinalState.hpp"

class FinalStateTest : public QObject {
    Q_OBJECT

private slots:
    void CallbackPropertyRoundTrip();
};

/**
 * @brief Verify final state callback property behavior.
 *
 * Use-case: Configure completion callback for final state entry.
 */
void FinalStateTest::CallbackPropertyRoundTrip() {
    auto state = QSharedPointer<model::FinalState>::create("Done");

    QVERIFY(state->setProperty("onStateChangedCallback", "completeCb"));
    QCOMPARE(QString("completeCb"), state->onStateChangedCallback());
    QCOMPARE(QString("completeCb"), state->getProperty("onStateChangedCallback").toString());
}

int runFinalStateTest(int argc, char** argv) {
    FinalStateTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "FinalStateTest.moc"
