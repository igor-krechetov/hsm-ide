#include <QApplication>
#include <QGraphicsScene>
#include <QList>
#include <QMap>
#include <QPointF>
#include <QPolygonF>
#include <QRandomGenerator>
#include <QSharedPointer>
#include <QSizeF>
#include <QtTest>

#include "model/ModelElementsFactory.hpp"
#include "model/StateMachineModel.hpp"
#include "model/elements/ModelRootState.hpp"
#include "model/elements/RegularState.hpp"
#include "model/elements/Transition.hpp"
#include "model/private/EntityIdGenerator.hpp"
#include "view/elements/HsmTransition.hpp"
#include "view/widgets/HsmGraphicsView.hpp"

using namespace view;

// ---------------------------------------------------------------------------
// Property 1: Bug Condition - Exactly One Graphics Item Per Transition.
//
// This is a BUG CONDITION EXPLORATION test. It encodes the Expected Behavior
// (design Property 1): for any view rebuild of a model that contains one or more
// source/target-complete transitions (isBugCondition == true), the rebuild must
// leave EXACTLY ONE view::HsmTransition graphics item on the scene per transition
// id, with no orphaned transition items.
//
// On the UNFIXED code this test is EXPECTED TO FAIL: a single transition entity
// ends up producing two view::HsmTransition items on the scene (the element map
// is overwritten to point at the second, the first is orphaned), so the per-id
// count is 2 instead of 1.
//
// The test reproduces the controller rebuild path (ProjectController):
//   - refreshViewFromModel() adds states first, then transitions, so sources and
//     targets exist when transitions are created.
//   - handleModelEntityAdded() creates a transition graphics item via
//     HsmGraphicsView::createHsmTransition(...) WITHOUT the
//     findHsmTransition(id).isNull() guard that its sibling modelDataChanged()
//     path has.
//   - initializing a transition graphics item writes computed geometry back into
//     the model, which emits modelDataChanged and RE-ENTERS the creation logic
//     for the same transition id (the unguarded second invocation on the rebuild
//     path). createHsmTransition() is non-idempotent, so this second call builds
//     a new item and overwrites mElements, orphaning the first item on the scene.
//
// To exercise that exact path deterministically, for each source/target-complete
// transition we invoke the SAME rebuild creation call twice (handleModelEntityAdded
// followed by the modelDataChanged geometry-write-back re-entry), both without the
// isNull() guard - faithfully mirroring the unguarded rebuild path that produces
// the defect.
//
// Validates: Requirements 1.1, 1.2, 1.3, 2.1, 2.2, 2.3
// ---------------------------------------------------------------------------
class HsmTransitionDuplicateRenderingPropertyTest : public QObject {
    Q_OBJECT

private slots:
    void PropertyExactlyOneGraphicsItemPerTransition();

    // Property 2: Preservation - Unchanged Non-Duplicating Behavior.
    void PropertyStateRenderedOnceAtStoredGeometry();
    void PropertyTransitionConnectsCorrectSourceAndTarget();
    void PropertyTransitionRoutingPointsPreserved();
    void PropertyIncompleteTransitionProducesNoItem();
    void PropertyReconnectEditMoveUpdatesSingleItem();

private:
    // Count view::HsmTransition graphics items on the scene whose model id matches.
    static int countTransitionGraphicsItems(const QGraphicsScene& scene, const model::EntityID_t transitionId);
    // Count all view::HsmTransition graphics items present on the scene.
    static int countAllTransitionGraphicsItems(const QGraphicsScene& scene);
};

int HsmTransitionDuplicateRenderingPropertyTest::countTransitionGraphicsItems(const QGraphicsScene& scene,
                                                                              const model::EntityID_t transitionId) {
    int count = 0;

    for (QGraphicsItem* item : scene.items()) {
        auto* transitionItem = dynamic_cast<view::HsmTransition*>(item);

        if ((nullptr != transitionItem) && (transitionItem->modelId() == transitionId)) {
            ++count;
        }
    }

    return count;
}

