#include <QSet>
#include <QSignalSpy>
#include <QtTest>

#include "model/ModelElementsFactory.hpp"
#include "model/ModelUtils.hpp"
#include "model/StateMachineModel.hpp"
#include "model/elements/ModelRootState.hpp"
#include "model/elements/RegularState.hpp"
#include "model/elements/Transition.hpp"

class StateMachineModelTest : public QObject {
    Q_OBJECT

private slots:
    void MoveAndReconnectElements();
    void CloneTransitionsUsesNewStateReferences();
    void ReparentedStateChildAddedIsEmittedOnce();
    void GenerateUniqueName();
    void ReassignImportedIdsGivesUniqueIds();
    void ReassignImportedIdsPreservesTransitionLinks();
};

/**
 * @brief Verify model move and reconnect operations for graph editing.
 *
 * Use-case: User drags states between parents and reconnects transitions.
 */
void StateMachineModelTest::MoveAndReconnectElements() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();

    auto a = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                 .dynamicCast<model::RegularState>();
    auto b = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                 .dynamicCast<model::RegularState>();
    auto parent = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                      .dynamicCast<model::RegularState>();

    root->addChildState(a);
    root->addChildState(b);
    root->addChildState(parent);

    auto tr = model::ModelElementsFactory::createUniqueTransition(a, b, model->idGenerator());
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

    auto sourceA = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, sourceModel->idGenerator())
                       .dynamicCast<model::RegularState>();
    auto sourceB = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, sourceModel->idGenerator())
                       .dynamicCast<model::RegularState>();

    sourceRoot->addChildState(sourceA);
    sourceRoot->addChildState(sourceB);

    auto sourceTransition = model::ModelElementsFactory::createUniqueTransition(sourceA, sourceB, sourceModel->idGenerator());
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

    auto topLevelParent = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                              .dynamicCast<model::RegularState>();
    auto childState = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                          .dynamicCast<model::RegularState>();

    root->addChildState(topLevelParent);
    root->addChildState(childState);

    QVERIFY(model->moveElement(childState->id(), topLevelParent->id()));

    QSignalSpy addedSpy(model.get(), &model::StateMachineModel::modelEntityAdded);
    auto selfTransition = model::ModelElementsFactory::createUniqueTransition(childState, childState, model->idGenerator());

    QVERIFY(selfTransition);
    QCOMPARE(addedSpy.count(), 1);
}

/**
 * @brief REQ-103f7: generateUniqueName returns free names unchanged and appends _copy on collision.
 *
 * Used to auto-uniquify generated names on create/drop.
 */
void StateMachineModelTest::GenerateUniqueName() {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();

    auto existing = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                        .dynamicCast<model::RegularState>();
    existing->setName("State");
    root->addChildState(existing);

    // A name not present in the model is returned unchanged.
    QCOMPARE(model->generateUniqueName("Fresh"), QString("Fresh"));

    // A colliding name gets a _copy suffix.
    QCOMPARE(model->generateUniqueName("State"), QString("State_copy"));

    // With both taken, another suffix is appended.
    auto copy = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                    .dynamicCast<model::RegularState>();
    copy->setName("State_copy");
    root->addChildState(copy);

    QCOMPARE(model->generateUniqueName("State"), QString("State_copy_copy"));
}

/**
 * @brief REQ-103f7: reassignImportedIds re-mints every id in an imported subtree from the
 *        destination model's generator, so repeated pastes of the same clipboard never
 *        produce colliding ids.
 *
 * Reproduces the paste-twice bug: each clipboard paste deserializes into a fresh model whose
 * id generator restarts from 1, so two pastes of the same copied state both arrive carrying
 * the same low id. Without re-iding against the destination, the two entities collide and are
 * treated as a single entity. After reassignImportedIds each subtree gets fresh, model-unique
 * ids.
 */
