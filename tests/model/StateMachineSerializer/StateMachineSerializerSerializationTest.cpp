#include "../../QtTestCompat.hpp"

#include "model/EntryPoint.hpp"
#include "model/ExitPoint.hpp"
#include "model/FinalState.hpp"
#include "model/HistoryState.hpp"
#include "model/IncludeEntity.hpp"
#include "model/InitialState.hpp"
#include "model/RegularState.hpp"
#include "model/StateMachineModel.hpp"
#include "model/StateMachineSerializer.hpp"
#include "model/Transition.hpp"

namespace {

/**
 * @brief Validate serialization of deep history with default transition.
 *
 * Use-case: Persist deep-history semantics to SCXML.
 *
 * @startuml
 * state S1
 * state H <<history:deep>>
 * H --> S1 : resume
 * @enduml
 */
TEST(StateMachineSerializerSerializationTest, SerializeDeepHistory) {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto s1 = QSharedPointer<model::RegularState>::create("S1");
    auto history = QSharedPointer<model::HistoryState>::create("H", model::HistoryType::DEEP);

    root->addChildState(s1);
    s1->addChildState(history);
    history->setDefaultTransition(QSharedPointer<model::Transition>::create(history, s1, "resume"));

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model);

    EXPECT_TRUE(scxml.contains("<history id=\"H\" type=\"deep\""));
    EXPECT_TRUE(scxml.contains("event=\"resume\""));
}

/**
 * @brief Validate serialization of shallow history.
 *
 * Use-case: Persist shallow history variant to SCXML.
 *
 * @startuml
 * state S1
 * state H <<history:shallow>>
 * @enduml
 */
TEST(StateMachineSerializerSerializationTest, SerializeShallowHistory) {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto s1 = QSharedPointer<model::RegularState>::create("S1");
    auto history = QSharedPointer<model::HistoryState>::create("H", model::HistoryType::SHALLOW);

    root->addChildState(s1);
    s1->addChildState(history);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model);

    EXPECT_TRUE(scxml.contains("<history id=\"H\" type=\"shallow\""));
}

/**
 * @brief Validate serialization of nested substates.
 *
 * Use-case: Persist parent-child hierarchy for regular states.
 *
 * @startuml
 * state Parent {
 *   state Child
 * }
 * @enduml
 */
TEST(StateMachineSerializerSerializationTest, SerializeSubstatesHierarchy) {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto parent = QSharedPointer<model::RegularState>::create("Parent");
    auto child = QSharedPointer<model::RegularState>::create("Child");

    root->addChildState(parent);
    parent->addChildState(child);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model);

    EXPECT_TRUE(scxml.contains("<state id=\"Parent\""));
    EXPECT_TRUE(scxml.contains("<state id=\"Child\""));
}

/**
 * @brief Validate serialization of multiple transitions on one state.
 *
 * Use-case: Persist multiple outgoing edges from a single source state.
 *
 * @startuml
 * S1 --> S2 : e1
 * S1 --> S3 : e2
 * @enduml
 */
TEST(StateMachineSerializerSerializationTest, SerializeMultipleTransitionsForSingleState) {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto s1 = QSharedPointer<model::RegularState>::create("S1");
    auto s2 = QSharedPointer<model::RegularState>::create("S2");
    auto s3 = QSharedPointer<model::RegularState>::create("S3");

    root->addChildState(s1);
    root->addChildState(s2);
    root->addChildState(s3);
    s1->addTransition(QSharedPointer<model::Transition>::create(s1, s2, "e1"));
    s1->addTransition(QSharedPointer<model::Transition>::create(s1, s3, "e2"));

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model);

    EXPECT_NE(scxml.indexOf("event=\"e1\""), -1);
    EXPECT_NE(scxml.indexOf("event=\"e2\""), -1);
}

/**
 * @brief Validate serialization of external/internal transition types.
 *
 * Use-case: Persist explicit SCXML transition type attributes.
 *
 * @startuml
 * S1 --> S2 : ext / external
 * S1 --> S3 : in / internal
 * @enduml
 */
