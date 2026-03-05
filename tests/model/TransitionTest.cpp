#include <QtTest>

#include "model/RegularState.hpp"
#include "model/Transition.hpp"

class TransitionTest : public QObject {
    Q_OBJECT

private slots:
    void PropertiesAndSourceTargetManagement();
};

/**
 * @brief Verify transition attributes and property keys, including guards.
 *
 * Use-case: Transition stores event, callback and guard condition values.
 */
void TransitionTest::PropertiesAndSourceTargetManagement() {
    auto s1 = QSharedPointer<model::RegularState>::create("S1");
    auto s2 = QSharedPointer<model::RegularState>::create("S2");
    auto s3 = QSharedPointer<model::RegularState>::create("S3");
    auto tr = QSharedPointer<model::Transition>::create(s1, s2, "E");

    QVERIFY(tr->setProperty("event", "E2"));
    QVERIFY(tr->setProperty("conditionCallback", "isReady"));
    QVERIFY(tr->setProperty(model::Transition::cKeyExpectedConditionValue, true));
    QVERIFY(tr->setProperty(model::Transition::cKeyTransitionType, static_cast<int>(model::TransitionType::INTERNAL)));

    QCOMPARE(QString("E2"), tr->event());
    QCOMPARE(QString("isReady"), tr->conditionCallback());
    QVERIFY(tr->expectedConditionValue());
    QCOMPARE(model::TransitionType::INTERNAL, tr->transitionType());

    tr->setTarget(s3);
    QCOMPARE(s3->id(), tr->targetId());
    QCOMPARE(s1->id(), tr->sourceId());
}

int runTransitionTest(int argc, char** argv) {
    TransitionTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "TransitionTest.moc"
