#include "../../QtTestCompat.hpp"

#include "model/ExitPoint.hpp"
#include "model/FinalState.hpp"
#include "model/HistoryState.hpp"
#include "model/IncludeEntity.hpp"
#include "model/InitialState.hpp"
#include "model/RegularState.hpp"
#include "model/StateMachineModel.hpp"
#include "model/StateMachineSerializer.hpp"
#include "model/Transition.hpp"
#include "../TestPaths.hpp"

namespace {

int countDirectTransitions(const QSharedPointer<model::RegularState>& state) {
    int count = 0;

    for (const auto& child : state->childrenEntities()) {
        if (child->type() == model::StateMachineEntity::Type::Transition) {
            ++count;
        }
    }

    return count;
}

/**
 * @brief Validate deserialization of deep history state.
 *
 * Use-case: Ensure `history` with `type="deep"` keeps default transition metadata.
 *
 * @startuml
 * state S1
 * state H <<history:deep>>
 * H --> S1 : resume
 * @enduml
 */
TEST(StateMachineSerializerDeserializationTest, DeserializeDeepHistory) {
    const QString scxml = test::loadScxmlFixture("history_deep.scxml");
    ASSERT_FALSE(scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    ASSERT_TRUE(model);
    auto history = model->root()->findChildStateByName("H").dynamicCast<model::HistoryState>();
    ASSERT_TRUE(history);
    EXPECT_EQ(model::HistoryType::DEEP, history->historyType());
    ASSERT_TRUE(history->defaultTransition());
    EXPECT_EQ(QString("resume"), history->defaultTransition()->event());
}

/**
 * @brief Validate deserialization of shallow history state.
 *
 * Use-case: Ensure `history` with `type="shallow"` is parsed as shallow history.
 *
 * @startuml
 * state S1
 * state H <<history:shallow>>
 * H --> S2 : resume
 * @enduml
 */
TEST(StateMachineSerializerDeserializationTest, DeserializeShallowHistory) {
    const QString scxml = test::loadScxmlFixture("history_shallow.scxml");
    ASSERT_FALSE(scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    ASSERT_TRUE(model);
    auto history = model->root()->findChildStateByName("H").dynamicCast<model::HistoryState>();
    ASSERT_TRUE(history);
    EXPECT_EQ(model::HistoryType::SHALLOW, history->historyType());
    ASSERT_TRUE(history->defaultTransition());
    EXPECT_EQ(QString("S2"), history->defaultTransition()->target()->name());
}

/**
 * @brief Validate deserialization of nested substates.
 *
 * Use-case: Ensure parent-child hierarchy is preserved for nested states.
 *
 * @startuml
 * state Parent {
 *   state ChildA
 *   state ChildB {
 *     state GrandChild
 *   }
 * }
 * @enduml
 */
TEST(StateMachineSerializerDeserializationTest, DeserializeSubstatesHierarchy) {
    const QString scxml = test::loadScxmlFixture("substates.scxml");
    ASSERT_FALSE(scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    ASSERT_TRUE(model);
    auto parent = model->root()->findChildStateByName("Parent").dynamicCast<model::RegularState>();
    ASSERT_TRUE(parent);
    auto childA = model->root()->findChildStateByName("ChildA").dynamicCast<model::RegularState>();
    auto childB = model->root()->findChildStateByName("ChildB").dynamicCast<model::RegularState>();
    auto grandChild = model->root()->findChildStateByName("GrandChild").dynamicCast<model::RegularState>();

    ASSERT_TRUE(childA);
    ASSERT_TRUE(childB);
    ASSERT_TRUE(grandChild);
    EXPECT_EQ(parent, model->root()->findParentState(childA->id()).dynamicCast<model::RegularState>());
    EXPECT_EQ(childB, model->root()->findParentState(grandChild->id()).dynamicCast<model::RegularState>());
}

/**
 * @brief Validate deserialization of multiple transitions on one state.
 *
 * Use-case: Ensure a single state can hold multiple outgoing transitions.
 *
 * @startuml
 * S1 --> S2 : e1
 * S1 --> S3 : e2
 * @enduml
 */
TEST(StateMachineSerializerDeserializationTest, DeserializeMultipleTransitionsForSingleState) {
    const QString scxml = test::loadScxmlFixture("multiple_transitions.scxml");
    ASSERT_FALSE(scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    ASSERT_TRUE(model);
    auto s1 = model->root()->findChildStateByName("S1").dynamicCast<model::RegularState>();
    ASSERT_TRUE(s1);
    EXPECT_EQ(2, countDirectTransitions(s1));
}

/**
 * @brief Validate transition type parsing for external and internal transitions.
 *
 * Use-case: Ensure SCXML `type` attribute maps to model transition type enum.
 *
 * @startuml
 * S1 --> S2 : e1 / external
 * S1 --> S3 : e2 / internal
 * @enduml
 */
TEST(StateMachineSerializerDeserializationTest, DeserializeExternalAndInternalTransitions) {
    const QString scxml = test::loadScxmlFixture("multiple_transitions.scxml");
    ASSERT_FALSE(scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    ASSERT_TRUE(model);
    auto s1 = model->root()->findChildStateByName("S1").dynamicCast<model::RegularState>();
    ASSERT_TRUE(s1);

    bool seenExternal = false;
    bool seenInternal = false;
    s1->forEachChildElement([&seenExternal, &seenInternal](QSharedPointer<model::StateMachineEntity> parent,
                                                           QSharedPointer<model::StateMachineEntity> child) {
        Q_UNUSED(parent);
        bool keepWalking = true;

        if (child->type() == model::StateMachineEntity::Type::Transition) {
            auto tr = child.dynamicCast<model::Transition>();
            if (tr && tr->event() == "e1") {
                seenExternal = (tr->transitionType() == model::TransitionType::EXTERNAL);
            } else if (tr && tr->event() == "e2") {
                seenInternal = (tr->transitionType() == model::TransitionType::INTERNAL);
            }
        }

        return keepWalking;
    });

    EXPECT_TRUE(seenExternal);
    EXPECT_TRUE(seenInternal);
}

/**
 * @brief Validate parsing of entry-point equivalent (`initial`) with transitions.
 *
 * Use-case: Ensure transitions from `<initial>` are parsed and linked to targets.
 *
 * @startuml
 * state Region {
 *   [*] --> A : toA
 *   [*] --> B : toB
 * }
 * @enduml
 */
TEST(StateMachineSerializerDeserializationTest, DeserializeEntryPointTransitions) {
    const QString scxml = test::loadScxmlFixture("entrypoint_multiple.scxml");
    ASSERT_FALSE(scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    ASSERT_TRUE(model);
    bool hasInitialState = false;
    bool hasTransitionToAorB = false;

    model->root()->forEachChildElement(
        [&hasInitialState, &hasTransitionToAorB](QSharedPointer<model::StateMachineEntity> parent,
                                                 QSharedPointer<model::StateMachineEntity> child) {
            Q_UNUSED(parent);
            bool keepWalking = true;

            if (child->type() == model::StateMachineEntity::Type::State) {
                auto state = child.dynamicCast<model::State>();
                if (state && state->stateType() == model::StateType::INITIAL) {
                    hasInitialState = true;
                    auto initial = state.dynamicCast<model::InitialState>();
                    if (initial && initial->transition()) {
                        const QString target = initial->transition()->target()->name();
                        hasTransitionToAorB = (target == "A" || target == "B");
                    }
                }
            }

            return keepWalking;
        });

    EXPECT_TRUE(hasInitialState);
    EXPECT_TRUE(hasTransitionToAorB);
}

/**
 * @brief Validate parsing of exit point and transitions targeting it.
 *
 * Use-case: Ensure nested `<final>` maps to ExitPoint and transitions can target it.
 *
 * @startuml
 * state Parent {
 *   state Worker
 *   Worker --> XP : done
 *   state XP <<exitPoint>>
 * }
 * @enduml
 */
TEST(StateMachineSerializerDeserializationTest, DeserializeExitPointAndTargetingTransition) {
    const QString scxml = test::loadScxmlFixture("exitpoint.scxml");
    ASSERT_FALSE(scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    ASSERT_TRUE(model);
    auto xp = model->root()->findChildStateByName("XP").dynamicCast<model::ExitPoint>();
    ASSERT_TRUE(xp);
    EXPECT_EQ(QString("leave"), xp->event());

    auto worker = model->root()->findChildStateByName("Worker").dynamicCast<model::RegularState>();
    ASSERT_TRUE(worker);

    bool hasTargetingTransition = false;
    worker->forEachChildElement([&hasTargetingTransition](QSharedPointer<model::StateMachineEntity> parent,
                                                          QSharedPointer<model::StateMachineEntity> child) {
        Q_UNUSED(parent);
        bool keepWalking = true;

        if (child->type() == model::StateMachineEntity::Type::Transition) {
            auto tr = child.dynamicCast<model::Transition>();
            if (tr && tr->target() && tr->target()->name() == "XP") {
                hasTargetingTransition = true;
            }
        }

        return keepWalking;
    });

    EXPECT_TRUE(hasTargetingTransition);
}

/**
 * @brief Validate include entity parsing from xi:include.
 *
 * Use-case: Ensure `<xi:include>` gets mapped to IncludeEntity path.
 *
 * @startuml
 * state IncludeNode
 * IncludeNode : xi:include href=subchart.scxml
 * @enduml
 */
TEST(StateMachineSerializerDeserializationTest, DeserializeIncludeEntity) {
    const QString scxml = test::loadScxmlFixture("include_entity.scxml");
    ASSERT_FALSE(scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    ASSERT_TRUE(model);
    auto include = model->root()->findChildStateByName("IncludeNode").dynamicCast<model::IncludeEntity>();
    ASSERT_TRUE(include);
    EXPECT_EQ(QString("subchart.scxml"), include->path());
}

/**
 * @brief Validate malformed SCXML handling for XML syntax errors.
 *
 * Use-case: Ensure parser behavior is deterministic when SCXML is not well-formed XML.
 *
 * @startuml
 * [*] --> ParseError
 * @enduml
 */
TEST(StateMachineSerializerDeserializationTest, DeserializeMalformedXml) {
    const QString scxml = test::loadScxmlFixture("malformed_not_xml.scxml");
    ASSERT_FALSE(scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    EXPECT_TRUE(model == nullptr || model->root()->childrenEntities().isEmpty());
}

/**
 * @brief Validate malformed SCXML handling for state without id attribute.
 *
 * Use-case: Ensure invalid state nodes are ignored without crashing parser.
 *
 * @startuml
 * state (missing id)
 * @enduml
 */
TEST(StateMachineSerializerDeserializationTest, DeserializeStateWithoutId) {
    const QString scxml = test::loadScxmlFixture("malformed_missing_state_id.scxml");
    ASSERT_FALSE(scxml.isEmpty());

    model::StateMachineSerializer serializer;
    auto model = serializer.deserializeFromScxml(scxml);

    ASSERT_TRUE(model);
    EXPECT_EQ(nullptr, model->root()->findChildStateByName(""));
    EXPECT_TRUE(model->root()->findChildStateByName("S2") != nullptr);
}

/**
 * @brief Validate structural validation checks required SCXML root attributes.
 *
 * Use-case: Fast pre-check for import before full parse.
 *
 * @startuml
 * [*] --> ValidOrInvalid
 * @enduml
 */
TEST(StateMachineSerializerDeserializationTest, ValidateStructure) {
    model::StateMachineSerializer serializer;
    const QString valid = "<scxml xmlns=\"http://www.w3.org/2005/07/scxml\" version=\"1.0\"></scxml>";
    const QString invalid = "<scxml version=\"1.0\"></scxml>";

    EXPECT_TRUE(serializer.validateScxmlStructure(valid));
    EXPECT_FALSE(serializer.validateScxmlStructure(invalid));
}

}  // namespace
