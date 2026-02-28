#include <QtTest>

#include "model/RegularState.hpp"
#include "model/SelfTransition.hpp"

class SelfTransitionTest : public QObject {
    Q_OBJECT

private slots:
    void BasicBehavior();
};

/**
 * @brief Verify self-transition keeps same source/target and supports type mutation.
 *
 * Use-case: State loops back to itself with configurable transition kind.
 */
void SelfTransitionTest::BasicBehavior() {
    auto state = QSharedPointer<model::RegularState>::create("S");
    auto selfTr = QSharedPointer<model::SelfTransition>::create(state, "tick");

    QVERIFY(selfTr);
    QCOMPARE(state->id(), selfTr->sourceId());
    QCOMPARE(state->id(), selfTr->targetId());

    selfTr->setType(model::TransitionType::INTERNAL);
    QCOMPARE(model::TransitionType::INTERNAL, selfTr->type());
}

int runSelfTransitionTest(int argc, char** argv) {
    SelfTransitionTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "SelfTransitionTest.moc"
