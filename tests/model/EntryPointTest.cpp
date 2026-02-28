#include <gtest/gtest.h>

#include "model/EntryPoint.hpp"
#include "model/RegularState.hpp"
#include "model/Transition.hpp"

namespace {

/**
 * @brief Verify entry point supports multiple transitions and lookup APIs.
 *
 * Use-case: Entry point dispatches to different targets based on event/guards.
 *
 */
TEST(EntryPointTest, ManagesOutgoingTransitions) {
    auto source = QSharedPointer<model::RegularState>::create("Src");
    auto targetA = QSharedPointer<model::RegularState>::create("A");
    auto targetB = QSharedPointer<model::RegularState>::create("B");
    auto entry = QSharedPointer<model::EntryPoint>::create("EP");

    auto t1 = QSharedPointer<model::Transition>::create(source, targetA, "E1");
    auto t2 = QSharedPointer<model::Transition>::create(source, targetB, "E2");

    EXPECT_TRUE(entry->addChild(t1));
    EXPECT_TRUE(entry->addChild(t2));
    EXPECT_EQ(2, entry->transitions().size());
    EXPECT_EQ(entry, entry->findParentState(t1->id()));
    EXPECT_EQ(t2, entry->findChild(t2->id()).dynamicCast<model::Transition>());

    entry->deleteChild(t1->id());
    EXPECT_EQ(1, entry->transitions().size());
}

}  // namespace
