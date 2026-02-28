#include <gtest/gtest.h>

#include "model/RegularState.hpp"
#include "model/SelfTransition.hpp"

namespace {

/**
 * @brief Verify self-transition keeps same source/target and supports type mutation.
 *
 * Use-case: State loops back to itself with configurable transition kind.
 */
TEST(SelfTransitionTest, BasicBehavior) {
    auto state = QSharedPointer<model::RegularState>::create("S");
    auto selfTr = QSharedPointer<model::SelfTransition>::create(state, "tick");

    ASSERT_TRUE(selfTr);
    EXPECT_EQ(state->id(), selfTr->sourceId());
    EXPECT_EQ(state->id(), selfTr->targetId());

    selfTr->setType(model::TransitionType::INTERNAL);
    EXPECT_EQ(model::TransitionType::INTERNAL, selfTr->type());
}

}  // namespace
