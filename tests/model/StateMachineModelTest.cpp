#include <QSignalSpy>
#include <QtTest>

#include "model/ModelElementsFactory.hpp"
#include "model/ModelRootState.hpp"
#include "model/RegularState.hpp"
#include "model/StateMachineModel.hpp"
#include "model/Transition.hpp"

class StateMachineModelTest : public QObject {
    Q_OBJECT

private slots:
    void MoveAndReconnectElements();
    void CloneTransitionsUsesNewStateReferences();
    void ReparentedStateChildAddedIsEmittedOnce();
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

void StateMachineModelTest::CloneTransitionsUsesNewStateReferences() {
    auto sourceModel = QSharedPointer<model::StateMachineModel>::create("Source");
    auto sourceRoot = sourceModel->root();

    auto sourceA = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR).dynamicCast<model::RegularState>();
    auto sourceB = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR).dynamicCast<model::RegularState>();

    sourceRoot->addChildState(sourceA);
    sourceRoot->addChildState(sourceB);

    auto sourceTransition = model::ModelElementsFactory::createUniqueTransition(sourceA, sourceB);
    QVERIFY(sourceTransition);

    model::StateMachineModel destinationModel("Destination");
    destinationModel = *sourceModel;

    auto destinationA = destinationModel.root()->findChildStateByName(sourceA->name());
    auto destinationB = destinationModel.root()->findChildStateByName(sourceB->name());
    QSharedPointer<model::Transition> destinationTransition;

    if (destinationA) {
        destinationA->forEachChildElement(
            [&destinationTransition, &destinationA](QSharedPointer<model::StateMachineEntity> parent,
                                                    QSharedPointer<model::StateMachineEntity> entity) {
                bool continueTraversal = true;

                if (parent && entity && (parent == destinationA) &&
                    (entity->type() == model::StateMachineEntity::Type::Transition)) {
                    destinationTransition = entity.dynamicCast<model::Transition>();
                    continueTraversal = false;
                }

                return continueTraversal;
            },
            1,
            false);
    }

    QVERIFY(destinationA);
    QVERIFY(destinationB);
    QVERIFY(destinationTransition);

    QCOMPARE(destinationTransition->source().data(), destinationA.data());
    QCOMPARE(destinationTransition->target().data(), destinationB.data());
    QVERIFY(destinationTransition->source().data() != sourceA.data());
    QVERIFY(destinationTransition->target().data() != sourceB.data());
}

void StateMachineModelTest::ReparentedStateChildAddedIsEmittedOnce() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();

    auto topLevelParent =
        model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR).dynamicCast<model::RegularState>();
    auto childState =
        model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR).dynamicCast<model::RegularState>();

    root->addChildState(topLevelParent);
    root->addChildState(childState);

    QVERIFY(model->moveElement(childState->id(), topLevelParent->id()));

    QSignalSpy addedSpy(model.get(), &model::StateMachineModel::modelEntityAdded);
    auto selfTransition = model::ModelElementsFactory::createUniqueTransition(childState, childState);

    QVERIFY(selfTransition);
    QCOMPARE(addedSpy.count(), 1);
}

int runStateMachineModelTest(int argc, char** argv) {
    StateMachineModelTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "StateMachineModelTest.moc"
