#include "../QtTestCompat.hpp"

#include "model/ModelElementsFactory.hpp"
#include "model/StateMachineModel.hpp"

namespace {

/**
 * @brief Verify model move and reconnect operations for graph editing.
 *
 * Use-case: User drags states between parents and reconnects transitions.
 *
 */
TEST(StateMachineModelTest, MoveAndReconnectElements) {
    auto model = QSharedPointer<model::StateMachineModel>::create("Machine");
    auto root = model->root();

    auto a = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR).dynamicCast<model::RegularState>();
    auto b = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR).dynamicCast<model::RegularState>();
    auto parent = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR).dynamicCast<model::RegularState>();

    root->addChildState(a);
    root->addChildState(b);
    root->addChildState(parent);

    auto tr = model::ModelElementsFactory::createUniqueTransition(a, b);
    ASSERT_TRUE(tr);

    EXPECT_TRUE(model->moveElement(b->id(), parent->id()));
    EXPECT_EQ(parent, root->findParentState(b->id()).dynamicCast<model::RegularState>());

    EXPECT_TRUE(model->reconnectElements(tr->id(), parent->id(), a->id()));
    EXPECT_EQ(parent->id(), tr->sourceId());
    EXPECT_EQ(a->id(), tr->targetId());
}

}  // namespace
