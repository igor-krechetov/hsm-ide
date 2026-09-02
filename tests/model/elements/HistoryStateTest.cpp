#include <QtTest>

#include "model/private/EntityIdGenerator.hpp"
#include "model/elements/HistoryState.hpp"
#include "model/ModelElementsFactory.hpp"
#include "model/elements/RegularState.hpp"
#include "model/elements/Transition.hpp"

class HistoryStateTest : public QObject {
    Q_OBJECT

private slots:
    void DefaultTransitionAndProperties();
};

/**
 * @brief Verify history state stores a default transition and history type property.
 *
 * Use-case: SCXML history node has default fallback transition and configurable depth.
 */
void HistoryStateTest::DefaultTransitionAndProperties() {
    model::EntityIdGenerator gen;
    auto source = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                      .dynamicCast<model::RegularState>();
    source->setName("S");
    auto target = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                      .dynamicCast<model::RegularState>();
    target->setName("T");
    auto history = model::ModelElementsFactory::createUniqueState(model::StateType::HISTORY, gen)
                       .dynamicCast<model::HistoryState>();
    auto tr = model::ModelElementsFactory::createTransitionWithId(source, target, "resume", gen.generateNextId());

    QVERIFY(history->addChild(tr));
    QCOMPARE(tr, history->defaultTransition());

    QVERIFY(history->setProperty(model::HistoryState::cKeyHistoryType, static_cast<int>(model::HistoryType::DEEP)));
    QCOMPARE(model::HistoryType::DEEP, history->historyType());

    history->deleteChild(tr->id());
    QCOMPARE(QSharedPointer<model::Transition>(), history->defaultTransition());
}

int runHistoryStateTest(int argc, char** argv) {
    HistoryStateTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "HistoryStateTest.moc"
