#include "../QtTestCompat.hpp"

#include "model/RegularState.hpp"
#include "model/Transition.hpp"

namespace {

/**
 * @brief Verify transition attributes and property keys, including guards.
 *
 * Use-case: Transition stores event, callback and guard condition values.
 *
 */
TEST(TransitionTest, PropertiesAndSourceTargetManagement) {
    auto s1 = QSharedPointer<model::RegularState>::create("S1");
    auto s2 = QSharedPointer<model::RegularState>::create("S2");
    auto s3 = QSharedPointer<model::RegularState>::create("S3");
    auto tr = QSharedPointer<model::Transition>::create(s1, s2, "E");

    EXPECT_TRUE(tr->setProperty("event", "E2"));
    EXPECT_TRUE(tr->setProperty("conditionCallback", "isReady"));
    EXPECT_TRUE(tr->setProperty(model::Transition::cKeyExpectedConditionValue, true));
    EXPECT_TRUE(tr->setProperty(model::Transition::cKeyTransitionType, static_cast<int>(model::TransitionType::INTERNAL)));

    EXPECT_EQ(QString("E2"), tr->event());
    EXPECT_EQ(QString("isReady"), tr->conditionCallback());
    EXPECT_TRUE(tr->expectedConditionValue());
    EXPECT_EQ(model::TransitionType::INTERNAL, tr->transitionType());

    tr->setTarget(s3);
    EXPECT_EQ(s3->id(), tr->targetId());
    EXPECT_EQ(s1->id(), tr->sourceId());
}

}  // namespace