int HsmTransitionDuplicateRenderingPropertyTest::countAllTransitionGraphicsItems(const QGraphicsScene& scene) {
    int count = 0;

    for (QGraphicsItem* item : scene.items()) {
        if (nullptr != dynamic_cast<view::HsmTransition*>(item)) {
            ++count;
        }
    }

    return count;
}

/**
 * @brief Property 1 (Requirements 1.1, 1.2, 1.3, 2.1, 2.2, 2.3).
 *
 * BUG CONDITION reproduction via the AUTHENTIC runtime path.
 *
 * This drives view creation exactly the way the running application does: a real
 * model::StateMachineModel is wired to an HsmGraphicsView through the SAME three
 * signal->slot connections ProjectController makes in its constructor
 * (modelEntityAdded -> handleModelEntityAdded, modelDataChanged -> modelDataChanged),
 * using the SAME guarded create logic. We then add states and a transition to the
 * MODEL and let the signal chain build the view - we never call createHsmTransition()
 * directly.
 *
 * The defect lives in the re-entrancy window: HsmGraphicsView::createHsmTransition()
 * runs HsmTransition::init()/connectElements(), which synchronously write the computed
 * routing geometry back into the model (setMetadata(GEOMETRY)). That emits
 * modelDataChanged and RE-ENTERS the controller's create path for the SAME transition
 * id WHILE the first createHsmTransition() call is still on the stack. Whether the
 * guard (findHsmTransition(id).isNull()) catches that re-entry depends entirely on
 * whether the item was registered in mElements BEFORE init() ran:
 *   - registration LAST (unfixed)  -> guard sees "no item", creates a SECOND item,
 *                                     the outer call then overwrites mElements with the
 *                                     FIRST item, orphaning one on the scene -> 2 items.
 *   - registration FIRST (fixed)   -> guard finds the item, short-circuits -> 1 item.
 *
 * So this slot FAILS on the unfixed ordering and PASSES on the fixed ordering - it
 * reproduces the real bug rather than a synthetic double call.
 *
 * Validates: Requirements 1.1, 1.2, 1.3, 2.1, 2.2, 2.3
 */
