#include <gtest/gtest.h>

#include "model/InitialState.hpp"
#include "model/RegularState.hpp"
#include "model/Transition.hpp"

namespace {

/**
 * @brief Verify initial state keeps a single outgoing transition.
 *
 * Use-case: Initial pseudostate in SCXML points to startup target.
 *
 */
TEST(InitialStateTest, StoresSingleTransition) {
    auto root = QSharedPointer<model::RegularState>::create("Root");
    auto target = QSharedPointer<model::RegularState>::create("S1");
    auto initial = QSharedPointer<model::InitialState>::create();
    auto tr = QSharedPointer<model::Transition>::create(root, target, "");

    EXPECT_TRUE(initial->addChild(tr));
    EXPECT_EQ(tr, initial->transition());

    initial->deleteDirectChild(tr);
    EXPECT_EQ(nullptr, initial->transition());
    EXPECT_TRUE(initial->properties().isEmpty());
}

}  // namespace