TEST(StateMachineSerializerSerializationTest, SerializeExternalAndInternalTransitions) {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto s1 = QSharedPointer<model::RegularState>::create("S1");
    auto s2 = QSharedPointer<model::RegularState>::create("S2");
    auto s3 = QSharedPointer<model::RegularState>::create("S3");

    auto ext = QSharedPointer<model::Transition>::create(s1, s2, "ext");
    auto in = QSharedPointer<model::Transition>::create(s1, s3, "in");
    in->setTransitionType(model::TransitionType::INTERNAL);

    root->addChildState(s1);
    root->addChildState(s2);
    root->addChildState(s3);
    s1->addTransition(ext);
    s1->addTransition(in);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model);

    EXPECT_TRUE(scxml.contains("event=\"ext\" type=\"external\""));
    EXPECT_TRUE(scxml.contains("event=\"in\" type=\"internal\""));
}

/**
 * @brief Validate serialization of entry-point equivalent with multiple transitions.
 *
 * Use-case: Persist entrypoint as `<initial>` with multiple transitions.
 *
 * @startuml
 * state Region {
 *   [*] --> A : toA
 *   [*] --> B : toB
 * }
 * @enduml
 */
TEST(StateMachineSerializerSerializationTest, SerializeEntryPointTransitions) {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto region = QSharedPointer<model::RegularState>::create("Region");
    auto a = QSharedPointer<model::RegularState>::create("A");
    auto b = QSharedPointer<model::RegularState>::create("B");
    auto entry = QSharedPointer<model::EntryPoint>::create("EP");

    entry->addTransition(QSharedPointer<model::Transition>::create(entry, a, "toA"));
    entry->addTransition(QSharedPointer<model::Transition>::create(entry, b, "toB"));

    root->addChildState(region);
    region->addChildState(a);
    region->addChildState(b);
    region->addChildState(entry);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model);

    EXPECT_TRUE(scxml.contains("<initial>"));
    EXPECT_TRUE(scxml.contains("event=\"toA\""));
    EXPECT_TRUE(scxml.contains("event=\"toB\""));
}

/**
 * @brief Validate serialization of exit point and transition targeting it.
 *
 * Use-case: Persist nested exitpoint as `<final event=...>` and transition target id.
 *
 * @startuml
 * state Parent {
 *   state Worker
 *   Worker --> XP : done
 *   state XP <<exitPoint>>
 * }
 * @enduml
 */
TEST(StateMachineSerializerSerializationTest, SerializeExitPointAndTargetingTransition) {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto parent = QSharedPointer<model::RegularState>::create("Parent");
    auto worker = QSharedPointer<model::RegularState>::create("Worker");
    auto xp = QSharedPointer<model::ExitPoint>::create("XP");

    xp->setEvent("leave");

    root->addChildState(parent);
    parent->addChildState(worker);
    parent->addChildState(xp);
    worker->addTransition(QSharedPointer<model::Transition>::create(worker, xp, "done"));

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model);

    EXPECT_TRUE(scxml.contains("<final id=\"XP\" event=\"leave\""));
    EXPECT_TRUE(scxml.contains("event=\"done\""));
    EXPECT_TRUE(scxml.contains("target=\"XP\""));
}

/**
 * @brief Validate serialization of include entity to xi:include.
 *
 * Use-case: Persist include path with xi:include tag.
 *
 * @startuml
 * state IncludeNode
 * IncludeNode : xi:include href=subchart.scxml
 * @enduml
 */
TEST(StateMachineSerializerSerializationTest, SerializeIncludeEntity) {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();
    auto include = QSharedPointer<model::IncludeEntity>::create("IncludeNode");

    include->setPath("subchart.scxml");
    root->addChildState(include);

    model::StateMachineSerializer serializer;
    const QString scxml = serializer.serializeToScxml(model);

    EXPECT_TRUE(scxml.contains("<xi:include href=\"subchart.scxml\" parse=\"xml\""));
}

}  // namespace