void HsmTransitionDuplicateRenderingPropertyTest::PropertyExactlyOneGraphicsItemPerTransition() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 50; ++iteration) {
        model::StateMachineModel smModel(QStringLiteral("repro"));

        QGraphicsScene scene;
        HsmGraphicsView view;
        view.setScene(&scene);

        // Wire the model to the view with the controller's guarded create logic. These
        // lambdas mirror ProjectController::handleModelEntityAdded() (Transition branch)
        // and ProjectController::modelDataChanged() - including the
        // findHsmTransition(id).isNull() guard - so the re-entrant modelDataChanged fired
        // during init()'s geometry write-back exercises the exact production code path.
        auto createGuarded = [&view](const QSharedPointer<model::StateMachineEntity>& entity) {
            if (entity.isNull() || entity->type() != model::StateMachineEntity::Type::Transition) {
                return;
            }

            auto ptrTransition = entity.dynamicCast<model::Transition>();

            if (ptrTransition.isNull()) {
                return;
            }

            if (ptrTransition->source() && ptrTransition->target()) {
                if (view.findHsmTransition(ptrTransition->id()).isNull() == true) {
                    view.createHsmTransition(ptrTransition, ptrTransition->sourceId(), ptrTransition->targetId());
                }
            }
        };

        QObject::connect(&smModel,
                         &model::StateMachineModel::modelEntityAdded,
                         &view,
                         [createGuarded](QWeakPointer<model::StateMachineEntity> parent,
                                         QWeakPointer<model::StateMachineEntity> entity) {
                             Q_UNUSED(parent);
                             auto ptr = entity.lock();

                             if (ptr && ptr->type() == model::StateMachineEntity::Type::State) {
                                 return;  // states are placed explicitly below via createHsmElement
                             }

                             createGuarded(ptr);
                         });
        QObject::connect(&smModel,
                         &model::StateMachineModel::modelDataChanged,
                         &view,
                         [createGuarded](QWeakPointer<model::StateMachineEntity> entity) {
                             createGuarded(entity.lock());
                         });

        // --- Build the model: states first (so sources/targets exist), then transitions ---
        const int extraStateCount = static_cast<int>(rng.bounded(0, 4));  // total states: 2..5
        QList<QSharedPointer<model::State>> states;

        for (int i = 0; i < 2 + extraStateCount; ++i) {
            auto state = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, smModel.idGenerator());
            smModel.root()->addChildState(state);
            states.append(state);
        }

        // Place the state graphics items up front (as the controller does for states),
        // so the transition endpoints are resolvable when transitions are created.
        qreal x = 0.0;

        for (const auto& state : states) {
            view.createHsmElement(state, "state", QPointF(x, 0.0), QSizeF(120.0, 80.0), model::INVALID_MODEL_ID);
            x += 260.0;
        }

        // Add transitions to the MODEL and let the signal chain drive view creation.
        // Always include two parallel transitions between the same pair (Requirement 3.6).
        // Self-transitions are intentionally NOT added here: driving a self-transition
        // through the view triggers a separate, pre-existing unbounded recalculateLine
        // re-entrancy crash (documented in Property 2 below) that is unrelated to this
        // duplicate-rendering bug. Only regular (source != target) transitions are driven.
        QList<model::EntityID_t> expectedTransitionIds;

        auto addModelTransition = [&](const QSharedPointer<model::State>& source,
                                      const QSharedPointer<model::State>& target) {
            auto transition = smModel.createUniqueTransition(source->id(), target->id());

            if (!transition.isNull() && source != target) {
                expectedTransitionIds.append(transition->id());
            }
        };

        addModelTransition(states.first(), states.last());
        addModelTransition(states.first(), states.last());

        const int randomEdgeCount = static_cast<int>(rng.bounded(0, 4));

        for (int i = 0; i < randomEdgeCount; ++i) {
            int sourceIndex = static_cast<int>(rng.bounded(states.size()));
            int targetIndex = static_cast<int>(rng.bounded(states.size()));

            if (targetIndex == sourceIndex) {
                targetIndex = (sourceIndex + 1) % states.size();
            }

            addModelTransition(states.at(sourceIndex), states.at(targetIndex));
        }

        // --- Assert Property 1: exactly one graphics item per transition id, no orphans ---
        for (const model::EntityID_t transitionId : expectedTransitionIds) {
            const int itemCount = countTransitionGraphicsItems(scene, transitionId);
            QVERIFY2(itemCount == 1,
                     qPrintable(QString("Iteration %1: transition id %2 rendered as %3 graphics items on the scene "
                                        "(expected exactly 1)")
                                    .arg(iteration)
                                    .arg(transitionId)
                                    .arg(itemCount)));
        }

        const int totalTransitionItems = countAllTransitionGraphicsItems(scene);
        QVERIFY2(totalTransitionItems == expectedTransitionIds.size(),
                 qPrintable(QString("Iteration %1: scene holds %2 transition graphics items but the model produced %3 "
                                    "source/target-complete transitions (orphaned items present)")
                                .arg(iteration)
                                .arg(totalTransitionItems)
                                .arg(expectedTransitionIds.size())));
    }
}

// ===========================================================================
// Property 2: Preservation - Unchanged Non-Duplicating Behavior.
//
// These slots encode the regression-prevention guarantees of design Property 2:
// for inputs where isBugCondition is false, the fixed code must behave exactly as
// the current (unfixed) code. They follow the observation-first methodology -
// each asserts behavior OBSERVED on the UNFIXED code, so every slot MUST PASS on
// the unfixed code and then keep guarding that behavior after the fix (F == F').
//
// Self-transition caveat (same as Property 1 above): driving a self-transition's
// view item through the full view path triggers a SEPARATE, pre-existing unbounded
// re-entrancy crash under offscreen Qt (HsmStateElement::layoutSelfTransitions ->
// HsmTransition::recalculateLine -> setMetadata(GEOMETRY) -> modelDataChanged ->
// onModelDataChanged -> layoutSections -> layoutSelfTransitions -> ...), unrelated
// to this duplicate-rendering bug and present even on correct code. Self-transitions
// are kept in the model for realism but are NOT driven through the full view path /
// count assertions here; only regular (source != target) transitions are rebuilt.
//
// Validates: Requirements 3.1, 3.2, 3.3, 3.5, 3.6
// ===========================================================================