void StateMachineModelTest::ReassignImportedIdsGivesUniqueIds() {
    auto destination = QSharedPointer<model::StateMachineModel>::create("Destination");
    auto root = destination->root();

    // An existing state already in the destination (occupies an id from the destination generator).
    auto existing = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, destination->idGenerator())
                        .dynamicCast<model::RegularState>();
    existing->setName("Existing");
    root->addChildState(existing);

    // Build two independent "clipboard" subtrees, each minted from its OWN fresh generator so
    // both restart at id 1 - exactly what two pastes of the same copied state produce.
    auto makeClipboardState = [](const QString& name) {
        model::EntityIdGenerator clipboardGenerator;
        auto state = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, clipboardGenerator)
                         .dynamicCast<model::RegularState>();
        state->setName(name);
        return state;
    };

    auto firstPaste = makeClipboardState("Pasted");
    auto secondPaste = makeClipboardState("Pasted");

    // Sanity: both clipboard states carry the same (colliding) id before re-iding.
    QCOMPARE(firstPaste->id(), secondPaste->id());

    // Merge both into the destination the way paste does.
    model::reassignImportedIds(firstPaste, destination->idGenerator());
    root->addChildState(firstPaste);

    model::reassignImportedIds(secondPaste, destination->idGenerator());
    root->addChildState(secondPaste);

    // Collect every id now in the destination model and assert there are no duplicates.
    QList<model::EntityID_t> allIds;
    allIds.push_back(root->id());

    root->forEachChildElement(
        [&allIds](QSharedPointer<model::StateMachineEntity> parent, QSharedPointer<model::StateMachineEntity> entity) {
            Q_UNUSED(parent);

            if (entity) {
                allIds.push_back(entity->id());
            }

            return true;
        },
        model::StateMachineEntity::DEPTH_INFINITE,
        false);

    const QSet<model::EntityID_t> uniqueIds(allIds.begin(), allIds.end());
    QCOMPARE(uniqueIds.size(), allIds.size());

    // The two pasted states are now distinct entities.
    QVERIFY(firstPaste->id() != secondPaste->id());
}

/**
 * @brief REQ-103f7: reassignImportedIds re-ids a subtree's states and transitions while keeping
 *        each transition linked to the same (now re-ided) source/target states.
 *
 * Transitions hold shared pointers to their endpoints, so re-iding the states must transparently
 * update the transitions' sourceId()/targetId().
 */
void StateMachineModelTest::ReassignImportedIdsPreservesTransitionLinks() {
    auto destination = QSharedPointer<model::StateMachineModel>::create("Destination");

    // Build a clipboard subtree: parent with two child states and a transition between them,
    // all minted from a fresh (clipboard) generator that collides with the destination.
    model::EntityIdGenerator clipboardGenerator;
    auto parent = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, clipboardGenerator)
                      .dynamicCast<model::RegularState>();
    auto childA = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, clipboardGenerator)
                      .dynamicCast<model::RegularState>();
    auto childB = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, clipboardGenerator)
                      .dynamicCast<model::RegularState>();
    parent->addChildState(childA);
    parent->addChildState(childB);

    auto transition = model::ModelElementsFactory::createUniqueTransition(childA, childB, clipboardGenerator);
    QVERIFY(transition);

    // Re-id the whole subtree against the destination generator.
    model::reassignImportedIds(parent, destination->idGenerator());
    destination->root()->addChildState(parent);

    // The transition still points to the same state objects, with their new ids.
    QCOMPARE(transition->source().data(), childA.data());
    QCOMPARE(transition->target().data(), childB.data());
    QCOMPARE(transition->sourceId(), childA->id());
    QCOMPARE(transition->targetId(), childB->id());

    // Every id in the merged subtree is unique.
    QList<model::EntityID_t> ids = {parent->id(), childA->id(), childB->id(), transition->id()};
    const QSet<model::EntityID_t> uniqueIds(ids.begin(), ids.end());
    QCOMPARE(uniqueIds.size(), ids.size());
}

int runStateMachineModelTest(int argc, char** argv) {
    StateMachineModelTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "StateMachineModelTest.moc"
