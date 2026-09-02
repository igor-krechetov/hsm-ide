#include <QtTest>

#include "model/private/EntityIdGenerator.hpp"
#include "model/ModelElementsFactory.hpp"
#include "model/elements/RegularState.hpp"
#include "model/elements/Transition.hpp"

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
    model::EntityIdGenerator gen;
    auto s1 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                  .dynamicCast<model::RegularState>();
    s1->setName("S1");
    auto s2 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                  .dynamicCast<model::RegularState>();
    s2->setName("S2");
    auto s3 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                  .dynamicCast<model::RegularState>();
    s3->setName("S3");
    auto tr = model::ModelElementsFactory::createTransitionWithId(s1, s2, "E", gen.generateNextId());

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