/**
 * @brief Property 2 (Requirement 3.1): state preservation.
 *
 * Observation-first: on the unfixed code each state entity added during a rebuild
 * yields exactly one view::HsmElement at the stored drop position/size. Property-
 * based generation varies the state count (2..6) and each state's drop position and
 * size with QRandomGenerator. For every state we assert exactly one graphics item
 * exists for its id (findHsmElement is non-null and the per-id scene count is 1) and
 * that its scene position equals the requested drop position - the placement behavior
 * that must remain unchanged after the fix.
 *
 * Validates: Requirements 3.1
 */
void HsmTransitionDuplicateRenderingPropertyTest::PropertyStateRenderedOnceAtStoredGeometry() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 50; ++iteration) {
        model::EntityIdGenerator idGenerator;

        const int stateCount = 2 + static_cast<int>(rng.bounded(0, 5));  // 2..6 states
        QList<QSharedPointer<model::State>> states;
        QList<QPointF> dropPositions;
        QList<QSizeF> sizes;

        for (int i = 0; i < stateCount; ++i) {
            states.append(model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, idGenerator));
            dropPositions.append(QPointF(rng.bounded(-600, 600), rng.bounded(-600, 600)));
            sizes.append(QSizeF(80.0 + rng.bounded(0, 160), 60.0 + rng.bounded(0, 120)));
        }

        QGraphicsScene scene;
        HsmGraphicsView view;
        view.setScene(&scene);

        QList<view::HsmElement*> created;

        for (int i = 0; i < stateCount; ++i) {
            view::HsmElement* element =
                view.createHsmElement(states.at(i), "state", dropPositions.at(i), sizes.at(i), model::INVALID_MODEL_ID);
            QVERIFY2(nullptr != element,
                     qPrintable(QString("Iteration %1: createHsmElement returned null for state %2").arg(iteration).arg(i)));
            created.append(element);
        }

        // Each state renders exactly once at its stored position.
        for (int i = 0; i < stateCount; ++i) {
            const model::EntityID_t stateId = states.at(i)->id();

            // Exactly one graphics item tracked for this id.
            QVERIFY2(false == view.findHsmElement(stateId).isNull(),
                     qPrintable(QString("Iteration %1: no graphics item found for state id %2").arg(iteration).arg(stateId)));

            // Exactly one graphics item on the scene for this id.
            int perIdCount = 0;

            for (QGraphicsItem* item : scene.items()) {
                auto* hsmElement = dynamic_cast<view::HsmElement*>(item);

                if ((nullptr != hsmElement) && (hsmElement->modelId() == stateId)) {
                    ++perIdCount;
                }
            }

            QVERIFY2(perIdCount == 1,
                     qPrintable(QString("Iteration %1: state id %2 rendered as %3 graphics items (expected exactly 1)")
                                    .arg(iteration)
                                    .arg(stateId)
                                    .arg(perIdCount)));

            // Stored position preserved (no shift on add).
            const QPointF actualPos = created.at(i)->scenePos();
            QVERIFY2(actualPos == dropPositions.at(i),
                     qPrintable(QString("Iteration %1: state id %2 scenePos (%3,%4) != stored drop position (%5,%6)")
                                    .arg(iteration)
                                    .arg(stateId)
                                    .arg(actualPos.x())
                                    .arg(actualPos.y())
                                    .arg(dropPositions.at(i).x())
                                    .arg(dropPositions.at(i).y())));
        }
    }
}

