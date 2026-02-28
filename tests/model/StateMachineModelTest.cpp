#include <QtTest>

#include "model/ModelElementsFactory.hpp"
#include "model/ModelRootState.hpp"
#include "model/RegularState.hpp"
#include "model/StateMachineModel.hpp"

class StateMachineModelTest : public QObject {
    Q_OBJECT

private slots:
    void MoveAndReconnectElements();
};

/**
 * @brief Verify model move and reconnect operations for graph editing.
 *
 * Use-case: User drags states between parents and reconnects transitions.
 */
void StateMachineModelTest::MoveAndReconnectElements() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();

    auto a = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR).dynamicCast<model::RegularState>();
    auto b = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR).dynamicCast<model::RegularState>();
    auto parent = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR).dynamicCast<model::RegularState>();

    root->addChildState(a);
    root->addChildState(b);
    root->addChildState(parent);

    auto tr = model::ModelElementsFactory::createUniqueTransition(a, b);
    QVERIFY(tr);

    QVERIFY(model->moveElement(b->id(), parent->id()));
    QCOMPARE(parent, root->findParentState(b->id()).dynamicCast<model::RegularState>());

    QVERIFY(model->reconnectElements(tr->id(), parent->id(), a->id()));
    QCOMPARE(parent->id(), tr->sourceId());
    QCOMPARE(a->id(), tr->targetId());
}

int runStateMachineModelTest(int argc, char** argv) {
    StateMachineModelTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "StateMachineModelTest.moc"
