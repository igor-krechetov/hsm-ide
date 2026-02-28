#include <gtest/gtest.h>

#include "model/ModelElementsFactory.hpp"
#include "model/RegularState.hpp"

namespace {

/**
 * @brief Verify factory creates unique states and source-owned transitions.
 *
 * Use-case: Editor creates new nodes and auto-attaches transitions.
 *
 */
TEST(ModelElementsFactoryTest, CreatesExpectedElements) {
    auto s1 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR).dynamicCast<model::RegularState>();
    auto s2 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR).dynamicCast<model::RegularState>();

    ASSERT_TRUE(s1);
    ASSERT_TRUE(s2);
    EXPECT_NE(s1->name(), s2->name());

    auto tr = model::ModelElementsFactory::createUniqueTransition(s1, s2);
    ASSERT_TRUE(tr);
    EXPECT_EQ(QString("NEW_EVENT"), tr->event());
    EXPECT_EQ(s1, tr->source());
    EXPECT_EQ(s2, tr->target());
}

}  // namespace