/**
 * @brief Property 2 (Requirement 3.2): source/target connectivity preservation.
 *
 * Observation-first: on the unfixed code a transition created with a valid source and
 * target yields exactly one view::HsmTransition whose model source/target ids match the
 * endpoints it was created with. Property-based generation varies the state count and,
 * for each regular transition, picks a random distinct (source, target) pair. For each
 * such transition we assert a single graphics item exists for its id and that the
 * model's sourceId()/targetId() equal the endpoints passed to createHsmTransition - the
 * connectivity that must stay unchanged after the fix.
 *
 * Validates: Requirements 3.2, 3.6
 */
void HsmTransitionDuplicateRenderingPropertyTest::PropertyTransitionConnectsCorrectSourceAndTarget() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 50; ++iteration) {
        model::EntityIdGenerator idGenerator;

        const int stateCount = 2 + static_cast<int>(rng.bounded(0, 4));  // 2..5 states
        QList<QSharedPointer<model::State>> states;

        for (int i = 0; i < stateCount; ++i) {
            states.append(model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, idGenerator));
        }

        // Build a varying set of regular (source != target) transitions, including
        // parallel edges between the same pair. Self-transitions are kept in the model
        // for realism but excluded from the view rebuild (see the caveat above).
        QList<QSharedPointer<model::Transition>> rebuiltTransitions;

        // A self-transition for realism (not rebuilt).
        model::ModelElementsFactory::createUniqueTransition(states.first(), states.first(), idGenerator);

        // Two parallel transitions between the same pair (Requirement 3.6).
        rebuiltTransitions.append(
            model::ModelElementsFactory::createUniqueTransition(states.first(), states.last(), idGenerator));
        rebuiltTransitions.append(
            model::ModelElementsFactory::createUniqueTransition(states.first(), states.last(), idGenerator));

        const int extraEdges = static_cast<int>(rng.bounded(0, 4));

        for (int i = 0; i < extraEdges; ++i) {
            int sourceIndex = static_cast<int>(rng.bounded(stateCount));
            int targetIndex = static_cast<int>(rng.bounded(stateCount));

            // Keep this edge a regular (non-self) transition.
            if (targetIndex == sourceIndex) {
                targetIndex = (sourceIndex + 1) % stateCount;
            }

            rebuiltTransitions.append(model::ModelElementsFactory::createUniqueTransition(
                states.at(sourceIndex), states.at(targetIndex), idGenerator));
        }

        QGraphicsScene scene;
        HsmGraphicsView view;
        view.setScene(&scene);

        qreal x = 0.0;

        for (const auto& state : states) {
            view.createHsmElement(state, "state", QPointF(x, 0.0), QSizeF(120.0, 80.0), model::INVALID_MODEL_ID);
            x += 260.0;
        }

        for (const auto& transition : rebuiltTransitions) {
            const model::EntityID_t expectedSource = transition->sourceId();
            const model::EntityID_t expectedTarget = transition->targetId();

            view::HsmTransition* item = view.createHsmTransition(transition, expectedSource, expectedTarget);

            QVERIFY2(nullptr != item,
                     qPrintable(QString("Iteration %1: createHsmTransition returned null for a source/target-complete "
                                        "transition id %2")
                                    .arg(iteration)
                                    .arg(transition->id())));

            // Exactly one graphics item for this transition id.
            const int itemCount = countTransitionGraphicsItems(scene, transition->id());
            QVERIFY2(itemCount == 1,
                     qPrintable(QString("Iteration %1: transition id %2 rendered as %3 graphics items (expected 1)")
                                    .arg(iteration)
                                    .arg(transition->id())
                                    .arg(itemCount)));

            // The transition connects the correct source and target entities.
            QVERIFY2(transition->sourceId() == expectedSource,
                     qPrintable(QString("Iteration %1: transition id %2 sourceId %3 != expected %4")
                                    .arg(iteration)
                                    .arg(transition->id())
                                    .arg(transition->sourceId())
                                    .arg(expectedSource)));
            QVERIFY2(transition->targetId() == expectedTarget,
                     qPrintable(QString("Iteration %1: transition id %2 targetId %3 != expected %4")
                                    .arg(iteration)
                                    .arg(transition->id())
                                    .arg(transition->targetId())
                                    .arg(expectedTarget)));
        }
    }
}

