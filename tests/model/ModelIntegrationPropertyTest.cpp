#include <QtTest>

#include <QRandomGenerator>

#include "model/ModelElementsFactory.hpp"
#include "model/elements/ModelRootState.hpp"
#include "model/elements/RegularState.hpp"
#include "model/StateMachineModel.hpp"

class ModelIntegrationPropertyTest : public QObject {
    Q_OBJECT

private slots:
    void PropertyUidStabilityAcrossEdits();
    void PropertyCopyPasteAssignsNewUids();
};

/**
 * @brief Property 2: UID Stability Across Edits.
 *
 * For any entity in the model, performing rename (changing SCXML id),
 * reparenting (moving to a different parent state), or transition reconnection
 * operations SHALL NOT change the entity's UID.
 *
 * Validates: Requirements 1.3, 1.4, 1.5, 9.2, 9.3
 */
void ModelIntegrationPropertyTest::PropertyUidStabilityAcrossEdits() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 100; ++iteration) {
        auto model = QSharedPointer<model::StateMachineModel>::create("TestModel");
        auto root = model->root();

        // Create a random number of states (3..10)
        const int stateCount = static_cast<int>(rng.bounded(3, 11));
        QList<QSharedPointer<model::RegularState>> states;

        for (int i = 0; i < stateCount; ++i) {
            auto state = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                             .dynamicCast<model::RegularState>();
            QVERIFY(state);
            root->addChildState(state);
            states.append(state);
        }

        // Create some transitions between random pairs of states
        const int transitionCount = static_cast<int>(rng.bounded(1, stateCount));
        QList<QSharedPointer<model::Transition>> transitions;

        for (int i = 0; i < transitionCount; ++i) {
            const int srcIdx = static_cast<int>(rng.bounded(static_cast<quint32>(states.size())));
            int dstIdx = static_cast<int>(rng.bounded(static_cast<quint32>(states.size())));

            if (dstIdx == srcIdx) {
                dstIdx = (dstIdx + 1) % states.size();
            }

            auto tr = model::ModelElementsFactory::createUniqueTransition(states[srcIdx], states[dstIdx], model->idGenerator());

            if (tr) {
                transitions.append(tr);
            }
        }

        // Record all entity IDs before edits
        QMap<model::EntityID_t, QString> stateIdsBefore;

        for (const auto& state : states) {
            stateIdsBefore.insert(state->id(), state->name());
        }

        QMap<model::EntityID_t, QPair<model::EntityID_t, model::EntityID_t>> transitionIdsBefore;

        for (const auto& tr : transitions) {
            transitionIdsBefore.insert(tr->id(), {tr->sourceId(), tr->targetId()});
        }

        // Perform random edits
        const int editCount = static_cast<int>(rng.bounded(3, 10));

        for (int e = 0; e < editCount; ++e) {
            const int editType = static_cast<int>(rng.bounded(3));

            switch (editType) {
                case 0: {
                    // Rename a random state
                    const int idx = static_cast<int>(rng.bounded(static_cast<quint32>(states.size())));
                    states[idx]->setName(QString("Renamed_%1_%2").arg(iteration).arg(e));
                    break;
                }
                case 1: {
                    // Move a state to a different parent
                    if (states.size() >= 2) {
                        const int childIdx = static_cast<int>(rng.bounded(static_cast<quint32>(states.size())));
                        int parentIdx = static_cast<int>(rng.bounded(static_cast<quint32>(states.size())));

                        if (parentIdx == childIdx) {
                            parentIdx = (parentIdx + 1) % states.size();
                        }

                        // Only move if the target isn't already a descendant of the source
                        // (to avoid cycles). Simple check: don't move if parent is a child of child.
                        auto childState = states[childIdx];
                        auto parentState = states[parentIdx];

                        // Verify the child isn't an ancestor of the target parent
                        bool isSafe = (childState->findChild(parentState->id()) == nullptr);

                        if (isSafe) {
                            model->moveElement(childState->id(), parentState->id());
                        }
                    }
                    break;
                }
                case 2: {
                    // Reconnect a transition to different source/target
                    if (!transitions.isEmpty() && states.size() >= 2) {
                        const int trIdx = static_cast<int>(rng.bounded(static_cast<quint32>(transitions.size())));
                        const int newSrcIdx = static_cast<int>(rng.bounded(static_cast<quint32>(states.size())));
                        int newDstIdx = static_cast<int>(rng.bounded(static_cast<quint32>(states.size())));

                        if (newDstIdx == newSrcIdx) {
                            newDstIdx = (newDstIdx + 1) % states.size();
                        }

                        model->reconnectElements(transitions[trIdx]->id(), states[newSrcIdx]->id(),
                                                 states[newDstIdx]->id());
                    }
                    break;
                }
                default:
                    break;
            }
        }

        // Verify all surviving states still have the same IDs
        for (const auto& state : states) {
            QVERIFY2(stateIdsBefore.contains(state->id()),
                     qPrintable(QString("Iteration %1: state '%2' changed its ID")
                                    .arg(iteration)
                                    .arg(state->name())));
        }

        // Verify all transitions still have the same IDs
        for (const auto& tr : transitions) {
            QVERIFY2(transitionIdsBefore.contains(tr->id()),
                     qPrintable(QString("Iteration %1: transition changed its ID to %2")
                                    .arg(iteration)
                                    .arg(tr->id())));
        }
    }
}

