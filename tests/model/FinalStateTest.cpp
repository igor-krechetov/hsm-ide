#include <QtTest>

#include "model/FinalState.hpp"

class FinalStateTest : public QObject {
    Q_OBJECT

private slots:
    void CallbackPropertyRoundTrip();
};

void FinalStateTest::CallbackPropertyRoundTrip() {
    auto state = QSharedPointer<model::FinalState>::create("Done");

    QVERIFY(state->setProperty("onStateChangedAction", "completeCb"));
    QVERIFY(state->hasOnStateChangedAction());
    QCOMPARE(QString("completeCb"), state->onStateChangedAction()->serialize());
    QCOMPARE(QString("completeCb"),
             state->getProperty("onStateChangedAction").value<QSharedPointer<model::IModelAction>>()->serialize());
}

int runFinalStateTest(int argc, char** argv) {
    FinalStateTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "FinalStateTest.moc"