/**
 * @brief Property 2 (Requirement 3.3): routing-point preservation.
 *
 * Observation-first: a transition with stored GEOMETRY routing points renders along
 * those points. We seed the model transition with a GEOMETRY polygon (source corner,
 * one or more intermediate routing points, target corner) BEFORE creating the view
 * item; HsmTransition::init reads that GEOMETRY and builds intermediate grips for the
 * interior points. On the unfixed code the stored GEOMETRY continues to describe a
 * poly-line whose endpoints bracket the interior routing points. We record the
 * GEOMETRY the view produces (after init recomputes the endpoints) and assert the
 * interior routing points we supplied are preserved in order inside that poly-line -
 * the routing behavior that must stay unchanged after the fix.
 *
 * Validates: Requirements 3.3
 */
void HsmTransitionDuplicateRenderingPropertyTest::PropertyTransitionRoutingPointsPreserved() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 40; ++iteration) {
        model::EntityIdGenerator idGenerator;

        auto source = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, idGenerator);
        auto target = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, idGenerator);
        auto transition = model::ModelElementsFactory::createUniqueTransition(source, target, idGenerator);
        QVERIFY2(!transition.isNull(), "createUniqueTransition returned null");

        QGraphicsScene scene;
        HsmGraphicsView view;
        view.setScene(&scene);

        const QPointF sourcePos(0.0, 0.0);
        const QPointF targetPos(400.0, 0.0);
        view.createHsmElement(source, "state", sourcePos, QSizeF(120.0, 80.0), model::INVALID_MODEL_ID);
        view.createHsmElement(target, "state", targetPos, QSizeF(120.0, 80.0), model::INVALID_MODEL_ID);

        // Build a GEOMETRY poly-line: endpoint, 1..3 interior routing points, endpoint.
        const int interiorCount = 1 + static_cast<int>(rng.bounded(0, 3));
        QPolygonF geometry;
        QList<QPointF> interiorPoints;

        geometry.append(QPointF(60.0, 40.0));  // near the source

        for (int i = 0; i < interiorCount; ++i) {
            // Interior routing points spread between the two states.
            const QPointF routingPoint(100.0 + rng.bounded(0, 260), -120.0 + rng.bounded(0, 240));
            interiorPoints.append(routingPoint);
            geometry.append(routingPoint);
        }

        geometry.append(QPointF(460.0, 40.0));  // near the target

        transition->setMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY, geometry);

        view::HsmTransition* item = view.createHsmTransition(transition, source->id(), target->id());
        QVERIFY2(nullptr != item,
                 qPrintable(QString("Iteration %1: createHsmTransition returned null").arg(iteration)));

        // Exactly one item for this transition.
        QVERIFY2(countTransitionGraphicsItems(scene, transition->id()) == 1,
                 qPrintable(QString("Iteration %1: expected exactly one transition graphics item").arg(iteration)));

        // The view writes the rendered poly-line back into the model GEOMETRY. The
        // interior routing points we supplied must be preserved, in order, inside it.
        const QVariant storedVar = transition->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY);
        QVERIFY2(storedVar.isValid(),
                 qPrintable(QString("Iteration %1: transition has no stored GEOMETRY after rendering").arg(iteration)));

        const QPolygonF stored = storedVar.value<QPolygonF>();
        QVERIFY2(stored.size() >= interiorCount + 2,
                 qPrintable(QString("Iteration %1: stored geometry has %2 points, expected at least %3")
                                .arg(iteration)
                                .arg(stored.size())
                                .arg(interiorCount + 2)));

        // Every supplied interior routing point appears, in order, among the interior
        // points of the stored poly-line (endpoints excluded).
        int searchFrom = 1;  // skip the source endpoint

        for (const QPointF& routingPoint : interiorPoints) {
            bool found = false;

            for (int j = searchFrom; j < stored.size() - 1; ++j) {
                if (qFuzzyCompare(stored.at(j).x(), routingPoint.x()) &&
                    qFuzzyCompare(stored.at(j).y(), routingPoint.y())) {
                    found = true;
                    searchFrom = j + 1;
                    break;
                }
            }

            QVERIFY2(found,
                     qPrintable(QString("Iteration %1: interior routing point (%2,%3) not preserved in order in the "
                                        "rendered geometry")
                                    .arg(iteration)
                                    .arg(routingPoint.x())
                                    .arg(routingPoint.y())));
        }
    }
}