/**
 * @brief Property 14: Copy/Paste Assigns New Unique UIDs.
 *
 * For any entity duplication (copy/paste) operation, the duplicated entity
 * SHALL receive a new UID that differs from all UIDs currently assigned or
 * previously assigned in the model.
 *
 * Validates: Requirements 9.4
 */
void ModelIntegrationPropertyTest::PropertyCopyPasteAssignsNewUids() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 100; ++iteration) {
        auto model = QSharedPointer<model::StateMachineModel>::create("TestModel");
        auto root = model->root();

        // Create a random number of states (2..8)
        const int stateCount = static_cast<int>(rng.bounded(2, 9));
        QList<QSharedPointer<model::State>> states;
        QSet<model::EntityID_t> allIds;

        for (int i = 0; i < stateCount; ++i) {
            auto state = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator());
            QVERIFY(state);
            root->addChildState(state);
            states.append(state);
            allIds.insert(state->id());
        }

        // Clone a random number of states and verify new unique UIDs
        const int cloneCount = static_cast<int>(rng.bounded(1, stateCount + 1));

        for (int c = 0; c < cloneCount; ++c) {
            const int sourceIdx = static_cast<int>(rng.bounded(static_cast<quint32>(states.size())));
            auto sourceState = states[sourceIdx];

            auto clonedState = model::ModelElementsFactory::cloneStateEntity(sourceState, model->idGenerator());
            QVERIFY2(clonedState, qPrintable(QString("Iteration %1: cloneStateEntity returned null").arg(iteration)));

            // Verify: cloned entity's ID is different from the source
            QVERIFY2(clonedState->id() != sourceState->id(),
                     qPrintable(QString("Iteration %1: cloned state has same ID %2 as source")
                                    .arg(iteration)
                                    .arg(clonedState->id())));

            // Verify: cloned entity's ID doesn't collide with any existing entity in the model
            QVERIFY2(!allIds.contains(clonedState->id()),
                     qPrintable(QString("Iteration %1: cloned state ID %2 collides with existing entity")
                                    .arg(iteration)
                                    .arg(clonedState->id())));

            // Verify: cloned entity's ID is valid
            QVERIFY2(clonedState->id() >= 1,
                     qPrintable(QString("Iteration %1: cloned state ID %2 below valid range")
                                    .arg(iteration)
                                    .arg(clonedState->id())));
            QVERIFY2(clonedState->id() <= 0xFFFFFFFE,
                     qPrintable(QString("Iteration %1: cloned state ID %2 above valid range")
                                    .arg(iteration)
                                    .arg(clonedState->id())));
            QVERIFY2(clonedState->id() != model::INVALID_MODEL_ID,
                     qPrintable(QString("Iteration %1: cloned state ID equals INVALID_MODEL_ID").arg(iteration)));

            allIds.insert(clonedState->id());
        }
    }
}

int runModelIntegrationPropertyTest(int argc, char** argv) {
    ModelIntegrationPropertyTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "ModelIntegrationPropertyTest.moc"
