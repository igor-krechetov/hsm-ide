#include <QtTest>

#include "model/EntryPoint.hpp"
#include "model/InitialState.hpp"
#include "model/ModelElementsFactory.hpp"
#include "model/RegularState.hpp"

class ModelElementsFactoryTest : public QObject {
    Q_OBJECT

private slots:
    void CreatesExpectedElements();
};

/**
 * @brief Verify factory creates unique states and source-owned transitions.
 *
 * Use-case: Editor creates new nodes and auto-attaches transitions.
 */
void ModelElementsFactoryTest::CreatesExpectedElements() {
    auto s1 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR).dynamicCast<model::RegularState>();
    auto s2 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR).dynamicCast<model::RegularState>();

    QVERIFY(s1);
    QVERIFY(s2);
    QVERIFY(s1->name() != s2->name());

    auto tr = model::ModelElementsFactory::createUniqueTransition(s1, s2);
    QVERIFY(tr);
    QCOMPARE(QString("NEW_EVENT"), tr->event());
    QCOMPARE(s1, tr->source());
    QCOMPARE(s2, tr->target());

    auto initial = model::ModelElementsFactory::createUniqueState(model::StateType::INITIAL).dynamicCast<model::InitialState>();
    QVERIFY(initial);

    auto trFromInitial = model::ModelElementsFactory::createUniqueTransition(initial, s1);
    QVERIFY(trFromInitial);
    QCOMPARE(QString(""), trFromInitial->event());

    auto entry = model::ModelElementsFactory::createUniqueState(model::StateType::ENTRYPOINT).dynamicCast<model::EntryPoint>();
    QVERIFY(entry);

    auto trFromEntry = model::ModelElementsFactory::createUniqueTransition(entry, s2);
    QVERIFY(trFromEntry);
    QCOMPARE(QString(""), trFromEntry->event());
}

int runModelElementsFactoryTest(int argc, char** argv) {
    ModelElementsFactoryTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "ModelElementsFactoryTest.moc"
