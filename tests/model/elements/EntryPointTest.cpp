#include <QtTest>

#include "model/private/EntityIdGenerator.hpp"
#include "model/elements/EntryPoint.hpp"
#include "model/ModelElementsFactory.hpp"
#include "model/elements/RegularState.hpp"
#include "model/elements/Transition.hpp"

class EntryPointTest : public QObject {
    Q_OBJECT

private slots:
    void ManagesOutgoingTransitions();
};

/**
 * @brief Verify entry point supports multiple transitions and lookup APIs.
 *
 * Use-case: Entry point dispatches to different targets based on event/guards.
 */
void EntryPointTest::ManagesOutgoingTransitions() {
    model::EntityIdGenerator gen;
    auto source = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                      .dynamicCast<model::RegularState>();
    source->setName("Src");
    auto targetA = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                       .dynamicCast<model::RegularState>();
    targetA->setName("A");
    auto targetB = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                       .dynamicCast<model::RegularState>();
    targetB->setName("B");
    auto entry = model::ModelElementsFactory::createUniqueState(model::StateType::ENTRYPOINT, gen)
                     .dynamicCast<model::EntryPoint>();

    auto t1 = model::ModelElementsFactory::createTransitionWithId(source, targetA, "E1", gen.generateNextId());
    auto t2 = model::ModelElementsFactory::createTransitionWithId(source, targetB, "E2", gen.generateNextId());

    QVERIFY(entry->addChild(t1));
    QVERIFY(entry->addChild(t2));
    QCOMPARE(2, entry->transitions().size());
    QCOMPARE(entry, entry->findParentState(t1->id()));
    QCOMPARE(t2, entry->findChild(t2->id()).dynamicCast<model::Transition>());

    entry->deleteChild(t1->id());
    QCOMPARE(1, entry->transitions().size());
}

int runEntryPointTest(int argc, char** argv) {
    EntryPointTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "EntryPointTest.moc"