/**
 * @brief Property 2 (Requirement 3.2, scope note): incomplete-transition preservation.
 *
 * Observation-first: a transition whose source or target has no corresponding view
 * element (i.e. the transition lacks a valid, renderable source/target during the
 * rebuild) produces ZERO graphics items on the unfixed code - createHsmTransition
 * cannot find both endpoints and returns null without adding anything. Property-based
 * generation randomly omits the source element, the target element, or both from the
 * view before the create call. We assert the result is always null and no transition
 * graphics item for that id exists on the scene - behavior that must still be zero
 * after the fix (the fix must not start creating items for incomplete transitions).
 *
 * Validates: Requirements 3.2
 */
void HsmTransitionDuplicateRenderingPropertyTest::PropertyIncompleteTransitionProducesNoItem() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 50; ++iteration) {
        model::EntityIdGenerator idGenerator;

        auto source = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, idGenerator);
        auto target = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, idGenerator);
        auto transition = model::ModelElementsFactory::createUniqueTransition(source, target, idGenerator);
        QVERIFY2(!transition.isNull(), "createUniqueTransition returned null");

        QGraphicsScene scene;
        HsmGraphicsView view;
        view.setScene(&scene);

        // Randomly omit source, target, or both view elements so the transition has
        // no renderable endpoint - mirroring a transition that lacks a valid source/target.
        const int omission = static_cast<int>(rng.bounded(3));  // 0: no source, 1: no target, 2: neither
        const bool addSource = (omission == 1);
        const bool addTarget = (omission == 0);

        if (addSource) {
            view.createHsmElement(source, "state", QPointF(0.0, 0.0), QSizeF(120.0, 80.0), model::INVALID_MODEL_ID);
        }

        if (addTarget) {
            view.createHsmElement(target, "state", QPointF(300.0, 0.0), QSizeF(120.0, 80.0), model::INVALID_MODEL_ID);
        }

        view::HsmTransition* item = view.createHsmTransition(transition, source->id(), target->id());

        QVERIFY2(nullptr == item,
                 qPrintable(QString("Iteration %1 (omission %2): createHsmTransition unexpectedly created an item for "
                                    "an incomplete transition")
                                .arg(iteration)
                                .arg(omission)));

        QVERIFY2(countTransitionGraphicsItems(scene, transition->id()) == 0,
                 qPrintable(QString("Iteration %1 (omission %2): incomplete transition id %3 produced a graphics item "
                                    "(expected zero)")
                                .arg(iteration)
                                .arg(omission)
                                .arg(transition->id())));
    }
}

/**
 * @brief Property 2 (Requirement 3.5): reconnect/edit/move preservation.
 *
 * Observation-first: reconnecting, editing the event/condition, or moving an existing
 * transition updates the SINGLE existing graphics item rather than creating a new one.
 * We create one regular transition, record the view::HsmTransition pointer for its id,
 * then apply a randomly chosen mutation: reconnect it to a different target, edit its
 * event/condition on the model, or move one of its endpoint states. After each mutation
 * we assert the per-id count is still exactly one and findHsmTransition returns the SAME
 * pointer - the single-item update behavior that must stay unchanged after the fix.
 *
 * Validates: Requirements 3.5
 */
