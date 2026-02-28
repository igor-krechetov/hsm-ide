#include <gtest/gtest.h>

#include "model/IncludeEntity.hpp"
#include "model/RegularState.hpp"
#include "model/Transition.hpp"

namespace {

/**
 * @brief Verify include entity accepts transitions only and exposes path.
 *
 * Use-case: External SCXML subchart include keeps transitions and reference path.
 *
 */
TEST(IncludeEntityTest, PathAndAllowedChildren) {
    auto include = QSharedPointer<model::IncludeEntity>::create("Inc");
    auto s1 = QSharedPointer<model::RegularState>::create("S1");
    auto s2 = QSharedPointer<model::RegularState>::create("S2");
    auto tr = QSharedPointer<model::Transition>::create(s1, s2, "go");

    include->setPath("sub.scxml");
    EXPECT_EQ(QString("sub.scxml"), include->path());

    EXPECT_TRUE(include->addChild(tr));
    EXPECT_FALSE(include->addChild(s2));
    EXPECT_EQ(tr, include->findTransition(tr->id()));
}

}  // namespace
