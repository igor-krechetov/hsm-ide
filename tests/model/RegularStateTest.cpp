#include "../QtTestCompat.hpp"

#include "model/ModelElementsFactory.hpp"
#include "model/RegularState.hpp"
#include "model/Transition.hpp"

namespace {

/**
 * @brief Verify regular state stores callbacks and supports property API.
 *
 * Use-case: User edits state callbacks in property panel.
 *
 */
TEST(RegularStateTest, CallbackPropertiesRoundTrip) {
    auto state = QSharedPointer<model::RegularState>::create("S");

    EXPECT_TRUE(state->setProperty("onEnteringCallback", "enterCb"));
    EXPECT_TRUE(state->setProperty("onExitingCallback", "exitCb"));
    EXPECT_TRUE(state->setProperty("onStateChangedCallback", "stateCb"));

    EXPECT_EQ(QString("enterCb"), state->onEnteringCallback());
    EXPECT_EQ(QString("exitCb"), state->onExitingCallback());
    EXPECT_EQ(QString("stateCb"), state->onStateChangedCallback());
}

/**
 * @brief Verify regular state child management and recursive search.
 *
 * Use-case: Hierarchical model resolves nested states and transitions by id and name.
 *
 */
TEST(RegularStateTest, ChildAndTransitionSearch) {
    auto parent = QSharedPointer<model::RegularState>::create("Parent");
    auto child = QSharedPointer<model::RegularState>::create("Child");
    parent->addChildState(child);

    auto tr = QSharedPointer<model::Transition>::create(parent, child, "ev");
    parent->addTransition(tr);

    EXPECT_EQ(child, parent->findChildStateByName("Child"));
    EXPECT_EQ(child, parent->findState(child->id()));
    EXPECT_EQ(tr, parent->findTransition(tr->id()));
    EXPECT_EQ(parent, parent->findParentState(child->id()));

    parent->deleteChild(child->id());
    EXPECT_EQ(nullptr, parent->findState(child->id()));
}

}  // namespace