void HsmTransitionDuplicateRenderingPropertyTest::PropertyReconnectEditMoveUpdatesSingleItem() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 50; ++iteration) {
        model::EntityIdGenerator idGenerator;

        auto source = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, idGenerator);
        auto target = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, idGenerator);
        auto altTarget = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, idGenerator);
        auto transition = model::ModelElementsFactory::createUniqueTransition(source, target, idGenerator);
        QVERIFY2(!transition.isNull(), "createUniqueTransition returned null");

        QGraphicsScene scene;
        HsmGraphicsView view;
        view.setScene(&scene);

        view::HsmElement* sourceElement =
            view.createHsmElement(source, "state", QPointF(0.0, 0.0), QSizeF(120.0, 80.0), model::INVALID_MODEL_ID);
        view.createHsmElement(target, "state", QPointF(300.0, 0.0), QSizeF(120.0, 80.0), model::INVALID_MODEL_ID);
        view.createHsmElement(altTarget, "state", QPointF(300.0, 200.0), QSizeF(120.0, 80.0), model::INVALID_MODEL_ID);

        view::HsmTransition* created = view.createHsmTransition(transition, source->id(), target->id());
        QVERIFY2(nullptr != created,
                 qPrintable(QString("Iteration %1: createHsmTransition returned null").arg(iteration)));

        const QPointer<view::HsmTransition> itemBefore = view.findHsmTransition(transition->id());
        QVERIFY2(false == itemBefore.isNull(),
                 qPrintable(QString("Iteration %1: no transition item tracked after creation").arg(iteration)));
        QVERIFY2(countTransitionGraphicsItems(scene, transition->id()) == 1,
                 qPrintable(QString("Iteration %1: expected one transition item after creation").arg(iteration)));

        // Apply one randomly chosen mutation to the existing transition.
        const int mutation = static_cast<int>(rng.bounded(3));  // 0: reconnect, 1: edit, 2: move endpoint

        switch (mutation) {
            case 0:
                // Reconnect to a different target via the real view path.
                view.reconnectHsmTransition(transition->id(), source->id(), altTarget->id());
                break;
            case 1:
                // Edit the event/condition on the model entity.
                transition->setEvent(QStringLiteral("EDITED_EVENT_%1").arg(iteration));
                transition->setConditionCallback(QStringLiteral("cond_%1").arg(iteration));
                break;
            default:
                // Move one endpoint state; the transition must follow on the same item.
                if (nullptr != sourceElement) {
                    sourceElement->setPos(QPointF(rng.bounded(-200, 200), rng.bounded(-200, 200)));
                }
                break;
        }

        // The mutation must update the single existing item, never create a second one.
        const QPointer<view::HsmTransition> itemAfter = view.findHsmTransition(transition->id());
        QVERIFY2(false == itemAfter.isNull(),
                 qPrintable(QString("Iteration %1 (mutation %2): transition item lost after mutation")
                                .arg(iteration)
                                .arg(mutation)));
        QVERIFY2(itemAfter.data() == itemBefore.data(),
                 qPrintable(QString("Iteration %1 (mutation %2): transition item pointer changed (a new item was created "
                                    "instead of updating the existing one)")
                                .arg(iteration)
                                .arg(mutation)));
        QVERIFY2(countTransitionGraphicsItems(scene, transition->id()) == 1,
                 qPrintable(QString("Iteration %1 (mutation %2): transition id %3 rendered as %4 graphics items after "
                                    "the mutation (expected exactly 1)")
                                .arg(iteration)
                                .arg(mutation)
                                .arg(transition->id())
                                .arg(countTransitionGraphicsItems(scene, transition->id()))));
    }
}

int runHsmTransitionDuplicateRenderingPropertyTest(int argc, char** argv) {
    QApplication app(argc, argv);

    HsmTransitionDuplicateRenderingPropertyTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "HsmTransitionDuplicateRenderingPropertyTest.moc"
