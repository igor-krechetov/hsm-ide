#include <QtTest>

#include "model/private/EntityIdGenerator.hpp"
#include "model/elements/InitialState.hpp"
#include "model/ModelElementsFactory.hpp"
#include "model/elements/RegularState.hpp"
#include "model/elements/Transition.hpp"

class InitialStateTest : public QObject {
    Q_OBJECT

private slots:
    void StoresSingleTransition();
};

/**
 * @brief Verify initial state keeps a single outgoing transition.
 *
 * Use-case: Initial pseudostate in SCXML points to startup target.
 */
void InitialStateTest::StoresSingleTransition() {
    model::EntityIdGenerator gen;
    auto root = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                    .dynamicCast<model::RegularState>();
    root->setName("Root");
    auto target = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, gen)
                      .dynamicCast<model::RegularState>();
    target->setName("S1");
    auto initial = model::ModelElementsFactory::createUniqueState(model::StateType::INITIAL, gen)
                       .dynamicCast<model::InitialState>();
    auto tr = model::ModelElementsFactory::createTransitionWithId(root, target, "", gen.generateNextId());

    QVERIFY(initial->addChild(tr));
    QCOMPARE(tr, initial->transition());

    initial->deleteDirectChild(tr);
    QCOMPARE(QSharedPointer<model::Transition>(), initial->transition());
    QVERIFY(initial->properties().isEmpty());
}

int runInitialStateTest(int argc, char** argv) {
    InitialStateTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "InitialStateTest.moc"
